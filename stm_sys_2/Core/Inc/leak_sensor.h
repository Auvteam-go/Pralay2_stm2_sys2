#ifndef LEAK_SENSOR_H
#define LEAK_SENSOR_H

#include "main.h"
#include <stdint.h>

/* Raw ADC threshold above which the sensor is considered to report a leak.
 * Calibrate against your specific probe/board (12-bit ADC, 0-4095). */
#define LEAK_THRESHOLD_RAW   2000U

typedef struct {
    uint16_t raw;            /* raw 12-bit ADC reading      */
    float    voltage;        /* converted to volts (3.3V ref) */
    uint8_t  leakDetected;   /* 1 = leak condition present    */
    uint8_t  valid;
    uint32_t errorCount;
} Leak_Data_t;

/* Must be called once at startup with the ADC handle to use. */
void LeakSensor_Init(ADC_HandleTypeDef *hadc);

/* Blocking single conversion read. */
HAL_StatusTypeDef LeakSensor_Read(Leak_Data_t *data);

#endif /* LEAK_SENSOR_H */
