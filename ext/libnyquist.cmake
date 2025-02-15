set(BUILD_EXAMPLE OFF CACHE INTERNAL "")

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(/wd4028 /wd4245 /wd4456 /wd4514 /wd4701 /wd4702 /wd4706)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-stringop-overread")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-reorder -Wno-unused-variable -Wno-sign-compare -Wno-range-loop-construct")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "^(Apple)?Clang$")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-shift-negative-value -Wno-macro-redefined -Wno-static-in-inline")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-reorder-ctor -Wno-unused-private-field -Wno-range-loop-construct -Wno-tautological-overlap-compare")
endif()

add_subdirectory(libnyquist)
