# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 2.8

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list

# Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/carol/vinicius/radiation-benchmarks/src/heterogeneous/opencl

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/carol/vinicius/radiation-benchmarks/src/heterogeneous/opencl

# Utility rule file for src_check.

# Include the progress variables for this target.
include src/CMakeFiles/src_check.dir/progress.make

src/CMakeFiles/src_check:

src_check: src/CMakeFiles/src_check
src_check: src/CMakeFiles/src_check.dir/build.make
.PHONY : src_check

# Rule to build all files generated by this target.
src/CMakeFiles/src_check.dir/build: src_check
.PHONY : src/CMakeFiles/src_check.dir/build

src/CMakeFiles/src_check.dir/clean:
	cd /home/carol/vinicius/radiation-benchmarks/src/heterogeneous/opencl/src && $(CMAKE_COMMAND) -P CMakeFiles/src_check.dir/cmake_clean.cmake
.PHONY : src/CMakeFiles/src_check.dir/clean

src/CMakeFiles/src_check.dir/depend:
	cd /home/carol/vinicius/radiation-benchmarks/src/heterogeneous/opencl && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/carol/vinicius/radiation-benchmarks/src/heterogeneous/opencl /home/carol/vinicius/radiation-benchmarks/src/heterogeneous/opencl/src /home/carol/vinicius/radiation-benchmarks/src/heterogeneous/opencl /home/carol/vinicius/radiation-benchmarks/src/heterogeneous/opencl/src /home/carol/vinicius/radiation-benchmarks/src/heterogeneous/opencl/src/CMakeFiles/src_check.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/CMakeFiles/src_check.dir/depend
