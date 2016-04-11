##############################################################
# GCGlib: Script to setup environment and compiler settings
##############################################################

# Find required packages and libraries
find_package(GLUT QUIET)
if(GLUT_FOUND)
  include_directories(${GLUT_INCLUDE_DIR})
endif()
