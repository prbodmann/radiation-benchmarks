set(CMAKE_SUPPRESS_REGENERATION TRUE)
cmake_minimum_required( VERSION 2.6.0 )

set( FOLDER_NAME kmeans_inter_beam )
set( SUBDIRECTORIES )
set( SRC_FILES 
  main.cc
  kmeans_benchmark.h
  kmeans_benchmark.cc
)
set( HEADER_FILES )
set( TEST_FILES )

# Group samples by folder
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set( FOLDER_GROUP ${FOLDER_GROUP}/${FOLDER_NAME} )
file( COPY kernels.cl DESTINATION . )

# Check if snack is available
find_file(have_snack NAMES snack.sh DOC "snack.sh file") 

# Check if HSA runtime is available
find_library(HSA_RUNTIME 
  NAMES hsa-runtime64
  PATHS $ENV{HSA_RUNTIME_PATH}/lib
  NO_DEFAULT_PATH)

# Style check
set( check_target ${FOLDER_NAME}_check )
set( check_target_src ${SRC_FILES} ${HEADER_FILES} ${TEST_FILES})
add_style_check_target(${check_target} "${check_target_src}" "${SUBDIRECTORIES}")

# Unit test
if (${TEST_FILES})
  add_executable(${FOLDER_NAME}_unittest ${TEST_FILES} ${SRC_FILES} ${HEADER_FILES})
  target_link_libraries(${FOLDER_NAME}_unittest gtest gtest_main)
  add_test(${FOLDER_NAME}_unittest ${FOLDER_NAME}_unittest)
endif (${TEST_FILES})

###############################################################

if (have_snack AND HSA_RUNTIME)
  #add_custom_command(
  #  OUTPUT kernels.o kernels.h
  #  COMMAND snack.sh -c kernels.cl
  #  DEPENDS kernels.cl
  #  COMMENT "Using snack to generate kernel library"
  #  WORKING_DIRECTORY ""
  #  VERBATIM
  #)
  add_executable(${FOLDER_NAME} kernels.h kernels.o ${HEADER_FILES} ${SRC_FILES})

#LOGS
if(LOGS)
  set_target_properties(${FOLDER_NAME} PROPERTIES COMPILE_DEFINITIONS "LOGS=1")
endif(LOGS)
 
  target_link_libraries(${FOLDER_NAME}
      time_measurement command_line_option benchmark logHelper ${HSA_RUNTIME})
else (have_snack AND HSA_RUNTIME)
  message(WARNING "Skipping ${FOLDER_NAME}, snack.sh is not found")
endif (have_snack AND HSA_RUNTIME)

