add_library(xcb-imdkit SHARED parser.c ximproto.c imdkit.c protocolhandler.c message.c common.c imclient.c clientprotocolhandler.c)
# add_library(xcb-imdkit SHARED parser.c imdkit.c)
target_link_libraries(xcb-imdkit XCB::XCB XCB::UTIL)

configure_file(xcb-imdkit.pc.in ${CMAKE_CURRENT_BINARY_DIR}/xcb-imdkit.pc)

install(TARGETS xcb-imdkit LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/xcb-imdkit.pc
  DESTINATION "${CMAKE_INSTALL_LIBDATADIR}/pkgconfig")
install(FILES imdkit.h ximproto.h ximproto-gen.h imclient.h DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/xcb-imdkit")
