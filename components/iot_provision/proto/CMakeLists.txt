cmake_minimum_required(VERSION 3.16)

set(PROTO_COMPILER "protoc")
set(PROTO_C_COMPILER "protoc-c")
set(C_OUT_PATH "${CMAKE_CURRENT_LIST_DIR}/../proto-c")

set(PROTO_SRCS "iot_provision.proto")

add_custom_target(c_proto
    COMMAND ${PROTO_C_COMPILER} --c_out=${C_OUT_PATH} -I . ${PROTOCOMM_INCL_PATH} ${PROTO_SRCS}
    VERBATIM
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    )

add_custom_target(proto ALL
    DEPENDS c_proto python_proto
    VERBATIM
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    )
