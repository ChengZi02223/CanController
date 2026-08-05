cmake_minimum_required(VERSION 3.5)
project(CanController LANGUAGES CXX)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Qt mingw固定路径（Release库目录）
set(CMAKE_PREFIX_PATH "C:/Qt/Qt5.12.8/5.12.8/mingw73_64")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS
    ${PROJECT_SOURCE_DIR}/src/Res/Uis
)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ========== 新增Release关键配置 ==========
# Release编译优化，去除调试符号
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -s -DNDEBUG")
# Debug编译保留调试信息
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")

# 分模式定义Qt调试宏
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_definitions(
        QT_NO_DEBUG
        QT_NO_DEBUG_OUTPUT
        NDEBUG
    )
else()
    add_compile_definitions(QT_DEBUG _DEBUG)
endif()
# ========================================

find_package(Qt5 COMPONENTS Widgets REQUIRED)

set(PCAN_DIR  ${PROJECT_SOURCE_DIR}/third_party/PCANBasic)
set(PCAN_INCLUDE_DIR ${PCAN_DIR}/include)
set(PCAN_LIB_DIR ${PCAN_DIR}/lib)
add_library(PCANBasic::PCANBasic SHARED IMPORTED)
set_target_properties(PCANBasic::PCANBasic PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES ${PCAN_INCLUDE_DIR}
    IMPORTED_LOCATION ${PCAN_LIB_DIR}/PCANBasic.dll
    IMPORTED_IMPLIB ${PCAN_LIB_DIR}/PCANBasic.lib
)

file(GLOB HDRS
    ${PROJECT_SOURCE_DIR}/src/QCommonPonents/*.h
)
file(GLOB SRCS
    ${PROJECT_SOURCE_DIR}/*.cpp
    ${PROJECT_SOURCE_DIR}/src/CO_Utils/*.cpp
    ${PROJECT_SOURCE_DIR}/src/MainWin/*.cpp
    ${PROJECT_SOURCE_DIR}/src/QCommonPonents/*.cpp
    ${PROJECT_SOURCE_DIR}/src/Manager/*.cpp
    ${PROJECT_SOURCE_DIR}/src/CAN_CMD/*.cpp
)
file(GLOB QRC_FILE
    ${PROJECT_SOURCE_DIR}/src/Res/tools/tool.qrc
)

add_executable(${PROJECT_NAME} ${SRCS} ${HDRS} ${QRC_FILE})

# ========== 新增：Release隐藏控制台窗口 ==========
if(WIN32 AND MINGW)
    set_target_properties(CanController PROPERTIES WIN32_EXECUTABLE TRUE)
    target_link_options(CanController PRIVATE -mwindows)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/CO_Utils
    ${PROJECT_SOURCE_DIR}/src/MainWin
    ${PROJECT_SOURCE_DIR}/src/QCommonPonents
    ${PROJECT_SOURCE_DIR}/src/Manager
    ${PROJECT_SOURCE_DIR}/src/CAN_CMD
)
target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Widgets PCANBasic::PCANBasic)

# 复制PCAN dll
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "D:/Desktop/yc/PartTimeJobs/Windows/PCAN-Basic/x64/PCANBasic.dll"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
)

add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")