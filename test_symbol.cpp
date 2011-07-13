#include "symbol.h"
#include "utilities.h"

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
			std::cout << "round trip: " << identifier << " -> " << symbol.code() << " -> " << decoded;
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
	if ( (sym1.code() & HIGH_BIT ) != HIGH_BIT ) {
		std::cout << "high bit of encoded " << identifier << " -> " << sym1.code() << " not set" << std::endl;
		return false;
	}

	if ( sym1.code() & PENULTIMATE_BIT ) {
		std::cout << "second-highest of encoded " << identifier << " -> " << sym1.code() << " is set" << std::endl;
		return false;
	}

	if ( verbose ) {
		std::cout << "lossy encoding: " << identifier << " -> " << sym1.code() << " -> " << sym1.decode() << std::endl;
	}

	// passed!
	if ( verbose ) {
		std::cout << "reliably encoded long identifier " << identifier << " as " << sym1.code() << std::endl;
	}
	return true;
}

bool testLossyFormatCheck(const char* word, bool expected) {
	bool result =symbol::matchesHashedFormat(word); 
	if ( verbose || result != expected ) {
		std::cout << "matchesHashedFormat(\"" << word << "\") returned " << result << ", expected " << expected << std::endl;
	}
	return result == expected;
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
	std::cout << std::boolalpha;

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

	passed &= testLossyFormatCheck("", false);
	passed &= testLossyFormatCheck("bdfsasd", false);
	passed &= testLossyFormatCheck("abc_1234abcd_dex", false);
	passed &= testLossyFormatCheck("abc_1_34abcd_de", false);
	passed &= testLossyFormatCheck("abc_1234aBcd_de", false);

	passed &= testLossyFormatCheck("abc_1234abcd_de", true);
	passed &= testLossyFormatCheck("abc_1234a____de", true);
	passed &= testLossyFormatCheck("_AZ_567890ef_09", true);

	if ( passed ) std::cout << "passed." << std::endl;
	else std::cout << "failed!" << std::endl;
	return passed ? 0 : 1;
}
