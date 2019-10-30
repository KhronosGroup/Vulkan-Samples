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

import android.app.NativeActivity;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.support.v4.app.NotificationCompat;
import android.support.v4.app.NotificationManagerCompat;
import android.support.v4.content.FileProvider;
import android.util.Log;
import android.view.View;
import android.webkit.MimeTypeMap;
import android.widget.Toast;

import java.io.File;
import java.lang.Runnable;
import java.util.concurrent.atomic.AtomicInteger;

public class NativeSampleActivity extends NativeActivity {

    private Context context;

    private static final String NOTIFICATION_PREFS = "NOTIFICATION_PREFS";

    private static final String NOTIFICATION_KEY = "NOTIFICATION_KEY";

    private static final String CHANNEL_ID = "vkb";

    private static int initialId = 0;

    @Override
    protected void onCreate(Bundle instance) {
        super.onCreate(instance);


         // Create Notification Channel API 26+
         //
         // API 26+ implements notifications using a channel where notifications are submitted to channel
         // The following initialises the channel used by vkb to push notifications to.
         // IMPORTANCE_HIGH = push notificaitons
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationChannel channel = new NotificationChannel(CHANNEL_ID, getString(R.string.channel_name), NotificationManager.IMPORTANCE_HIGH);
            channel.setDescription(getString(R.string.channel_desc));
            NotificationManager nm = getSystemService(NotificationManager.class);
            nm.createNotificationChannel(channel);
        }

        context = this;
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) {
            View decorView = getWindow().getDecorView();
            decorView.setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY |
                View.SYSTEM_UI_FLAG_LAYOUT_STABLE |
                View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION |
                View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN |
                View.SYSTEM_UI_FLAG_HIDE_NAVIGATION |
                View.SYSTEM_UI_FLAG_FULLSCREEN);
        }
    }

    private int getId() {
        SharedPreferences prefs = this.getSharedPreferences(NOTIFICATION_PREFS, context.MODE_PRIVATE);
        int id = initialId;
        if(prefs.contains(NOTIFICATION_KEY)) {
            id = prefs.getInt(NOTIFICATION_KEY, initialId);
        }

        SharedPreferences.Editor editor = prefs.edit();
        editor.putInt(NOTIFICATION_KEY, id + 1);
        editor.apply();

        return id;
    }

    public void fatalError(final String log_file) {
        File file = new File(log_file);

        Uri path = FileProvider.getUriForFile(context, context.getApplicationContext().getPackageName() + ".provider", file);
        
        Intent intent = new Intent(Intent.ACTION_VIEW);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

        Intent applicationChooser = Intent.createChooser(intent, context.getResources().getString(R.string.open_file_with));
        intent.setDataAndType(path, context.getContentResolver().getType(path));
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);

        PendingIntent pi = PendingIntent.getActivity(context, 0, intent, PendingIntent.FLAG_CANCEL_CURRENT);

        NotificationCompat.Builder builder = new NotificationCompat.Builder(this, CHANNEL_ID)
                .setSmallIcon(R.drawable.icon)
                .setContentTitle("Vulkan Samples Error")
                .setContentText("Fatal Error: click to view")
                .setStyle(new NotificationCompat.BigTextStyle().bigText("Log: " + log_file))
                .setAutoCancel(true)
                .setContentIntent(pi)
                .setPriority(NotificationCompat.PRIORITY_HIGH);

        NotificationManagerCompat nm = NotificationManagerCompat.from(context);
        nm.notify(getId(), builder.build());
    }

    /**
     * @brief Create a push notification from JNI with a message
     *
     * @param message A string of a message to be shown
     */
    private void notification(final String message) {
        NotificationCompat.Builder builder = new NotificationCompat.Builder(this, CHANNEL_ID)
                .setSmallIcon(R.drawable.icon)
                .setContentTitle("Vulkan Samples Error")
                .setContentText(message)
                .setStyle(new NotificationCompat.BigTextStyle()
                .bigText(message))
                .setPriority(NotificationCompat.PRIORITY_HIGH);

        NotificationManagerCompat nm = NotificationManagerCompat.from(context);
        nm.notify(getId(), builder.build());
    }

    /**
     * @brief Create a toast from JNI on the android main UI thread with a message
     *
     * @param message A string of a message to be shown
     */
    private void toast(final String message) {
        new Handler(Looper.getMainLooper()).post(new Runnable() {
            @Override
            public void run() {
                Toast.makeText(context, message, Toast.LENGTH_LONG).show();
            }
        });
    }
}
