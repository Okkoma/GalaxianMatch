# Helper function to create a test
function(add_unit_test TEST_NAME)
    set(TEST_TARGET "test_${TEST_NAME}")
    add_executable(${TEST_TARGET} ${ARGN})
    target_include_directories(${TEST_TARGET} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../Source ${URHO3D_INCLUDE_DIRS})
    target_link_libraries(${TEST_TARGET} PRIVATE Catch2::Catch2WithMain Urho3D)
    add_test(NAME ${TEST_TARGET} COMMAND ${TEST_TARGET} WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endfunction()


add_unit_test(
    "SplinePath2D"
    Components/splinepath2d_test.cpp
    ../Source/Components/SplinePath2D.cpp
)

# add_unit_test(
#     "Network"
#     Network/network_test.cpp
#     ../Source/Network/Network.cpp
#     ../Source/Network/NetworkUtils.cpp
# )
