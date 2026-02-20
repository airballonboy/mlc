cmake_minimum_required(VERSION 3.20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug) #allow Debug Symbols for gdb debuger

# DELETE EVERYTHING IN BUILD DIR BEFORE CHANGING
option(PRODUCTION_BUILD "Make this a production build" OFF)

project(mlc)

file(GLOB_RECURSE MLC_SOURCES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)
file(GLOB_RECURSE MLC_TEST_SOURCES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/mlc-test/*.cpp"
)

add_executable("mlc" ${MLC_SOURCES})
add_executable("mlc-test" ${MLC_TEST_SOURCES})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

target_compile_definitions("mlc" PUBLIC PROJECT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/")
target_compile_definitions("mlc" PUBLIC MSTD_PATH="${CMAKE_CURRENT_SOURCE_DIR}/mlang-std/")
target_compile_definitions("mlc" PUBLIC OUTPUT_PATH="${CMAKE_BINARY_DIR}/")
target_compile_definitions("mlc-test" PUBLIC OUTPUT_PATH="${CMAKE_BINARY_DIR}/")
target_compile_definitions("mlc-test" PUBLIC MTEST_PATH="${CMAKE_CURRENT_SOURCE_DIR}/mlc-test/")

if(MSVC) # If using the VS compiler...

    target_compile_definitions("mlc" PUBLIC _CRT_SECURE_NO_WARNINGS)
    target_compile_definitions("mlc-test" PUBLIC _CRT_SECURE_NO_WARNINGS)

    target_compile_features("mlc" PRIVATE cxx_std_20)
    target_compile_features("mlc-test" PRIVATE cxx_std_20)

    # this is for my version of msvc you can change the path if you need
    set(CMAKE_AR 'C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.43.34808/bin/Hostx64/x64/lib.exe'
            CACHE FILEPATH "Archiver tool")

    # Makes the libs static
    set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_C_FLAGS} -static -s")
    set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS} -static -s")

elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")
    set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS} -static -lc++")
else()
    # Makes the libs static
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static-libgcc")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -static-libgcc -static-libstdc++")
    set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_C_FLAGS} -static -s")
    set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS} -static -s")
endif()


target_include_directories("mlc"
                           PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/"
)
target_include_directories("mlc-test"
                           PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/"
)



#create sim link for compile_commands.json
execute_process(
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
)
