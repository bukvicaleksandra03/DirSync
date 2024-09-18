add_executable(unit_tests
            unit_tests.cpp
            map_test.cpp)
target_include_directories(unit_tests PRIVATE ../src/inc)
add_test(NAME MapTest COMMAND unit_tests 1)