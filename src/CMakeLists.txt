
add_library(xcb-imdkit SHARED
parser.c ximproto.c imdkit.c protocolhandler.c message.c common.c imclient.c clientprotocolhandler.c
encoding.c
xlibi18n/lcCT.c
xlibi18n/lcUTF8.c
xlibi18n/lcCharSet.c
)

add_library(XCBImdkit::XCBImdkit ALIAS xcb-imdkit)

target_include_directories(xcb-imdkit
                           PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                           INTERFACE $<INSTALL_INTERFACE:${CMAKE_INSTALL_FULL_INCLUDEDIR}>)

set_target_properties(xcb-imdkit PROPERTIES
                      VERSION 0.1
                      SOVERSION 0
                      C_VISIBILITY_PRESET hidden
                      LINK_FLAGS "-Wl,--no-undefined"
                      EXPORT_NAME XCBImdkit)

target_link_libraries(xcb-imdkit
                      PUBLIC XCB::XCB
                      PRIVATE XCB::UTIL UTHash::UTHash)

configure_file(xcb-imdkit.pc.in ${CMAKE_CURRENT_BINARY_DIR}/xcb-imdkit.pc @ONLY)

install(TARGETS xcb-imdkit EXPORT XCBImdkitLibraryTargets LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/xcb-imdkit.pc
  DESTINATION "${CMAKE_INSTALL_LIBDATADIR}/pkgconfig")
install(FILES imdkit.h ximcommon.h ximproto.h ximproto-gen.h imclient.h encoding.h DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/xcb-imdkit")
