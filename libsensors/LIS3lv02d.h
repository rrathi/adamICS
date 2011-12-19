/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_ADAM_LIS3lv02d_H
#define ANDROID_ADAM_LIS3lv02d_H
#include <hardware/sensors.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>
#include <poll.h>
#include <pthread.h>
#include <stdlib.h>

#include <utils/Atomic.h>
#include <utils/Log.h>

#include "SensorBase.h"

class LIS3lv02d : public SensorBase
{
  public:
    LIS3lv02d();
    ~LIS3lv02d();
    int activate(int enabled);
    int setDelay(int64_t ns);
    int poll(sensors_event_t* data, int count);
};

#define LOG_TAG "Sensors"

#endif /* ANDROID_ADAM_LIS3lv02d_H */
