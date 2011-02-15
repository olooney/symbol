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
	passed &= expectSymbolError("0123456789A");
	return passed ? 0 : 1;
}
