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

#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <poll.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>
#include <android/log.h>
//#include <linux/akm8975.h>

#include <cutils/log.h>

#include "MagneticSensor.h"
#define MLOGV(...) __android_log_print(ANDROID_LOG_DEBUG, "MSensorLog", __VA_ARGS__)
/*****************************************************************************/

MagneticSensor::MagneticSensor()
    : SensorBase(MAGNETIC_DEVICE_NAME, NULL),
      mEnabled(0),
      mPendingMask(0),
      mInputReader(32)
{
    memset(mPendingEvents, 0, sizeof(mPendingEvents));

    mPendingEvents[MagneticField].version = sizeof(sensors_event_t);
    mPendingEvents[MagneticField].sensor = ID_M;
    mPendingEvents[MagneticField].type = SENSOR_TYPE_MAGNETIC_FIELD;
    mPendingEvents[MagneticField].magnetic.status = SENSOR_STATUS_ACCURACY_HIGH;
    mMinDelay = 0;
    mLastUpdate = 0;
    int err = readCSVFromFile(MAGNETIC_CALIBRATION_FILE, magCalibration, 3);
    if (err <= 0) {
	// use defaults
	magCalibration[0] = magCalibration[1] = magCalibration[2] = 0;
	MLOGV("Failed to read magnetic calibration file '%s'. Using defaults.", MAGNETIC_CALIBRATION_FILE);
    }	
}

MagneticSensor::~MagneticSensor() {
}

int MagneticSensor::enable(int32_t handle, int en)
{
     int flags = en ? 1 : 0;
     mEnabled = en;
  return 0;
}

int MagneticSensor::setDelay(int32_t handle, int64_t ns)
{
    mMinDelay = ns;
    return 0;
}

int MagneticSensor::update_delay()
{
    return 0;
}

bool MagneticSensor::hasPendingEvents() const {
   bool ret = 0;
   if (mEnabled) {
      if(mLastUpdate) {
         if(getTimestamp() - (mLastUpdate + mMinDelay) > 0)
            ret = 1;
      } else {
         // First time since init
         ret = 1;
      }
   }

   return ret;
}

int MagneticSensor::readEvents(sensors_event_t* data, int count)
{
   if (count < 1)
      return -EINVAL;

   int vals[3] = {0, 0, 0};
   int ret = readCSVFromFile(MAGNETIC_DEVICE_NAME, vals, 3);
   mLastUpdate = mPendingEvents[MagneticField].timestamp = getTimestamp();
   mPendingEvents[MagneticField].magnetic.x = (vals[1]/5.12f - 800.f)  + magCalibration[1];
   mPendingEvents[MagneticField].magnetic.y = (vals[0]/5.12f - 800.f) + magCalibration[0];
   mPendingEvents[MagneticField].magnetic.z = (vals[2]/5.12f - 800.f) + magCalibration[2];
   *data = mPendingEvents[MagneticField];
   
   return 1;
}

void MagneticSensor::processEvent(int code, int value)
{
    MLOGV("Process Mag Event %d\n", code);
    switch (code) {
        case EVENT_TYPE_MAGV_X:
            mPendingMask |= 1<<MagneticField;
            mPendingEvents[MagneticField].magnetic.x = value * CONVERT_M_X;
            break;
        case EVENT_TYPE_MAGV_Y:
            mPendingMask |= 1<<MagneticField;
            mPendingEvents[MagneticField].magnetic.y = value * CONVERT_M_Y;
            break;
        case EVENT_TYPE_MAGV_Z:
            mPendingMask |= 1<<MagneticField;
            mPendingEvents[MagneticField].magnetic.z = value * CONVERT_M_Z;
            break;
    }
}
