/* Copyright (c) 2019-2020, Arm Limited and Contributors
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

package com.khronos.vulkan_samples.model;

import android.text.TextUtils;

import com.khronos.vulkan_samples.common.Utils;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.LinkedList;
import java.util.List;

public class Sample implements Comparable<Sample> {
    private String id;
    private String category;
    private String author;
    private String name;
    private String description;

    private String tagText;
    private List<String> tags;

    public Sample(String id, String category, String author, String name, String description, String[] tags) {
        this.id = id;
        this.category = category;
        this.author = author;
        this.name = name;
        this.description = description;

        this.tags = new LinkedList<>(Arrays.asList(tags));
    }

    /**
     * Generate the Tag Text that is shown in the sample list
     *
     * @return A concatenated string of a samples tags
     */
    private String generateTagText() {
        List<String> formatted_tags = new ArrayList<>(this.tags.size());

        for (String tag : this.tags) {
            formatted_tags.add(Utils.capitalize(tag));
        }

        // If the sample has a tag other than "Any" then "Any" can be removed
        if (formatted_tags.size() > 1) {
            formatted_tags.remove("Any");
        }

        return TextUtils.join(", ", formatted_tags);
    }

    /**
     * Get the Sample's ID
     *
     * @return A Sample ID
     */
    public String getId() {
        return id;
    }

    /**
     * Get the sample's category
     *
     * @return The sample's category
     */
    public String getCategory() {
        return category;
    }

    /**
     * Get the sample's author
     *
     * @return The sample's author
     */
    public String getAuthor() {
        return author;
    }

    /**
     * Get the sample's name
     *
     * @return The sample's name
     */
    public String getName() {
        return name;
    }

    /**
     * Get the sample's description
     *
     * @return The sample's description
     */
    public String getDescription() {
        return description;
    }

    /**
     * Get the sample's tags
     *
     * @return The sample's tags
     */
    public List<String> getTags() {
        return tags;
    }

    /**
     * Get the sample's tag text
     *
     * @return The sample's tag text
     */
    public String getTagText() {
        if (tagText == null) {
            tagText = generateTagText();
        }
        return tagText;
    }

    /**
     * Checks whether two samples are equal
     * Can be used with a HashMap
     *
     * @return True if samples are equal; false otherwise
     */
    public boolean equals(Object object) {
        if (this == object) {
            return true;
        }

        if (object instanceof Sample) {
            Sample sample = (Sample) object;
            return this.id.equals(sample.id);
        }

        return false;
    }

    /**
     * Used when comparing two samples - Alphabetical Case Insensitive
     * Sample id's are unique so this is used to compare  
     *
     * @param sample To compare too
     * @return - if less, 0 if equal + if greater than
     */
    public int compareTo(Sample sample) {
        return sample.id.compareToIgnoreCase(this.id);
    }
}
