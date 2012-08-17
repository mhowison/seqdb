#!/usr/bin/env python

import subprocess
import numpy

fastq = "/users/mhowison/scratch/agalma-old/preassembly/HWI-ST625:73:C0JUVACXX:7:TTAGGC/273/HWI-ST625:73:C0JUVACXX:7:TTAGGC.1.fastq"
ntrials = 10
nreads = [10**i for i in xrange(3,8)] + [512, 4096, 16384, 262144, 4194304, 33554432]
data = dict()

with open('benchmem.raw.log', 'w') as f:
	for i in xrange(ntrials):
		for j, n in enumerate(nreads):
			output = subprocess.check_output(['./benchmem', fastq, str(n)])
			f.write(output)
			for line in output.split('\n'):
				if line.startswith('time_'):
					col = line[5:].split('\t')
					if not col[0] in data:
						data[col[0]] = numpy.zeros((len(nreads), ntrials))
					data[col[0]][j][i] = float(col[1])

keys = sorted(data.keys())

with open('benchmem.time.txt', 'w') as f:
	f.write("sequences\ttrial\t")
	f.write('\t'.join(keys))
	f.write('\n')
	for j, n in enumerate(nreads):
		for i in xrange(ntrials):
			f.write("%d\t%d\t" % (n, i))
			f.write('\t'.join(['%g' % (data[k][j][i]) for k in keys]))
			f.write('\n')

with open('benchmem.time.min.txt', 'w') as f:
	f.write("sequences\t")
	f.write('\t'.join(keys))
	f.write('\n')
	for j, n in enumerate(nreads):
		f.write("%d\t" % n)
		f.write('\t'.join(['%g' % (data[k][j].min()) for k in keys]))
		f.write('\n')

