#ifndef SYMBOL_H
#define SYMBOL_H
#include <string>
#include <stdexcept>
#include <stdint.h>

namespace symbol {

// This error is thrown by Symbol encode/decode errors.
class SymbolError: public std::runtime_error {
public:
	explicit SymbolError(const std::string& message): std::runtime_error(message) {}
};

class Symbol {
	uint64_t _code;
public:
	// construct from string or numeric symbol code.  Throw if bad format.
	Symbol(uint64_t symbol) throw(SymbolError);
	Symbol(const std::string& identifier) throw(SymbolError); // implicit ok

	// Note: default copy/assignment/dtor are fine

	// read-only access to the numeric code.
	uint64_t code() const throw() { return _code; }

	// returns true if the symbol was too long to encode exactly and was hashed instead.
	bool isHashed();

	// constructor is also available as Symbol::encode(identifier)...
	static Symbol encode(const std::string& identifier) throw(SymbolError) { 
		return Symbol(identifier);
	}
	// TODO: I think these static methods might be clearer as functions in the symbol namespace.

	// validate a potential identifier or symbol.  The constructors validate too, so
	// you only need to use these if you'd prefer to avoid having to catch an exception.
	static bool validate(const std::string& identifier) throw();
	bool validate(uint64_t symbol) const throw();

	// return the string representation, if possible.
	std::string decode() const throw();

	// implicit conversion to string via decode()
	operator std::string() const throw() { return decode(); }
	// TODO: can I also have an implicit conversion to uint_64 without introducing ambiguities?

	// all comparison operators
	friend bool operator==(const Symbol& lhs, const Symbol& rhs) throw() { return lhs._code == rhs._code; }
	friend bool operator!=(const Symbol& lhs, const Symbol& rhs) throw() { return lhs._code != rhs._code; }
	friend bool operator<=(const Symbol& lhs, const Symbol& rhs) throw() { return lhs._code <= rhs._code; }
	friend bool operator>=(const Symbol& lhs, const Symbol& rhs) throw() { return lhs._code >= rhs._code; }
	friend bool operator< (const Symbol& lhs, const Symbol& rhs) throw() { return lhs._code <  rhs._code; }
	friend bool operator> (const Symbol& lhs, const Symbol& rhs) throw() { return lhs._code >  rhs._code; }

};

// TODO stand-alone symbol::encode() and symbol::decode() functions too.
// uint64_t encode(const std::string& identifier);
// uint64_t encode(Symbol symbol);
// std::string decode(uint64_t symbolCode);
// std::string decode(Symbol symbol);

}
#endif
