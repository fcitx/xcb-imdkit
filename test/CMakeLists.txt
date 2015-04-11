add_executable(test_server test_server.c)
target_link_libraries(test_server XCB::XCB XCB::UTIL XCB::KEYSYMS xcb-imdkit)
add_executable(test_client test_client.c)
target_link_libraries(test_client XCB::XCB XCB::UTIL XCB::KEYSYMS xcb-imdkit)

add_executable(test_encoding test_encoding.c)
target_link_libraries(test_encoding xcb-imdkit)
add_test(test_encoding COMMAND test_encoding)
