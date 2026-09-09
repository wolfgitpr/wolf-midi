project(wolf-midi-test)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE
        wolf-midi::wolf-midi
)

add_test(NAME wolf-midi COMMAND ${PROJECT_NAME})
