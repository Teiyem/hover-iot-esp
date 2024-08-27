cmake_minimum_required(VERSION 3.5)
set(PROJECT_VER 1.0.0)

if(DEFINED ENV{HOVER_ESP_PATH})
    set(HOVER_ESP_PATH $ENV{HOVER_ESP_PATH})
else()
    set(HOVER_ESP_PATH ${CMAKE_CURRENT_LIST_DIR}/../..)
endif(DEFINED ENV{HOVER_ESP_PATH})

set(EXTRA_COMPONENT_DIRS ${HOVER_ESP_PATH}/components)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(hover-iot-esp-simple)