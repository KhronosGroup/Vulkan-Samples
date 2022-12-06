# Copyright (c) 2022, Arm Limited and Contributors
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

FROM ubuntu:20.04

# Vulkan Samples Docker
# This image is using the the KhronosGroup/Vulkan-Samples project
# 
# When adding new dependencies the /tmp directory can be used for building and installation
# This is cleaned at the end of the docker build

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
        wget \
        alien \
        git \
        libssl-dev \
        build-essential \
        checkinstall \
        dos2unix \
        unzip \
        openjdk-11-jdk \
        python3 \
        python3-pip \
        flex \
        libglu1-mesa-dev \
        xorg-dev \
        adb \
        software-properties-common

RUN pip3 --no-cache-dir install \
    pyyaml \
    requests

# Install Clang-Format and Clang-Tidy
# Clang-Tidy-9 required for Doxygen
ENV LLVM_VERSION=10

RUN wget -O- https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add - \
    && apt-add-repository "deb http://apt.llvm.org/focal/ llvm-toolchain-focal-$LLVM_VERSION main" \
    && apt-add-repository "deb http://apt.llvm.org/focal/ llvm-toolchain-focal-9 main" \
    && apt-get -yqq update \
    && apt-get -yqq install \
        clang-format-$LLVM_VERSION \
        clang-tidy-$LLVM_VERSION \
        clang-tidy-9 \
        ccache

# Install Gradle
ENV GRADLE_VERSION_MAJOR=7 \
    GRADLE_VERSION_MINOR=0 \
    GRADLE_VERSION_PATCH=2 \
    GRADLE_HOME=/usr/local/gradle

ENV GRADLE_VERSION=$GRADLE_VERSION_MAJOR.$GRADLE_VERSION_MINOR.$GRADLE_VERSION_PATCH

RUN wget -q https://services.gradle.org/distributions/gradle-$GRADLE_VERSION-bin.zip -O /tmp/gradle.zip \
    && unzip -qq /tmp/gradle.zip -d /usr/local \
    && mv /usr/local/gradle-* $GRADLE_HOME

# Configure Android env variables
ENV ANDROID_SDK=/usr/local/android-sdk
ENV ANDROID_HOME=$ANDROID_SDK \
    ANDROID_SDK_TOOLS=$ANDROID_SDK/cmdline-tools
ENV PATH="$PATH:$GRADLE_HOME/bin:/usr/local/doxygen/bin"
RUN mkdir -p $ANDROID_HOME

# Confugre SDK Tools
ENV SDK_TOOLS_VERSION=6514223

ADD files/package-list.txt $ANDROID_HOME/package-list.txt

RUN set -x && wget -q https://dl.google.com/android/repository/commandlinetools-linux-${SDK_TOOLS_VERSION}_latest.zip -O /tmp/sdk-tools-linux.zip \
    && echo ${ANDROID_SDK_TOOLS} \
    && mkdir -p ${ANDROID_SDK_TOOLS} \
    && unzip -qq /tmp/sdk-tools-linux.zip -d ${ANDROID_SDK_TOOLS} \
    && echo y | $ANDROID_SDK_TOOLS/tools/bin/sdkmanager --package_file=$ANDROID_HOME/package-list.txt --verbose

# Install Doxygen
ENV DOXYGEN_VERSION_MAJOR=1 \
    DOXYGEN_VERSION_MINOR=9 \
    DOXYGEN_VERSION_PATCH=5

ENV DOXYGEN_VERSION=$DOXYGEN_VERSION_MAJOR.$DOXYGEN_VERSION_MINOR.$DOXYGEN_VERSION_PATCH

RUN wget https://doxygen.nl/files/doxygen-$DOXYGEN_VERSION.linux.bin.tar.gz -O /tmp/doxygen.tar.gz \
    && tar -xzf /tmp/doxygen.tar.gz -C /usr/local \
    && mv /usr/local/doxygen-* /usr/local/doxygen

# Install CMake
ENV CMAKE_VERSION_MAJOR=3 \
    CMAKE_VERSION_MINOR=22 \
    CMAKE_VERSION_PATCH=1

ENV CMAKE_VERSION=$CMAKE_VERSION_MAJOR.$CMAKE_VERSION_MINOR.$CMAKE_VERSION_PATCH

RUN set -x && echo "Installing $CMAKE_VERSION" \
    && apt-get remove --purge --auto-remove cmake \
    && wget https://cmake.org/files/v$CMAKE_VERSION_MAJOR.$CMAKE_VERSION_MINOR/cmake-$CMAKE_VERSION.tar.gz -O /tmp/cmake.tar.gz \
    && cd /tmp && tar -xzvf /tmp/cmake.tar.gz \
    && cd /tmp/cmake-${CMAKE_VERSION} \
    && ./bootstrap \
    && make -j$(nproc) \
    && make install \
    && cd ~/ && cmake --version

# Install Latest ImageMagick
RUN wget https://www.imagemagick.org/download/ImageMagick.tar.gz -O /tmp/ImageMagick.tar.gz \
    && cd /tmp && tar -xf /tmp/ImageMagick.tar.gz \
    && cd /tmp/ImageMagick-* \
    && ./configure \
    && make \
    && make install \
    && ldconfig /usr/local/lib

# Create lexer used in snake_case.py
ADD scripts/vk_convert.lex /tmp/vk_convert.lex
RUN set -x \
    && cd /tmp \
    && flex -+ vk_convert.lex \
    && gcc lex.yy.cc -ll -lstdc++ \
    && mv ./a.out /usr/local/bin 

# Add Other scripts
ADD scripts/check_copyright_headers.py /usr/local/bin/check_copyright_headers.py
ADD scripts/snake_case.py /usr/local/bin/snake_case.py
ADD scripts/run-clang-tidy.py /usr/local/bin/run-clang-tidy.py
ADD scripts/clang_format.py /usr/local/bin/clang_format.py

# Convert script file endings from dos to unix.
# Fixes an issue when trying to run the script from inside the docker container
RUN set -x \
    && dos2unix /usr/local/bin/check_copyright_headers.py \
    && dos2unix /usr/local/bin/snake_case.py \
    && dos2unix /usr/local/bin/run-clang-tidy.py \
    && dos2unix /usr/local/bin/clang_format.py 

# Finalise script setup
RUN set -x \
    && git config --global clangFormat.binary clang-format-10 \
    && git config --global clangFormat.style file \
    && git config --global clangFormat.extensions h,hpp,c,cpp \
    && chmod +x /usr/local/bin/snake_case.py \
    && chmod +x /usr/local/bin/run-clang-tidy.py \
    && chmod +x /usr/local/bin/clang_format.py 

# Remove all files created in the /tmp folder
# Dependencies should be installed above this command
RUN rm -rf /tmp/*
