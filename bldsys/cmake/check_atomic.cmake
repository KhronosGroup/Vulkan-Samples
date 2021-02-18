# Copyright (c) 2020, Arm Limited and Contributors
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 the "License";
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# check whether need to link atomic library explicitly
INCLUDE(CheckCXXSourceCompiles)
INCLUDE(CheckLibraryExists)

if(NOT DEFINED VULKAN_COMPILER_IS_GCC_COMPATIBLE)
  if(CMAKE_COMPILER_IS_GNUCXX)
    set(VULKAN_COMPILER_IS_GCC_COMPATIBLE ON) 
  elseif( MSVC )
    set(VULKAN_COMPILER_IS_GCC_COMPATIBLE OFF)
  elseif( "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" )
    set(VULKAN_COMPILER_IS_GCC_COMPATIBLE ON) 
  elseif( "${CMAKE_CXX_COMPILER_ID}" MATCHES "Intel" )
    set(VULKAN_COMPILER_IS_GCC_COMPATIBLE ON) 
  endif()
endif()

# Sometimes linking against libatomic is required for atomic ops, if
# the platform doesn't support lock-free atomics.

function(check_working_cxx_atomics varname)
  set(OLD_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
  set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -std=c++11")
  CHECK_CXX_SOURCE_COMPILES("
#include <atomic>
std::atomic<int> x;
std::atomic<short> y;
std::atomic<char> z;
int main() {
  ++z;
  ++y;
  return ++x;
}
" ${varname})
  set(CMAKE_REQUIRED_FLAGS ${OLD_CMAKE_REQUIRED_FLAGS})
endfunction(check_working_cxx_atomics)

function(check_working_cxx_atomics64 varname)
  set(OLD_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
  set(CMAKE_REQUIRED_FLAGS "-std=c++11 ${CMAKE_REQUIRED_FLAGS}")
  CHECK_CXX_SOURCE_COMPILES("
#include <atomic>
#include <cstdint>
std::atomic<uint64_t> x (0);
int main() {
  uint64_t i = x.load(std::memory_order_relaxed);
  (void)i;
  return 0;
}
" ${varname})
  set(CMAKE_REQUIRED_FLAGS ${OLD_CMAKE_REQUIRED_FLAGS})
endfunction(check_working_cxx_atomics64)

set(NEED_LINK_ATOMIC  OFF CACHE BOOL "whether need to link against atomic library")
if(VULKAN_COMPILER_IS_GCC_COMPATIBLE)
    # check if non-64-bit atomics work without the library.
    check_working_cxx_atomics(HAVE_CXX_ATOMICS_WITHOUT_LIB)
    # check 64-bit atomics work without the library.
    check_working_cxx_atomics64(HAVE_CXX_ATOMICS64_WITHOUT_LIB)
    if (NOT HAVE_CXX_ATOMICS_WITHOUT_LIB OR NOT HAVE_CXX_ATOMICS64_WITHOUT_LIB)
        set(NEED_LINK_ATOMIC  ON CACHE BOOL "whether need to link to atomic library" FORCE)
    endif()
endif()
