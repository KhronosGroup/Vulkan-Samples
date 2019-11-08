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

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v4.app.DialogFragment;

import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;

public class FilterDialog extends DialogFragment {

    private ViewPagerAdapter adapter;

    private List<String> filterList;

    private List<Integer> selectedItems = new LinkedList<>();

    public void setup(ViewPagerAdapter adapter, List<String> filterList) {
        this.adapter = adapter;
        this.filterList = filterList;
        for(int i = 0; i < this.filterList.size(); ++i) {
            this.selectedItems.add(i);
        }
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
    }

    @NonNull
    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        List<String> formattedFilterList = new ArrayList<>(filterList);

        formattedFilterList.remove("any");
        formattedFilterList.add(0, "any");

        for(int i = 0; i < formattedFilterList.size(); ++i) {
            String tag = formattedFilterList.get(i);
            String formattedTag = tag.substring(0, 1).toUpperCase() + tag.substring(1);
            formattedFilterList.set(i, formattedTag);
        }

        final CharSequence[] charSequenceItems = formattedFilterList.toArray(new CharSequence[0]);

        boolean[] checkedItems = new boolean[formattedFilterList.size()];

        for(int i = 0; i < formattedFilterList.size(); ++i) {
            checkedItems[i] = selectedItems.contains(i);
        }

        AlertDialog.Builder builder = new AlertDialog.Builder(getContext())
            .setTitle("Filter Samples by Tag")
            .setMultiChoiceItems(charSequenceItems, checkedItems,
                new DialogInterface.OnMultiChoiceClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which, boolean isChecked) {
                        if (isChecked) {
                            selectedItems.add(which);
                        } else if (selectedItems.contains(which)) {
                            selectedItems.remove(Integer.valueOf(which));
                        }
                    }
                }
            )
            .setPositiveButton("Apply", new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int id) {
                    List<String> filterTags = new ArrayList<>();
                    for(int i = 0; i < filterList.size(); ++i) {
                        if(selectedItems.contains(Integer.valueOf(i))){
                            filterTags.add(filterList.get(i));
                        }
                    }

                    adapter.applyFilter(filterTags);
                    adapter.notifyDataSetChanged();
                }
            });

        return builder.create();
    }
}
