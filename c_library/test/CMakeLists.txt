CMAKE_MINIMUM_REQUIRED(VERSION 3.18.4)
PROJECT(test_bmstools)

FILE(GLOB test_sources src/*.c)
INCLUDE_DIRECTORIES(../bmstools/include/)

ADD_EXECUTABLE(
	${PROJECT_NAME}
	${test_sources}
)
TARGET_LINK_LIBRARIES(
	${PROJECT_NAME}
	PUBLIC bmstools
)