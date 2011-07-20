# build the symbol library and optionally test it.

default: symbol.a

# automatically compute and include header dependencies
makefile.d:
	gcc -MM *.h *.cpp > makefile.d
-include makefile.d

%.o: %.cpp
	g++ -Wall -c -o $@ $<

symbol.a: symbol.o 
	ar rcs $@ $^

test_symbol: test_symbol.o symbol.a
	g++ -Wall -o $@ $^

test: test_symbol
	./test_symbol

clean:
	rm -fv *.o test_symbol makefile.d
