#include "symbol.h"
#include "hsfh.h"
  // provides SuperFastHash
#include <string.h>
  // provides strlen
#include <stdio.h>
 // provides sprintf
#include <ctype.h>

namespace symbol {

const unsigned short SYMBOL_LEN = 10;

// number of bits used to encode each letter.
static const unsigned short LETTER_BITS = 6;

// compute the one-letter mask. For LETTER_BITS=6, the mask is binary 111111.
static const uint64_t LETTER_MASK = (1UL<<LETTER_BITS)-1;

// masks to select the upper or lower half of a 64-bit number.
static const uint64_t LOWER_32 = (1UL<<32)-1;
static const uint64_t UPPER_32 = LOWER_32 >> 32;

static const uint64_t HIGH_BIT = 1UL << 63;

// represent a single character (a-zA-Z0-9_) in 6 bits.
// returns zero for any other character.  Never throws.
uint64_t encode_letter(char letter) throw() {
	// all encodable characters are in the range 48-122.  That range also
	// includes some non-encodable characters.  This lookup table maps
	// characters in that range to a 5-bit number.  Gaps are filled with 0.
	static uint64_t lookup_table[75] = {
		1,2,3,4,5,6,7,8,9,10, // digits
		0,0,0,0,0,0,0,
		11,12,13,14,15,16,17,18,19,20,21,22,23,
		24,25,26,27,28,29,30,31,32,33,34,35,36, // uppercase
		0,0,0,0,
		37, // underscore
		0,
		38,39,40,41,42,43,44,45,46,47,48,49,50,
		51,52,53,54,55,56,57,58,59,60,61,62,63 // lowercase
	};

	// alias to treat the char as a number.
	const unsigned short& ascii_code = (unsigned short)(letter);

	// make sure the letter is within range.
	if ( ascii_code < 48 || ascii_code > 122 ) {
		return 0;
	}

	// look the code up in the table.
	return lookup_table[ ascii_code-48 ];
}

uint64_t encode_letter_or_throw(char letter) throw(SymbolError) {
	uint64_t code = encode_letter(letter);
	if ( code == 0 ) throw SymbolError(std::string("unable to encode letter '") + letter + "'");
	return code;
}


// returns a letter (a-z or _) for the give 6-bit code.
// returns the null character for 0 or codes more than 6-bits.
char decode_letter(uint64_t code) throw() {
	// reverse lookup table.  Since codes are in a compact
	// sequence from 1-63 we can simply use a string.
	static const char* lookup_table = 
		"\0"
		"0123456789"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"_"
		"abcdefghijklmnopqrstuvwxyz";
	if ( code > 63 ) return 0;
	return lookup_table[code];
}

bool is_identifier(const char c) { 
	return c == '_' || isalnum(c); 
}

// note: only lower-case hexidecimal characters are checked for.
bool is_hex(const char c) { 
	return 
		( '0' <= c && c <= '9' ) || 
		( 'a' <= c && c <= 'z' ); 
}

// determines if a word is an identifier in the 'abc_1234abcd_de'
// format used to decode lossy symbols.
bool matches_lossy_format(const char* word) {
	const char* pc = word;
	if ( !is_identifier(*pc) ) return false;
	pc++;
	if ( !is_identifier(*pc) ) return false;
	pc++;
	if ( !is_identifier(*pc) ) return false;
	pc++;
	if ( *pc != '_' ) return false;
	pc++;
	if ( !is_hex(*pc) ) return false;
	pc++;
	size_t length = 1;
	while ( is_hex(*pc) && length < 8 ) {
		pc++;
		length++;
	}
	while ( length < 8 ) {
		if ( *pc != '_' ) return false;
		pc++;
		length++;
	}
	if ( *pc != '_' ) return false;
	pc++;
	if ( !is_identifier(*pc) ) return false;
	pc++;
	if ( !is_identifier(*pc) ) return false;
	pc++;
	if ( *pc != '\0' ) return false;
	return true;
}

bool Symbol::is_lossy() {
	return _code & HIGH_BIT;
}

Symbol::Symbol(const std::string& identifier) throw(SymbolError):
	_code(0)
{
	size_t length = identifier.length();

	if ( length > 10 ) {
		const char* cid = identifier.c_str();
	
		// There are two ways to calculate the lossy hash of the middle.
		// If the middle looks like a hex value surrounded by underscores,
		// simply evaluate the hex value.  This ensures that decoding and
		// re-encoding a long identifier has a consistent value.
		// Otherwise, simply take the hashed value of the whole string.
		if ( matches_lossy_format(cid) ) {
			// extract the hex value from the middle
			char hex_buffer[16];
			strcpy(hex_buffer, cid+4);
			char* pc=hex_buffer;
			for ( ; *pc != '_'; pc++ );
			*pc = '\0';

			// read the hex string into the code, filling the lower 32
			sscanf(hex_buffer, "%lx", &_code);
		} else {
			// validate the middle
			const char* pend = cid + length - 5;
			for ( const char* pc = cid+3; pc < pend; pc++ ) encode_letter_or_throw(*pc);

			// hash the middle into 32 bits
			_code = SuperFastHash(cid + 3, length - 5);
		}

		// first three
		_code |= encode_letter_or_throw(identifier[0]) << 32;
		_code |= encode_letter_or_throw(identifier[1]) << (32 + LETTER_BITS);
		_code |= encode_letter_or_throw(identifier[2]) << (32 + LETTER_BITS * 2);

		// last two
		_code |= encode_letter_or_throw(identifier[length-2]) << (32 + LETTER_BITS * 3);
		_code |= encode_letter_or_throw(identifier[length-1]) << (32 + LETTER_BITS * 4);

		// set the high bit to indicate lossy encoding
		_code |= HIGH_BIT;
	} else {
		for( short i=0; i<SYMBOL_LEN && identifier[i] != '\0'; ++i ) {
			const uint64_t letter_code = encode_letter_or_throw(identifier[i]);
			// Stack the letters up from right to left in the symbol.
			_code |= letter_code << (LETTER_BITS*i);
		}
	}
}

Symbol::Symbol(uint64_t symbol) throw(SymbolError):
	_code(symbol)
{ }

// decodes the symbol into the given identifier buffer.
// the buffer must be able to hold SYMBOL_LEN characters 
// plus a null terminator.
void decode_in_place(uint64_t symbol, char* identifier) throw() {
	for ( short i=0; i<SYMBOL_LEN; ++i ) {
		// decode the rightmost letter of the symbol.
		const uint64_t code = symbol & LETTER_MASK;
		const char letter = decode_letter(code);
		identifier[i] = letter;
		if ( letter == '\0' ) return;

		// shift the next letter into place.
		symbol >>= LETTER_BITS;
	}

	// add a final null terminator to cover the max length case.
	identifier[SYMBOL_LEN] = '\0';
}

std::string Symbol::decode() const throw() {
	if ( _code & HIGH_BIT ) {
		// maximum possible length is 8 for the hex of the hash, 5 for the
		// first-three/last-two, two understores, and a null character (in a
		// pear tree) for a total of 16.
		char identifier[16];
		// The first three/last two are stored in the upper 32 bits.
		// We'll put them in separate variable and shift them off one by one.
		unsigned long symbol = _code >> 32;

		// first three
		identifier[0] = decode_letter( symbol & LETTER_MASK );
		symbol >>= LETTER_BITS;
		identifier[1] = decode_letter( symbol & LETTER_MASK );
		symbol >>= LETTER_BITS;
		identifier[2] = decode_letter( symbol & LETTER_MASK );
		identifier[3] = '_';

		// print the hex value of the hash number (stored in the lower 32 bits)
		// into the identifier buffer after the underscore.
		sprintf(identifier+4, "%lx", _code & LOWER_32);

		// the hex value will be 8 or fewer characters depending on the
		// magnitude of the hashed value. If fewer, pad it out with extra
		// underscores.
		size_t length = strlen(identifier);
		while ( length < 13 ) {
			identifier[length] = '_';
			length++;
		}

		// last two
		symbol >>= LETTER_BITS;
		identifier[13] = decode_letter( symbol & LETTER_MASK );
		symbol >>= LETTER_BITS;
		identifier[14] = decode_letter( symbol & LETTER_MASK );

		// null terminate
		identifier[15] = '\0';

		return identifier;
	} else {
		char identifier[SYMBOL_LEN+1];
		decode_in_place(_code, identifier);
		return identifier;
	}
}

// stand alone function API
Symbol encode(const std::string& identifier) throw(SymbolError) {
	return Symbol(identifier);
}
std::string decode(uint64_t code) throw() {
	return Symbol(code).decode();
}
std::string decode(Symbol symbol) throw() {
	return symbol.decode();
}

// make sure the symbol name consists only of allowed characters.
bool validate(const std::string& identifier) throw() {
	try {
		Symbol symbol(identifier);
		return true;
	} catch ( SymbolError& e ) {
		return false;
	}
}


} // end namespace symbol.
