CC=g++
NVCC=/usr/local/cuda/bin/nvcc
PRECISION?=single # fazer uma precision para C
					
LOWERCASED_PRECISION=$(shell echo $(PRECISION) | tr A-Z a-z)

ARCH= 	-gencode arch=compute_60,code=[sm_60,compute_60]	# Tesla P100
ARCH+= 	-gencode arch=compute_61,code=[sm_61,compute_61]	# Titan X | Titan Xp
ARCH+= 	-gencode arch=compute_62,code=[sm_62,compute_62]	# Tegra X2
ARCH+= 	-gencode arch=compute_70,code=[sm_70,compute_70]	# Titan V | Tesla V100

NVCC_FLAGS= $(ARCH)

NVIDIA_INC=/usr/local/cuda/include/ -I/usr/local/cuda/samples/common/inc
NVIDIA_LIB=/usr/local/cuda/lib64/
LOGHELPER_INC=../../include/
LOGHELPER_LIB=../../include/

SOURCES=./cuda_tensorcore_gemm.cu
EXEC_RADIX=./cuda_tensorcore_gemm_
EXEC=$(EXEC_RADIX)$(LOWERCASED_PRECISION)

DEF_SIZE=1024

DEF_A_INPUT=./mxm_$(LOWERCASED_PRECISION)_A_8192.matrix
DEF_B_INPUT=./mxm_$(LOWERCASED_PRECISION)_B_8192.matrix
DEF_C_INPUT=./mxm_$(LOWERCASED_PRECISION)_C_8192.matrix
DEF_GOLD=./mxm_$(LOWERCASED_PRECISION)_GOLD_$(DEF_TEST_SIZE).matrix

MATRIX_NAMES=$(DEF_A_INPUT) $(DEF_B_INPUT) $(DEF_C_INPUT) $(DEF_GOLD) ./mxm*.matrix

all: $(EXEC)

check-benchtype:
	echo "syntax: make PRECISION=<double|half|single>"
	test $(PRECISION)

$(EXEC): check-benchtype $(RUN_SOURCES)
	$(NVCC) -I$(NVIDIA_INC) -I$(LOGHELPER_INC) -L$(NVIDIA_LIB) -L$(LOGHELPER_LIB) -DLOGS -Dtest_precision_$(LOWERCASED_PRECISION) -O3 -lLogHelper -std=c++11 -lcublas $(NVCC_FLAGS) -Xcompiler "-fopenmp -O3" $(LOGS) $(SOURCES) -o $(EXEC)

clean:
	rm -rf $(EXEC_RADIX)* $(MATRIX_NAMES)

generate: $(EXEC)
	$(EXEC) -generate -size=$(DEF_SIZE) -input_a=$(DEF_A_INPUT) -input_b=$(DEF_B_INPUT) -input_c=$(DEF_C_INPUT) -gold=$(DEF_GOLD) 

test: $(EXEC) generate
	$(EXEC) -size=$(DEF_SIZE) -input_a=$(DEF_A_INPUT) -input_b=$(DEF_B_INPUT) -input_c=$(DEF_C_INPUT) -gold=$(DEF_GOLD) -iterations=1000 -verbose 