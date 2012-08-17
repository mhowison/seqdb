OPT = -O3 -msse4.2
LDLIBS = -lhdf5 -lsz -lz

ifeq ($(CXX),icpc)
CXXFLAGS += -openmp -fPIC -restrict
LDFLAGS += -pthread -openmp -fPIC
endif

ifeq ($(CXX),g++)
CXXFLAGS += -Wall -fopenmp -fPIC
LDFLAGS += -pthread -fopenmp -fPIC
endif

ifeq ($(STATIC), 1)
CXXFLAGS += -static
LDFLAGS += -static
endif

ifeq ($(DEBUG), 1)
CXXFLAGS += -g -DDEBUG
LDFLAGS += -g
OPT = -O2
endif

ifdef GPERF
CXXFLAGS += -g3
LDFLAGS += -g3 -L$(GPERF)/lib
LDLIBS += -lprofiler
endif

ifdef HDF5
CXXFLAGS += -I$(HDF5)/include
LDFLAGS += -L$(HDF5)/lib
endif

CC = gcc -std=c99
CXXFLAGS += $(OPT)
CFLAGS = $(CXXFLAGS)

