gcc_options = -std=c++17 -Wall -O2 --pedantic-errors

sun_moon: sun_moon.o calc.o file.o time.o delta_t.o
	g++102 $(gcc_options) -o $@ $^

sun_moon.o : sun_moon.cpp
	g++102 $(gcc_options) -c $<

calc.o : calc.cpp
	g++102 $(gcc_options) -c $<

file.o : file.cpp
	g++102 $(gcc_options) -c $<

time.o : time.cpp
	g++102 $(gcc_options) -c $<

delta_t.o : delta_t.cpp
	g++102 $(gcc_options) -c $<

run : sun_moon
	./sun_moon

clean :
	rm -f ./sun_moon
	rm -f ./*.o

.PHONY : run clean

