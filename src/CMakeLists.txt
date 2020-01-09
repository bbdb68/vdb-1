

# message(${CMAKE_CURRENT_SOURCE_DIR})
add_library(vdb vdb.cpp)
target_include_directories(vdb PUBLIC
                    ${CMAKE_CURRENT_SOURCE_DIR}/../include )
target_include_directories(vdb PRIVATE
                    ${CMAKE_CURRENT_SOURCE_DIR}/../lib/freetype/include
                    ${CMAKE_CURRENT_SOURCE_DIR}/../include/SDL 
					${CMAKE_CURRENT_SOURCE_DIR}/../include/vdb
					)
