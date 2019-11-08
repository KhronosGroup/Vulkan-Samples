/* Copyright (c) 2019, Arm Limited and Contributors
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

package com.khronos.vulkan_samples;

import java.util.Arrays;
import java.util.LinkedList;
import java.util.List;

public class Sample {
    private String id;
    private String category;
    private String author;
    private String name;
    private String description;
    private List<String> tags;

    public Sample(String id, String category, String author, String name, String description, String[] tags) {
        this.id = id;
        this.category = category;
        this.author = author;
        this.name = name;
        this.description = description;
        this.tags = new LinkedList<>(Arrays.asList(tags));
    }

    public String getId() {
        return id;
    }

    public String getCategory() {
        return category;
    }

    public String getAuthor() { return author; }

    public String getName() {
        return name;
    }

    public String getDescription() {
        return description;
    }

    public List<String> getTags() { return tags; }
}
