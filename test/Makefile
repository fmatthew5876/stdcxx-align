CXXFLAGS+=-I../include -std=c++1y

all: test64 test32

test64: test.cc
	$(CXX) $(CXXFLAGS) -o $@ $^

test32: test.cc
	$(CXX) -m32 $(CXXFLAGS) -o $@ $^

clean:
	-rm test64 test32
