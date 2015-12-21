function(copy_after_build sourceGlob relativeTargetDirectory)
	# Set `sourcePaths`
	file(GLOB sourcePaths "${sourceGlob}")
	
	foreach(sourcePath ${sourcePaths})
		# Set `fileName`
		get_filename_component(fileName "${sourcePath}" NAME)
		
		# Set `targetPath`
		set(targetPath "${CMAKE_BINARY_DIR}/${relativeTargetDirectory}/${fileName}")
		
		add_custom_command(TARGET rhubarb POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy "${sourcePath}" "${targetPath}"
			COMMENT "Creating '${relativeTargetDirectory}/${fileName}'"
		)
	endforeach()
endfunction()
