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

import android.annotation.SuppressLint;
import android.content.Context;
import android.support.annotation.NonNull;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.TextView;

import com.khronos.vulkan_samples.model.Sample;

import java.util.List;

public class SampleArrayAdapter extends ArrayAdapter<Sample> {

    SampleArrayAdapter(Context context, List<Sample> sample_list) {
        super(context, R.layout.listview_item, sample_list);
    }

    /**
     * Get a view for a position in the sample list
     *
     * @param position The position in the list
     * @param view     The view object that was previously at this position
     * @param parent   The parent view group
     * @return A view object representing a given sample at a list position
     */
    @NonNull
    @SuppressLint("SetTextI18n")
    @Override
    public View getView(int position, View view, @NonNull ViewGroup parent) {
        // Recycle view objects
        if (view == null) {
            view = LayoutInflater.from(getContext()).inflate(R.layout.listview_item, parent, false);
        }

        Sample sample = getItem(position);
        assert sample != null;

        TextView title = view.findViewById(R.id.title_text);
        title.setText(sample.getName());

        TextView description = view.findViewById(R.id.description_text);
        description.setText(sample.getDescription());

        TextView vendorTag = view.findViewById(R.id.vendor_text);
        vendorTag.setText(sample.getTagText());

        return view;
    }
}
