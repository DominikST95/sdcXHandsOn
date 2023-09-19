################################################################################
# setting the RPATH Variable to the current directory "."
################################################################################
message(STATUS "Setting RPATH Variables...")
set(CMAKE_INSTALL_RPATH ".")
message(STATUS "- CMAKE_INSTALL_RPATH set to ${CMAKE_INSTALL_RPATH}")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
message(STATUS "- CMAKE_INSTALL_RPATH_USE_LINK_PATH set to ${CMAKE_INSTALL_RPATH_USE_LINK_PATH}")
################################################################################ 
