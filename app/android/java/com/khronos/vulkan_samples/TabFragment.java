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

import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ListView;

import com.khronos.vulkan_samples.model.Sample;

import java.util.ArrayList;
import java.util.List;

public class TabFragment extends Fragment {

    private String category;

    private List<Sample> sampleList = new ArrayList<>();

    private AdapterView.OnItemClickListener clickListener;

    /**
     * Create a new Tab Fragment Instance
     *
     * @param category      The Title of the TabFragment
     * @param sampleList    The Samples that this tab will render
     * @param clickListener The listener used when a item is clicked
     * @return A new Tab Fragment
     */
    public static Fragment getInstance(String category, List<Sample> sampleList, AdapterView.OnItemClickListener clickListener) {
        Bundle bundle = new Bundle();
        bundle.putString("category", category);
        TabFragment tabFragment = new TabFragment();
        tabFragment.setArguments(bundle);
        tabFragment.setSampleList(sampleList);
        tabFragment.setClickListener(clickListener);
        return tabFragment;
    }

    /**
     * Set the sample list that the fragment should render
     *
     * @param list A list of samples
     */
    public void setSampleList(List<Sample> list) {
        this.sampleList = list;
    }

    /**
     * Set the action to perform when a sample list item is clicked
     *
     * @param clickListener A OnItemClickListener
     */
    public void setClickListener(AdapterView.OnItemClickListener clickListener) {
        this.clickListener = clickListener;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        assert getArguments() != null;
        category = getArguments().getString("category");
        setRetainInstance(true);
    }

    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_tab, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        ListView listView = view.findViewById(R.id.sample_list);
        SampleArrayAdapter sampleArrayAdapter = new SampleArrayAdapter(view.getContext(), sampleList);
        listView.setAdapter(sampleArrayAdapter);
        listView.setOnItemClickListener(this.clickListener);
    }

    public String getCategory() {
        return category;
    }
}
