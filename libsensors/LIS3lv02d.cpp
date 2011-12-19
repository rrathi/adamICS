/*
 * Copyright (C) 2011 The Android Open-Source Project
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

#include "LIS3lv02d.h"

LIS3lv02d::LIS3lv02d()
{
  // Open device etc
};

LIS3lv02d::~LIS3lv02d()
{

};


int LIS3lv02d::activate(int enabled)
{
return 0;
};

int LIS3lv02d::setDelay(int64_t ns)
{
return 0;
};

int LIS3lv02d::poll(sensors_event_t* data, int count)
{
return 0;
}
