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
uint64_t encode_letter(char letter) 
throw()
{
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

// returns a letter (a-z or _) for the give 6-bit code.
// returns the null character for 0 or codes more than 6-bits.
char decode_letter(uint64_t code)
throw()
{
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

bool isIdentifier(const char c) 
{ 
	return c == '_' || isalnum(c); 
}

// note: only lower-case hexidecimal characters are checked for.
bool isHex(const char c) { 
	return 
		( '0' <= c && c <= '9' ) || 
		( 'a' <= c && c <= 'z' ); 
}

// determines if a word is an identifier in the 'abc_1234abcd_de'
// format used to decode lossy symbols.
bool matchesHashedFormat(const char* word) {
	const char* pc = word;
	if ( !isIdentifier(*pc) ) return false;
	pc++;
	if ( !isIdentifier(*pc) ) return false;
	pc++;
	if ( !isIdentifier(*pc) ) return false;
	pc++;
	if ( *pc != '_' ) return false;
	pc++;
	if ( !isHex(*pc) ) return false;
	pc++;
	size_t length = 1;
	while ( isHex(*pc) && length < 8 ) {
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
	if ( !isIdentifier(*pc) ) return false;
	pc++;
	if ( !isIdentifier(*pc) ) return false;
	pc++;
	if ( *pc != '\0' ) return false;
	return true;
}

bool Symbol::isHashed() 
{
	return _code & HIGH_BIT;
}

// make sure the symbol name consists only of allowed characters.
bool Symbol::validate(const std::string& identifier)
throw()
{
	size_t len = identifier.length();
	for ( size_t i=0; i < len; ++i ) {
		if ( !encode_letter(identifier[i]) ) return false;
	}
	return true;
}

Symbol::Symbol(const std::string& identifier) 
	throw(SymbolError):
	_code(0)
{
	// TODO: this is duplicate work...
	if ( !validate(identifier) ) {
		throw SymbolError("invalid character in identifier.");
	}

	size_t length = identifier.length();
	// TODO: handle decoded long identifiers as a special case
	// so that decoding and encoding a symbol always returns the original.
	if ( length > 10 ) {
		// hash the middle into 32 bits
		_code = SuperFastHash(identifier.c_str() + 3, length - 5);

		// first three
		_code |= encode_letter(identifier[0]) << 32;
		_code |= encode_letter(identifier[1]) << (32 + LETTER_BITS);
		_code |= encode_letter(identifier[2]) << (32 + LETTER_BITS * 2);

		// last two
		_code |= encode_letter(identifier[length-2]) << (32 + LETTER_BITS * 3);
		_code |= encode_letter(identifier[length-1]) << (32 + LETTER_BITS * 4);

		// set the high bit to indicate lossy encoding
		_code |= HIGH_BIT;
	} else {
		for( short i=0; i<SYMBOL_LEN && identifier[i] != '\0'; ++i ) {
			const uint64_t letter_code = encode_letter(identifier[i]);
			// Stack the letters up from right to left in the symbol.
			_code |= letter_code << (LETTER_BITS*i);
		}
	}
}

Symbol::Symbol(uint64_t symbol)
throw(SymbolError):
	_code(symbol)
{
	// TODO: throw if invalid?  Pretty much the only way
	// a number can be an invalid symbol code is if it's 
	// penultimate bit is set. That's not very useful.
}

// decodes the symbol into the given identifier buffer.
// the buffer must be able to hold SYMBOL_LEN characters 
// plus a null terminator.
void decode_in_place(uint64_t symbol, char* identifier) 
throw()
{
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

std::string Symbol::decode() const
throw()
{
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

		// TODO extract the hash and hex format it
		// format the first three and last two
		// put them together as "abc_2468ABCD_de"
		// ... or something.
		return identifier;
	} else {
		char identifier[SYMBOL_LEN+1];
		decode_in_place(_code, identifier);
		return identifier;
	}
}


} // end namespace symbol.
