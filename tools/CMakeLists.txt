include_directories(${PROJECT_SOURCE_DIR}/main)
set(TOOLS_SRC UDPThread.cpp SimpleMutex.cpp)
add_library(libtools ${TOOLS_SRC})
set_target_properties(libtools PROPERTIES OUTPUT_NAME "tools")
