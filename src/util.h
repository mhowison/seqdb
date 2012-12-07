#ifndef __UTIL_H__
#define __UTIL_H__

#include <errno.h>
#include <stdlib.h>
#include <sys/time.h>
#include <iostream>
#include "../config.h"

/* workaround to get variable as a string */
#define STRINGIFY(x) #x

#define NOTIFY(msg) cerr << PROGNAME": " << msg << endl;
#define ERROR(msg) do{NOTIFY(msg) exit(EXIT_FAILURE);}while(0);
#define PERROR(msg) do{\
	NOTIFY(msg) perror(PROGNAME); exit(EXIT_FAILURE);\
}while(0);
#define ARG_ERROR(msg) do{NOTIFY(msg) print_usage(); exit(EXIT_FAILURE);}while(0);
#define CHECK_ERR(call) do{\
	errno = 0;\
	call;\
	if (errno > 0) PERROR("error at " << __FILE__ << ":" << __LINE__)\
}while(0);
#define CHECK_ALLOC(buf) do{\
	if (buf == NULL) ERROR("out of memory at " << __FILE__ << ":" << __LINE__)\
}while(0);

#define PRINT_VERSION cout << "SeqDB " << PACKAGE_VERSION << endl;

#endif
