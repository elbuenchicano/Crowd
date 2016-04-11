# GCGlib: Script to build GCGlib
#
#  Path for Windows:
#     set path=%path%;"c:\Program Files (x86)\Codeblocks\MinGW\bin";"c:\Program Files (x86)\doxygen\bin";"c:\Program Files (x86)\Git\bin\";"c:\Program Files (x86)\Git\cmd\";"C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin\amd64";
#
#  Configuration examples (Release is the default configuration):
#     cmake .. -G "Unix Makefiles"
#     cmake .. -G "CodeBlocks - Unix Makefiles"
#     cmake .. -G "MinGW Makefiles"
#     cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
#     cmake .. -G "CodeBlocks - MinGW Makefiles"
#     cmake .. -G "Visual Studio 10 2010"
#     cmake .. -G "Visual Studio 10 2010 Win64"
#     cmake .. -G "Visual Studio 10 2010 Win64" -DCMAKE_BUILD_TYPE=Debug
#     cmake .. -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX=./installfolder
#
#  Build example (after configuration):
#     cmake --build . --config Release
#     cmake --build . --config Debug
#
#  Install example (after configuration):
#     cmake --build . --config Release --target install
#
#      or (after building)
#
#     make install
#
#  Create package (after configuration):
#     cmake --build . --config Release --target package
#
#     or (after building)
#
#     cpack -C Release
#
##############################################################


cmake_minimum_required(VERSION 2.8.12) # Building really won't work if it is less than this

# Project name
project(GCGlib CXX C)

####################################################
# GCGlib GCG_ARCHITECTURE and operating system detection
####################################################

# Detect operating system
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(GCG_OPERATING_SYSTEM "Linux")  # Linux specific code
  elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
        set(GCG_OPERATING_SYSTEM "Windows")  # Windows specific code
      elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
            set(GCG_OPERATING_SYSTEM "Mac OS X") # Mac OS X specific code
          else()
            set(GCG_OPERATING_SYSTEM ${CMAKE_SYSTEM_NAME})
endif()

set(WORDSIZE 32)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^i.86$")
    set(GCG_ARCHITECTURE "i686")
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "amd64")
    set(GCG_ARCHITECTURE "amd64")
    set(WORDSIZE 64)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")
    set(GCG_ARCHITECTURE "arm")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES ".*64") # Accounts for ppc64 and sparc64
    string(REGEX REPLACE "64" "" GCG_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
    set(WORDSIZE 64)
else()
    set(GCG_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
endif()

# Discover if it is a 64 bits compiler/build
set(GCG_PLATFORM_X64 FALSE)
if(MSVC)
  if(CMAKE_CL_64)
    set(GCG_PLATFORM_X64 TRUE)
  endif()
else()
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(GCG_PLATFORM_X64 TRUE)
  endif()
endif()

message("GCG_ARCHITECTURE: ${GCG_ARCHITECTURE}/${CMAKE_SYSTEM_PROCESSOR}   OS: ${GCG_OPERATING_SYSTEM}   64-bits compiler: ${GCG_PLATFORM_X64}")


############################################
# GCGlib required packages and libraries
############################################

# Place where external projects should be installed
set(GCG_INSTALL_EXTERNAL_DIR "${CMAKE_BINARY_DIR}")

# Scripts to find Windows SDK, DirectShow, DirectX, GLUI
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/external/cmake)

# Find required packages and libraries: OpenGL
find_package(OpenGL REQUIRED)  # System dependent
if(OPENGL_FOUND)
  include_directories(${OpenGL_INCLUDE_DIR})
else()
  message(FATAL_ERROR "Package OpenGL missing.")
endif()

# Find required packages and libraries: jpeglib
find_package(JPEG QUIET)
if(JPEG_FOUND)
  include_directories(${JPEG_INCLUDE_DIR})
else()
  include(ExternalProject)

  # Include internal version of libjpeg
  ExternalProject_Add(libjpeg
                      SOURCE_DIR  ${CMAKE_SOURCE_DIR}/external/ijpg6b
                      PREFIX      ${CMAKE_BINARY_DIR}/external/jpeg
                      BINARY_DIR  ${CMAKE_BINARY_DIR}/external/jpeg
                      CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX:PATH=${GCG_INSTALL_EXTERNAL_DIR}
                      BUILD_COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/external/jpeg --config ${CMAKE_BUILD_TYPE} --target jpeg
                      INSTALL_DIR   ${GCG_INSTALL_EXTERNAL_DIR} # It is not the same as CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX:PATH=${GCG_INSTALL_EXTERNAL_DIR}
                      INSTALL_COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/external/jpeg --config ${CMAKE_BUILD_TYPE} --target install
                      UPDATE_COMMAND "")

  # Path to headers and library
  include_directories(${GCG_INSTALL_EXTERNAL_DIR}/include)
  link_directories(${GCG_INSTALL_EXTERNAL_DIR}/lib)

  # Set library name and project dependency
  set(JPEG_LIBRARIES jpeg)
  set(GCG_LOCAL_DEPENDENCIES ${GCG_LOCAL_DEPENDENCIES} libjpeg)
endif()

# Find required packages and libraries: pthreads
find_package(Threads)
if(NOT CMAKE_USE_PTHREADS_INIT)
  if(MSVC OR MINGW)
    # Include our local copy
    include_directories(${CMAKE_SOURCE_DIR}/external/pthread/include)

    # Define library name
    if(MSVC)
      set(CMAKE_THREAD_LIBS_INIT "pthreadVC2") # Visual C++ version
    else()
      set(CMAKE_THREAD_LIBS_INIT "pthreadGC2") # MINGW version
    endif()

    # Note that link_directories() only works BEFORE target creation
    if(GCG_PLATFORM_X64)
      link_directories(${CMAKE_SOURCE_DIR}/external/pthread/lib/x64)
      set(GCG_EXTRA_INSTALL_BIN_FILES ${CMAKE_SOURCE_DIR}/external/pthread/bin/x64/${CMAKE_THREAD_LIBS_INIT}.dll)
    else()
      link_directories(${CMAKE_SOURCE_DIR}/external/pthread/lib)
      set(GCG_EXTRA_INSTALL_BIN_FILES ${CMAKE_SOURCE_DIR}/external/pthread/bin/${CMAKE_THREAD_LIBS_INIT}.dll)
    endif()
  else()
    message(FATAL_ERROR "Package pthreads missing.")
  endif()
endif()


# Find required packages and libraries for MSVC: baseclasses
if(MSVC AND GCG_OPERATING_SYSTEM MATCHES "Windows")
  # Find WindowsSDK
  find_package(WindowsSDK)

  if(WINDOWSSDK_FOUND)
    include_directories(${DIRECTSHOW_BASECLASS_DIR} ${DIRECTSHOW_WINDOWSSDK_INCLUDE_DIR})
  endif()

  # Also include our local baseclasses copy
  include_directories(${CMAKE_SOURCE_DIR}/external/baseclasses)

  # Note that link_directories() only works BEFORE target creation
  if(GCG_PLATFORM_X64)
    link_directories(${CMAKE_SOURCE_DIR}/external/baseclasses/x64)
  else()
    link_directories(${CMAKE_SOURCE_DIR}/external/baseclasses)
  endif()
endif()


# Include video API for GCGlib version compiled for use with MinGW
if(MINGW AND GCG_OPERATING_SYSTEM MATCHES "Windows")
  # Check if the vapiwin.dll is already generated
  if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vapiwin/vapiwin.dll")
    include(ExternalProject)
    # This generation is time expensive while building MinGW version
    set(GCG_OUTPUT_VAPIWIN ${CMAKE_CURRENT_SOURCE_DIR}/vapiwin)
    ExternalProject_Add(vapiwin
                        SOURCE_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/vapiwin
                        PREFIX      ${CMAKE_CURRENT_BINARY_DIR}/vapiwin
                        BINARY_DIR  ${CMAKE_CURRENT_BINARY_DIR}/vapiwin
                        CMAKE_GENERATOR "Visual Studio 10 2010"
                        CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX:PATH=${GCG_OUTPUT_VAPIWIN}
                                    -DGCG_ROOT_DIRECTORY:STRING=${CMAKE_SOURCE_DIR}
                        BUILD_COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR}/vapiwin --config Release --target vapiwin
                        INSTALL_DIR ${GCG_OUTPUT_VAPIWIN}   # It is not the same as CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX:PATH=${GCG_INSTALL_BINARY_DIR}
                        INSTALL_COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR}/vapiwin --config Release --target install
                        UPDATE_COMMAND "")
  endif()
endif()


##########################################################################
# GCGlib: include video API for GCGlib version compiled with MinGW
##########################################################################

if(MINGW AND GCG_OPERATING_SYSTEM MATCHES "Windows")
  # Check if the vapiwin.dll is already generated
  if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vapiwin/vapiwin.dll")
    include(ExternalProject)
    # This generation is time expensive while building MinGW version
    set(GCG_OUTPUT_VAPIWIN ${CMAKE_CURRENT_SOURCE_DIR}/vapiwin)
    ExternalProject_Add(vapiwin
                        SOURCE_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/vapiwin
                        PREFIX      ${CMAKE_CURRENT_BINARY_DIR}/vapiwin
                        BINARY_DIR  ${CMAKE_CURRENT_BINARY_DIR}/vapiwin
                        CMAKE_GENERATOR "Visual Studio 10 2010"
                        CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX:PATH=${GCG_OUTPUT_VAPIWIN}
                                    -DGCG_ROOT_DIRECTORY:STRING=${CMAKE_SOURCE_DIR}
                        BUILD_COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR}/vapiwin --config Release --target vapiwin
                        INSTALL_DIR ${GCG_OUTPUT_VAPIWIN}   # It is not the same as CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX:PATH=${GCG_INSTALL_BINARY_DIR}
                        INSTALL_COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR}/vapiwin --config Release --target install
                        UPDATE_COMMAND "")
  endif()

  # Indicate for install vapiwin.dll
  set(GCG_EXTRA_INSTALL_BIN_FILES ${GCG_EXTRA_INSTALL_BIN_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/vapiwin/vapiwin.dll)
endif()


############################################
# GCGlib general setup
############################################

# Find GIT to retrieve version information
find_package(Git)
if(GIT_FOUND)
  # Git found: get last version tag
  execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --tags
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GCG_GIT_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  if(GCG_GIT_VERSION STREQUAL "")
      # Git problem: force version
      set(GCG_GIT_VERSION "0.0.0")
      set(GCG_VERSION_MAJOR 0)
      set(GCG_VERSION_MINOR 0)
      set(GCG_VERSION_PATCH 0)
    else()
      # Git worked: decode version
      string(REGEX MATCH "^([0-9]+)\\.([0-9]+)-([0-9]+)-" GCG_GIT_VERSION ${GCG_GIT_VERSION})
      set(GCG_VERSION_MAJOR ${CMAKE_MATCH_1})
      set(GCG_VERSION_MINOR ${CMAKE_MATCH_2})
      set(GCG_VERSION_PATCH ${CMAKE_MATCH_3})
  endif()

  # Using Git, get change log
  # ATTENTION only commit messages starting with "Changelog:" will be added to the log
  set(PRETTYFORMAT "* %ad: - %s%n") # To avoid damn CMake warning
  execute_process(
    COMMAND ${GIT_EXECUTABLE} log ${GCG_VERSION_MAJOR}.${GCG_VERSION_MINOR}..HEAD --pretty=format:${PRETTYFORMAT} --grep=^Changelog:
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GCG_CHANGE_LOG
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  if(NOT GCG_CHANGE_LOG STREQUAL "")
      # Process the log text: remove any substrings of hour OR timezone OR Changelog: mark
      string(REGEX REPLACE " ..:..:..| [-|+]....[:|.:]|Changelog:[ *]" "" GCG_CHANGE_LOG ${GCG_CHANGE_LOG})
  endif()
else()
  message("Git unavailable: current revision could not be retrieved.")

  # Could not retrieve version information: use generic
  set(GCG_VERSION_MAJOR 0)
  set(GCG_VERSION_MINOR 0)
  set(GCG_VERSION_PATCH 0)
  set(GCG_CHANGE_LOG "This version was built without information from GCGlib GIT repository.")
endif(GIT_FOUND)

# Set general identification of GCGlib
set(PACKAGE_NAME "GCG")
set(PACKAGE_VERSION ${GCG_VERSION_MAJOR}.${GCG_VERSION_MINOR}.${GCG_VERSION_PATCH})
set(GCG_VERSION ${PACKAGE_VERSION})
# Set versions: SOVERSION is not necessarily the major version. It must be incremented when the
# library breaks compatibility of existing functions with older versions.
set(GCG_SOVERSION ${GCG_VERSION_MAJOR}) # Used in Linux and Windows
set(PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(PACKAGE_BUGREPORT "http://www.gcg.ufjf.br/gcglib/bugs/")

# Get current date as building date
string(TIMESTAMP GCG_BUILD_DATE "%Y-%m-%d %H:%M:%S UTC" UTC)

# Get current year
string(TIMESTAMP CURRENT_YEAR "%Y" UTC)

############################################
# Extracting information from gcg.h to
# generate LICENSE and README files.
############################################

# Set license output file
if(GCG_OPERATING_SYSTEM MATCHES "Linux")
  set(GCG_LICENSE_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/copyright)
else()
  set(GCG_LICENSE_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/LICENSE.txt)
endif()

# Load header file gcg.h
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/gcg.h GCG_HEADER_FILE)
string(REGEX REPLACE "2007-...." "2007-${CURRENT_YEAR}" GCG_HEADER_FILE ${GCG_HEADER_FILE}) # Update year

# Extracts the documentation portion of gcg.h
string(REGEX MATCH "\\\\section introduction(.+)\\**///" GCG_HEADER_FILE ${GCG_HEADER_FILE}) # Extract README documentation
string(REGEX REPLACE "([^\n])\n([^\n -])" "\\1 \\2" GCG_HEADER_FILE ${CMAKE_MATCH_1}) # Compact paragraphs
#string(REGEX REPLACE "[ \t]+" " " GCG_HEADER_FILE ${GCG_HEADER_FILE}) # Remove extra spaces
string(REGEX REPLACE "\n\n[\n]+" "\n\n" GCG_HEADER_FILE ${GCG_HEADER_FILE}) # More than two lines reduce to two
string(REGEX REPLACE "\\\\verbatim[\n ]*|[\n ]*\\\\endverbatim" "" GCG_HEADER_FILE ${GCG_HEADER_FILE}) # Remove doxygen commands

# Process LICENSE information extracted from gcg.h
string(REGEX MATCH "\\\\section license(.+)\\\\section reference" GCG_LICENSE ${GCG_HEADER_FILE})
string(STRIP ${CMAKE_MATCH_1} GCG_LICENSE) # Trim leading and tailing spaces
string(REGEX REPLACE "Graphics Engine" "Graphics Engine (version ${GCG_VERSION})" GCG_LICENSE ${GCG_LICENSE}) # Insert version number

# Save LICENSE file
file(WRITE ${GCG_LICENSE_OUTPUT} ${GCG_LICENSE}) # Compact form of license

# Process README information extracted from gcg.h
string(STRIP ${GCG_HEADER_FILE} GCG_HEADER_FILE) # Trim leading and tailing spaces
string(REGEX REPLACE "^[\n ]*Introduction[\n ]*" "UFJF - Universidade Federal de Juiz de Fora\nICE - Instituto de Ciências Exatas\nDCC - Departamento de Ciência da Computação\nGCG - Group for Computer Graphics, Image and Vision (www.gcg.ufjf.br)\n\n-----------------------------------------------------------------------\n1) GCGlib - Graphics Engine (version ${GCG_VERSION})\n\n" GCG_HEADER_FILE ${GCG_HEADER_FILE}) # Insert header
string(REGEX REPLACE "\\\\section runtime[\n ]*" "-----------------------------------------------------------------------\n2) " GCG_HEADER_FILE ${GCG_HEADER_FILE}) # 2nd section
string(REGEX REPLACE "\\\\section vercomp[\n ]*" "-----------------------------------------------------------------------\n3) " GCG_HEADER_FILE ${GCG_HEADER_FILE}) # 3rd section
string(REGEX REPLACE "\\\\section building[\n ]*" "-----------------------------------------------------------------------\n4) " GCG_HEADER_FILE ${GCG_HEADER_FILE}) # 4th section
string(REGEX REPLACE "\\\\subsection source[\n ]*|\\\\subsection configgcglib[\n ]*|\\\\subsection compilegcglib[\n ]*" "-- " GCG_HEADER_FILE ${GCG_HEADER_FILE}) # Subsections
string(REGEX REPLACE "\\\\section license(.+)\\\\section reference" "-----------------------------------------------------------------------\n5) ${GCG_LICENSE}\n\n-----------------------------------------------------------------------\n6) " GCG_HEADER_FILE ${GCG_HEADER_FILE}) # 5th and 6th sections

# Save README file
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/README.txt ${GCG_HEADER_FILE})

############################################
# Configuring files
############################################

# Generate change log
configure_file(CHANGE.LOG.in  ${CMAKE_CURRENT_BINARY_DIR}/CHANGE.LOG @ONLY)

# Remove leading zeros from the MINOR version to generate version.c
string(REGEX REPLACE "^0+([1-9][0-9]*)$" "\\1" GCG_MINOR_NUMBER ${GCG_VERSION_MINOR})
configure_file(version.c.in  ${CMAKE_CURRENT_BINARY_DIR}/version.c @ONLY)

# Configure icon path
set(GCG_IMAGE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/images)

# Generate resource file: Windows only
if(GCG_OPERATING_SYSTEM MATCHES "Windows")
  configure_file(gcg.rc.in  ${CMAKE_CURRENT_BINARY_DIR}/gcg.rc @ONLY) # Resource for Windows systems

  # Extra sources to be included in library
  set(GCG_EXTRA_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/gcg.rc)
endif()


############################################
# GCGlib target definition
############################################

# Add this directory as include directory
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Define GCGlib sources
set(GCG_SOURCES_C   algebra.c draw.c ${CMAKE_CURRENT_BINARY_DIR}/version.c)
set(GCG_SOURCES_CXX clientserver.cpp compressrle.cpp font.cpp cache.cpp frustum.cpp geometry.cpp system.cpp image.cpp plot.cpp
                    polygon.cpp datastruct.cpp dataordered.cpp datamap.cpp prodcons.cpp semiregular48.cpp statistics.cpp
                    text.cpp texture.cpp thread.cpp signalmain.cpp matrixmain.cpp queue.cpp report.cpp video.cpp)

# Force C files to be compiled with C++ compiler
set_source_files_properties(${GCG_SOURCES_C} PROPERTIES LANGUAGE CXX)

# Add GCGlib library
add_library(gcg SHARED ${GCG_SOURCES_C} ${GCG_SOURCES_CXX} ${GCG_EXTRA_SOURCES} gcg.h system.h version.c.in CHANGE.LOG.in README.txt.in)
set_target_properties(gcg PROPERTIES PUBLIC_HEADER gcg.h) # Set main header file

# Set local project dependencies
if(DEFINED GCG_LOCAL_DEPENDENCIES)
  add_dependencies(gcg ${GCG_LOCAL_DEPENDENCIES})
endif()

# Set library dependencies
target_link_libraries(gcg PRIVATE ${OPENGL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${JPEG_LIBRARIES})

# Set versions: SOVERSION is not necessarily the major version. It must be incremented when the
# library breaks compatibility of existing functions with older versions.
set_target_properties(gcg PROPERTIES VERSION ${GCG_VERSION} SOVERSION ${GCG_SOVERSION})

# Dependencies needed during GCGlib development in Windows
if(MSVC OR MINGW)
  foreach(dep ${GCG_EXTRA_INSTALL_BIN_FILES})
    add_custom_command(TARGET gcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${dep} $<TARGET_FILE_DIR:gcg>)
  endforeach()
endif()

# Get target information
get_target_property(GCG_LIBRARY_NAME gcg NAME)

############################################
# GCGlib documentation
############################################

find_package(Doxygen)
if(DOXYGEN_FOUND)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/doxyfile @ONLY)

  add_custom_command( OUTPUT ${CMAKE_BINARY_DIR}/html/index.html
                      COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/doxyfile
                      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/gcg.h ${CMAKE_CURRENT_SOURCE_DIR}/doxyfile.in ${GCG_IMAGE_PATH}/gcgdoxygen.png
                      WORKING_DIRECTORY ${CMAKE_BINARY_DIR} VERBATIM
                      COMMENT "Generating HTML documentation")
  add_custom_target(doc ALL DEPENDS ${CMAKE_BINARY_DIR}/html/index.html)

else()
  message("Doxygen unavailable: documentation will not be generated.")
endif()


####################################################
# GCGlib overall compiler and linker settings
####################################################

# Set compiler and linker flags for each operating system and GCG_ARCHITECTURE
if((CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX) AND (GCG_OPERATING_SYSTEM MATCHES "Linux" OR GCG_OPERATING_SYSTEM MATCHES "Windows"))
  # Set GENERAL compiler and linker options GNU utilities
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")            # Enable all warnings
  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")                # Disable all warnings
  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated")  # Disable deprecated warnings
  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-write-strings")  # Disable warnings for obsolete strings
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic")        # Enable conformance checks
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic-errors") # Enable errors for conformance checks
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")        # Disable RTTI
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math")      # Yield faster math operations but eventually gives results incompatible with IEEE/ISO rules

  # Release options for compiler
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O")                        # Optimize code
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")                       # Enable all optimizations
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fexpensive-optimizations") # Enable expensive optimizations
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")                        # Remove all symbol table and relocation information from the executable.
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG")                  # Define NDEBUG

  # Debug options for compiler
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")      # Include debug information
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG") # Define DEBUG

  string(TOUPPER ${GCG_OPERATING_SYSTEM} GCG_OSUPPERCASE)
  if(GCG_PLATFORM_X64)
    set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -m64")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64 -march=native -msse4.2 -mtune=core2 -mmmx -mfpmath=sse")
    target_compile_definitions(gcg PRIVATE "-DGCG_${GCG_OSUPPERCASE}64")
  else()
    set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -m32")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 -march=native -msse3 -mtune=pentium4 -mmmx -mfpmath=sse")

    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER "4.3.0")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mthreads")
    endif()

    target_compile_definitions(gcg PRIVATE "-DGCG_${GCG_OSUPPERCASE}32")
  endif()

  if(GCG_OPERATING_SYSTEM MATCHES "Windows")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mwindows")
    target_link_libraries(gcg PRIVATE winmm wsock32)
  endif()

  set(CMAKE_C_FLAGS ${CMAKE_CXX_FLAGS})
elseif(MSVC AND GCG_OPERATING_SYSTEM MATCHES "Windows")
  # Set general compiler options for MSVC
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GF")             # StringPooling: true == /GF  false == /GF-
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:wchar_t")     # TreatWChar_tAsBuiltInType: true == /Zc:wchar_t  false == /Zc:wchar_t-
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")             # WarningLevel: /W<level 0 to 4> or /Wall
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /nologo")         # Suppresses display of sign-on banner.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX-")            # Treats all compiler warnings as errors.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Gm-")            # Enables minimal rebuild.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")           # Exception Handling Model
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GS-")            # Buffers security check.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GR-")            # Disables run-time type information (RTTI).
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Gd")             # Calling convention.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /analyze-")       # Disable code analysis.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:fast")        # Specify floating-point behavior: fast, precise, strict
  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")             # Compiles multiple source files by using multiple processes.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP1")            # Compile using one process.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D \"_UNICODE\"") # Define character set: _UNICODE or _MBCS. Must match the baseclasses character set.

  # Set general linker options for MSVC
  set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} /NOLOGO")     # Suppresses display of sign-on banner.
  set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} /TLBID:1")    # Specifies the resource ID of the linker-generated type library.
  set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} /NXCOMPAT")   # Marks an executable as verified to be compatible with the Windows Data Execution Prevention feature.
  set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} /ignore:4099")# Ignore PDB not found error
  set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} /RELEASE")    # Sets the Checksum in the header of an .exe file.
  set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} /ORDER")    # Optimize library putting functions in order

  # Release options for compiler
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")           # RuntimeLibrary: (MultiThreaded) == /MT  (MultiThreadedDebug) == /MTd  (MultiThreadedDLL) == /MD (MultiThreadedDebugDLL) == /MDd
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Ox")           # Uses maximum optimization (/Ob2gity /Gs).
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Ot")           # Favors fast code.
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Oy-")          # Omits frame pointer (x86 only).
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -D\"NDEBUG\"")  # Define NDEBUG

  # Debug options for compiler
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")        # RuntimeLibrary: (MultiThreaded) == /MT  (MultiThreadedDebug) == /MTd  (MultiThreadedDLL) == /MD (MultiThreadedDebugDLL) == /MDd
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D\"DEBUG\"") # Define DEBUG
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Z7")         # Generate information inside program.

  if(GCG_PLATFORM_X64)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  /arch:SSE2") # Specifies the GCG_ARCHITECTURE for code generation.
    set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS}  /MACHINE:X64") # Specifies the GCG_ARCHITECTURE for linking.
    set_target_properties(gcg PROPERTIES COMPILE_FLAGS "/D \"_CRT_SECURE_NO_WARNINGS\" /D \"_WINDLL\" /D \"GCG_WINDOWS64\"")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE2")  # Specifies the GCG_ARCHITECTURE for code generation.
    set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} /MACHINE:X86")  # Specifies the GCG_ARCHITECTURE for linking.
    set_target_properties(gcg PROPERTIES COMPILE_FLAGS "/D \"_CRT_SECURE_NO_WARNINGS\" /D \"_WINDLL\" /D \"GCG_WINDOWS32\"")
  endif()

  set_target_properties(gcg PROPERTIES LINK_FLAGS "/NODEFAULTLIB:\"libcmt.lib\" /NODEFAULTLIB:\"libcmtd.lib\" /NODEFAULTLIB:\"libcmt.lib\"")
  #set_target_properties(gcg PROPERTIES LINK_FLAGS "/NODEFAULTLIB:\"msvcrt.lib\" /NODEFAULTLIB:\"msvcrtd.lib\" /NODEFAULTLIB:\"shell32.lib\"")

  # Specific MSVC libraries
  target_link_libraries(gcg PRIVATE winmm wsock32 strmiids Ws2_32)
  target_link_libraries(gcg PRIVATE optimized strmbase debug strmbasd)

  set(CMAKE_C_FLAGS ${CMAKE_CXX_FLAGS})
else()
  # Unknown compiler: let it go and see if it works
endif()


##############################################################
# GCGlib: general directory settings
##############################################################

# Output building directories for binaries, libraries, archives and temporaries
set(RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH   ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH      ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_BUILD_FILES_DIRECTORY ${CMAKE_BINARY_DIR}/tmp)
set(CMAKE_BUILD_DIRECTORY ${CMAKE_BINARY_DIR}/tmp)

# Default directories for installing binaries and libraries
set(GCG_INSTALL_BINARY_DIR  bin CACHE PATH "Installation directory for executables")
set(GCG_INSTALL_ARCHIVE_DIR lib CACHE PATH "Installation directory for archives")
set(GCG_INSTALL_LIBRARY_DIR lib CACHE PATH "Installation directory for libraries")

if(GCG_PLATFORM_X64)
  # Directories for 64bit GCG_ARCHITECTUREs
  if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    # Windows folders for x64 GCG_ARCHITECTURE
    set(GCG_INSTALL_BINARY_DIR  ${GCG_INSTALL_BINARY_DIR}/x64)
    set(GCG_INSTALL_ARCHIVE_DIR ${GCG_INSTALL_ARCHIVE_DIR}/x64)
    set(GCG_INSTALL_LIBRARY_DIR ${GCG_INSTALL_LIBRARY_DIR}/x64)
  elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
          # Linux folders

          # Don't skip the full RPATH for the build tree
          set(CMAKE_SKIP_BUILD_RPATH  FALSE)

          # Needed for executables that use GCGlib in Linux
          set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/${GCG_INSTALL_LIBRARY_DIR})

          # Add the automatically determined parts of the RPATH which point to directories outside the build tree to the install RPATH
          set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

          # When building, don't use the install RPATH already (but later on when installing)
          set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

        elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
              # OSX folders for amd64 GCG_ARCHITECTURE
              # Use default
            else()
                # Unknown system name: try the most common folder names
                # Use default
  endif()
else()
  # Directories for 32bit GCG_ARCHITECTUREs
  if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    # Windows folders for x86 GCG_ARCHITECTURE
    # Use default
  elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
          # Linux folders for i686 GCG_ARCHITECTURE
          # Use default
        elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
              # OSX folders for x86 GCG_ARCHITECTURE
              # Use default
            else()
                # Unknown system name: try the most common folder names
                # Use default
  endif()
endif()

# Headers and cmake files destinations
set(GCG_INSTALL_HEADER_DIR include CACHE PATH "Installation directory for header files")
set(GCG_INSTALL_CMAKE_DIR  ${GCG_INSTALL_LIBRARY_DIR}/${GCG_LIBRARY_NAME}/cmake CACHE PATH "Installation directory for CMake files")

# Set destination for documentation files and examples and offer the user the choice of overriding it
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  set(GCG_CMAKE_DOC_DIR . CACHE PATH "Installation directory for documentation files")
else()
  set(GCG_CMAKE_DOC_DIR share/doc/${GCG_LIBRARY_NAME} CACHE PATH "Installation directory for documentation files")
endif()
set(GCG_INSTALL_EXAMPLES_DIR "${GCG_CMAKE_DOC_DIR}/examples" CACHE PATH "Installation directory for examples")


############################################
# GCGlib generation directories
############################################

# Set output directory for libraries and archives
set_target_properties(gcg PROPERTIES LIBRARY_OUTPUT_DIRECTORY                ${LIBRARY_OUTPUT_DIRECTORY})
set_target_properties(gcg PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE        ${LIBRARY_OUTPUT_DIRECTORY})
set_target_properties(gcg PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG          ${LIBRARY_OUTPUT_DIRECTORY})
set_target_properties(gcg PROPERTIES LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL     ${LIBRARY_OUTPUT_DIRECTORY})
set_target_properties(gcg PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${LIBRARY_OUTPUT_DIRECTORY})

set_target_properties(gcg PROPERTIES ARCHIVE_OUTPUT_DIRECTORY                ${ARCHIVE_OUTPUT_DIRECTORY})
set_target_properties(gcg PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELEASE        ${ARCHIVE_OUTPUT_DIRECTORY})
set_target_properties(gcg PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_DEBUG          ${ARCHIVE_OUTPUT_DIRECTORY})
set_target_properties(gcg PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL     ${ARCHIVE_OUTPUT_DIRECTORY})
set_target_properties(gcg PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO ${ARCHIVE_OUTPUT_DIRECTORY})


############################################
# GCGlib install instructions
############################################

# Install targets and associate them with the "GCGTargets" export set to export later
install(TARGETS gcg
        EXPORT GCGTargets # Add these targets to the "export-set" that will be used to generate GCGTarget.cmake
        RUNTIME DESTINATION       ${GCG_INSTALL_BINARY_DIR}  PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_WRITE GROUP_READ WORLD_EXECUTE WORLD_READ COMPONENT Binaries
        LIBRARY DESTINATION       ${GCG_INSTALL_LIBRARY_DIR} PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_WRITE GROUP_READ WORLD_EXECUTE WORLD_READ COMPONENT Binaries
        ARCHIVE DESTINATION       ${GCG_INSTALL_ARCHIVE_DIR} PERMISSIONS OWNER_WRITE OWNER_READ GROUP_WRITE GROUP_READ WORLD_READ COMPONENT Libraries
        PUBLIC_HEADER DESTINATION ${GCG_INSTALL_HEADER_DIR}  PERMISSIONS OWNER_WRITE OWNER_READ GROUP_WRITE GROUP_READ WORLD_READ COMPONENT Headers
        INCLUDES DESTINATION ${GCG_INSTALL_HEADER_DIR})


# Generate GCGConfig.cmake file
include(CMakePackageConfigHelpers)
configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/GCGConfig.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/GCGConfig.cmake" INSTALL_DESTINATION ${GCG_INSTALL_CMAKE_DIR} PATH_VARS GCG_INSTALL_HEADER_DIR)

# Creates and installs the GCGTarget.cmake file
install(EXPORT GCGTargets FILE GCGTargets.cmake DESTINATION ${GCG_INSTALL_CMAKE_DIR} CONFIGURATIONS ${CMAKE_BUILD_TYPE}
        PERMISSIONS OWNER_WRITE OWNER_READ GROUP_WRITE GROUP_READ WORLD_READ COMPONENT CMake)

# Install the GCGConfig.cmake
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/GCGConfig.cmake DESTINATION ${GCG_INSTALL_CMAKE_DIR}
        PERMISSIONS OWNER_WRITE OWNER_READ GROUP_WRITE GROUP_READ WORLD_READ COMPONENT CMake)

# Install information files
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/CHANGE.LOG ${CMAKE_CURRENT_BINARY_DIR}/README.txt ${GCG_LICENSE_OUTPUT} DESTINATION ${GCG_CMAKE_DOC_DIR}
        PERMISSIONS OWNER_WRITE OWNER_READ GROUP_WRITE GROUP_READ WORLD_READ COMPONENT References)

# Install documentation
if(DOXYGEN_FOUND)
  install(DIRECTORY ${CMAKE_BINARY_DIR}/html DESTINATION ${GCG_CMAKE_DOC_DIR} COMPONENT References
          FILE_PERMISSIONS      OWNER_WRITE OWNER_READ GROUP_WRITE GROUP_READ WORLD_READ
          DIRECTORY_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_WRITE GROUP_READ WORLD_EXECUTE WORLD_READ)
endif()

# Extra possible files needed to run GCGlib (generally Windows)
if(DEFINED GCG_EXTRA_INSTALL_BIN_FILES)
  install(FILES ${GCG_EXTRA_INSTALL_BIN_FILES} DESTINATION "${GCG_INSTALL_BINARY_DIR}"
          PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_WRITE GROUP_READ WORLD_EXECUTE WORLD_READ COMPONENT Binaries)
endif()

##########################################################################
# GCGlib: build examples
##########################################################################
add_subdirectory(examples)


##########################################################################
# GCGlib: CPack configuration
##########################################################################

# First, install all required system libraries
set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE) # Let us install the needed files where we want...
include(InstallRequiredSystemLibraries)
install(FILES ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION "${GCG_INSTALL_BINARY_DIR}"
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_WRITE GROUP_READ WORLD_EXECUTE WORLD_READ COMPONENT Binaries)

# General packaging options
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_CURRENT_BINARY_DIR};GCG;ALL;/")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "GCGlib - Graphics Engine")
set(CPACK_PACKAGE_NAME ${PACKAGE_NAME})
set(CPACK_PACKAGE_VENDOR "GCG")
set(CPACK_PACKAGE_CONTACT "http://www.gcg.ufjf.br")
set(CPACK_PACKAGE_DESCRIPTION_FILE ${CMAKE_CURRENT_BINARY_DIR}/README.txt)
set(CPACK_RESOURCE_FILE_LICENSE ${GCG_LICENSE_OUTPUT})
set(CPACK_PACKAGE_VERSION ${GCG_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR ${GCG_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${GCG_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${GCG_VERSION_PATCH})
set(CPACK_SYSTEM_NAME ${CMAKE_SYSTEM_NAME})
set(CPACK_SOURCE_IGNORE_FILES "CMakeLists.*")
set(CPACK_PACKAGE_ICON ${GCG_IMAGE_PATH}/gcg.ico)
set(CPACK_PACKAGE_INSTALL_DIRECTORY "GCGlib ${CPACK_PACKAGE_VERSION}")

# Choosing package generator
string(TOLOWER ${CMAKE_SYSTEM_PROCESSOR} CURRENT_GCG_ARCHITECTURE)
if(GCG_OPERATING_SYSTEM MATCHES "Linux")
  # Choose package generator and setup basic options for Linux
  #set(CPACK_GENERATOR "RPM" CACHE STRING "Generators for GCGlib package.") # List of generators: TGZ STGZ TZ ZIP TBZ2 RPM DEB
  set(CPACK_GENERATOR "DEB" CACHE STRING "Generators for GCGlib package.") # List of generators: TGZ STGZ TZ ZIP TBZ2 RPM DEB
elseif(GCG_OPERATING_SYSTEM MATCHES "Windows")
  # Choose package generator for Windows
  #set(CPACK_GENERATOR "NSIS" CACHE STRING "Generators for GCGlib package.") # List of generators: NSIS NSIS64 TGZ STGZ TZ ZIP TBZ2 WIX
  set(CPACK_GENERATOR "WIX" CACHE STRING "Generators for GCGlib package.") # List of generators: NSIS NSIS64 TGZ STGZ TZ ZIP TBZ2 WIX
else()
  # Unknown platform: generate a zip package
  set(CPACK_GENERATOR "ZIP" CACHE STRING "Generators for GCGlib package.") # List of generators: TGZ STGZ TZ ZIP TBZ2
endif()

# Specific options for WIX generator in Windows
if(GCG_OPERATING_SYSTEM MATCHES "Windows" AND CPACK_GENERATOR MATCHES "WIX")
  set(CPACK_WIX_ROOT "C:/Program Files (x86)/WiX Toolset v3.9") # Really an annoying thing

  if(MINGW)
    # Setup basic options for MinGW/Windows
    if(GCG_PLATFORM_X64)
      set(CPACK_PACKAGE_FILE_NAME  "gcg-${GCG_VERSION}-x86_64-MinGW")
      set(CPACK_PACKAGE_NAME "GCGlib for MinGW64")
    else()
      set(CPACK_PACKAGE_FILE_NAME  "gcg-${GCG_VERSION}-win32-MinGW")
      set(CPACK_PACKAGE_NAME "GCGlib ${GCG_VERSION_MAJOR}.${GCG_VERSION_MINOR} for MinGW")
    endif()

    set(CPACK_WIX_PROGRAM_MENU_FOLDER "GCG/GCGlib ${GCG_VERSION_MAJOR}.${GCG_VERSION_MINOR} MinGW")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "GCG/GCGlib ${GCG_VERSION_MAJOR}.${GCG_VERSION_MINOR} (MinGW)")
  elseif(MSVC)
    # Setup basic options for MSVC/Windows
    if(GCG_PLATFORM_X64)
      set(CPACK_PACKAGE_FILE_NAME  "gcg-${GCG_VERSION}-x86_64-MSVC")
      set(CPACK_PACKAGE_NAME "GCGlib ${GCG_VERSION} for MSVC (x64)")
    else()
      set(CPACK_PACKAGE_FILE_NAME  "gcg-${GCG_VERSION}-win32-MSVC")
      set(CPACK_PACKAGE_NAME "GCGlib ${GCG_VERSION} for MSVC")
    endif()

    set(CPACK_WIX_PROGRAM_MENU_FOLDER "GCG/GCGlib ${GCG_VERSION_MAJOR}.${GCG_VERSION_MINOR} MSVC")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "GCG/GCGlib ${GCG_VERSION_MAJOR}.${GCG_VERSION_MINOR} (MSVC)")
  endif()

  set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "${CPACK_PACKAGE_NAME} ${CPACK_PACKAGE_VERSION}") # Package name has changed

  # Main GUIDs for WiX
  set(GCG_PRODUCT_GUID "*") # Every build generates a new product.
  # The upgrade GUI defines a compatible FAMILY:
  #   - It MUST BE KEPT for MINOR and BUILD changes: 1.x.y -> 1.m.n
  #   - It MUST BE CHANGED for new MAJOR product:  1.0.x -> 2.0.y
  set(GCG_UPGRADE_GUID C126E2CB-0284-421D-BC15-63160BD25B68) # Fixed GUID for 0.0 GCGlib upgrades: 0.xx.y

  # WIX setup
  set(CPACK_WIX_PRODUCT_GUID ${GCG_PRODUCT_GUID}) # It should be explicitly set to a constant generated globally unique identifier (GUID) to allow your installers to replace existing installations that use the same GUID.
  set(CPACK_WIX_UPGRADE_GUID ${GCG_UPGRADE_GUID}) # The installer will abort if it detects a pre-existing installation that uses the same GUID.
  set(CPACK_WIX_PRODUCT_ICON ${CPACK_PACKAGE_ICON}) # The Icon shown next to the program name in Add/Remove programs.
  set(CPACK_WIX_PROPERTY_ARPCOMMENTS ) # Add or remove comments
  set(CPACK_WIX_PROPERTY_ARPHELPLINK "http://www.gcg.ufjf.br/doc") # Add or remove: help and support information URL
  set(CPACK_WIX_PROPERTY_ARPURLINFOABOUT "http://www.gcg.ufjf.br/developments") # Add or remove: general information URL
  set(CPACK_WIX_PROPERTY_URLUPDATEINFO "http://www.gcg.ufjf.br/developments") # Add or remove: general information URL
  set(CPACK_WIX_UI_REF WixUI_FeatureTree) #This variable allows you to override the Id of the <UIRef> element in the WiX template.
  set(CPACK_WIX_CANDLE_EXTRA_FLAGS "") # This list variable allows you to pass additional flags to the WiX Candle
  set(CPACK_WIX_LIGHT_EXTRA_FLAGS "-dcl:high") # This list variable allows you to pass additional flags to the WiX Light
  set(CPACK_WIX_UI_BANNER "${GCG_IMAGE_PATH}/bannerWIX.bmp") # The bitmap at the top of all installer pages other than the welcome and completion dialogs. It must be 493 by 58 pixels.
  set(CPACK_WIX_UI_DIALOG "${GCG_IMAGE_PATH}/dialogWIX.bmp") # Background bitmap used on the welcome and completion dialogs.It must be 493 by 312 pixels.

  # Insert patches for further customization
  #set(CPACK_WIX_PATCH_FILE ${CMAKE_CURRENT_BINARY_DIR}/WiXpatch.wxs) # Optional XML file with fragments to be inserted into generated WiX sources
  # Let CPack handle these: CPACK_WIX_EXTENSIONS CPACK_WIX_PROPERTY_ARPHELPTELEPHONE CPACK_WIX_PROPERTY_ARPSIZE
endif()



# Specific options for NSIS generator in Windows
if(GCG_OPERATING_SYSTEM MATCHES "Windows" AND (CPACK_GENERATOR MATCHES "NSIS" OR CPACK_GENERATOR MATCHES "NSIS64"))
  if(MINGW)
    # Setup basic options for MinGW/Windows
    if(GCG_PLATFORM_X64)
      set(CPACK_GENERATOR "NSIS64") # Checking
      set(CPACK_PACKAGE_FILE_NAME  "gcg-${GCG_VERSION}-x86_64-MinGW")
      set(CPACK_PACKAGE_NAME "GCGlib ${GCG_VERSION} for MinGW64")
    else()
      set(CPACK_GENERATOR "NSIS") # Checking
      set(CPACK_PACKAGE_FILE_NAME  "gcg-${GCG_VERSION}-win32-MinGW")
      set(CPACK_PACKAGE_NAME "GCGlib ${GCG_VERSION} for MinGW")
    endif()

    # Install directory and registry for MinGW/Windows
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "GCGlib ${GCG_VERSION_MAJOR}.${GCG_VERSION_MINOR} (MinGW)")
  elseif(MSVC)
    # Setup basic options for MSVC/Windows
    if(CPACK_GENERATOR MATCHES "NSIS64")
      set(CPACK_GENERATOR "NSIS64") # Checking
      set(CPACK_PACKAGE_FILE_NAME  "gcg-${GCG_VERSION}-x86_64-MSVC")
      set(CPACK_PACKAGE_NAME "GCGlib ${GCG_VERSION} for MSVC (x64)")
    else()
      set(CPACK_GENERATOR "NSIS") # Checking
      set(CPACK_PACKAGE_FILE_NAME  "gcg-${GCG_VERSION}-win32-MSVC")
      set(CPACK_PACKAGE_NAME "GCGlib ${GCG_VERSION} for MSVC")
    endif()

    # Install directory and registry for MSVC/Windows
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "GCGlib ${GCG_VERSION_MAJOR}.${GCG_VERSION_MINOR} (MSVC)")
  endif()
  set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "${CPACK_PACKAGE_NAME} ${CPACK_PACKAGE_VERSION}") # Package name has changed

  # There is a bug in NSI that does not handle full unix paths properly. Make sure there is at least one set of 4 backlashes.
  set(CPACK_NSIS_PACKAGE_NAME "${CPACK_PACKAGE_NAME}") # The title displayed at the top of the installer.
  set(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_NAME}") # The display name string that appears in the Windows Add/Remove Program control panel
  set(CPACK_NSIS_MUI_ICON ${GCG_IMAGE_PATH}/gcg.ico)  # The icon file (.ico) for the generated install program.
  set(CPACK_NSIS_MUI_UNIICON ${GCG_IMAGE_PATH}/gcgright.ico) # The icon file (.ico) for the generated uninstall program.
  set(CPACK_PACKAGE_ICON "${GCG_IMAGE_PATH}\\\\gcgNSIS.bmp")  # Image that will be displayed on the top bar inside the installer.
  set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS 	"") # Extra NSIS commands that will be added to the install Section.
  set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "") # Extra NSIS commands that will be added to the uninstall Section.
  set(CPACK_NSIS_COMPRESSOR "/SOLID lzma") # The arguments that will be passed to the NSIS SetCompressor command.
  set(CPACK_NSIS_MODIFY_PATH ON)  # If this is set to "ON", an extra page will appear for choosing whether the program directory should be added to the system PATH.
  set(CPACK_NSIS_INSTALLED_ICON_NAME ${GCG_IMAGE_PATH}/gcg.ico) # Set the icon used for the Windows "Add or Remove Programs" tool.
  set(CPACK_NSIS_HELP_LINK 	"http://www.gcg.ufjf.br") # Adds link to registry. URI.
  set(CPACK_NSIS_URL_INFO_ABOUT "http://www.gcg.ufjf.br/developments") # Adds link to registry and the vendor in add/remove programs.
  set(CPACK_NSIS_CONTACT 	"admin@gcg.ufjf.br") # Adds link to add/remove programs
  set(CPACK_NSIS_CREATE_ICONS_EXTRA "") # Additional NSIS commands for creating start menu shortcuts.
  set(CPACK_NSIS_DELETE_ICONS_EXTRA "") # Possibly: Additional NSIS commands to uninstall start menu shortcuts.
  set(CPACK_NSIS_EXECUTABLES_DIRECTORY 	"bin") # Override the default path ("bin/") where NSIS will find executables.
  set(CPACK_NSIS_MUI_FINISHPAGE_RUN "") # If used, will make it possible for user to choose (on an additional page, displayed at the end of the installation) to run intalled program.
  set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON) # Ask about uninstalling previous versions first.
  set(CPACK_NSIS_MENU_LINKS "html/index.html" "GCGlib Help" "http://www.gcg.ufjf.br/doc/index.html" "GCGlib Online Reference") # Specify links in [application] menu. This should contain a list of pair link link name.
  set(CPACK_NSIS_INSTALLER_MUI_ICON_CODE "")
  set(CPACK_NSIS_ADD_REMOVE OFF) # Add remove function

  # Adjust destinations
  if(CMAKE_CL_64)
    set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64\\\\GCG") # The default installation directory presented to the end user by the NSIS installer is under this root dir.
  else()
    set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES\\\\GCG") # The default installation directory presented to the end user by the NSIS installer is under this root dir.
  endif()
endif()



# Specific options for RPM generator in Linux
if(GCG_OPERATING_SYSTEM MATCHES "Linux" AND CPACK_GENERATOR MATCHES "RPM")
  # Setup basic options RPM
  if(GCG_PLATFORM_X64)
    set(CPACK_PACKAGE_FILE_NAME  "gcg-${GCG_VERSION}-${CURRENT_GCG_ARCHITECTURE}")
  else()
    set(CPACK_PACKAGE_FILE_NAME  "gcg-${GCG_VERSION}-i686")
  endif()
  set(CPACK_PACKAGE_INSTALL_DIRECTORY "")

  set(CPACK_RPM_PACKAGE_SUMMARY ${CPACK_PACKAGE_DESCRIPTION_SUMMARY}) # The RPM package summary
  set(CPACK_RPM_PACKAGE_NAME ${CPACK_PACKAGE_NAME}) # The RPM package name
  set(CPACK_RPM_PACKAGE_GCG_ARCHITECTURE ${CURRENT_GCG_ARCHITECTURE}) # The RPM package GCG_ARCHITECTURE. This may be set to "noarch" if you know you are building a noarch package.
  set(CPACK_RPM_PACKAGE_RELEASE 1) # The RPM package release. This is the numbering of the RPM package itself, i.e. the version of the packaging and not the version of the content (see CPACK_RPM_PACKAGE_VERSION).
  set(CPACK_RPM_PACKAGE_LICENSE "custom") # The RPM package license policy.
  set(CPACK_RPM_PACKAGE_GROUP "Development/Libraries") # The RPM package group (see /usr/share/doc/rpm-*/GROUPS)
  set(CPACK_RPM_PACKAGE_URL "http://www.gcg.ufjf.br/developments") # The RPM package group (see /usr/share/doc/rpm-*/GROUPS )
  set(CPACK_RPM_COMPRESSION_TYPE "lzma") # May be used to override RPM compression type to be used to build the RPM.
  set(CPACK_RPM_PACKAGE_VENDOR "${CPACK_PACKAGE_VENDOR}") # The RPM package vendor
  set(CPACK_RPM_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION}") # The RPM package version.
  set(CPACK_RPM_PACKAGE_DESCRIPTION "${CPACK_PACKAGE_DESCRIPTION_SUMMARY}") # The RPM package group (see /usr/share/doc/rpm-*/GROUPS )
  set(CPACK_RPM_PACKAGE_REQUIRES "") # May be used to set RPM dependencies. see [RPM dependencies specification]) for precise syntax.
  set(CPACK_RPM_PACKAGE_REQUIRES_PRE "") # May be used to set RPM pre-install dependencies
  set(CPACK_RPM_PACKAGE_REQUIRES_POST "") # May be used to set RPM post-install dependencies
  set(CPACK_RPM_PACKAGE_REQUIRES_PREUN "") # May be used to set RPM pre-uninstall dependencies
  set(CPACK_RPM_PACKAGE_REQUIRES_POSTUN "") # May be used to set RPM post-uninstall dependencies
  set(CPACK_RPM_PACKAGE_SUGGESTS "") # May be used to set weak RPM dependencies (suggests).
  set(CPACK_RPM_PACKAGE_PROVIDES "") # May be used to set the virtual packages provided by the RPM. It is complimentary to CPACK_RPM_PACKAGE_REQUIRES. You do not need to list libraries, etc.: rpmbuild will figure that out when generating the RPM.
  set(CPACK_RPM_PACKAGE_OBSOLETES "gcg<${CPACK_PACKAGE_VERSION}") # May be used to set RPM packages that are obsoleted by this one.
  set(CPACK_RPM_PACKAGE_RELOCATABLE TRUE) # If this variable is set to TRUE or ON CPackRPM will try to build a relocatable RPM package.
  set(CPACK_RPM_SPEC_INSTALL_POST "") # May be used to set an RPM post-install command inside the spec file. For example setting it to "/bin/true" may be used to prevent rpmbuild to strip binaries.
  set(CPACK_RPM_SPEC_MORE_DEFINE "") #  May be used to add any %define lines to the generated spec file.
  set(CPACK_RPM_GENERATE_USER_BINARY_SPECFILE_TEMPLATE "") # May be used to generate a template for a user provided spec file.
  set(CPACK_RPM_PRE_INSTALL_SCRIPT_FILE "") # The content of the specified files will be embedded in the RPM spec file in the appropriate sections.
  set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE "") # The content of the specified files will be embedded in the RPM spec file in the appropriate sections.
  set(CPACK_RPM_PRE_UNINSTALL_SCRIPT_FILE "") # The content of the specified files will be embedded in the RPM spec file in the appropriate sections.
  set(CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE "") # The content of the specified files will be embedded in the RPM spec file in the appropriate sections.
  set(CPACK_RPM_PACKAGE_DEBUG 0) # May be set in order to trace debug informations during CPack RPM run.
  set(CPACK_RPM_USER_BINARY_SPECFILE "") # May be used to specify a user provided spec file instead of generating one.
  set(CPACK_RPM_USER_FILELIST "") # May be used to explicitly specify %(<directive>) file line in the spec file.
  #set(CPACK_RPM_CHANGELOG_FILE ${CMAKE_CURRENT_BINARY_DIR}/CHANGE.LOG) # May be used to embed a changelog in the spec file. The refered file will be read and directly put after the %changelog section.
  # Unset features: CPACK_RPM_GENERATE_USER_BINARY_SPECFILE_TEMPLATE  CPACK_RPM_<COMPONENT>_USER_FILELIST CPACK_RPM_<COMPONENT>_PACKAGE_PREFIX
  #                 CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION  CPACK_RPM_RELOCATION_PATHS
endif()


# Specific options for DEB generator in Linux
if(GCG_OPERATING_SYSTEM MATCHES "Linux" AND CPACK_GENERATOR MATCHES "DEB")
  # Setup basic options DEB
  if(GCG_PLATFORM_X64)
      set(CPACK_PACKAGE_FILE_NAME  "gcg-${GCG_VERSION}-amd64")
    else()
      set(CPACK_PACKAGE_FILE_NAME  "gcg-${GCG_VERSION}-i386")
  endif()
  set(CPACK_PACKAGE_INSTALL_DIRECTORY "")

  set(CPACK_DEBIAN_PACKAGE_NAME ${CPACK_PACKAGE_NAME}) # The Debian package summary
  set(CPACK_DEBIAN_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION}") # The Debian package version
  set(CPACK_DEBIAN_PACKAGE_SECTION "libs") # Section devel, libdevel, science, web, TeX Authoring, Games, libs
  set(CPACK_DEBIAN_COMPRESSION_TYPE "lzma") # The compression used for creating the Debian package. Possible values are: lzma, xz, bzip2 and gzip.
  set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "http://www.gcg.ufjf.br/developments") # May be set to ON in order to use dpkg-shlibdeps to generate better package dependency list.
  set(CPACK_DEBIAN_PACKAGE_DEPENDS "") # Sets the Debian dependencies of this package.
  set(CPACK_DEBIAN_EXAMPLEBIN_PACKAGE_DEPENDS "libglui-dev (>=2.0.0)") # Sets the Debian dependencies of this package.
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_CONTACT}") # The Debian package maintainer
  set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "${CPACK_PACKAGE_DESCRIPTION_SUMMARY}") # The Debian package description
  set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional") # The Debian package priority
  set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS OFF) # May be set to ON in order to use dpkg-shlibdeps to generate better package dependency list.
  set(CPACK_DEBIAN_BINARIES_PACKAGE_SHLIBDEPS ON) # May be set to ON in order to use dpkg-shlibdeps to generate better package dependency list.
  set(CPACK_DEBIAN_PACKAGE_DEBUG OFF) # May be set when invoking cpack in order to trace debug information during CPackDeb run.
  set(CPACK_DEBIAN_PACKAGE_PREDEPENDS "") # Sets the Pre-Depends field of the Debian package. Like Depends, except that it also forces dpkg to complete installation of the packages named before even starting the installation of the package which declares the pre-dependency.
  set(CPACK_DEBIAN_PACKAGE_ENHANCES "") # Sets the Enhances field of the Debian package. Similar to Suggests but works in the opposite direction: declares that a package can enhance the functionality of another package.
  set(CPACK_DEBIAN_PACKAGE_BREAKS "") # Sets the Breaks field of the Debian package. When a binary package (P) declares that it breaks other packages (B), dpkg will not allow the package (P) which declares Breaks be unpacked unless the packages that will be broken (B) are deconfigured first. As long as the package (P) is configured, the previously deconfigured packages (B) cannot be reconfigured again.
  set(CPACK_DEBIAN_PACKAGE_CONFLICTS "") # Sets the Conflicts field of the Debian package. When one binary package declares a conflict with another using a Conflicts field, dpkg will not allow them to be unpacked on the system at the same time.
  set(CPACK_DEBIAN_PACKAGE_PROVIDES "") # Sets the Provides field of the Debian package. A virtual package is one which appears in the Provides control field of another package.
  set(CPACK_DEBIAN_PACKAGE_REPLACES "gcg (<<${CPACK_PACKAGE_VERSION}),gcg (>=${GCG_VERSION_MAJOR}.${GCG_VERSION_MINOR})") # Sets the Replaces field of the Debian package. Packages can declare in their control file that they should overwrite files in certain other packages, or completely replace other packages.
  set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "") # Sets the Replaces field of the Debian package. Packages can declare in their control file that they should overwrite files in certain other packages, or completely replace other packages.
  set(CPACK_DEBIAN_PACKAGE_SUGGESTS "") # Sets the Suggests field of the Debian package. Allows packages to declare a suggested package install grouping.
  set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "") # This variable allow advanced user to add custom script to the control.tar.gz. Typical usage is for conffiles, postinst, postrm, prerm.
  # Not used or left to CPack to fill in: CPACK_COMPONENT_<COMPONENT>_DESCRIPTION CPACK_DEBIAN_PACKAGE_GCG_ARCHITECTURE
endif()

# Include CPack to introduce the appropriate targets
include(CPack)

# Include CPackComponent for using the following macros
include(CPackComponent)

# Installation types
cpack_add_install_type(Full DISPLAY_NAME "Complete installation")
cpack_add_install_type(Minimum DISPLAY_NAME "Minimum components")

# Component definition
cpack_add_component_group(Runtime DISPLAY_NAME "Binaries and applications"
                                  DESCRIPTION "Installs all applications and runtime binaries for using GCGlib."
                                  BOLD_TITLE
                                  INSTALL_TYPES Minimum Full)
cpack_add_component_group(Development DISPLAY_NAME "Development"
                                      DESCRIPTION "Installs C/C++ header files, static libraries and examples for developing software with GCGlib."
                                      EXPANDED
                                      INSTALL_TYPES Full)
cpack_add_component_group(Document DISPLAY_NAME "Documentation"
                                   DESCRIPTION "Installs documentation content for using GCGlib."
                                   INSTALL_TYPES Full)
cpack_add_component_group(Examples DISPLAY_NAME "Examples"
                                   DESCRIPTION "Installs C/C++ code examples using GCGlib for helping software development."
                                   PARENT_GROUP Document
                                   INSTALL_TYPES Full)

cpack_add_component(Binaries DISPLAY_NAME "Binaries"
                             DESCRIPTION "Installs all runtime binaries for using GCGlib. A minimum set of executables will be copied into your system."
                             REQUIRED
                             GROUP Runtime)
cpack_add_component(Headers DISPLAY_NAME "Headers for C/C++"
                            DESCRIPTION "Installs C/C++ Headers for development with GCGlib."
                            GROUP Development
                            DEPENDS Binaries)
cpack_add_component(Libraries DISPLAY_NAME "Static Libraries for C/C++"
                              DESCRIPTION "Installs C/C++ Static Libraries for linking with GCGlib."
                              GROUP Development
                              DEPENDS Headers Binaries)
cpack_add_component(ExamplesSrc DISPLAY_NAME "Source code"
                                DESCRIPTION "Installs the C/C++ source code for examples."
                                DISABLED
                                GROUP Examples
                                DEPENDS Headers Libraries Binaries)
cpack_add_component(ExamplesBin DISPLAY_NAME "Binaries"
                                DESCRIPTION "Installs the compiled binaries for each example."
                                DISABLED
                                GROUP Examples
                                DEPENDS Binaries)
cpack_add_component(CMake DISPLAY_NAME "CMake files"
                          DESCRIPTION "Installs CMake files for finding GCGlib inside CMake environment."
                          DISABLED
                          GROUP Development
                          DEPENDS Headers Libraries Binaries)
cpack_add_component(References DISPLAY_NAME "Reference guide"
                               DESCRIPTION "Installs reference documentation for GCGlib classes, functions and macros."
                               DISABLED
                               GROUP Document
                               DEPENDS Headers Libraries Binaries)

