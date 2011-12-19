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

#define LOG_NDEBUG 0
#define LOG_TAG "Sensors"

#if LOG_NDEBUG
#define FUNC_LOG
#else
#define FUNC_LOG LOGV("%s", __PRETTY_FUNCTION__)
#endif

#include <hardware/sensors.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>
#include <poll.h>
#include <pthread.h>
#include <stdlib.h>

#include <linux/input.h>

#include <utils/Atomic.h>
#include <utils/Log.h>

#include "sensors.h"
#include "SensorBase.h"
#include "ISL29023.h"
#include "MMC31xx.h"
#include "LIS3lv02d.h"

#define SENSORS_ACCELERATION (1<<ID_A)
#define SENSORS_LIGHT (1<<ID_L)
#define SENSORS_MAGNETIC_FIELD (1<<ID_M)

#define SENSORS_ACCELERATION_HANDLE     (ID_A)
#define SENSORS_LIGHT_HANDLE            (ID_L)
#define SENSORS_MAGNETIC_FIELD_HANDLE   (ID_M)

// TODO: Fix values to actually match what the drivers output? 
static struct sensor_t sSensorList[] = {
    { "ISL29023 Light Sensor",
        "ISL",
        1, SENSORS_LIGHT_HANDLE,
        SENSOR_TYPE_LIGHT, 3000.0f, 1.0f, 0.75f, 0, {} },
    { "lis3lv02d Acceleration Sensor", 
        "LIS",
        1, SENSORS_ACCELERATION_HANDLE,
        SENSOR_TYPE_ACCELEROMETER, 10240.0f, 1.0f, 0.5f, 10000, { } },
    { "MMC31xx Orientation Sensor",
        "Memsic",
	1, SENSORS_MAGNETIC_FIELD_HANDLE,
        SENSOR_TYPE_ORIENTATION, 360.0f, 1.0f, 9.7f, 10000, { } },
};

static const int numSensors = ARRAY_SIZE(sSensorList);


static int open_sensors(const struct hw_module_t* module, const char* id,
                        struct hw_device_t** device);


static int sensors__get_sensors_list(struct sensors_module_t* module,
                                     struct sensor_t const** list)
{
    FUNC_LOG;
    *list = sSensorList;
    return numSensors;
}

static struct hw_module_methods_t sensors_module_methods = {
    open: open_sensors
};

struct sensors_module_t HAL_MODULE_INFO_SYM = {
    common: {
        tag: HARDWARE_MODULE_TAG,
        version_major: 1,
        version_minor: 0,
        id: SENSORS_HARDWARE_MODULE_ID,
        name: "Adam Sensor module",
        author: "Jens Andersen",
        methods: &sensors_module_methods,
        dso: 0,
        reserved: {},
    },
    get_sensors_list: sensors__get_sensors_list,
};

/*****************************************************************************/
struct sensors_poll_context_t
{
    struct sensors_poll_device_t device; // must be first
    sensors_poll_context_t();
    ~sensors_poll_context_t();
   /* SensorBase& getSensor(uint32_t id)
    {
        if(id < numSensors)
            return m_Sensors[id];
        return NULL;
    }*/
    int activate(int handle, int enabled);
    int setDelay(int handle, int64_t ns);
    int pollEvents(sensors_event_t* data, int count);
private:
    SensorBase* m_Sensors[numSensors];
};

static int poll__close(struct hw_device_t *dev)
{
    FUNC_LOG;
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    if (ctx) {
        delete ctx;
    }
    return 0;
}

static int poll__activate(struct sensors_poll_device_t *dev,
                          int handle, int enabled)
{
    FUNC_LOG;
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->activate(handle, enabled);
}

static int poll__setDelay(struct sensors_poll_device_t *dev,
                          int handle, int64_t ns)
{
    FUNC_LOG;
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->setDelay(handle, ns);
}

static int poll__poll(struct sensors_poll_device_t *dev,
                      sensors_event_t* data, int count)
{
    FUNC_LOG;
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->pollEvents(data, count);
}


sensors_poll_context_t::sensors_poll_context_t()
{
    FUNC_LOG;
    m_Sensors[SENSORS_ACCELERATION_HANDLE] = new LIS3lv02d();
    m_Sensors[SENSORS_MAGNETIC_FIELD_HANDLE] = new MMC31xx();
    m_Sensors[SENSORS_LIGHT_HANDLE] = new ISL29023();
};

sensors_poll_context_t::~sensors_poll_context_t()
{
    for(int i=0; i<numSensors; i++)
        delete m_Sensors[i];
};

int sensors_poll_context_t::activate(int handle, int enabled)
{
    FUNC_LOG;
    if(handle < 0 || handle >= numSensors)
        return EINVAL;
    return m_Sensors[handle]->activate(enabled);
}

int sensors_poll_context_t::setDelay(int handle, int64_t ns)
{
    FUNC_LOG;
    if(handle < 0 || handle >= numSensors)
        return EINVAL;
    return m_Sensors[handle]->setDelay(ns);
}

int sensors_poll_context_t::pollEvents(sensors_event_t* data, int count)
{
    FUNC_LOG;
    int local_count = count;
    for(int i=0; i<numSensors; i++)
    {
        int numRead = m_Sensors[i]->poll(data, count);
        if(numRead < 0)
            return EINVAL;
        count -= numRead;
    }
    return 0;
}


/** Open a new instance of a sensor device using name */
static int open_sensors(const struct hw_module_t* module, const char* id,
                        struct hw_device_t** device)
{
    FUNC_LOG;
    int status = -EINVAL;
    sensors_poll_context_t *dev = new sensors_poll_context_t();
    
    memset(&dev->device, 0, sizeof(sensors_poll_device_t));
    
    dev->device.common.tag = HARDWARE_DEVICE_TAG;
    dev->device.common.version  = 0;
    dev->device.common.module   = const_cast<hw_module_t*>(module);
    dev->device.common.close    = poll__close;
    dev->device.activate        = poll__activate;
    dev->device.setDelay        = poll__setDelay;
    dev->device.poll            = poll__poll;
    
    *device = &dev->device.common;
    status = 0;
    
    return status;
}

