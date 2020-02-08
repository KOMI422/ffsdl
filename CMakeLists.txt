cmake_minimum_required(VERSION 3.2)

project(ffsdl)
set(CMAKE_VERBOSE_MAKEFILE ON)

#add_definitions(-m64)
add_compile_options(-std=c++11)
add_compile_options(-pthread)
add_compile_options(-Wl,--no-as-needed)

aux_source_directory(./ SRC_LIST)
aux_source_directory(./ffcodec SRC_LIST)
aux_source_directory(./flvcpp SRC_LIST)

add_executable(ffsdl ${SRC_LIST})
#message(${PROJECT_SOURCE_DIR})
target_include_directories(ffsdl PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_directories(ffsdl PUBLIC ${PROJECT_SOURCE_DIR}/lib)
target_link_libraries(ffsdl -lavutil -lavformat -lavfilter -lavcodec -lswscale -lSDL2main -lSDL2)