/* Copyright (c) 2020, Arm Limited and Contributors
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

import com.khronos.vulkan_samples.common.Utils;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeSet;

/**
 * A one stop store of samples that are pre indexed for use in the App
 */
public class SampleStore {
    private final Set<Sample> samples = new TreeSet<>();

    private final Set<String> categories = new TreeSet<>(new Utils.PredefinedOrderComparator("api", "performance", "extensions"));
    private final Map<String, List<Sample>> categoryIndex = new HashMap<>();

    private final Set<String> tags = new TreeSet<>(new Utils.PredefinedOrderComparator("any"));
    private final Map<String, List<Sample>> tagIndex = new HashMap<>();

    public SampleStore(List<Sample> samples) {
        if (samples == null) {
            samples = new ArrayList<>();
        }

        // Index all samples by tag and by category
        for (Sample sample : samples) {
            if (sample == null) {
                continue;
            }

            this.samples.add(sample);

            if (!categoryIndex.containsKey(sample.getCategory())) {
                categoryIndex.put(sample.getCategory(), new ArrayList<>());
            }

            List<Sample> category = categoryIndex.get(sample.getCategory());
            if (category != null) {
                category.add(sample);
            }

            for (String tag : sample.getTags()) {
                if (!tagIndex.containsKey(tag)) {
                    tagIndex.put(tag, new ArrayList<>());
                }

                List<Sample> index = tagIndex.get(tag);
                if (index != null) {
                    index.add(sample);
                }

                this.tags.add(tag);
            }

            this.categories.add(sample.getCategory());
        }
    }

    /**
     * Find a sample by its id
     *
     * @param id of a given sample
     * @return A sample if it is found
     */
    public Sample findByID(String id) {
        for (Sample sample : samples) {
            if (sample.getId().equals(id)) {
                return sample;
            }
        }
        return null;
    }

    /**
     * Get the category index
     *
     * @return A map of category to samples
     */
    public Map<String, List<Sample>> getCategoryIndex() {
        return this.categoryIndex;
    }

    /**
     * Get all category names
     *
     * @return A list of category names
     */
    public List<String> getCategories() {
        return new ArrayList<>(this.categories);
    }

    /**
     * Get all tag names
     *
     * @return A list of tag names
     */
    public List<String> getTags() {
        return new ArrayList<>(this.tags);
    }

    /**
     * Get a list of sample by tag name
     *
     * @return A list of samples
     */
    public List<Sample> getByTag(String tag) {
        return this.tagIndex.get(tag);
    }

    /**
     * Get a list of sample by multiple tag names
     *
     * @return A list of samples
     */
    public List<Sample> getByTags(List<String> tags) {
        Set<Sample> samples = new HashSet<>();

        for (String tag : tags) {
            Collection<Sample> samplesByTag = getByTag(tag);
            if (samplesByTag != null) {
                samples.addAll(samplesByTag);
            }
        }

        return new ArrayList<>(samples);
    }

    /**
     * Get a list of sample by category name
     *
     * @return A list of samples
     */
    public List<Sample> getByCategory(String category) {
        return this.categoryIndex.get(category);
    }
}
