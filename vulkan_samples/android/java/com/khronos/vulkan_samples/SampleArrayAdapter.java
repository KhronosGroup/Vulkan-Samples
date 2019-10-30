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

import android.annotation.SuppressLint;
import android.content.Context;
import android.support.annotation.NonNull;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.List;

public class SampleArrayAdapter extends ArrayAdapter<Sample> {

    SampleArrayAdapter(Context context, List<Sample> sample_list) {
        super(context, R.layout.listview_item, sample_list);
    }

    @NonNull
    @SuppressLint("SetTextI18n")
    @Override
    public View getView(int position, View view, @NonNull ViewGroup parent) {
        if(view == null) {
            view = LayoutInflater.from(getContext()).inflate(R.layout.listview_item, parent,false);
        }

        Sample sample = getItem(position);
        assert sample != null;

        TextView title = view.findViewById(R.id.title_text);
        title.setText(sample.getName());

        TextView description = view.findViewById(R.id.description_text);
        description.setText(sample.getDescription());

        StringBuilder vendorTagText = new StringBuilder();

        List<String> tags = new ArrayList<>(sample.getTags());
        if(tags.size() > 1) {
            tags.remove("any");
            for (int i = 0; i < tags.size(); ++i) {
                String tag = tags.get(i);
                String formattedTag = tag.substring(0, 1).toUpperCase() + tag.substring(1);
                vendorTagText.append(formattedTag);
                if (i < tags.size() - 1) {
                    vendorTagText.append(", ");
                }
            }
        }
        else {
           vendorTagText.append("Any");
        }

        TextView vendorTag = view.findViewById(R.id.vendor_text);
        vendorTag.setText(vendorTagText.toString());

        return view;
    }
}
