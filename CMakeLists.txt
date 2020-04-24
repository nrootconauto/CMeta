cmake_minimum_required(VERSION 3.13)

project(cMeta)

add_subdirectory("${CMAKE_SOURCE_DIR}/ext/argparse/")

add_library(sds STATIC "${CMAKE_SOURCE_DIR}/ext/sds/sds.c")
add_library(unescaper STATIC "${CMAKE_SOURCE_DIR}/ext/C_Unescaper/escaper.c" "${CMAKE_SOURCE_DIR}/ext/C_Unescaper/numberParser.c")

add_custom_command(
  OUTPUT "${CMAKE_SOURCE_DIR}/lexer.c"
  COMMAND re2c -T  "${CMAKE_SOURCE_DIR}/lexer.re.c" -o "${CMAKE_SOURCE_DIR}/lexer.c"
  DEPENDS "${CMAKE_SOURCE_DIR}/lexer.re.c" )

add_custom_command(
  OUTPUT "${CMAKE_SOURCE_DIR}/CMeta.c"
  COMMAND re2c -T  "${CMAKE_SOURCE_DIR}/CMeta.re.c" -o "${CMAKE_SOURCE_DIR}/CMeta.c"
  DEPENDS "${CMAKE_SOURCE_DIR}/CMeta.re.c" )

add_custom_command(
  OUTPUT "${CMAKE_SOURCE_DIR}/fillOutTemplate.c"
  COMMAND re2c -T  "${CMAKE_SOURCE_DIR}/fillOutTemplate.re.c" -o "${CMAKE_SOURCE_DIR}/fillOutTemplate.c"
  DEPENDS "${CMAKE_SOURCE_DIR}/lexer.re.c" )

add_executable(CMeta "${CMAKE_SOURCE_DIR}/fillOutTemplate.c" "${CMAKE_SOURCE_DIR}/lexer.c" "${CMAKE_SOURCE_DIR}/util.c" "${CMAKE_SOURCE_DIR}/main.c" "${CMAKE_SOURCE_DIR}/CMeta.c")
target_link_libraries(CMeta PUBLIC sds argparse unescaper)

install(PROGRAMS "${CMAKE_BINARY_DIR}/CMeta" DESTINATION bin)
