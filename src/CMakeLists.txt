file(GLOB ${PROJECT_NAME}_SOURCES *.cpp)
file(GLOB ${PROJECT_NAME}_HEADERS *.h)
file(GLOB_RECURSE ${PROJECT_NAME}_HEADERS cppcodec/*.hpp)

set(${PROJECT_NAME}_DEPS
    ${${PROJECT_NAME}_SOURCES}
    ${${PROJECT_NAME}_HEADERS}
)

if(WIN32)
    add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_DEPS} windows/resource.rc)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -static")
    target_link_libraries(${PROJECT_NAME} ws2_32)
else()
    add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_DEPS})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
    target_link_libraries(${PROJECT_NAME} pthread)
endif()

target_link_libraries(${PROJECT_NAME} sodium)
