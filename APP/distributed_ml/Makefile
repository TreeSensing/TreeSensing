GCC = g++
CFLAGS = -O2 -std=c++11
FILES = classification.out regression.out

all: $(FILES) 

classification.out: classification.cpp
	$(GCC) $(CFLAGS) classification.cpp -o classification.out

regression.out: regression.cpp
	$(GCC) $(CFLAGS) regression.cpp -o regression.out

clean:
	rm $(all) -f *~ *.o *.out
