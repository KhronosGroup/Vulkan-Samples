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

package com.khronos.vulkan_samples.common;

import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.content.SharedPreferences;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import androidx.core.app.NotificationCompat;
import androidx.core.app.NotificationManagerCompat;
import android.widget.Toast;
import android.content.Context;

import com.khronos.vulkan_samples.R;

public class Notifications {
    private static final String NOTIFICATION_PREFS = "NOTIFICATION_PREFS";
    private static final String NOTIFICATION_KEY = "NOTIFICATION_KEY";
    public static final String CHANNEL_ID = "vkb";

    private static int initialId = 0;

    public static void initNotificationChannel(Context context) {
        // Create Notification Channel API 26+
        //
        // API 26+ implements notifications using a channel where notifications are submitted to channel
        // The following initialises the channel used by vkb to push notifications to.
        // IMPORTANCE_HIGH = push notifications
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationChannel channel = new NotificationChannel(CHANNEL_ID, context.getString(R.string.channel_name), NotificationManager.IMPORTANCE_HIGH);
            channel.setDescription(context.getString(R.string.channel_desc));
            NotificationManager nm = context.getSystemService(NotificationManager.class);
            nm.createNotificationChannel(channel);
        }
    }

    public static int getNotificationId(Context context) {
        SharedPreferences prefs = context.getSharedPreferences(NOTIFICATION_PREFS, Context.MODE_PRIVATE);
        int id = initialId;
        if (prefs.contains(NOTIFICATION_KEY)) {
            id = prefs.getInt(NOTIFICATION_KEY, initialId);
        }

        SharedPreferences.Editor editor = prefs.edit();
        editor.putInt(NOTIFICATION_KEY, id + 1);
        editor.apply();

        return id;
    }

    /**
     * Create a push notification from JNI with a message
     *
     * @param message A string of a message to be shown
     */
    public static void notification(Context context, final String message) {
        NotificationCompat.Builder builder = new NotificationCompat.Builder(context, CHANNEL_ID)
                .setSmallIcon(R.drawable.icon)
                .setContentTitle("Vulkan Samples Error")
                .setContentText(message)
                .setStyle(new NotificationCompat.BigTextStyle()
                        .bigText(message))
                .setPriority(NotificationCompat.PRIORITY_HIGH);

        NotificationManagerCompat nm = NotificationManagerCompat.from(context);
        nm.notify(getNotificationId(context), builder.build());
    }

    /**
     * Create a toast from JNI on the android main UI thread with a message
     *
     * @param message A string of a message to be shown
     */
    public static void toast(final Context context, final String message) {
        new Handler(Looper.getMainLooper()).post(() -> Toast.makeText(context, message, Toast.LENGTH_LONG).show());
    }

}
