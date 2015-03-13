cmake_minimum_required(VERSION 2.8.4)
project(httpd)

set(SOURCE_FILES echo-server-prod.c parseRequest.c )

add_executable(httpd ${SOURCE_FILES} )


TARGET_LINK_LIBRARIES(httpd event )