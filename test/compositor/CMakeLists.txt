
find_package(Qt5Core)
find_package(Qt5Test)

set(CMAKE_AUTOMOC ON)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ../../src/compositor ../../src/utils)

add_executable(tst_stringview tst_stringview.cpp ../../src/utils/stringview.cpp)
add_test(tst_stringview tst_stringview)
add_dependencies(check tst_stringview)
target_link_libraries(tst_stringview Qt5::Core Qt5::Test)

add_executable(tst_maybe tst_maybe.cpp)
add_test(tst_maybe tst_maybe)
add_dependencies(check tst_maybe)
target_link_libraries(tst_maybe Qt5::Core Qt5::Test)
