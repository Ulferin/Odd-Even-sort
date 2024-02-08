CXX = g++ -std=c++17
CXXFLAGS  	= -g

ifndef FF_ROOT 
FF_ROOT		= ${HOME}/fastflow
endif

LDFLAGS_1 = -pthread
LDFLAGS_2 = -I ${FF_ROOT}
OPTFLAGS = -O3 $(DEBUG) #-ftree-vectorize -fopt-info-vec

TARGETS = oe-sortseq oe-sortparnofs oe-sortmw

.PHONY = clean all test
.SUFFIXES = .cpp

all: $(TARGETS)

oe-sortseq: oe-sortseq.cpp
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -o $@ $< 

oe-sortmw: oe-sortmw.cpp utils.cpp
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -o $@ $< $(LDFLAGS_1) $(LDFLAGS_2)

%: %.cpp utils.cpp
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -o $@ $< $(LDFLAGS_1)

test: test-seq test-par test-mw

test-seq:
	./oe-sortseq $(SEED) $(LEN) $(MAX)

test-par:
	./oe-sortparnofs $(SEED) $(LEN) $(NW) $(CACHE) $(MAX)

test-mw:
	./oe-sortmw $(SEED) $(LEN) $(NW) $(CACHE) $(MAX)

clean:
	rm -f $(TARGETS)
