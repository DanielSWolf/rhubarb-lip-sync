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

enable_testing()

file(GLOB TEST_FILES *Test.cpp)
foreach(file ${TEST_FILES})
  get_filename_component(file_basename ${file} NAME_WE)
  add_executable(${file_basename} ${file})
  target_include_directories(${file_basename} PUBLIC ../include/ ${PROJECT_BINARY_DIR})
  default_opts(${file_basename})
  add_test(NAME ${file_basename} COMMAND ${file_basename})
endforeach()
