include Make.inc

TESTS = benchmem

ifdef PAPI
CXXFLAGS += -I$(PAPI)/include
LDLIBS += -L$(PAPI)/lib
endif

LDLIBS += -lpapi

ifdef DSRC
TESTS += benchrand
CXXFLAGS += -I$(DSRC)
LDLIBS += -L$(DSRC) -ldsrc
endif

CXXFLAGS += -I../src

all:	$(TESTS)

papi-util.o:	papi-util.c papi-util.h

benchmem:	benchmem.o papi-util.o ../src/.libs/libseqdb.a
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

benchrand:	benchrand.o papi-util.o ../src/.libs/libseqdb.a
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean:
	-rm *.o benchmem benchrand

