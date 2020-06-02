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

package com.khronos.vulkan_samples.model;

import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;

public class Permission {
    private String name;
    private int code;
    private boolean requested = false;

    public Permission(String name, int code) {
        this.name = name;
        this.code = code;
    }

    /**
     * Set the requested state of the permission
     *
     * @param requested The desired requested state
     */
    public void setRequested(boolean requested) {
        this.requested = requested;
    }

    /**
     * Query if the permission has already been requested
     *
     * @return True if previously requested; false otherwise
     */
    public boolean isRequested() {
        return requested;
    }

    /**
     * Get the name of the permission
     *
     * @return The permissions name
     */
    public String getName() {
        return name;
    }

    /**
     * Get the code of the permission
     *
     * @return The permissions code
     */
    public int getCode() {
        return code;
    }

    /**
     * Query whether the permission has been granted for the App
     *
     * @param ctx The activity context that needs to be queried for the permission
     * @return True if permission granted; false otherwise
     */
    public boolean granted(Context ctx) {
        return ContextCompat.checkSelfPermission(ctx, name) == PackageManager.PERMISSION_GRANTED;
    }

    /**
     * Request the permission for a given Activity
     *
     * @param activity Activity to request the permission for
     */
    public void request(Activity activity) {
        ActivityCompat.requestPermissions(activity, new String[]{name}, code);
        requested = true;
    }
}
