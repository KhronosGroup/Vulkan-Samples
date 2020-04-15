/* Copyright (c) 2020, Broadcom Inc. and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 the "License";
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "stats_provider.h"

namespace vkb
{
// Default graphing values for stats. May be overridden by individual providers.
std::map<StatIndex, StatGraphData> StatsProvider::def_graph_map{
    // clang-format off
    // StatIndex                        Name shown in graph           Format          Scale           fixed_max  max_value
    {StatIndex::frame_times,           {"Frame Times",               "{:3.1f} ms",    1000.0f}},
    {StatIndex::cpu_cycles,            {"CPU Cycles",                "{:4.1f} M/s",   float(1e-6)}},
    {StatIndex::cpu_instructions,      {"CPU Instructions",          "{:4.1f} M/s",   float(1e-6)}},
    {StatIndex::cpu_cache_miss_ratio,  {"Cache Miss Ratio",          "{:3.1f}%",      100.0f,         true,      100.0f}},
    {StatIndex::cpu_branch_miss_ratio, {"Branch Miss Ratio",         "{:3.1f}%",      100.0f,         true,      100.0f}},
    {StatIndex::gpu_cycles,            {"GPU Cycles",                "{:4.1f} M/s",   float(1e-6)}},
    {StatIndex::gpu_vertex_cycles,     {"Vertex Cycles",             "{:4.1f} M/s",   float(1e-6)}},
    {StatIndex::gpu_tiles,             {"Tiles",                     "{:4.1f} k/s",   float(1e-3)}},
    {StatIndex::gpu_killed_tiles,      {"Tiles killed by CRC match", "{:4.1f} k/s",   float(1e-3)}},
    {StatIndex::gpu_fragment_jobs,     {"Fragment Jobs",             "{:4.0f}/s"}},
    {StatIndex::gpu_fragment_cycles,   {"Fragment Cycles",           "{:4.1f} M/s",   float(1e-6)}},
    {StatIndex::gpu_tex_cycles,        {"Shader Texture Cycles",     "{:4.0f} k/s",   float(1e-3)}},
    {StatIndex::gpu_ext_reads,         {"External Reads",            "{:4.1f} M/s",   float(1e-6)}},
    {StatIndex::gpu_ext_writes,        {"External Writes",           "{:4.1f} M/s",   float(1e-6)}},
    {StatIndex::gpu_ext_read_stalls,   {"External Read Stalls",      "{:4.1f} M/s",   float(1e-6)}},
    {StatIndex::gpu_ext_write_stalls,  {"External Write Stalls",     "{:4.1f} M/s",   float(1e-6)}},
    {StatIndex::gpu_ext_read_bytes,    {"External Read Bytes",       "{:4.1f} MiB/s", 1.0f / (1024.0f * 1024.0f)}},
    {StatIndex::gpu_ext_write_bytes,   {"External Write Bytes",      "{:4.1f} MiB/s", 1.0f / (1024.0f * 1024.0f)}},
    // clang-format on
};

// Static
const StatGraphData &StatsProvider::default_graph_data(StatIndex index)
{
	return def_graph_map.at(index);
}

}        // namespace vkb
