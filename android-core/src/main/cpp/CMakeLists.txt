cmake_minimum_required(VERSION 3.4.1)

add_library(nHelper SHARED
            nHelper.cpp)

# Include libraries needed for nHelper lib
target_link_libraries(nHelper
                      android
                      log)