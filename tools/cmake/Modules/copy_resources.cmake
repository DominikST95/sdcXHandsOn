
function(copy_resources dest)
	message(STATUS "--- Check missing resource files in destination directory")
	set(PATH_TO_RESOURCES ${CMAKE_SOURCE_DIR}/resources)
				
	set(RESOURCE_FILES "")
	set(FOLDERS_TO_COPY "")

	# NOTE: APPEND ALL FOLDERS AND SUBFOLDERS THAT NEEDS TO BE COPIED
	list(APPEND FOLDERS_TO_COPY "certificates")
	# ...

	foreach(currentFolder ${FOLDERS_TO_COPY})
		# 1. For 1.st time CMake configuration, copy the whole folder if it does not exist
		if(NOT EXISTS "${dest}/${currentFolder}")
			file(COPY ${PATH_TO_RESOURCES}/${currentFolder} DESTINATION ${dest})
			#message(STATUS "--- - Folder: ${PATH_TO_RESOURCES}/${currentFolder} copied!")
		else()
			#message(STATUS "- Folder: ${dest}/${currentFolder} already exists!")
		endif()
		
		# 2. For re-configuration with CMake, check if all resource files of a source folder
		# are found in the destination folder. If a resource file does not exist copy the missing file.
		file(GLOB XMLFilesList ${PATH_TO_RESOURCES}/${currentFolder}/*.xml)
		foreach(xmlfile ${XMLFilesList})
			get_filename_component(barename ${xmlfile} NAME)
			configure_file(${xmlfile} ${dest}/${currentFolder} COPYONLY)
			#message(STATUS "--- --- File: ${xmlfile} copied to destination!")
		endforeach() 
	endforeach()
	
	#Some individual XML files to copy from the root resources folder 
	file(GLOB RootXMLFilesList ${PATH_TO_RESOURCES}/*.xml)
		foreach(xmlfile ${RootXMLFilesList})
			get_filename_component(barename ${xmlfile} NAME)
			configure_file(${xmlfile} ${dest} COPYONLY)
			#message(STATUS "--- --- File: ${xmlfile} copied to destination!")
		endforeach() 
endfunction()
