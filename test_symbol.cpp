#include "symbol.h"

#include<iostream>

using symbol::Symbol;
using symbol::SymbolError;

bool verbose = true;

bool roundTrip(std::string identifier) {
	try { 
		Symbol symbol(identifier);
		std::string decoded = symbol.decode();
		bool recovered = (identifier == decoded);
		
		if ( verbose || !recovered ) {
			std::cout << "round trip: " << identifier << " -> " << symbol.value() << " -> " << decoded;
			if ( recovered ) std::cout << " recovered.\n";
			else std::cout << "NOT RECOVERED!\n";
		}

		return recovered;
	} catch ( SymbolError& e ) {
		std::cout << "unexpected SymbolError " << e.what() << std::endl;
		return false;
	}
}

bool expectSymbolError(const std::string& identifier) {
	try { 
		Symbol symbol(identifier);
		std::string decoded = symbol.decode();
	} catch ( SymbolError& e ) {
		if ( verbose ) {
			std::cout << "caught expected error: " << e.what() << std::endl;
		}
		return true;
	}
	std::cout << "no exception thrown when encoding and decoding: " << identifier << std::endl;
	return false;
}

bool lossy(const std::string& identifier) {
	Symbol sym1(identifier);
	Symbol sym2(identifier);

	// make sure it's non-random
	if ( sym1 != sym2 ) {
		std::cout << "unreliable encoding of " << identifier << std::endl;
		return false;
	}

	// make sure high bit was set
	static const unsigned long HIGH_BIT = (1UL << 63);
	static const unsigned long PENULTIMATE_BIT = (1UL << 62);
	if ( (sym1.value() & HIGH_BIT ) != HIGH_BIT ) {
		std::cout << "high bit of encoded " << identifier << " -> " << sym1.value() << " not set" << std::endl;
		return false;
	}

	if ( sym1.value() & PENULTIMATE_BIT ) {
		std::cout << "second-highest of encoded " << identifier << " -> " << sym1.value() << " is set" << std::endl;
		return false;
	}

	// passed!
	if ( verbose ) {
		std::cout << "reliably encoded long identifier " << identifier << " as " << sym1.value() << std::endl;
	}
	return true;
}

bool option(char** argv, char option) {
	while ( *argv ) {
		char* arg = *argv;
		if ( arg[0] == '-' ) {
			while ( *(++arg) != '\0' ) {
				if ( *arg == option ) return true;
			}
		}
		++argv;
	}
	return false;
}

int main(int argc, char** argv) {
	if ( option(argv, 'h') ) {
		std::cout << "usage: test_symbol [-v] [-h]\n"
			<< "returns zero if all tests pass\n"
			<< "-v option for verbose output\n"
			<< "-h option for these instructions\n";
		return 0;
	}
	verbose = option(argv, 'v');
	bool passed = true;
	passed &= roundTrip("");
	passed &= roundTrip("hello");
	passed &= roundTrip("abyz019_AZ");
	passed &= roundTrip("0123456789");
	passed &= expectSymbolError("!@#$#%");
	passed &= expectSymbolError("hi there");
	passed &= expectSymbolError("Mwahaha!!!");

	passed &= lossy("0123456789A");
	passed &= lossy("abcdefghijklmnopqrstuvwxyz");
	passed &= lossy("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
	passed &= lossy("abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ");

	if ( passed ) std::cout << "passed." << std::endl;
	else std::cout << "failed!" << std::endl;
	return passed ? 0 : 1;
}
