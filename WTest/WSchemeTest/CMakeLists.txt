file(GLOB_RECURSE SOURCE_FILES
    *.cpp)
file(GLOB_RECURSE HEADER_FILES
    *.h
    *.hpp)

add_executable(WSchemeTest ${SOURCE_FILES} ${HEADER_FILES})

target_include_directories(WSchemeTest PUBLIC ${SDK_PATH})
target_include_directories(WSchemeTest PUBLIC ${CMAKE_SOURCE_DIR})

target_link_libraries(WSchemeTest
    WBase
    WFramework
    WScheme)
