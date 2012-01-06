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

#ifndef ANDROID_MAGNETIC_SENSOR_H
#define ANDROID_MAGNETIC_SENSOR_H

#define MAGNETIC_CALIBRATION_FILE "/data/misc/magnetic_calibration"
#include <stdint.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>


#include "nusensors.h"
#include "SensorBase.h"
#include "InputEventReader.h"

/*****************************************************************************/

struct input_event;

class MagneticSensor : public SensorBase {
public:
            MagneticSensor();
    virtual ~MagneticSensor();

    enum {
        MagneticField   = 0,
        numSensors
    };

    virtual int setDelay(int32_t handle, int64_t ns);
    virtual int enable(int32_t handle, int enabled);
    virtual int readEvents(sensors_event_t* data, int count);
    virtual bool hasPendingEvents() const;
    void processEvent(int code, int value);

private:
    int magCalibration[3];
    int update_delay();
    uint32_t mEnabled;
    uint32_t mPendingMask;
    InputEventCircularReader mInputReader;
    sensors_event_t mPendingEvents[numSensors];
    uint64_t mDelays[numSensors];
    int64_t mMinDelay;
    int64_t mLastUpdate;
};

/*****************************************************************************/

#endif  // ANDROID_MAGNETIC_SENSOR_H
