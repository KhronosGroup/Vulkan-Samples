/* Copyright (c) 2020-2021, Arm Limited and Contributors
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

package com.khronos.vulkan_samples.views;

import android.content.Intent;
import android.support.design.widget.TabLayout;
import android.support.v4.view.ViewPager;
import android.view.View;
import android.widget.AdapterView;

import com.khronos.vulkan_samples.FilterDialog;
import com.khronos.vulkan_samples.NativeSampleActivity;
import com.khronos.vulkan_samples.R;
import com.khronos.vulkan_samples.SampleLauncherActivity;
import com.khronos.vulkan_samples.ViewPagerAdapter;
import com.khronos.vulkan_samples.model.Sample;

/**
 * A container for all elements related to the sample view
 */
public class SampleListView {
    private TabLayout tabLayout;
    public ViewPager viewPager;
    public FilterDialog dialog;

    public SampleListView(SampleLauncherActivity activity) {
        viewPager = activity.findViewById(R.id.viewpager);
        ViewPagerAdapter adapter = new ViewPagerAdapter(activity.getSupportFragmentManager(), activity.samples, new SampleItemClickListener(activity));
        viewPager.setAdapter(adapter);

        dialog = new FilterDialog();
        adapter.setDialog(dialog);

        tabLayout = activity.findViewById(R.id.tabs);
        tabLayout.setupWithViewPager(viewPager);
    }

    /**
     * Show the sample view
     */
    public void show() {
        tabLayout.setVisibility(View.VISIBLE);
        viewPager.setVisibility(View.VISIBLE);
    }

    /**
     * Hide the sample view
     */
    public void hide() {
        tabLayout.setVisibility(View.INVISIBLE);
        viewPager.setVisibility(View.INVISIBLE);
    }
}

/**
 * Click listener for a Sample List Item
 * Start the Native Activity for the clicked Sample
 */
class SampleItemClickListener implements AdapterView.OnItemClickListener {
    private SampleLauncherActivity activity;

    SampleItemClickListener(SampleLauncherActivity activity) {
        this.activity = activity;
    }

    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        String sampleID = ((Sample) parent.getItemAtPosition(position)).getId();
        activity.launchSample(sampleID);
    }
}