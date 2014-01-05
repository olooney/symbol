// Ad-hoc unit tests for the symbol library.
// prints "passed." and returns a zero exit code if all the tests pass.
// Returns non-zero exit-code and outputs error messages for any failing tests.
//   "-v" option for verbose mode, which prints the details of each test
//   "-h" option prints the help message and exits.
// regardless of if it passed or failed.

#include<iostream>
#include<sstream>
#include "symbol.h"
#include "symbol_space.h"

// global variable for verbose mode. Test functions will do additional output if set
bool verbose = true;

// some test functions. Return true if the test passed.
bool testEncodeDecode(std::string identifier);
bool expectSymbolError(const std::string& identifier);
bool testLossy(const std::string& identifier);
bool testDecodeReencode(const char* word, bool expected);
bool testAPI();

// ad-hoc utility to look for an option (-x, -y, etc.) in the command line arguments.
bool option(char** argv, char option);

bool testSymbolSpace();

int main(int argc, char** argv) {
	std::cout << std::boolalpha;

	if ( option(argv, 'h') ) {
		std::cout << "usage: test_symbol [-v] [-h]\n"
			<< "exits with zero if all tests pass\n"
			<< "-v option for verbose output\n"
			<< "-h option for these instructions\n";
		return 0;
	}
	verbose = option(argv, 'v');

	bool passed = true;

	passed &= testAPI();

	// the empty string is encoded as 0 as a special case.
	passed &= testEncodeDecode("");

	// some simple short identifiers which should be encoded exactly.
	passed &= testEncodeDecode("hello");
	passed &= testEncodeDecode("abyz019_AZ");
	passed &= testEncodeDecode("0123456789");

	// some bad identifiers which can't be encoded at all.
	passed &= expectSymbolError(" ");
	passed &= expectSymbolError("trailing ");
	passed &= expectSymbolError(" padded ");
	passed &= expectSymbolError("hi there");
	passed &= expectSymbolError("!@#$#%");
	passed &= expectSymbolError("excited!");
	passed &= expectSymbolError("@ruby");
	passed &= expectSymbolError("#yolo");
	passed &= expectSymbolError("$ngRoute");
	passed &= expectSymbolError("Mwahaha!!");

	// these can be reliably encoded, but some information is lost so they
	// can't be entirely decoded again.
	passed &= testLossy("0123456789A");
	passed &= testLossy("abcdefghijklmnopqrstuvwxyz");
	passed &= testLossy("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
	passed &= testLossy("abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ");

	// short symbol identifiers should be recovered exactly
	passed &= testDecodeReencode("", true);
	passed &= testDecodeReencode("hello", true);
	passed &= testDecodeReencode("_", true);
	passed &= testDecodeReencode("__init__", true);
	passed &= testDecodeReencode("0", true);
	passed &= testDecodeReencode("1", true);
	passed &= testDecodeReencode("42", true);
	passed &= testDecodeReencode("0123456789", true);
	// long symbol identifiers will lose information
	passed &= testDecodeReencode("abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ", false);

	// these are all examples of correctly formatted lossy-encoded symbols, so should match when re-encoded.
	passed &= testDecodeReencode("abc_1234abcd_de", true);
	passed &= testDecodeReencode("abc_1234a____de", true);
	passed &= testDecodeReencode("_AZ_567890ef_09", true);

	// these don't quite match, middle should be hashed and won't match exactly when re-encoded.
	passed &= testDecodeReencode("abc_1234abcd_dex", false);
	passed &= testDecodeReencode("abc_1_34abcd_de", false);
	passed &= testDecodeReencode("abc_1234aBcd_de", false);

    passed &= testSymbolSpace();

	if ( passed ) std::cout << "passed." << std::endl;
	else std::cout << "failed!" << std::endl;

	return passed ? 0 : 1;
}

bool testEncodeDecode(std::string identifier) {
	try { 
		symbol::Symbol sym(identifier);
		std::string decoded = sym.decode();
		bool recovered = (identifier == decoded);
		
		if ( verbose || !recovered ) {
			std::cout << "round trip: " << identifier << " -> " << sym.code() << " -> " << decoded;
			if ( recovered ) std::cout << " recovered.\n";
			else std::cout << "NOT RECOVERED!\n";
		}

		bool lossy = sym.is_lossy();
		if ( lossy ) {
			std::cout << "lost information encoding " << identifier << std::endl;
		}

		return recovered && !lossy;
	} catch ( symbol::SymbolError& e ) {
		std::cout << "unexpected SymbolError " << e.what() << std::endl;
		return false;
	}
}

bool expectSymbolError(const std::string& identifier) {
	try { 
		symbol::Symbol sym(identifier);
		std::string decoded = sym.decode();
	} catch ( symbol::SymbolError& e ) {
		if ( verbose ) {
			std::cout << "caught expected error: " << e.what() << std::endl;
		}
		return true;
	}
	std::cout << "no exception thrown when encoding and decoding: " << identifier << std::endl;
	return false;
}

bool testLossy(const std::string& identifier) {
	symbol::Symbol sym1(identifier);
	symbol::Symbol sym2(identifier);

	// make sure it's consistent (non-random)
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

	if ( !sym1.is_lossy() ) {
		std::cout << "symbol " << identifier << " does not self-report as lossy." << std::endl;
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

bool testDecodeReencode(const char* word, bool expected) {
	symbol::Symbol encoded(word);
	std::string decoded = encoded.decode();
	symbol::Symbol re_encoded(decoded);
	bool matched = decoded == word;
	bool success = (matched == expected) && (re_encoded == encoded);
	if ( verbose || !success ) {
		std::cout << "original: " << word << ", encoded: " << encoded.code() << ", decoded: " << decoded << ", re-encoded: " << re_encoded.code() << ", sucess: " << success << std::endl;
	}
	return success;
}

// test the API for C++ intuitiveness, example of use.
bool testAPI() {
	bool passed = true;
	passed &= symbol::validate("");
	passed &= symbol::validate("0");
	passed &= symbol::validate("testing");
	passed &= !symbol::validate("help!");
	passed &= !symbol::validate("save me");

	uint64_t code = symbol::encode("Test").code();
	// test decode(uint64_t) override
	std::string identifier = symbol::decode(code); 
	passed &= (identifier == "Test");

	// test implicit and explicit construction from unsigned long
	passed &= symbol::Symbol(code) > 0; 
	
	symbol::Symbol sym("Testing");
	std::string name;
	name = sym; // implicit conversion from Symbol to string
	passed &= (name == "Testing");

	// test decode(Symbol) override
	name = symbol::decode(sym); 
	passed &= (name == "Testing");

	// standard output operator
	std::stringstream ss;
	ss << sym; 
	passed &= (ss.str() == "Testing");

	// test copy constructor... and setup for comparison operators.
	symbol::Symbol sym2(sym);

	// comparison operators.
	symbol::Symbol sym3("thisIsARatherLongSymbol");
	passed &= (sym == sym);
	passed &= !(sym != sym);
	passed &= (sym <= sym);
	passed &= (sym >= sym);

	passed &= (sym == sym2);
	passed &= (sym <= sym2);
	passed &= (sym >= sym2);

	passed &=  (sym  != sym3);
	passed &= !(sym  == sym3);
	passed &=  (sym  <  sym3);
	passed &= !(sym  >  sym3);
	passed &=  (sym3 >  sym );
	passed &= !(sym3 <  sym );
	
	// we don't need detailed error reporting here. If this test compiles 
	// it's unlikely to fail at runtime.
	if ( !passed ) {
		std::cout << "failed basic API usage test." << std::endl;
	}

	return passed;
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

bool testSymbolSpace() {
    bool passed = true;
    symbol::Space<int> point;
    symbol::Symbol x("x");
    symbol::Symbol y("y");
    symbol::Symbol z("z"); // reserved for "misses"

    // nothing to get from empty space
    passed &= (point.get(x) == NULL);
    passed &= (point.get(y) == NULL);
    passed &= (point.get(z) == NULL);

    point.del(x);
    point.del(y);
    point.del(z);

    // one key
    point.set(x, 42);
    passed &= (*point.get(x) == 42);
    passed &= (point.get(z) == NULL);
    point.del(z); 
    point.set(x, 99);
    passed &= (*point.get(x) == 99);
    point.del(x);
    passed &= (point.get(x) == NULL);
    point.del(z); 

    // two keys
    point.set(x, 1);
    point.set(y, 2); // append
    passed &= (*point.get(x) == 1);
    passed &= (*point.get(y) == 2);

    passed &= (point.get(z) == NULL);
    point.del(z); 
    point.del(x);
    passed &= (point.get(x) == NULL);
    passed &= (*point.get(y) == 2);
    passed &= (point.get(z) == NULL);
    point.del(z); 
    point.del(y);
    passed &= (point.get(x) == NULL);
    passed &= (point.get(y) == NULL);
    passed &= (point.get(z) == NULL);

    // two keys, again, different order
    point.set(y, 2);
    point.set(x, 1); // insert before head
    passed &= (*point.get(x) == 1);
    passed &= (*point.get(y) == 2);
    passed &= (point.get(z) == NULL);
    point.del(z); 
    point.del(x);
    passed &= (point.get(x) == NULL);
    passed &= (*point.get(y) == 2);
    passed &= (point.get(z) == NULL);
    point.del(z); 
    point.del(y);
    passed &= (point.get(x) == NULL);
    passed &= (point.get(y) == NULL);
    passed &= (point.get(z) == NULL);

    // two keys, with overwritting this time
    point.set(y, 2);
    point.set(x, 1);
    passed &= (*point.get(x) == 1);
    passed &= (*point.get(y) == 2);
    point.set(x, 3);
    passed &= (*point.get(x) == 3);
    passed &= (*point.get(y) == 2);
    point.set(y, 4);
    passed &= (*point.get(x) == 3);
    passed &= (*point.get(y) == 4);
    point.del(x);
    point.del(y);
    passed &= (point.get(x) == NULL);
    passed &= (point.get(y) == NULL);
    passed &= (point.get(z) == NULL);

    // three keys
    point.set(x, 1);
    point.set(z, 3);
    point.set(y, 2); // insert in the middle!
    passed &= (*point.get(x) == 1);
    passed &= (*point.get(y) == 2);
    passed &= (*point.get(z) == 3);
    // overwrite at head, middle, and end
    point.set(x, 4);
    point.set(y, 5); 
    point.set(z, 6);
    passed &= (*point.get(x) == 4);
    passed &= (*point.get(y) == 5);
    passed &= (*point.get(z) == 6);
    // del from middle
    point.del(y);
    passed &= (*point.get(x) == 4);
    passed &= (point.get(y) == NULL);
    passed &= (*point.get(z) == 6);

    symbol::Space<int> letters;
    std::string test_data = 
    "Templates are a way of making your classes more abstract by letting you"
    "define the behavior of the class without actually knowing what datatype"
    "will be handled by the operations of the class. In essence, this is what is"
    "known as generic programming; this term is a useful way to think about"
    "templates because it helps remind the programmer that a templated class"
    "does not depend on the datatype (or types) it deals with. To a large"
    "degree, a templated class is more focused on the algorithmic thought rather"
    "than the specific nuances of a single datatype. Templates can be used in"
    "conjunction with abstract datatypes in order to allow them to handle any"
    "type of data. For example, you could make a templated stack class that can"
    "handle a stack of any datatype, rather than having to create a stack class"
    "for every different datatype for which you want the stack to function. The"
    "ability to have a single class that can handle several different datatypes"
    "means the code is easier to maintain, and it makes classes more reusable.";

    for ( std::string::iterator it = test_data.begin(); it != test_data.end(); ++it) {
        std::string letter(1, *it);
        if ( symbol::validate(letter) ) { 
            int count = 0;
            if ( letters.get(letter) == NULL ) {
                count = 0;
            } else {
                count = *letters.get(letter);
            }
            letters.set(letter, count+1);
       }
    }
    passed &= (*letters.get(symbol::Symbol("e")) == 100); // count them yourself!
    passed &= (*letters.get(symbol::Symbol("T")) == 4); 

    if ( !passed ) {
        std::cout << "failed symbol::Space tests." << std::endl;
    }

    return passed;

}
