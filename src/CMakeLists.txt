add_library(xcb-imdkit SHARED parser.c ximproto.c imdkit.c protocolhandler.c message.c common.c imclient.c clientprotocolhandler.c)
# add_library(xcb-imdkit SHARED parser.c imdkit.c)
target_link_libraries(xcb-imdkit XCB::XCB XCB::UTIL)
