cmake_minimum_required(VERSION 3.10)
project(hxr_mqtt)

option(hxr_mqtt_wrapper "mqtt wrapper for hxr team" ON)
option(hxr_mqtt_wrapper_test "test project using hxr_mqtt_wrapper" ON)

if(hxr_mqtt_wrapper)
    add_subdirectory(mqtt_wrapper)
endif()

if(hxr_mqtt_wrapper_test)
    add_subdirectory(example)
endif()
