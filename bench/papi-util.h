#ifndef __PAPI_UTIL_H__
#define __PAPI_UTIL_H__

#include <papi.h>

#define PAPI_UTIL_SANDY_BRIDGE 3
#define PAPI_UTIL_NEHALEM_L3 4

#define MAX_COUNTERS 4

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int ncounters;
    int nvalues;
    int events[MAX_COUNTERS];
    long long values[MAX_COUNTERS+2];
    char names[MAX_COUNTERS+2][16];
} CounterGroup;

void init_papi_library(void);
void init_papi_threads(void);
void init_papi_counters(int group, CounterGroup *counters);
void zero_papi_counters(CounterGroup *counters);
void start_papi_counters(CounterGroup *counters);
void stop_papi_counters(CounterGroup *counters);
void print_papi_counters(const char* prefix, CounterGroup *counters);

#ifdef __cplusplus
}
#endif

#endif
    
