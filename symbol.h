#ifndef SYMBOL_H
#define SYMBOL_H
#include <string>
#include <stdexcept>
// #include "types.h"

namespace symbol {

// This error is thrown by Symbol encode/decode errors.
class SymbolError: public std::runtime_error {
public:
	explicit SymbolError(const std::string& message): std::runtime_error(message) {}
};

class Symbol {
	uint64_t _value;
public:
	// construct from string or numeric symbol.  Throw if bad format.
	Symbol(uint64_t symbol) throw(SymbolError);
	Symbol(const std::string& identifier) throw(SymbolError); // implicit ok

	// implicit conversion to string via decode()
	operator const std::string() throw() const;

	// read-only access to the underlying value.
	uint64_t value() throw() const { return _value; }

	// constructor is also available as Symbol::encode(identifier)...
	static Symbol encode(const std::string& identifier) throw(SymbolError) { 
		return Symbol(identifier);
	}

	// validate a potential identifier or symbol.  The constructors validate too, so
	// you only need to use these if you'd prefer to avoid having to catch an exception.
	static bool validate(const std::string& identifier) throw();
	static bool validate(uint64_t symbol) throw() const;

	std::string decode() const throw();

	// all comparison operators
	friend bool operator==(const Symbol& lhs, const Symbol& rhs) throw() { return lhs._value == rhs._value; }
	friend bool operator<=(const Symbol& lhs, const Symbol& rhs) throw() { return lhs._value <= rhs._value; }
	friend bool operator>=(const Symbol& lhs, const Symbol& rhs) throw() { return lhs._value >= rhs._value; }
	friend bool operator< (const Symbol& lhs, const Symbol& rhs) throw() { return lhs._value <  rhs._value; }
	friend bool operator> (const Symbol& lhs, const Symbol& rhs) throw() { return lhs._value >  rhs._value; }

	// default copy/assignment/dtor are fine
};

}
#endif
