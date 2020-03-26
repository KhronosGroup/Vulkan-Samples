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

import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

public class Utils {
    /**
     * A helper to capitalize a string
     *
     * @param str The string to be capitalized
     * @return A capitalized string
     */
    public static String capitalize(String str) {
        return str.substring(0, 1).toUpperCase() + str.substring(1);
    }

    /**
     * Convert List of Boolean to primitive boolean array
     * @param list The list to be converted
     * @return A primitive boolean array containing the lists values
     */
    public static boolean[] toBoolArray(List<Boolean> list) {
        boolean[] booleans = new boolean[list.size()];
        for (int i = 0; i < list.size(); i++) {
            booleans[i] = list.get(i);
        }
        return booleans;
    }

    /**
     * Convert List of String to String array
     * @param list The list to be converted
     * @return A String array containing the lists values
     */
    public static String[] toStringArray(List<String> list) {
        String[] strings = new String[list.size()];
        for (int i = 0; i < list.size(); i++) {
            strings[i] = list.get(i);
        }
        return strings;
    }

    /**
     * Comparator orders with a predefined list
     */
    public static class PredefinedOrderComparator implements Comparator<String> {
        private List<String> defined_order;

        public PredefinedOrderComparator(String... predefined_order) {
            defined_order = Collections.unmodifiableList(Arrays.asList(predefined_order));
        }

        @Override
        public int compare(String o1, String o2) {
            // Remove duplicates
            if (o1.equals(o2)) {
                return 0;
            }

            int ret;
            int o1_order = defined_order.indexOf(o1);
            int o2_order = defined_order.indexOf(o2);

            //if neither are found in the order list, then do natural sort - string comparison
            if (o1_order < 0 && o2_order < 0) ret = o1.compareTo(o2);

                //if only one is found in the order list, float it above the other
            else if (o1_order < 0) ret = 1;
            else if (o2_order < 0) ret = -1;

                //if both are found in the order list, then do the index comparision
            else ret = o1_order - o2_order;

            return ret;
        }
    }
}
