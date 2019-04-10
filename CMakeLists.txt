cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(6lowpan_conn_sample_app)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
