set(CMAKE_CXX_FLAGS_RELEASE "-O3")
message(STATUS "making library for tips ds")
file(GLOB tipsDS "*.c" "*.h" "*.cpp" "*.hpp")

add_library(tipsDS STATIC ${tipsDS})

target_include_directories(tipsDS PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
INSTALL(TARGETS tipsDS
        ARCHIVE DESTINATION ${CMAKE_SOURCE_DIR}
        )

