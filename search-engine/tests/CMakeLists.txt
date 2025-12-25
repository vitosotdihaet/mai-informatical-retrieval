cmake_minimum_required(VERSION 3.16)

find_package(GTest REQUIRED)

add_executable(skiplist_tests 
    test_skiplist.cpp
)

target_include_directories(skiplist_tests 
    PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(skiplist_tests 
    PRIVATE 
    GTest::gtest 
    GTest::gtest_main
)

gtest_discover_tests(skiplist_tests)

# # Optionally, add a manual test (non-GoogleTest version)
# add_executable(skiplist_manual_test 
#     manual_test.cpp
# )
# target_include_directories(skiplist_manual_test PRIVATE 
#     ${CMAKE_SOURCE_DIR}/include
# )

add_executable(hashmap_tests 
    test_hashmap.cpp
)

target_include_directories(hashmap_tests 
    PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(hashmap_tests 
    PRIVATE 
    GTest::gtest 
    GTest::gtest_main
)

gtest_discover_tests(hashmap_tests)


add_executable(boolean_index_tests 
    test_boolean_index.cpp
)

target_include_directories(boolean_index_tests 
    PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(boolean_index_tests 
    PRIVATE 
    GTest::gtest 
    GTest::gtest_main
)

gtest_discover_tests(boolean_index_tests)


# Add to 'test' target
add_custom_target(test_all
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS skiplist_tests
)