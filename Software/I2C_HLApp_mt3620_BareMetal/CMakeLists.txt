#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required(VERSION 3.10)

project(I2C_HLApp_mt3620_BareMetal C)

azsphere_configure_tools(TOOLS_REVISION "20.07")
azsphere_configure_api(TARGET_API_SET "6")

add_executable(${PROJECT_NAME} main.c eventloop_timer_utilities.c)
target_link_libraries(${PROJECT_NAME} applibs pthread gcc_s c)

target_include_directories(${PROJECT_NAME} PUBLIC
                           ./)

azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "../../Hardware/usi_mt3620_bt_combo" TARGET_DEFINITION "usi_mt3620_bt_combo.json")

azsphere_target_add_image_package(${PROJECT_NAME})
