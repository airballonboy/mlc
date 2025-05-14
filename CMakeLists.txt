cmake_minimum_required(VERSION 3.28)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug) #allow Debug Symbols for gdb debuger

# DELETE EVERYTHING IN BUILD DIR BEFORE CHANGING
option(PRODUCTION_BUILD "Make this a production build" OFF)

project(mLangC)

file(GLOB_RECURSE MY_SOURCES CONFIGURE_DEPENDS
	 "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_executable("${CMAKE_PROJECT_NAME}" ${MY_SOURCES})

set_property(TARGET "${CMAKE_PROJECT_NAME}" PROPERTY CXX_STANDARD 20)

if(MSVC) # If using the VS compiler...

	target_compile_definitions("${CMAKE_PROJECT_NAME}"
							 PUBLIC _CRT_SECURE_NO_WARNINGS)

	set_target_properties(
		"${CMAKE_PROJECT_NAME}"
		PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
	)# no console
	# this is for my version of msvc you can change the path if you need
	set(CMAKE_AR 'C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.43.34808/bin/Hostx64/x64/lib.exe'
			CACHE FILEPATH "Archiver tool")

	# Makes the libs static
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static")
	set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_C_FLAGS} -static -s")
	set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS} -static -s")

else()
	# Makes the libs static
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static-libgcc")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
	set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_C_FLAGS} -static -s")
	set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS} -static -s")

endif()

target_include_directories("${CMAKE_PROJECT_NAME}"
						   PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/"
)



#create sim link for compile_commands.json
execute_process(
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
)
