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
	// note that both of these are *implicit* constructors, and will
	// automatically cast symbols from unsigned longs or strings.
	Symbol(uint64_t symbol) throw(SymbolError);
	Symbol(const std::string& identifier) throw(SymbolError);

	// Note: default copy/assignment/dtor are fine

	// read-only access to the numeric code.
	uint64_t code() const throw() { return _code; }

	// returns true if the symbol was too long to encode exactly and was hashed instead.
	bool is_lossy();

	// return the string representation.
	std::string decode() const throw();

	// implicit conversion to string via decode()
	operator std::string() const throw() { return decode(); }

	// all comparison operators
	friend bool operator==(const Symbol& lhs, const Symbol& rhs) throw() { return lhs._code == rhs._code; }
	friend bool operator!=(const Symbol& lhs, const Symbol& rhs) throw() { return lhs._code != rhs._code; }
	friend bool operator<=(const Symbol& lhs, const Symbol& rhs) throw() { return lhs._code <= rhs._code; }
	friend bool operator>=(const Symbol& lhs, const Symbol& rhs) throw() { return lhs._code >= rhs._code; }
	friend bool operator< (const Symbol& lhs, const Symbol& rhs) throw() { return lhs._code <  rhs._code; }
	friend bool operator> (const Symbol& lhs, const Symbol& rhs) throw() { return lhs._code >  rhs._code; }
};

inline std::ostream& operator<<(std::ostream& out, const Symbol& sym) {
	out << sym.decode();
	return out;
}

// standalone functions are somewhat clearer than the Symbol constructor.
Symbol encode(const std::string& identifier) throw(SymbolError);
std::string decode(uint64_t symbolCode) throw();
std::string decode(Symbol symbol) throw();

// validate a potential identifier .  The constructors validate too, so you
// only need to use validate() if you'd prefer to avoid having to catch an exception.
bool validate(const std::string& identifier) throw();

}
#endif
