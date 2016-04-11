##############################################################
# GCGlib/vapiwin: Script to build video API for Windows
##############################################################

cmake_minimum_required(VERSION 2.8.12)

if(NOT DEFINED GCG_ROOT_DIRECTORY)
  set(GCG_ROOT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../..)
endif()

set(CMAKE_BUILD_TYPE Release FORCE)

# Project name
project(vapiwin CXX)

#############################################################
# GCGlib/vapiwin architecture and operating system detection
#############################################################

# Detect operating system
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(OPERATING_SYSTEM "Linux")  # Linux specific code
  elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
        set(OPERATING_SYSTEM "Windows")  # Windows specific code
      elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
            set(OPERATING_SYSTEM "Mac OS X") # Mac OS X specific code
          else()
            set(OPERATING_SYSTEM ${CMAKE_SYSTEM_NAME})
endif()

# Detect architecture
set(WORDSIZE 32)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^i.86$")
    set(ARCHITECTURE "i686")
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "amd64")
    set(ARCHITECTURE "amd64")
    set(WORDSIZE 64)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")
    set(ARCHITECTURE "arm")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES ".*64") # Accounts for ppc64 and sparc64
    string(REGEX REPLACE "64" "" ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
    set(WORDSIZE 64)
else()
    set(ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
endif()

# Discover if it is a 64 bits compiler/build
set(PLATFORM_X64 FALSE)
if(MSVC)
  if(CMAKE_CL_64)
    set(PLATFORM_X64 TRUE)
  endif()
else()
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(PLATFORM_X64 TRUE)
  endif()
endif()


#################################################
# GCGlib/vapiwin required packages and libraries
#################################################

# Scripts to find Windows SDK, DirectShow, DirectX
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${GCG_ROOT_DIRECTORY}/external/cmake)

# Find required packages and libraries for MSVC: baseclasses
if(MSVC AND OPERATING_SYSTEM MATCHES "Windows")
  # Find WindowsSDK
  find_package(DirectShow)

  if(WINDOWSSDK_FOUND)
    include_directories(${DIRECTSHOW_BASECLASS_DIR} ${DIRECTSHOW_WINDOWSSDK_INCLUDE_DIR})
  endif()

  # Also include our local baseclasses copy
  include_directories(${GCG_ROOT_DIRECTORY}/external/baseclasses)

  # Note that link_directories() only works BEFORE target creation
  if(PLATFORM_X64)
    link_directories(${GCG_ROOT_DIRECTORY}/external/baseclasses/x64)
  else()
    link_directories(${GCG_ROOT_DIRECTORY}/external/baseclasses)
  endif()
endif()


############################################
# GCGlib/vapiwin target definition
############################################

# Add this directory as include directory
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${GCG_ROOT_DIRECTORY}/src)

# Define vapiwin sources
set(VAPIWIN_SOURCES_CXX vapiwin.cpp)

# Create GCGlib target
add_library(vapiwin SHARED ${VAPIWIN_SOURCES_CXX} vapiwin.h dxtrans.h qedit.h)


######################################################
# GCGlib/vapiwin overall compiler and linker settings
######################################################
if(MSVC AND OPERATING_SYSTEM MATCHES "Windows")
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
  set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} /nologo")     # Suppresses display of sign-on banner.
  set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} /TLBID:1")    # Specifies the resource ID of the linker-generated type library.
  set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} /NXCOMPAT")   # Marks an executable as verified to be compatible with the Windows Data Execution Prevention feature.
  set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} /ignore:4099")# Ignore PDB not found error

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

  if(PLATFORM_X64)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  /arch:SSE2") # Specifies the architecture for code generation.
    set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS}  /MACHINE:X64") # Specifies the architecture for linking.
    set_target_properties(vapiwin PROPERTIES COMPILE_FLAGS "/D \"_CRT_SECURE_NO_WARNINGS\" /D \"_WINDLL\" /D \"GCG_WINDOWS64\"")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE2")  # Specifies the architecture for code generation.
    set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} /MACHINE:X86")  # Specifies the architecture for linking.
    set_target_properties(vapiwin PROPERTIES COMPILE_FLAGS "/D \"_CRT_SECURE_NO_WARNINGS\" /D \"_WINDLL\" /D \"GCG_WINDOWS32\"")
  endif()

  set_target_properties(vapiwin PROPERTIES LINK_FLAGS "/NODEFAULTLIB:\"libcmt.lib\" /NODEFAULTLIB:\"libcmtd.lib\"")

  # Specific MSVC libraries
  target_link_libraries(vapiwin PRIVATE winmm wsock32 strmiids Ws2_32)
  target_link_libraries(vapiwin PRIVATE optimized strmbase debug strmbasd)
else()
  # Unknown compiler: let it go and see if it works
endif()

# Install targets
install(TARGETS vapiwin RUNTIME DESTINATION "." PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_WRITE GROUP_READ WORLD_EXECUTE WORLD_READ COMPONENT Binaries)
