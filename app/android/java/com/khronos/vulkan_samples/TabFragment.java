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

import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ListView;

import java.util.List;

public class TabFragment extends Fragment {

    private String category;

    private List<Sample> sampleList;

    private AdapterView.OnItemClickListener clickListener;

    public static Fragment getInstance(String category) {
        Bundle bundle = new Bundle();
        bundle.putString("category", category);
        TabFragment tabFragment = new TabFragment();
        tabFragment.setArguments(bundle);
        return tabFragment;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        assert getArguments() != null;
        category = getArguments().getString("category");
        setRetainInstance(true);
    }

    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_tab, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        ListView listView = view.findViewById(R.id.sample_list);
        SampleArrayAdapter sampleArrayAdapter = new SampleArrayAdapter(view.getContext(),
                sampleList);
        listView.setAdapter(sampleArrayAdapter);
        listView.setOnItemClickListener(this.clickListener);
    }

    public void prepare(List<Sample> sampleList, AdapterView.OnItemClickListener clickListener) {
        this.sampleList = sampleList;
        this.clickListener = clickListener;
    }

    public String getCategory() {
        return category;
    }
}
