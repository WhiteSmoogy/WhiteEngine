file(GLOB_RECURSE source_list
	RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
	*.h
    *.cpp
	)

add_library(WFramework STATIC ${source_list})

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${source_list})


target_include_directories(WFramework PUBLIC ${SDK_PATH})
target_include_directories(WFramework PUBLIC ${CMAKE_SOURCE_DIR})
