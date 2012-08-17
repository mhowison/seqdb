include Make.inc

TESTS = benchmem

ifdef LIKWID
CXXFLAGS += -I$(LIKWID)/include -DLIKWID
LDLIBS += -L$(LIKWID)/lib -llikwid $(LDLIBS)
endif

ifdef DSRC
TESTS += benchrand
CXXFLAGS += -I$(DSRC)/include
LDLIBS += -L$(DSRC)/lib -ldsrc
endif

CXXFLAGS += -I../src
CFLAGS = $(CXXFLAGS)

all:	$(TESTS)

benchmem:	benchmem.o ../src/libseqdb.a
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

benchrand:	benchrand.o ../src/libseqdb.a
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean:
	-rm *.o benchmem benchrand

