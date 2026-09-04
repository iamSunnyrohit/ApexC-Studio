cmake_minimum_required(VERSION 3.22.1)
project("apexc")

add_library(
        apexc
        SHARED
        native-lib.c
        apex/eval.c
        apex/lexer.c
        apex/parser.c
        apex/codegen.c
)

target_include_directories(apexc PRIVATE apex)

find_library(log-lib log)
target_link_libraries(apexc ${log-lib})