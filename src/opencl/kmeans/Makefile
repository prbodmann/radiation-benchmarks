CC = g++
CC_FLAGS = -g -O2 

NVIDIA_INC=-I/usr/local/cuda/include/
NVIDIA_LIB=-L/usr/local/cuda/lib64/

INTEL_INC=-I/opt/intel/opencl-1.2-sdk-4.6.0.92/include/
INTEL_LIB=-L/opt/intel/opencl-1.2-4.5.0.8/lib64/

AMD_INC=-I/opt/AMDAPPSDK-3.0-0-Beta/include/
AMD_LIB=-L/opt/AMDAPPSDK-3.0-0-Beta/lib/x86_64/


SOURCES=cluster.c  kmeans.c  kmeans_clustering.c
SOURCES_LOGS=cluster.c  kmeans.c  kmeans_clustering.c ../../include/log_helper.c
SOURCES_GEN=cluster.c  kmeans_clustering.c  kmeans_generate.c


amd: $(SOURCES_GEN) $(SOURCES)
	$(CC) $(KERNEL_DIM) $(CC_FLAGS) $(SOURCES_GEN) $(AMD_INC) $(AMD_LIB) -o kmeans_gen -lOpenCL
	$(CC) $(KERNEL_DIM) $(CC_FLAGS) $(SOURCES) $(AMD_INC) $(AMD_LIB) -o kmeans_nologs -lOpenCL
	$(CC) $(KERNEL_DIM) $(CC_FLAGS) -DLOGS=1 $(SOURCES_LOGS) $(AMD_INC) $(AMD_LIB) -o kmeans -lOpenCL
                                                                                                     
intel: $(SOURCES_GEN) $(SOURCES)                                                                                
	$(CC) $(KERNEL_DIM) $(CC_FLAGS) -lOpenCL $(SOURCES_GEN) $(INTEL_INC) $(INTEL_LIB) -o kmeans_gen 
	$(CC) $(KERNEL_DIM) $(CC_FLAGS) -lOpenCL $(SOURCES) $(INTEL_INC) $(INTEL_LIB) -o kmeans_nologs
	$(CC) $(KERNEL_DIM) $(CC_FLAGS) -lOpenCL -DLOGS=1 $(SOURCES_LOGS) $(INTEL_INC) $(INTEL_LIB) -o kmeans
                                                                                                     
nvidia: $(SOURCES_GEN) $(SOURCES)                                                                         
	$(CC) $(KERNEL_DIM) $(CC_FLAGS) $(SOURCES_GEN) $(NVIDIA_INC) $(NVIDIA_LIB) -lOpenCL -o kmeans_gen
	$(CC) $(KERNEL_DIM) $(CC_FLAGS) $(SOURCES) $(NVIDIA_INC) $(NVIDIA_LIB) -lOpenCL -o kmeans_nologs
	$(CC) $(KERNEL_DIM) $(CC_FLAGS) -DLOGS=1 $(SOURCES_LOGS) $(NVIDIA_INC) $(NVIDIA_LIB) -lOpenCL -o kmeans

clean:
	rm -f *.o *~ kmeans  kmeans_gen input_* output_*
