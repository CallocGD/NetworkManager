cmake_minimum_required(VERSION 3.21)

project(networkmanager VERSION 0.0.1)

include_directories(include/pthreads include)

add_library(${PROJECT_NAME} STATIC
    src/networkmanager.cpp
    include/link/libcrypto.lib
    include/link/libcurl_a.lib
    include/link/libssl.lib
    include/link/libpthreadVC3.lib
)

