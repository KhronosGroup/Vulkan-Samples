/* Copyright (c) 2019-2021, Arm Limited and Contributors
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
import androidx.annotation.NonNull;
import androidx.fragment.app.DialogFragment;

import com.khronos.vulkan_samples.common.Utils;

import java.util.ArrayList;
import java.util.List;

public class FilterDialog extends DialogFragment {
    // The adapter that is notified by a filter changing
    private ViewPagerAdapter adapter;

    // Saved raw filters that are used to update the adapter
    private List<String> filters;

    // Capitalised raw filters
    private List<String> labels;

    // Save state of the current applied filter
    // This is used to initialise the filter every time it is opened
    protected List<Boolean> values;

    // Mutable copy of the saved state
    // This is used when a dialog is open to track the state of the labels
    // If a filter is applied values is replaced with this list; otherwise This is discarded
    protected List<Boolean> alteredValues;

    public void setup(ViewPagerAdapter adapter, List<String> filters) {
        this.adapter = adapter;
        this.filters = new ArrayList<>(filters);
        this.labels = new ArrayList<>(filters.size());
        this.values = new ArrayList<>(filters.size());

        // Initialise labels only once
        // Default set all filters to checked
        for (int i = 0; i < filters.size(); ++i) {
            this.labels.add(i, Utils.capitalize(filters.get(i)));
            this.values.add(i, true);
        }
    }

    @Override
    public void onAttach(@NonNull Context context) {
        super.onAttach(context);
    }

    @NonNull
    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        // Initial labels and values for the list of filters
        final CharSequence[] labels = this.labels.toArray(new CharSequence[0]);
        boolean[] values = Utils.toBoolArray(this.values);

        // Initiate the mutable filter list
        this.alteredValues = new ArrayList<>(this.values);

        AlertDialog.Builder builder = new AlertDialog.Builder(getContext())
                .setTitle("Filter Samples by Tag")
                .setMultiChoiceItems(labels, values, new FilterMultiChoiceClickListener(this))
                .setPositiveButton("Apply", new FilterClickListener(this));

        return builder.create();
    }

    /**
     * Notify the adapter that the current applied filter has changed
     */
    protected void notifyAdapter() {
        adapter.applyFilter(getFilter());
        adapter.notifyDataSetChanged();
    }

    /**
     * Get all raw filters that have been applied
     *
     * @return List of applied filters
     */
    public List<String> getFilter() {
        List<String> filters = new ArrayList<>();

        for (int i = 0; i < this.filters.size(); i++) {
            if (this.values.get(i)) {
                filters.add(this.filters.get(i));
            }
        }

        return filters;
    }
}

/**
 * Changes to a filter item's state must not persist if the dialog is closed without applying a filter.
 * Update alteredValues, changes are discarded if the filter is not applied
 */
class FilterMultiChoiceClickListener implements DialogInterface.OnMultiChoiceClickListener {
    private final FilterDialog dialog;

    FilterMultiChoiceClickListener(FilterDialog dialog) {
        this.dialog = dialog;
    }

    @Override
    public void onClick(DialogInterface dialog, int which, boolean isChecked) {
        if (which > this.dialog.alteredValues.size() || which < 0) {
            // Which is not in the label array - should never happen
            return;
        }

        this.dialog.alteredValues.set(which, isChecked);
    }
}

/**
 * When applying a filter override the current values and then notify the adapter that the current filter has been changed
 */
class FilterClickListener implements DialogInterface.OnClickListener {
    private final FilterDialog dialog;

    FilterClickListener(FilterDialog dialog) {
        this.dialog = dialog;
    }

    @Override
    public void onClick(DialogInterface dialog, int id) {
        this.dialog.values = new ArrayList<>(this.dialog.alteredValues);
        this.dialog.notifyAdapter();
    }
}