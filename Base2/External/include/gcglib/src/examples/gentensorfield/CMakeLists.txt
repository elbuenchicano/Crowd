##############################################################
# GCGlib: script to build tensor field generator example
##############################################################

# Find required packages and libraries
if(GLUT_FOUND)
  # Add example executable depending on GCGlib
  add_executable(gentensorfield main.cpp squadric.cpp superquadric.h squadric.h tensorglyph.cpp tensorglyph.h  ${GCG_EXAMPLES_EXTRA_SOURCES})

  # Options for MSVC and MinGW
  if(MSVC OR MINGW)
    if(MINGW)
        target_link_libraries(gentensorfield PRIVATE gcg ${GLUT_LIBRARIES} ${OPENGL_LIBRARIES})
        set_target_properties(gentensorfield PROPERTIES LINK_FLAGS "-Wl,-subsystem,console") # Add console window
        set_target_properties(gentensorfield PROPERTIES COMPILE_FLAGS "-w") # Disable all warnings
    else()
        target_link_libraries(gentensorfield PRIVATE gcg ${GLUT_LIBRARIES} ${OPENGL_LIBRARIES})
        set_target_properties(gentensorfield PROPERTIES COMPILE_FLAGS "/D \"_CRT_SECURE_NO_WARNINGS\"")
    endif()

    # Dependencies needed while developing in Windows
    foreach(dep ${GCG_EXTRA_INSTALL_BIN_EXAMPLES})
      add_custom_command(TARGET gentensorfield POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${dep} $<TARGET_FILE_DIR:gentensorfield>)
    endforeach()
  else()
    target_link_libraries(gentensorfield PRIVATE gcg rt ${GLUT_LIBRARIES} ${OPENGL_LIBRARIES})
  endif()

  add_dependencies(gentensorfield gcg)

  # Install sources
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} DESTINATION "${GCG_INSTALL_EXAMPLES_DIR}/src" COMPONENT ExamplesSrc
          FILE_PERMISSIONS      OWNER_WRITE OWNER_READ GROUP_WRITE GROUP_READ WORLD_READ
          DIRECTORY_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_WRITE GROUP_READ WORLD_EXECUTE WORLD_READ
          PATTERN "CMake*.*" EXCLUDE)

  # Install binary examples into documentation directory
  install(TARGETS gentensorfield
        RUNTIME DESTINATION       "${GCG_INSTALL_BINARY_DIR}"           PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_WRITE GROUP_READ WORLD_EXECUTE WORLD_READ COMPONENT ExamplesBin
        LIBRARY DESTINATION       "${GCG_INSTALL_LIBRARY_DIR}"          PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_WRITE GROUP_READ WORLD_EXECUTE WORLD_READ COMPONENT ExamplesBin
        ARCHIVE DESTINATION       "${GCG_INSTALL_EXAMPLES_DIR}/lib"     PERMISSIONS OWNER_WRITE OWNER_READ GROUP_WRITE GROUP_READ WORLD_READ COMPONENT ExamplesSrc
        PUBLIC_HEADER DESTINATION "${GCG_INSTALL_EXAMPLES_DIR}/include" PERMISSIONS OWNER_WRITE OWNER_READ GROUP_WRITE GROUP_READ WORLD_READ COMPONENT ExamplesSrc)
else()
  message("GLUT not found: \"gentensorfield\" example was not included in the build list.")
endif()
