#!/usr/bin/python

import os
import sys
import ConfigParser

print "Generating mergesort for CUDA K40"

confFile = '/etc/radiation-benchmarks.conf'
try:
	config = ConfigParser.RawConfigParser()
	config.read(confFile)

	installDir = config.get('DEFAULT', 'installdir')+"/"
	varDir =  config.get('DEFAULT', 'vardir')+"/"
	logDir =  config.get('DEFAULT', 'logdir')+"/"
	tmpDir =  config.get('DEFAULT', 'tmpdir')+"/"

except IOError as e:
	print >> sys.stderr, "Configuration setup error: "+str(e)
	sys.exit(1)

data_path=installDir+"data/mergesort_h/"
bin_path=installDir+"bin/"
src_mergesort = installDir+"src/cuda/mergesort_h/"

if not os.path.isdir(data_path):
	os.mkdir(data_path, 0777);
	os.chmod(data_path, 0777);

os.system("cd "+src_mergesort)

os.system("make logs=-DLOGS")

os.system("sudo ./mergesort_h -size=1048576 -input=mergesort_input_134217728 -gold=mergesort_gold_1048576 -generate -iterations=1")
os.system("sudo ./mergesort_h -size=33554432 -input=mergesort_input_134217728 -gold=mergesort_gold_33554432 -generate -iterations=1")
os.system("sudo ./mergesort_h -size=134217728 -input=mergesort_input_134217728 -gold=mergesort_gold_134217728 -generate -iterations=1")
os.system("sudo chmod 777 mergesort_*");
os.system("mv mergesort_gold* "+data_path);
os.system("mv mergesort_input* "+data_path);
os.system("mv ./mergesort_h "+bin_path)

fp = open(installDir+"scripts/how_to_run_mergesort_h_cuda_K40", 'w')
print >>fp, "sudo "+bin_path+"/mergesort_h -size=1048576 -input="+data_path+"/mergesort_input_134217728 -gold="+data_path+"/mergesort_gold_1048576 -iterations=10000000"
print >>fp, "sudo "+bin_path+"/mergesort_h -size=33554432 -input="+data_path+"/mergesort_input_134217728 -gold="+data_path+"/mergesort_gold_33554432 -iterations=10000000"
print >>fp, "sudo "+bin_path+"/mergesort_h -size=134217728 -input="+data_path+"/mergesort_input_134217728 -gold="+data_path+"/mergesort_gold_134217728 -iterations=10000000"

print "\nConfiguring done, to run check file: "+installDir+"scripts/how_to_run_mergesort_h_cuda\n"

sys.exit(0)
