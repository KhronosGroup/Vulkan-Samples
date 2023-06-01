:: Copyright (c) 2019-2023, Arm Limited and Contributors
::
:: SPDX-License-Identifier: Apache-2.0
::
:: Licensed under the Apache License, Version 2.0 the "License";
:: you may not use this file except in compliance with the License.
:: You may obtain a copy of the License at
::
::     http://www.apache.org/licenses/LICENSE-2.0
::
:: Unless required by applicable law or agreed to in writing, software
:: distributed under the License is distributed on an "AS IS" BASIS,
:: WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
:: See the License for the specific language governing permissions and
:: limitations under the License.


@echo off

set SCRIPT_DIR=%~dp0

set ROOT_DIR=%SCRIPT_DIR%..\..

if [%1] == [] (
    set SAMPLE_NAME=SampleTest
) else (
    set SAMPLE_NAME=%1
)

if [%2] == [] (
    set CATEGORY=api
) else (
    set CATEGORY=%2
)

if [%3] == [] (
    set BUILD_DIR=%ROOT_DIR%\samples\%CATEGORY%
) else (
    set BUILD_DIR=%3
)

call cmake.exe -DSAMPLE_NAME=%SAMPLE_NAME%^
			   -DOUTPUT_DIR="%BUILD_DIR%"^
			   -P "%ROOT_DIR%\bldsys\cmake\create_sample_project.cmake"
