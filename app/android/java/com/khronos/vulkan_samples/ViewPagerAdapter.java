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

package com.khronos.vulkan_samples;

import android.support.annotation.NonNull;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentStatePagerAdapter;
import android.view.ViewGroup;
import android.widget.AdapterView;

import com.khronos.vulkan_samples.model.Sample;
import com.khronos.vulkan_samples.model.SampleStore;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

public class ViewPagerAdapter extends FragmentStatePagerAdapter {
    private TabFragment currentFragment;

    private List<Sample> viewableSamples = new ArrayList<>();

    private SampleStore samples;

    private AdapterView.OnItemClickListener clickListener;

    public ViewPagerAdapter(FragmentManager manager, SampleStore samples, AdapterView.OnItemClickListener clickListener) {
        super(manager);
        this.samples = samples;
        this.clickListener = clickListener;
        applyFilter(samples.getTags());
    }

    public void setDialog(FilterDialog dialog) {
        dialog.setup(this, samples.getTags());
    }

    /**
     * Modifies the filter so that the viewableSamples updates
     */
    public void applyFilter(List<String> filterTags) {
        viewableSamples = new ArrayList<>(samples.getByTags(filterTags));
    }

    @Override
    public Fragment getItem(int position) {
        List<Sample> fragmentSamples = new ArrayList<>();

        String category = samples.getCategories().get(position);
        assert category != null;

        for (Sample sample : Objects.requireNonNull(samples.getByCategory(category))) {
            for (Sample viewableSample : this.viewableSamples) {
                if (sample.equals(viewableSample)) {
                    fragmentSamples.add(sample);
                }
            }
        }

        return TabFragment.getInstance(category, fragmentSamples, clickListener);
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
        return samples.getCategoryIndex().size();
    }

    @Override
    public CharSequence getPageTitle(int position) {
        return samples.getCategories().get(position);
    }

    TabFragment getCurrentFragment() {
        return currentFragment;
    }
}