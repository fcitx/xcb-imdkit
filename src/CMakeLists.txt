add_library(xcb-imdkit SHARED parser.c imdkit.c)
target_link_libraries(xcb-imdkit XCB::XCB XCB::UTIL)
