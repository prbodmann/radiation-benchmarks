cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
# --- custom targets: ---
if (NOT TARGET distclean)
   INCLUDE( cmake/Modules/TargetDistclean.cmake OPTIONAL)
endif (NOT TARGET distclean)

# enable @rpath in the install name for any shared library being built
set(CMAKE_MACOSX_RPATH 1)

project (MESH)
# The version number.
set (MESH_VERSION_MAJOR 2)
set (MESH_VERSION_MINOR 0.7)
set (MESH_VERSION "${MESH_VERSION_MAJOR}.${MESH_VERSION_MINOR}")

if (NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif(NOT CMAKE_BUILD_TYPE)

# uncomment to build debug
#set(CMAKE_BUILD_TYPE Debug)

if ("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
   # using Clang
   set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ftree-vectorize")
   #set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ftree-vectorize -ftree-vectorizer-verbose=5")
elseif ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
   # using GCC
   set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ftree-vectorize")
   #set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ftree-vectorize -ftree-vectorizer-verbose=5")
elseif ("${CMAKE_C_COMPILER_ID}" STREQUAL "Intel")
   # using Intel C++
   #set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -vec-report3")
elseif ("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
   # using Visual Studio C++
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
   # using Clang
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftree-vectorize")
   #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftree-vectorize -ftree-vectorizer-verbose=5")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
   # using GCC
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftree-vectorize")
   #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftree-vectorize -ftree-vectorizer-verbose=5")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
   # using Intel C++
   #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -vec-report3")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
   # using Visual Studio C++
endif()

if (MIC_NATIVE)
   set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static-intel -std=c99 -mmic")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-intel -mmic")
   link_directories("/projects/opt/intel/compilers/parallel_studio_xe_2013/composer_xe_2013_sp1.2.144/compiler/lib/mic")
else (MIC_NATIVE)
#  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")
   set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
endif (MIC_NATIVE)

if (APPLE AND "${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=native -mtune=native -Wa,-q")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -mtune=native -Wa,-q")
else (APPLE AND "${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=native -mtune=native")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -mtune=native")
endif (APPLE AND "${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -g -O0 -Wextra")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -g -O0 -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g -O3")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -g -O3")

#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fno-alias -restrict -opt_report")

#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -std=c++11 -pedantic -Wc++11-long-long -Wno-overlength-strings")
#set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -pedantic")

include_directories(${PROJECT_SOURCE_DIR})

if (NOT DEFINED OPENCL_FOUND AND NOT MIC_NATIVE)
   find_package(OpenCL)
   #  OPENCL_FOUND        : TRUE if found, FALSE otherwise
   #  OPENCL_INCLUDE_DIRS : Include directories for OpenCL
   #  OPENCL_LIBRARIES    : The libraries to link against
   if (OPENCL_FOUND)
      include_directories(${OPENCL_INCLUDE_DIRS})
   endif (OPENCL_FOUND)
endif (NOT DEFINED OPENCL_FOUND AND NOT MIC_NATIVE)

if (NOT DEFINED MPI_FOUND)
   find_package(MPI)
endif (NOT DEFINED MPI_FOUND)
if (MPI_FOUND)
   include_directories(${MPI_INCLUDE_PATH})
endif (MPI_FOUND)

if (NOT DEFINED OPENMP_FOUND)
   find_package( OpenMP)
endif (NOT DEFINED OPENMP_FOUND)

if (NOT PRECISION_TYPE)
   set (PRECISION_TYPE full_precision)
endif (NOT PRECISION_TYPE)

if (PRECISION_TYPE STREQUAL "mixed_precision")
   set (MIXED_PRECISION on)
   set (PRECISION_TYPE mixed_precision)
   add_definitions(-DMIXED_PRECISION)
endif (PRECISION_TYPE STREQUAL "mixed_precision")

if (PRECISION_TYPE STREQUAL "full_precision")
   set (FULL_PRECISION on)
   set (PRECISION_TYPE full_precision)
   add_definitions(-DFULL_PRECISION)
endif (PRECISION_TYPE STREQUAL "full_precision")

if (PRECISION_TYPE STREQUAL "minimum_precision")
   set (MINIMUM_PRECISION on)
   set (PRECISION_TYPE minimum_precision)
   add_definitions(-DMINIMUM_PRECISION)
endif (PRECISION_TYPE STREQUAL "minimum_precision")

enable_testing()

add_subdirectory(kdtree)
add_subdirectory(hsfc)
add_subdirectory(zorder)
add_subdirectory(tests)

########### global settings ###############

set(H_SRCS mesh.h partition.h reduce.h)

set(CXX_SRCS mesh.cpp partition.cpp)

set(C_SRCS reduce.c)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

########### embed source target ###############
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/mesh_kernel.inc
                  COMMAND ${CMAKE_SOURCE_DIR}/embed_source.pl ${CMAKE_CURRENT_LIST_DIR}/mesh_kern.cl > ${CMAKE_CURRENT_BINARY_DIR}/mesh_kernel.inc
                  DEPENDS ${CMAKE_CURRENT_LIST_DIR}/mesh_kern.cl ${CMAKE_SOURCE_DIR}/embed_source.pl)
add_custom_target(mesh_kernel_source ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/mesh_kernel.inc)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/reduce_kernel.inc
                  COMMAND ${CMAKE_SOURCE_DIR}/embed_source.pl ${CMAKE_CURRENT_LIST_DIR}/reduce.cl > ${CMAKE_CURRENT_BINARY_DIR}/reduce_kernel.inc
                  DEPENDS ${CMAKE_CURRENT_LIST_DIR}/reduce.cl ${CMAKE_SOURCE_DIR}/embed_source.pl)
add_custom_target(reduce_kernel_source ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/reduce_kernel.inc)

########### mesh target ###############
set(mesh_LIB_SRCS ${CXX_SRCS} ${C_SRCS} ${H_SRCS})

add_library(mesh STATIC ${mesh_LIB_SRCS})

set_target_properties(mesh PROPERTIES VERSION ${MESH_VERSION} SOVERSION ${MESH_VERSION_MAJOR})

target_link_libraries(mesh hsfc kdtree zorder)

install(TARGETS mesh DESTINATION lib)

########### mpmesh target ###############
if(OPENMP_FOUND)
   set(mpmesh_LIB_SRCS ${CXX_SRCS} ${C_SRCS} ${H_SRCS})

   add_library(mpmesh STATIC ${mesh_LIB_SRCS})

   set_target_properties(mpmesh PROPERTIES VERSION ${MESH_VERSION} SOVERSION ${MESH_VERSION_MAJOR})
   set_target_properties(mpmesh PROPERTIES COMPILE_FLAGS ${OpenMP_C_FLAGS})
   set_target_properties(mpmesh PROPERTIES COMPILE_FLAGS ${OpenMP_CXX_FLAGS})
   set_target_properties(mpmesh PROPERTIES LINK_FLAGS ${OpenMP_C_FLAGS})

   target_link_libraries(mpmesh hsfc kdtree zorder)

   install(TARGETS mpmesh DESTINATION lib)
endif(OPENMP_FOUND)

########### pmesh target ###############
if (MPI_FOUND)
   set(pmesh_LIB_SRCS ${CXX_SRCS} ${C_SRCS} ${H_SRCS})

   add_library(pmesh STATIC ${pmesh_LIB_SRCS})

   set_target_properties(pmesh PROPERTIES VERSION ${MESH_VERSION} SOVERSION ${MESH_VERSION_MAJOR})
   set_target_properties(pmesh PROPERTIES COMPILE_DEFINITIONS HAVE_MPI)

   target_link_libraries(pmesh hsfc kdtree zorder)
   target_link_libraries(pmesh ${MPI_LIBRARIES})

   install(TARGETS pmesh DESTINATION lib)
endif (MPI_FOUND)

########### pmpmesh target ###############
if(OPENMP_FOUND AND MPI_FOUND)
   set(pmpmesh_LIB_SRCS ${CXX_SRCS} ${C_SRCS} ${H_SRCS})

   add_library(pmpmesh STATIC ${mesh_LIB_SRCS})

   set_target_properties(pmpmesh PROPERTIES VERSION ${MESH_VERSION} SOVERSION ${MESH_VERSION_MAJOR})
   set_target_properties(pmpmesh PROPERTIES COMPILE_DEFINITIONS HAVE_MPI)
   set_target_properties(pmpmesh PROPERTIES COMPILE_FLAGS ${OpenMP_C_FLAGS})
   set_target_properties(pmpmesh PROPERTIES COMPILE_FLAGS ${OpenMP_CXX_FLAGS})
   set_target_properties(pmpmesh PROPERTIES LINK_FLAGS ${OpenMP_C_FLAGS})

   target_link_libraries(pmpmesh hsfc kdtree zorder)
   target_link_libraries(pmpmesh ${MPI_LIBRARIES})

   add_dependencies(pmpmesh reduce_kernel_source)
   install(TARGETS pmpmesh DESTINATION lib)
endif(OPENMP_FOUND AND MPI_FOUND)

########### dmesh target ###############
if (OPENCL_FOUND)
   set(dmesh_LIB_SRCS ${CXX_SRCS} ${C_SRCS} ${H_SRCS})

   add_library(dmesh STATIC ${dmesh_LIB_SRCS})

   set_target_properties(dmesh PROPERTIES VERSION ${MESH_VERSION} SOVERSION ${MESH_VERSION_MAJOR})
   set_target_properties(dmesh PROPERTIES COMPILE_DEFINITIONS HAVE_OPENCL)

   target_link_libraries(dmesh hsfc kdtree zorder)
   target_link_libraries(dmesh ${OPENCL_LIBRARIES})

   add_dependencies(dmesh mesh_kernel_source)
   add_dependencies(dmesh reduce_kernel_source)
   install(TARGETS dmesh DESTINATION lib)
endif (OPENCL_FOUND)

########### dpmesh target ###############
if(OPENCL_FOUND AND MPI_FOUND)
   set(dpmesh_LIB_SRCS ${CXX_SRCS} ${C_SRCS} ${H_SRCS})

   add_library(dpmesh STATIC ${dpmesh_LIB_SRCS})

   set_target_properties(dpmesh PROPERTIES VERSION ${MESH_VERSION} SOVERSION ${MESH_VERSION_MAJOR})
   set_target_properties(dpmesh PROPERTIES COMPILE_DEFINITIONS "HAVE_MPI;HAVE_OPENCL")

   target_link_libraries(dpmesh hsfc kdtree zorder)
   target_link_libraries(dpmesh ${MPI_LIBRARIES})
   target_link_libraries(dpmesh ${OPENCL_LIBRARIES})

   add_dependencies(dpmesh mesh_kernel_source)
   add_dependencies(dpmesh reduce_kernel_source)
   install(TARGETS dpmesh DESTINATION lib)
endif (OPENCL_FOUND AND MPI_FOUND)

########### install files ###############

install(FILES  mesh.h DESTINATION include/mesh)
install(FILES  kdtree/KDTree.h DESTINATION include/kdtree)
install(FILES  kdtree/Globals.h DESTINATION include)
install(FILES  kdtree/Bounds.h DESTINATION include)
install(FILES  partition.h DESTINATION include/mesh)

########### doc targets ################

if (${CMAKE_PROJECT_NAME} MATCHES ${PROJECT_NAME})
   add_custom_target(doc
                     COMMAND make pdfdoc
                     COMMAND make htmldoc
                     COMMAND make singlehtmldoc)
   set_target_properties(doc PROPERTIES EXCLUDE_FROM_ALL TRUE)

   add_custom_command(OUTPUT _build/latex/Mesh.pdf
                      WORKING_DIRECTORY docs
                      COMMAND make latexpdf >& pdfdoc.out
                      DEPENDS ${PDFDOC_SRCS})
   add_custom_target(pdfdoc DEPENDS _build/latex/Mesh.pdf)
   set_target_properties(pdfdoc PROPERTIES EXCLUDE_FROM_ALL TRUE)

   add_custom_command(OUTPUT _build/html/UsersGuide.html
                      WORKING_DIRECTORY docs
                      COMMAND make html >& htmldoc.out
                      DEPENDS ${DOC_SRCS})
   add_custom_target(htmldoc DEPENDS _build/html/UsersGuide.html)
   set_target_properties(htmldoc PROPERTIES EXCLUDE_FROM_ALL TRUE)

   add_custom_command(OUTPUT _build/singlehtml/index.html
                      WORKING_DIRECTORY docs
                      COMMAND make singlehtml >& singlehtmldoc.out
                      DEPENDS ${DOC_SRCS})
   add_custom_target(singlehtmldoc DEPENDS _build/singlehtml/index.html)
   set_target_properties(singlehtmldoc PROPERTIES EXCLUDE_FROM_ALL TRUE)
else (${CMAKE_PROJECT_NAME} MATCHES ${PROJECT_NAME})
   add_custom_target(Mesh_doc
                     COMMAND make Mesh_pdfdoc
                     COMMAND make Mesh_htmldoc
                     COMMAND make Mesh_singlehtmldoc)
   set_target_properties(Mesh_doc PROPERTIES EXCLUDE_FROM_ALL TRUE)

   add_custom_command(OUTPUT _build/latex/Mesh.pdf
                      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/docs
                      COMMAND make latexpdf >& pdfdoc.out
                      DEPENDS ${PDFDOC_SRCS})
   add_custom_target(Mesh_pdfdoc DEPENDS _build/latex/Mesh.pdf)
   set_target_properties(Mesh_pdfdoc PROPERTIES EXCLUDE_FROM_ALL TRUE)

   add_custom_command(OUTPUT _build/html/UsersGuide.html
                      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/docs
                      COMMAND make html >& htmldoc.out
                      DEPENDS ${DOC_SRCS})
   add_custom_target(Mesh_htmldoc DEPENDS _build/html/UsersGuide.html)
   set_target_properties(Mesh_htmldoc PROPERTIES EXCLUDE_FROM_ALL TRUE)

   add_custom_command(OUTPUT _build/singlehtml/index.html
                      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/docs
                      COMMAND make singlehtml >& singlehtmldoc.out
                      DEPENDS ${DOC_SRCS})
   add_custom_target(Mesh_singlehtmldoc DEPENDS _build/singlehtml/index.html)
   set_target_properties(Mesh_singlehtmldoc PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif (${CMAKE_PROJECT_NAME} MATCHES ${PROJECT_NAME})

########### clean files ################
SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES
   "mesh_kernel.inc;reduce_kernel.inc;install_manifest.txt;docs/_build;docs/htmldoc.out;docs/pdfdoc.out;docs/singlehtmldoc.out")

