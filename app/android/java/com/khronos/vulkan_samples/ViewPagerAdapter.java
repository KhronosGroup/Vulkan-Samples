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

import android.support.annotation.NonNull;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentStatePagerAdapter;
import android.util.ArraySet;
import android.view.ViewGroup;
import android.widget.AdapterView;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Objects;
import java.util.Set;
import java.util.SortedSet;
import java.util.TreeSet;

public class ViewPagerAdapter extends FragmentStatePagerAdapter {

    private TabFragment currentFragment;

    private List<String> categories;

    private List<String> filterTags;

    private HashMap<String, List<Sample>> sampleMap;

    private List<Sample> viewableSamples;

    private AdapterView.OnItemClickListener clickListener;

    ViewPagerAdapter(FragmentManager manager, @NonNull HashMap<String,
            List<Sample>> categorizedSampleMap, AdapterView.OnItemClickListener clickListener) {
        super(manager);
        SortedSet<String> keys = new TreeSet<>(categorizedSampleMap.keySet());
        this.categories = new ArrayList<>();
        this.categories.addAll(keys);
        this.sampleMap = categorizedSampleMap;
        this.viewableSamples = new ArrayList<>();
        this.clickListener = clickListener;
        applyFilter(getTags());

    }

    public void setDialog(FilterDialog dialog) {
        dialog.setup(this, getTags());
    }

    /**
     * @brief Modifies the filter so that the viewableSamples updates
     */
    public void applyFilter(List<String> filterTags) {
        viewableSamples.clear();
        for(String filter : filterTags) {
            for(List<Sample> sampleList : sampleMap.values()) {
                for(Sample sample : sampleList) {
                    if(sample.getTags().contains(filter) && !viewableSamples.contains(sample)) {
                        viewableSamples.add(sample);
                    }
                }
            }
        }
        this.filterTags = filterTags;
    }

    /**
     * @brief Returns a list of unique tags pulled from CMake used to filter the view
     */
    private List<String> getTags() {
        Set<String> tagSet = new ArraySet<>();
        for(List<Sample> sampleList : sampleMap.values()) {
            for(Sample sample : sampleList) {
                tagSet.addAll(sample.getTags());
            }
        }
        return new ArrayList<>(tagSet);
    }

    public List<String> getFilter() {
        return filterTags;
    }

    @Override
    public Fragment getItem(int position) {
        TabFragment fragment = (TabFragment) TabFragment.getInstance(categories.get(position));

        List<Sample> fragmentSamples = new ArrayList<>();

        for(Sample sample : Objects.requireNonNull(sampleMap.get(categories.get(position)))) {
            for(Sample viewableSample : this.viewableSamples) {
                if(sample.getId().equals(viewableSample.getId())) {
                    fragmentSamples.add(sample);
                }
            }
        }

        fragment.prepare(fragmentSamples, clickListener);
        return fragment;
    }

    public int getItemPosition(@NonNull Object object) {
        return POSITION_NONE;
    }

    @Override
    public void setPrimaryItem(@NonNull ViewGroup container, int position, @NonNull Object object) {
        if (getCurrentFragment() != object) {
            currentFragment = (TabFragment) object;
        }
        super.setPrimaryItem(container, position, object);
    }

    @Override
    public int getCount() {
        return categories.size();
    }

    @Override
    public CharSequence getPageTitle(int position) {
        return categories.get(position);
    }

    TabFragment getCurrentFragment() {
        return currentFragment;
    }
}