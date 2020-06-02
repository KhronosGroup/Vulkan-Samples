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

package com.khronos.vulkan_samples.views;

import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import com.khronos.vulkan_samples.R;
import com.khronos.vulkan_samples.SampleLauncherActivity;

/**
 * A container for all elements related to the permission view
 */
public class PermissionView {
    private Button buttonPermissions;
    private TextView textPermissions;

    public PermissionView(SampleLauncherActivity activity) {
        buttonPermissions = activity.findViewById(R.id.button_permissions);
        buttonPermissions.setOnClickListener(new CheckPermissionClickListener(activity));
        textPermissions = activity.findViewById(R.id.text_permissions);
    }

    /**
     * Show the permission view
     */
    public void show() {
        buttonPermissions.setVisibility(View.VISIBLE);
        textPermissions.setVisibility(View.VISIBLE);
    }

    /**
     * Hide the permission view
     */
    public void hide() {
        buttonPermissions.setVisibility(View.INVISIBLE);
        textPermissions.setVisibility(View.INVISIBLE);
    }
}

/**
 * Click listener for the Permission View button
 */
class CheckPermissionClickListener implements View.OnClickListener {
    private SampleLauncherActivity activity;

    CheckPermissionClickListener(SampleLauncherActivity activity) {
        this.activity = activity;
    }

    public void onClick(View v) {
        activity.requestNextPermission();
    }
}