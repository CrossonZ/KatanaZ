include_directories(${PROJECT_SOURCE_DIR}/net)
set(MAIN_SRC test_main.cpp)
add_executable(Server ${MAIN_SRC})
target_link_libraries(Server libnet libtools libgame pthread)
