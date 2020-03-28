

add_executable(demo_app main.cpp)

target_include_directories(demo_app PRIVATE
                    ${CMAKE_CURRENT_SOURCE_DIR}/../lib/freetype/include
                    ${CMAKE_CURRENT_SOURCE_DIR}/../include/SDL )

target_link_directories(demo_app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../lib/sdl/x64)
target_link_libraries(demo_app SDL2)