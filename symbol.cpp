#include "symbol.h"

namespace symbol {

const unsigned short SYMBOL_LEN = 10;

// number of bits used to encode each letter.
static const unsigned short LETTER_BITS = 6;

// compute the one-letter mask. For LETTER_BITS=6, the mask is binary 111111.
static const Symbol LETTER_MASK = (1<<LETTER_BITS)-1;

// number of bits used for the checksum.
static const unsigned short CHECKSUM_BITS = 3;

// compute the checksum mask.
static const unsigned short CHECKSUM_MASK = (1 << CHECKSUM_BITS) - 1;

// the high-bit indicates the symbol is on the table.  If it is cleared
// the name was short enough for the symbol to be encoded in-place.
static const Symbol ON_TABLE_FLAG = static_cast<Symbol>(1) << 63;

// the globally shared symbol table.
SymbolTable the_symbol_table( ON_TABLE_FLAG );

// represent a single character (a-zA-Z0-9_) in 6 bits.
// returns zero for any other character.  Never throws.
inline unsigned short encode_letter(char letter) 
throw()
{
	// all encodable characters are in the range 48-122.  That range also
	// includes some non-encodable characters.  This lookup table maps
	// characters in that range to a 5-bit number.  Gaps are filled with 0.
	static unsigned short lookup_table[75] = {
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
inline char decode_letter(unsigned short code)
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
bool is_valid(std::string name)
{
	size_t len = name.length();
	for ( size_t i=0; i < len; ++i ) {
		if ( !encode_letter(name[i]) ) return false;
	}
	return true;
}

Symbol encode_in_place(const char* symbol_name) 
{
	Symbol symbol = 0; // holds the resulting symbol.
	unsigned short checksum = 0;

	short i=0;
	for( ; i<SYMBOL_LEN && symbol_name[i] != '\0'; ++i ) {
		const char letter = symbol_name[i];
		const unsigned short code = encode_letter(letter);

		// twiddle the checksum.
		checksum ^= code & CHECKSUM_MASK;

		// Stack the letters up from right to left in the symbol.
		symbol |= ((Symbol)(code) << (LETTER_BITS*i));
	}

	// store the checksum using the next 3 bits to the left. (we 
	// used 60 for the letters, 3 for the checksum, and one for 
	// the ON_TABLE_FLAG.)
	symbol |= (((Symbol)checksum)) << 60;

	return symbol;
}

Symbol encode(const std::string& name) 
throw(SymbolError)
{
	if ( !is_valid(name) ) {
		throw SymbolError("bad encode: invalid character.");
	}
	if ( name.length() > 10 ) {
		return the_symbol_table.get_or_add_symbol(name);
	}
	return encode_in_place(name.c_str());
}

Symbol lookup(const std::string& name) 
throw(SymbolError)
{
	if ( !is_valid(name) ) {
		throw SymbolError("bad encode: invalid character.");
	}
	if ( name.length() > 10 ) {
		return the_symbol_table.get_symbol(name);
	}
	return encode_in_place(name.c_str());
}


// decodes the symbol into the given symbol_name buffer.
// the buffer must be able to hold SYMBOL_LEN characters 
// plus a null terminator.
void decode_in_place(Symbol symbol, char* symbol_name) 
throw(SymbolError)
{
	unsigned short checksum = 0;

	for ( short i=0; i<SYMBOL_LEN; ++i ) {
		// decode the rightmost letter of the symbol.
		const unsigned short code = symbol & LETTER_MASK;
		const char letter = decode_letter(code);
		symbol_name[i] = letter;

		// twiddle the checksum.
		checksum ^= code & CHECKSUM_MASK;

		// shift the next letter into place.
		symbol >>= LETTER_BITS;
	}

	// compare the last 3-bits of the actual and computed checksums.
	if ( (symbol & CHECKSUM_MASK) != checksum ) {
		throw SymbolError("bad decode: symbol checksum didn't match.");
	}

	// add a final null terminator to cover the max length case.
	symbol_name[SYMBOL_LEN] = '\0';
}

std::string decode(Symbol symbol) 
throw(SymbolError)
{
	if ( symbol & ON_TABLE_FLAG ) {
		return the_symbol_table.get_name(symbol);
	}

	char symbol_name[SYMBOL_LEN+1];
	decode_in_place(symbol, symbol_name);
	return std::string(symbol_name);
}


} // end namespace symbol.
