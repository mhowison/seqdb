#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "papi-util.h"

#define ERROR(msg) do{\
	fprintf(stderr, "papi-util: %s (%d): %s!\n", __FILE__, __LINE__, msg);\
	fflush(stderr);\
	exit(EXIT_FAILURE);\
}while(0)

/* Initializes the PAPI library: only needs to be called by one thread.
 */
void init_papi_library()
{
	int status = PAPI_library_init(PAPI_VER_CURRENT);
	if (status != PAPI_VER_CURRENT) ERROR("PAPI library init error");
}
 
/* Tell PAPI what function to use for distinguishing among multiple threads.
 * pthread_self() is a safe bet for both pthreads and OpenMP code.
 */
void init_papi_threads()
{
	int status = PAPI_thread_init(pthread_self);
	if (status != PAPI_OK) ERROR(PAPI_strerror(status));
}

/* Sets up the counter group.
 *
 * PAPI can typically only record four values at a time because of
 * hardware counter limits. Also, some counters are incompatible with
 * each other because they use the same hardware counter
 * (this is the motivation for the "check" function below).
 *
 * Group 0 = FLOP/s, L1 data cache accesses/misses, TLB misses
 * Group 1 = L2 data cache accesses/misses, total instructions,
 *           vector instructions
 * Group 2 = FP multiply instructions, FP add instructions, branches taken,
 *           branches mispredicted
 *
 * Also, verify that the counters in the group are compatible with each
 * other on the present architecture.
 *
 * The counters in these groups are compatible on the Opteron
 * architecture.
 *
 * Group 3 is for the Intel Sandy Bridge EP.
 */
void init_papi_counters(int group, CounterGroup* counters)
{
	int i, status;
	PAPI_event_info_t info;

	// set counters
	if (group == 0) {
		counters->events[0] = PAPI_FP_OPS;
		counters->events[1] = PAPI_L1_DCA;
		counters->events[2] = PAPI_L1_DCM;
		counters->events[3] = PAPI_TLB_DM;
		counters->ncounters = 4;
	} else if (group == 1) {
		counters->events[0] = PAPI_L2_DCA;
		counters->events[1] = PAPI_L2_DCM;
		counters->events[2] = PAPI_TOT_INS;
		counters->events[3] = PAPI_VEC_INS;
		counters->ncounters = 4;
	} else if (group == 2) {
		counters->events[0] = PAPI_FML_INS;
		counters->events[1] = PAPI_FAD_INS;
		counters->events[2] = PAPI_BR_TKN;
		counters->events[3] = PAPI_BR_MSP;
		counters->ncounters = 4;
	} else if (group == PAPI_UTIL_SANDY_BRIDGE) {
		counters->events[0] = PAPI_BR_INS;
		//counters->events[1] = PAPI_TOT_INS;
		counters->ncounters = 1;
	} else if (group == PAPI_UTIL_NEHALEM_L3) {
		counters->events[0] = PAPI_L3_TCM;
		counters->events[1] = PAPI_L3_TCA;
		counters->events[2] = PAPI_TLB_DM;
		counters->ncounters = 3;
	} else {
		ERROR("Unrecognized PAPI counter group");
	}
	
	printf("\n### PAPI v%d\n", PAPI_VER_CURRENT);
	for (i=0; i<counters->ncounters; i++) {
		status = PAPI_get_event_info(counters->events[i], &info);
		if (status != PAPI_OK) {
			printf("# Counter %d is unavailable.\n", i);
		} else if (info.count > 0) {
			printf("# %s is available.\n", info.symbol);
		}
		strncpy(counters->names[i], info.symbol, 15);
		counters->names[i][15] = '\0';
	}
	printf("###\n");

	/* append the cycles and usecs counters */
	strcpy(counters->names[counters->ncounters+0],"CYCLES");
	strcpy(counters->names[counters->ncounters+1],"USECS");
	counters->nvalues = counters->ncounters + 2;

	zero_papi_counters(counters);
}

void zero_papi_counters(CounterGroup *counters)
{
	int i;
	for (i=0; i<counters->nvalues; i++) counters->values[i] = 0;
}

void start_papi_counters(CounterGroup *counters)
{
	int status = PAPI_start_counters(counters->events, counters->ncounters);
	if (status != PAPI_OK) ERROR(PAPI_strerror(status));

	counters->values[counters->ncounters+0] = PAPI_get_real_cyc();
	counters->values[counters->ncounters+1] = PAPI_get_real_usec();
}

void stop_papi_counters(CounterGroup *counters)
{
	int i, status;
	long long val;
	
	status = PAPI_stop_counters(counters->values, counters->ncounters);
	if (status != PAPI_OK) ERROR(PAPI_strerror(status));

	counters->values[counters->ncounters+0] =
		PAPI_get_real_cyc() - counters->values[counters->ncounters+0];
	counters->values[counters->ncounters+1] =
		PAPI_get_real_usec() - counters->values[counters->ncounters+1];
}

void print_papi_counters(const char* prefix, CounterGroup *counters)
{
	int i;
	for (i=0; i<counters->nvalues; i++) {
		printf("%s\t%s\t%lld\n",
			prefix, counters->names[i], counters->values[i]);
	}
}

