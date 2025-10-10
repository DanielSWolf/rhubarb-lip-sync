set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIRECTORY}")
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIRECTORY}")

set(gcc_like_cxx "$<OR:$<CXX_COMPILER_ID:ARMClang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>")
set(msvc_cxx "$<CXX_COMPILER_ID:MSVC>")
  
function(default_opts target)
target_compile_options(${target} PRIVATE
    "$<${gcc_like_cxx}:$<BUILD_INTERFACE:-Wall;-Wextra;-Weffc++>>"
    "$<${msvc_cxx}:$<BUILD_INTERFACE:-W3>>"
)
endfunction()

function(add_example name)
    add_executable(${name} ${ARGN})
    target_include_directories(${name} PUBLIC ../include/ ${PROJECT_BINARY_DIR})
    default_opts(${name})
endfunction()

# Check we don't define any hard symbols in the headers
add_example(test-hard test-hard-1.cpp test-hard-2.cpp)

file(GLOB TEST_FILES test[0-9]*.cpp)
foreach(file ${TEST_FILES})
    get_filename_component(file_basename ${file} NAME_WE)
    add_example(${file_basename} ${file})
endforeach()
