set(CMAKE_CXX_FLAGS_DEBUG_TOOL "${CMAKE_CXX_FLAGS_DEBUG_TOOL} -D_USRDLL -DWS_BUILD_DLL")
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
	set(CMAKE_CXX_FLAGS_DEBUG_TOOL "${CMAKE_CXX_FLAGS_DEBUG_TOOL} /wd4251 /wd4275")
endif()

file(GLOB_RECURSE SOURCE_FILES
    *.cpp)
file(GLOB_RECURSE HEADER_FILES
    *.h
    *.hpp)

add_library(WScheme SHARED ${SOURCE_FILES} ${HEADER_FILES})

target_include_directories(WScheme PUBLIC ${SDK_PATH})
target_include_directories(WScheme PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(WScheme WBase WFramework)
