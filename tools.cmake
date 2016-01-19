function(copy_and_install sourceGlob relativeTargetDirectory)
	# Set `sourcePaths`
	file(GLOB sourcePaths "${sourceGlob}")
	
	foreach(sourcePath ${sourcePaths})
		if(NOT IS_DIRECTORY ${sourcePath})
			# Set `fileName`
			get_filename_component(fileName "${sourcePath}" NAME)

			# Copy file during build
			add_custom_command(TARGET rhubarb POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy "${sourcePath}" "$<TARGET_FILE_DIR:rhubarb>/${relativeTargetDirectory}/${fileName}"
				COMMENT "Creating '${relativeTargetDirectory}/${fileName}'"
			)

			# Install file
			install(
				FILES "${sourcePath}"
				DESTINATION "${relativeTargetDirectory}"
			)
		endif()
	endforeach()
endfunction()
