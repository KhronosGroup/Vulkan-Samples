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

import android.Manifest;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.design.widget.TabLayout;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v4.view.ViewPager;
import android.support.v7.app.AppCompatActivity;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;

import android.support.v7.widget.Toolbar;

public class SampleLauncherActivity extends AppCompatActivity {

    private List<String> args = new ArrayList<>();

    private static final int RC_READ_EXTERNAL_STORAGE = 1;
    private static final int RC_WRITE_EXTERNAL_STORAGE = 2;

    private TabLayout tabLayout;
    private ViewPager viewPager;

    private FilterDialog dialog;

    private Button buttonPermissions;
    private TextView textPermissions;

    private boolean isBenchmarkMode = false;
    private boolean isHeadless = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        List<Sample> sampleList = new ArrayList<>();

        if (loadNativeLibrary(getResources().getString(R.string.native_lib_name))) {
            //Initialize cpp android platform
            File external_files_dir = getExternalFilesDir("");
            File temp_files_dir = getCacheDir();
            if (external_files_dir != null && temp_files_dir != null){
                initFilePath(external_files_dir.toString(), temp_files_dir.toString());
            }

            //Get sample info from cpp cmake generated file
            sampleList = Arrays.asList(getSamples());
        }

        AdapterView.OnItemClickListener clickListener = new AdapterView.OnItemClickListener() {
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                String sampleId = ((Sample)parent.getItemAtPosition(position)).getId();
                args.add("--sample");
                args.add(sampleId);
                setArguments(args);
                Intent intent = new Intent(SampleLauncherActivity.this,
                        NativeSampleActivity.class);
                startActivity(intent);
            }
        };

        HashMap<String, List<Sample>> categorizedSampleMap = categorize(sampleList);

        viewPager = findViewById(R.id.viewpager);
        ViewPagerAdapter adapter = new ViewPagerAdapter(getSupportFragmentManager(),
                categorizedSampleMap, clickListener);

        dialog = new FilterDialog();
        adapter.setDialog(dialog);

        viewPager.setAdapter(adapter);

        tabLayout = findViewById(R.id.tabs);
        tabLayout.setupWithViewPager(viewPager);

        buttonPermissions = findViewById(R.id.button_permissions);
        buttonPermissions.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                checkPermissions();
            }
        });
        textPermissions = findViewById(R.id.text_permissions);
        checkPermissions();


        // Handle argument passing
        Bundle extras = this.getIntent().getExtras();
        if (extras != null) {
            if (extras.containsKey("sample")) {
                args.add("--sample");
                args.add(extras.getString("sample"));
                setArguments(args);
                Intent intent = new Intent(SampleLauncherActivity.this,
                        NativeSampleActivity.class);
                startActivity(intent);
            }

            else if (extras.containsKey("test")) {
                setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
                args.add("--test");
                args.add(extras.getString("test"));
                setArguments(args);
                Intent intent = new Intent(SampleLauncherActivity.this,
                        NativeSampleActivity.class);
                startActivity(intent);
            }
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        super.onCreateOptionsMenu(menu);
        getMenuInflater().inflate(R.menu.main_menu, menu);
        return true;
    }

    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        super.onPrepareOptionsMenu(menu);
        MenuItem checkable = menu.findItem(R.id.menu_benchmark_mode);
        checkable.setChecked(isBenchmarkMode);

        checkable = menu.findItem(R.id.menu_headless);
        checkable.setChecked(isHeadless);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch(item.getItemId()) {
            case R.id.filter_button:
                dialog.show(getSupportFragmentManager(), "filter");
                return true;
            case R.id.menu_run_samples:
                String category = "";
                List<String> filterTags = new ArrayList<>();
                ViewPagerAdapter adapter = ((ViewPagerAdapter)viewPager.getAdapter());
                if(adapter != null) {
                    category = adapter.getCurrentFragment().getCategory();
                    filterTags = adapter.getFilter();
                }
                args.add("--batch");
                args.add(category);
                args.addAll(filterTags);
                setArguments(args);
                Intent intent = new Intent(SampleLauncherActivity.this,
                        NativeSampleActivity.class);
                startActivity(intent);
                return true;
            case R.id.menu_benchmark_mode:
                isBenchmarkMode = !item.isChecked();
                item.setChecked(isBenchmarkMode);
                return true;
            case R.id.menu_headless:
                isHeadless = !item.isChecked();
                item.setChecked(isHeadless);
                return true;
            default:
                return super.onOptionsItemSelected(item);
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions,
                                           @NonNull int[] grantResults) {
        switch (requestCode) {
            case RC_WRITE_EXTERNAL_STORAGE: {
                // If request is cancelled, the result arrays are empty.
                if (grantResults.length > 0
                        && grantResults[0] != PackageManager.PERMISSION_GRANTED) {
                    showPermissionsMessage();
                    break;
                }
            }
            case RC_READ_EXTERNAL_STORAGE: {
                if (grantResults.length > 0
                        && grantResults[0] != PackageManager.PERMISSION_GRANTED) {
                    showPermissionsMessage();
                    break;
                }
            }
            default:
                showSamples();
        }
    }

    private void setArguments(List<String> args) {
        if (isBenchmarkMode) {
            args.add("--benchmark");
            args.add("2000");
        }
        if (isHeadless) {
            args.add("--headless");
        }
        String[] argArray = new String[args.size()];
        sendArgumentsToPlatform(args.toArray(argArray));
        args.clear();
    }

    private HashMap<String, List<Sample>> categorize(@NonNull List<Sample> sampleList) {
        HashMap<String, List<Sample>> sampleMap = new HashMap<>();
        for(Sample sample : sampleList) {
            List<Sample> sampleListCategory = sampleMap.get(sample.getCategory());
            if(sampleListCategory == null) {
                sampleListCategory = new ArrayList<>();
            }
            sampleListCategory.add(sample);
            sampleMap.put(sample.getCategory(), sampleListCategory);
        }
        return sampleMap;
    }

    private boolean loadNativeLibrary(String native_lib_name) {
        boolean status = true;

        try {
            System.loadLibrary(native_lib_name);
        } catch (UnsatisfiedLinkError e) {
            Toast
                .makeText(getApplicationContext(),
                          "Native code library failed to load.",
                          Toast.LENGTH_SHORT)
                .show();

            status = false;
        }

        return status;
    }

    private void checkPermissions() {
        if (ContextCompat.checkSelfPermission(this,
                Manifest.permission.WRITE_EXTERNAL_STORAGE)
                != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(SampleLauncherActivity.this,
                    new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE},
                    RC_WRITE_EXTERNAL_STORAGE);
        } else if (ContextCompat.checkSelfPermission(this,
                Manifest.permission.READ_EXTERNAL_STORAGE)
                != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(SampleLauncherActivity.this,
                    new String[]{Manifest.permission.READ_EXTERNAL_STORAGE},
                    RC_READ_EXTERNAL_STORAGE);
        } else {
            showSamples();
        }
    }

    public void showPermissionsMessage() {
        textPermissions.setVisibility(View.VISIBLE);
        buttonPermissions.setVisibility(View.VISIBLE);
        viewPager.setVisibility(View.INVISIBLE);
        tabLayout.setVisibility(View.INVISIBLE);
    }

    public void showSamples() {
        textPermissions.setVisibility(View.INVISIBLE);
        buttonPermissions.setVisibility(View.INVISIBLE);
        viewPager.setVisibility(View.VISIBLE);
        tabLayout.setVisibility(View.VISIBLE);
    }

    private native Sample[] getSamples();

    private native void sendArgumentsToPlatform(String[] args);

    private native void initFilePath(String external_dir, String temp_path);
}
