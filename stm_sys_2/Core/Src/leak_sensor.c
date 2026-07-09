#include "leak_sensor.h"

static ADC_HandleTypeDef *s_hadc = NULL;

#define ADC_TIMEOUT_MS 20
#define ADC_VREF       3.3f
#define ADC_MAX_COUNT  4095.0f

void LeakSensor_Init(ADC_HandleTypeDef *hadc)
{
    s_hadc = hadc;
    /* Run one throwaway conversion so the ADC's internal sample/hold and
     * calibration settle before the first real reading. */
    HAL_ADCEx_Calibration_Start(s_hadc);
}

HAL_StatusTypeDef LeakSensor_Read(Leak_Data_t *data)
{
    HAL_StatusTypeDef st;

    st = HAL_ADC_Start(s_hadc);
    if (st != HAL_OK) goto fail;

    st = HAL_ADC_PollForConversion(s_hadc, ADC_TIMEOUT_MS);
    if (st != HAL_OK)
    {
        HAL_ADC_Stop(s_hadc);
        goto fail;
    }

    data->raw          = (uint16_t)HAL_ADC_GetValue(s_hadc);
    data->voltage      = ((float)data->raw / ADC_MAX_COUNT) * ADC_VREF;
    data->leakDetected = (data->raw >= LEAK_THRESHOLD_RAW) ? 1 : 0;
    data->valid        = 1;

    HAL_ADC_Stop(s_hadc);
    return HAL_OK;

fail:
    data->valid = 0;
    data->errorCount++;
    return st;
}
