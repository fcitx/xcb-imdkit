add_executable(test_server test_server.c)
target_link_libraries(test_server XCB::XCB XCB::UTIL XCB::KEYSYMS xcb-imdkit)
add_executable(test_client test_client.c)
target_link_libraries(test_client XCB::XCB XCB::UTIL XCB::KEYSYMS xcb-imdkit)
add_executable(client_demo client_demo.c)
target_link_libraries(client_demo XCB::XCB XCB::UTIL XCB::KEYSYMS xcb-imdkit)

add_executable(test_encoding test_encoding.c)
target_link_libraries(test_encoding xcb-imdkit)
add_test(NAME test_encoding COMMAND test_encoding)

if (ENABLE_TEST_WITH_XSERVER)
    add_executable(test_xserver_ready test_xserver_ready.c)
    target_link_libraries(test_xserver_ready XCB::XCB)
    add_library(test_xim_server STATIC test_xim_server.c)
    target_link_libraries(test_xim_server
        XCB::XCB
        XCB::UTIL
        Threads::Threads
        xcb-imdkit
    )

    function(add_xserver_test TEST_NAME)
        add_executable(${TEST_NAME} ${TEST_NAME}.c)
        target_link_libraries(${TEST_NAME}
            test_xim_server
        )
        add_test(
            NAME ${TEST_NAME}
            COMMAND /bin/sh "${CMAKE_CURRENT_SOURCE_DIR}/xvfb_wrapper.sh"
                    "${XVFB_BIN}" "$<TARGET_FILE:test_xserver_ready>"
                    "$<TARGET_FILE:${TEST_NAME}>"
        )
        set_tests_properties(${TEST_NAME} PROPERTIES TIMEOUT 15)
    endfunction()

    add_xserver_test(test_sync)
    add_xserver_test(test_basic)
    add_xserver_test(test_multiple)
endif()
