configure_file(Registry.ini.in ${CMAKE_CURRENT_BINARY_DIR}/Registry.ini)
configure_file(Runtime.ini.in ${CMAKE_CURRENT_BINARY_DIR}/Runtime.ini)
configure_file(Zmq.ini.in ${CMAKE_CURRENT_BINARY_DIR}/Zmq.ini)

add_executable(ResultReplyObject_test ResultReplyObject_test.cpp)
target_link_libraries(ResultReplyObject_test ${TESTLIBS})

add_test(ResultReplyObject ResultReplyObject_test)
