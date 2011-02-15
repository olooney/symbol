#include "symbol.h"

namespace symbol {

const unsigned short SYMBOL_LEN = 10;

// number of bits used to encode each letter.
static const unsigned short LETTER_BITS = 6;

// compute the one-letter mask. For LETTER_BITS=6, the mask is binary 111111.
static const uint64_t LETTER_MASK = (1<<LETTER_BITS)-1;

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

uint64_t encode_in_place(const char* identifier) 
throw()
{
	uint64_t symbol = 0; // holds the resulting symbol.

	short i=0;
	for( ; i<SYMBOL_LEN && identifier[i] != '\0'; ++i ) {
		const uint64_t letter_code = encode_letter(identifier[i]);

		// Stack the letters up from right to left in the symbol.
		symbol |= letter_code << (LETTER_BITS*i);
	}

	return symbol;
}

Symbol::Symbol(const std::string& identifier) 
throw(SymbolError)
{
	// TODO: this is duplicate work...
	if ( !validate(identifier) ) {
		throw SymbolError("invalid character in identifier.");
	}
	if ( identifier.length() > 10 ) {
		throw SymbolError("identifier too long for lossless encoding.");
	}
	_value = encode_in_place(identifier.c_str());
}

Symbol::Symbol(uint64_t symbol)
throw(SymbolError):
	_value(symbol)
{
	// TODO: throw if invalid
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
	char identifier[SYMBOL_LEN+1];
	decode_in_place(_value, identifier);
	return std::string(identifier);
}


} // end namespace symbol.
