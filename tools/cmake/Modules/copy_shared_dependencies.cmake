
function(copy_shared_dependencies dest)
	message(STATUS "--- Check missing shared dependencies files in destination directory")
	set(PATH_TO_SHARED_DEPENDENCIES ${SDCX_INSTALL_PATH}/bin)
				
	# For re-configuration with CMake, check if all shared dependencies files of the source folder
	# are found in the destination folder. If a file does not exist copy the missing file.
	set(SHARED_DEPENDENCIES_LIST "")
	if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
		file(GLOB SHARED_DEPENDENCIES_LIST ${PATH_TO_SHARED_DEPENDENCIES}/*.dll)
	elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
		file(GLOB SHARED_DEPENDENCIES_LIST ${PATH_TO_SHARED_DEPENDENCIES}/*.so)
	endif()
	foreach(file ${SHARED_DEPENDENCIES_LIST})
		get_filename_component(barename ${file} NAME)
		configure_file(${file} ${dest} COPYONLY)
		#message(STATUS "--- --- File: ${file} copied to destination!")
	endforeach() 
endfunction()
