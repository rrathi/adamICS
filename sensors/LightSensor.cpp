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

//#include <linux/max9635.h>

#include <cutils/log.h>

#include "LightSensor.h"

/*****************************************************************************/

LightSensor::LightSensor()
    : SensorBase(LIGHTING_DEVICE_NAME, NULL),
      mEnabled(0),
      mInputReader(1),
      mHasPendingEvent(false)
{
    mPendingEvent.version = sizeof(sensors_event_t);
    mPendingEvent.sensor = ID_L;
    mPendingEvent.type = SENSOR_TYPE_LIGHT;
    memset(mPendingEvent.data, 0, sizeof(mPendingEvent.data));
}

LightSensor::~LightSensor() {
}

int LightSensor::enable(int32_t, int en) {
    int err;
    mEnabled = !!en;
/*    if(mEnabled != en) {
        if (en) {
            open_device();
        }
        err = ioctl(dev_fd, MAX9635_IOCTL_SET_ENABLE,&en);
        err = err<0 ? -errno : 0;
        LOGE_IF(err, "MAX9635_IOCTL_SET_ENABLE failed (%s)", strerror(-err));
        if (!err) {
            mEnabled = en;
        }
        if (!en) {
            close_device();
        }
    }*/
    return 0;
}

bool LightSensor::hasPendingEvents() const {
    return mEnabled;
}

int LightSensor::readEvents(sensors_event_t* data, int count)
{
    if (count < 1 || data == NULL)
        return -EINVAL;

    if(!mEnabled)
	return 0;

    unsigned int value;
    readIntFromFile(LIGHTING_DEVICE_NAME, &value);
    mPendingEvent.timestamp = getTimestamp();
    mPendingEvent.light = (float)value;
    *data = mPendingEvent;
    return 1;
}

float LightSensor::indexToValue(size_t index) const
{
    return float(index);
}
