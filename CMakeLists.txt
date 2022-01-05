# Copyright 2021 Daniel Weber, Ahmad Ibrahim, Hamed Nemati, Michael Schwarz, Christian Rossow
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
#     limitations under the License.


cmake_minimum_required(VERSION 3.10)
project(osiris)

# set default build type to release if nothing else is specified
set(default_build_type "Release")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
    # overwrite empty build type
    set(CMAKE_BUILD_TYPE "${default_build_type}")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -Wextra -pedantic")

# Set compile flags for target architecture
if (ARCH STREQUAL INTEL)
    message(STATUS "Compiling for Intel processor.")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DINTEL")
elseif(ARCH STREQUAL AMD)
    message(STATUS "Compiling for AMD processor.")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DAMD")
else()
    message(WARNING "Illegal or missing value for target processor. Set '-DARCH=[INTEL|AMD]'. Defaulting to INTEL.")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DINTEL")
endif()

set(CMAKE_CXX_FLAGS_RELEASE  "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DDEBUGMODE=0")

set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -DDEBUGMODE=1 -ggdb")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -Wno-unused-variable")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUGMODE=1 -ggdb")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -fsanitize=undefined")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize-recover=address")

add_executable(osiris
        src/osiris.cc
        src/executor.cc src/executor.h
        src/code_generator.cc src/code_generator.h
        src/core.cc src/core.h
        src/logger.cc src/logger.h
        src/utils.cc src/utils.h
        src/filter.cc src/filter.h)

# dependencies
find_package(OpenSSL REQUIRED)
target_link_libraries(osiris OpenSSL::Crypto)
target_link_libraries(osiris capstone)
target_link_libraries(osiris stdc++fs)  # GCC version < 9 needs this to support c++ filesystem lib
