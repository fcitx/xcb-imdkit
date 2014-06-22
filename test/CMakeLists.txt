add_executable(test_server test_server.c)
target_link_libraries(test_server XCB::XCB XCB::UTIL XCB::KEYSYMS xcb-imdkit)
