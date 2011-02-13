
default: test

# automatically compute and include header dependencies
makefile.d:
	gcc -MM  > makefile.d
-include makefile.d

%.o: %.cpp
	g++ -c -o $@ $<

symbol.a: symbol.o 
	ar rcs $@ $^

test_symbol: test_symbol.o 
	g++ -o $@ $^

test: test_symbol
	./test_symbol
