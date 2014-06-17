add_executable(test_server test_server.c)
target_link_libraries(test_server ${XCB_LIBRARIES} xcb-imdkit)
