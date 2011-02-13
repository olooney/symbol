#include "symbol.h"

#include<iostream>

using symbol::Symbol;

int main(int argc, char** argv) {
	std::cout << "hello, symbol world!" << std::endl;
	Symbol hello("hello");
	std::cout << hello.value() << " -> " << hello.decode() << std::endl;
}
