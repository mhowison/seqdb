/*
 * SeqDB - storage model for Next Generation Sequencing data
 *
 * Copyright 2011-2012, Brown University, Providence, RI. All Rights Reserved.
 *
 * This file is part of SeqDB.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose other than its incorporation into a
 * commercial product is hereby granted without fee, provided that the
 * above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Brown University not be used in
 * advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.
 *
 * BROWN UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR ANY
 * PARTICULAR PURPOSE.  IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE FOR
 * ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <iostream>
#include <vector>
#include "seqdb.h"

#define PROGNAME "seqdb-mount"
#include "util.h"

using namespace std;

void print_usage()
{
	cout << "\n"
"usage: "PROGNAME" SEQDB MOUNT\n"
"\n"
"Outputs the SEQDB file in FASTQ format at the specified MOUNT path."
"\n"
"A named pipe is created at the MOUNT path with the mkfifo system call."
"Then the contents of the SEQDB file are streamed to the pipe in FASTQ format."
"\n"
"This process must remain active while the mount is in use.\n"
"\n"
"Example usage:\n"
PROGNAME" 1.seqdb /tmp/1.fq\n"
	<< endl;
}

static const char* mnt_path = NULL;
static FILE* mnt_out = NULL;

static void unmount()
{
	NOTIFY("unmounting " << mnt_path)

	if (mnt_out != NULL) {
		errno = 0;
		fclose(mnt_out);
		if (errno != 0)
			PERROR("cannot close mount point " << mnt_path)
	}

	errno = 0;
	unlink(mnt_path);
	if (errno != 0)
		PERROR("cannot unlink mount point " << mnt_path)
}

void handler(int signal)
{
	switch (signal) {
		case SIGHUP:
			NOTIFY("caught SIGHUP")
			break;
		case SIGINT:
			NOTIFY("caught SIGINT")
			break;
		case SIGKILL:
			NOTIFY("caught SIGKILL")
			break;
		case SIGPIPE:
			NOTIFY("caught SIGPIPE")
			break;
		case SIGTERM:
			NOTIFY("caught SIGTERM")
			break;
	}

	unmount();
	exit(EXIT_FAILURE);
}

int main(int argc, char** argv)
{
	int c;
	while ((c = getopt(argc, argv, "hv")) != -1)
		switch (c) {
			case 'v':
				PRINT_VERSION
				return EXIT_SUCCESS;
			case 'h':
			default:
				print_usage();
				return EXIT_SUCCESS;
		}

	if (argc - optind < 2)
		ARG_ERROR("not enough parameters")

	const char* input_path = argv[optind];
	mnt_path = argv[optind+1];

	/* Open the input. */

	NOTIFY("mounting " << input_path << " at " << mnt_path)

	SeqDB* db = SeqDB::open(input_path);

	/* Create the mount point as a FIFO. */

	errno = 0;
	mkfifo(mnt_path, S_IRUSR | S_IWUSR);
	if (errno != 0)
		PERROR("cannot create mount point " << mnt_path)

	/* Setup signal handlers. */

	signal(SIGHUP, &handler);
	signal(SIGINT, &handler);
	signal(SIGKILL, &handler);
	signal(SIGPIPE, &handler);
	signal(SIGTERM, &handler);

	/* Open the mount point. */

	errno = 0;
	int fd = open(mnt_path, O_WRONLY);
	if (errno != 0)
		PERROR("cannot open mount point " << mnt_path)

	errno = 0;
	mnt_out = fdopen(fd, "w");
	if (errno != 0)
		PERROR("cannot open mount point as stream")

	/* Output to the mount point in 1MB buffers. */

	setvbuf(mnt_out, NULL, _IOFBF, 1024*1024);

	db->exportFASTQ(mnt_out);

	/* Cleanup. */

	NOTIFY("reached end of file")

	delete db;
	unmount();
	
	return EXIT_SUCCESS;
}

