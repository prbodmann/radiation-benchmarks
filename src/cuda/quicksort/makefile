CC=g++
NVCC=nvcc

NVIDIA_INC=/usr/local/cuda/include/
NVIDIA_LIB=/usr/local/cuda/lib64/
LOGHELPER_INC=../../include/
LOGHELPER_LIB=../../include/

all: quicksort

quicksort: cdpAdvancedQuicksort.cu cdpBitonicSort.cu
	$(NVCC) $(logs) $(NVCC_FLAGS) cdpAdvancedQuicksort.cu cdpBitonicSort.cu -rdc=true -arch=sm_35 -I$(LOGHELPER_INC) -L$(LOGHELPER_LIB) $(ITER) -lLogHelper -DLOGS $(NVCCFLAGS) -Xcompiler "-O3 -fopenmp" -o quicksort

clean:
	rm -rf quicksort quicksort_*
