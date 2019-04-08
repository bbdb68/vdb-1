

message(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../lib/freetype/include
                    ${CMAKE_CURRENT_SOURCE_DIR}/../include 
                    ${CMAKE_CURRENT_SOURCE_DIR}/../include/SDL 
					${CMAKE_CURRENT_SOURCE_DIR}/../include/vdb
					)
add_library(vdb vdb.cpp)
