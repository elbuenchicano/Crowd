##############################################################
# GCGlib: GCGlib examples
##############################################################

# Find required packages and libraries for examples: GLUI
find_package(GLUI QUIET)
if(GLUI_FOUND)
  include_directories(GLUI_INCLUDE_DIRS)
else()
  if(MSVC OR MINGW)
    include(ExternalProject)

    # Include internal version of glui
    ExternalProject_Add(libglui
                        SOURCE_DIR  ${CMAKE_SOURCE_DIR}/external/glui
                        PREFIX      ${CMAKE_BINARY_DIR}/external/glui
                        BINARY_DIR  ${CMAKE_BINARY_DIR}/external/glui
                        CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX:PATH=${GCG_INSTALL_EXTERNAL_DIR}
                        BUILD_COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/external/glui --config ${CMAKE_BUILD_TYPE} --target glui
                        INSTALL_DIR ${GCG_INSTALL_EXTERNAL_DIR}   # It is not the same as CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX:PATH=${GCG_INSTALL_EXTERNAL_DIR}
                        INSTALL_COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/external/glui --config ${CMAKE_BUILD_TYPE} --target install
                        UPDATE_COMMAND "")

    # Path to headers and library
    set(GLUI_INCLUDE_DIRS ${GCG_INSTALL_EXTERNAL_DIR}/include)
    link_directories(${GCG_INSTALL_EXTERNAL_DIR}/lib) # Note that link_directories() only works BEFORE target creation

    # Set directories
    include_directories(${GLUI_INCLUDE_DIRS})
    set(GLUI_LIBRARIES glui)
    set(GLUI_FOUND TRUE)
  endif()
endif()



# Find required packages and libraries for examples: GLUT
find_package(GLUT QUIET)
if(GLUT_FOUND)
  include_directories(${GLUT_INCLUDE_DIRS})
else()
  if(MSVC OR MINGW)
    # Include our local copy
    set(GLUT_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/external/glut/include)

    # Define library name
    if(MINGW)
      set(GLUT_LIBRARIES "freeglut_static;freeglut;winmm") # MinGW version
    elseif(MSVC)
      set(GLUT_LIBRARIES "freeglut;winmm") # MSVC version
    endif()

    # Note that link_directories() only works BEFORE target creation
    if(GCG_PLATFORM_X64)
      link_directories(${CMAKE_SOURCE_DIR}/external/glut/lib/x64)
      set(GCG_EXTRA_INSTALL_BIN_EXAMPLES ${CMAKE_SOURCE_DIR}/external/glut/bin/x64/freeglut.dll)
    else()
      link_directories(${CMAKE_SOURCE_DIR}/external/glut/lib)
      set(GCG_EXTRA_INSTALL_BIN_EXAMPLES ${CMAKE_SOURCE_DIR}/external/glut/bin/freeglut.dll)
    endif()
  endif()

  include_directories(${GLUT_INCLUDE_DIRS})
  set(GLUT_LIBRARY ${GLUT_LIBRARIES})
  set(GLUT_FOUND TRUE)
endif()


############################################################################
# Windows specific information: resources files for examples
############################################################################
if(OPERATING_SYSTEM MATCHES "Windows")
  configure_file(examples.rc.in  ${CMAKE_CURRENT_BINARY_DIR}/examples.rc @ONLY) # Resource for Windows systems
  set(GCG_EXAMPLES_EXTRA_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/examples.rc)
endif()


##########################################################################
# GCGlib: add example targets
##########################################################################
# Get all file names from the examples directory, and scan them
file(GLOB examples RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)
foreach(example ${examples})
  if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${example})
    # Found a subdirectory: get all sources inside it
    file(GLOB sources RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/${example} ${CMAKE_CURRENT_SOURCE_DIR}/${example}/*.cpp
                                                                      ${CMAKE_CURRENT_SOURCE_DIR}/${example}/*.c
                                                                      ${CMAKE_CURRENT_SOURCE_DIR}/${example}/*.h
                                                                      ${CMAKE_CURRENT_SOURCE_DIR}/${example}/*.txt)
    if(sources MATCHES "CMakeLists.txt")
      # Example has a CMakeLists.txt script: include it
      add_subdirectory(${example})
    else()
      # Create target from files inside the directory
      set(examplesrc "")
      foreach(source ${sources})
        list(APPEND examplesrc ${CMAKE_CURRENT_SOURCE_DIR}/${example}/${source})
      endforeach()

      # Add example executable depending on GCGlib
      add_executable(${example} ${examplesrc} ${GCG_EXAMPLES_EXTRA_SOURCES})

      # Options for MSVC and MinGW
      if(MSVC OR MINGW)
        if(MINGW)
          target_link_libraries(${example} PRIVATE gcg)
          set_target_properties(${example} PROPERTIES LINK_FLAGS "-Wl,-subsystem,console") # Add console window
          set_target_properties(${example} PROPERTIES COMPILE_FLAGS "-w") # Disable all warnings
        else()
          target_link_libraries(${example} PRIVATE gcg)
          set_target_properties(${example} PROPERTIES COMPILE_FLAGS "/D \"_CRT_SECURE_NO_WARNINGS\"")
        endif()
      else()
        target_link_libraries(${example} PRIVATE gcg rt)
      endif()

      add_dependencies(${example} gcg)

      # Install sources
      install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${example} DESTINATION "${GCG_INSTALL_EXAMPLES_DIR}/src" COMPONENT ExamplesSrc
              FILE_PERMISSIONS      OWNER_WRITE OWNER_READ GROUP_WRITE GROUP_READ WORLD_READ
              DIRECTORY_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_WRITE GROUP_READ WORLD_EXECUTE WORLD_READ
              PATTERN "CMake*.*" EXCLUDE)

      # Install binary examples into examples directory
      install(TARGETS ${example}
              RUNTIME DESTINATION       "${GCG_INSTALL_BINARY_DIR}"           PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_WRITE GROUP_READ WORLD_EXECUTE WORLD_READ COMPONENT ExamplesBin
              LIBRARY DESTINATION       "${GCG_INSTALL_LIBRARY_DIR}"          PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_WRITE GROUP_READ WORLD_EXECUTE WORLD_READ COMPONENT ExamplesBin
              ARCHIVE DESTINATION       "${GCG_INSTALL_EXAMPLES_DIR}/lib"     PERMISSIONS OWNER_WRITE OWNER_READ GROUP_WRITE GROUP_READ WORLD_READ COMPONENT ExamplesSrc
              PUBLIC_HEADER DESTINATION "${GCG_INSTALL_EXAMPLES_DIR}/include" PERMISSIONS OWNER_WRITE OWNER_READ GROUP_WRITE GROUP_READ WORLD_READ COMPONENT ExamplesSrc)

    endif()
  endif()
endforeach()


############################################################################
# Define installation for external components needed by examples
############################################################################
if(MSVC OR MINGW)
  # Extra files needed to run examples in Windows
  if(DEFINED GCG_EXTRA_INSTALL_BIN_EXAMPLES)
    install(FILES ${GCG_EXTRA_INSTALL_BIN_EXAMPLES} DESTINATION ${GCG_INSTALL_BINARY_DIR} COMPONENT Binaries
            PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_WRITE GROUP_READ WORLD_EXECUTE WORLD_READ)
  endif()
endif()
