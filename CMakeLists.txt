cmake_minimum_required(VERSION 3.5)
project(CanController LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_PREFIX_PATH "C:/Qt/Qt5.12.8/5.12.8/mingw73_64")

# set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_AUTOUIC_SEARCH_PATHS
    ${PROJECT_SOURCE_DIR}/src/Res/Uis
)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

set(PCAN_INCLUDE "D:/Desktop/yc/PartTimeJobs/Windows/PCAN-Basic/Include")

file(GLOB HDRS
    ${PROJECT_SOURCE_DIR}/src/QCommonPonents/*.h
)
# 源文件
file(GLOB SRCS
    ${PROJECT_SOURCE_DIR}/*.cpp
    ${PROJECT_SOURCE_DIR}/src/CO_Utils/*.cpp
    ${PROJECT_SOURCE_DIR}/src/MainWin/*.cpp
    ${PROJECT_SOURCE_DIR}/src/QCommonPonents/*.cpp
)

# file(GLOB UI_FILES
#     ${PROJECT_SOURCE_DIR}/src/Res/Uis/*.ui
# )

add_executable(${PROJECT_NAME} ${SRCS} ${HDRS})

# 头文件路径
target_include_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/CO_Utils
    ${PROJECT_SOURCE_DIR}/src/MainWin
    ${PROJECT_SOURCE_DIR}/src/QCommonPonents
    ${PCAN_INCLUDE}
)

target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Widgets)

# 自动复制 DLL
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "D:/Desktop/yc/PartTimeJobs/Windows/PCAN-Basic/x64/PCANBasic.dll"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
)

add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")