#include "dht11.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "task.h"

/* ===================== DWT microsecond delay ===================== */
void DWT_Delay_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;
}

void DWT_Delay_us(uint32_t us)
{
    uint32_t start  = DWT->CYCCNT;
    uint32_t ticks  = us * (SystemCoreClock / 1000000U);
    while ((DWT->CYCCNT - start) < ticks) { /* spin */ }
}

/* ===================== Pin direction helpers ===================== */
static void DHT11_SetOutput(void)
{
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin   = DHT11_GPIO_PIN;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_GPIO_PORT, &gpio);
}

static void DHT11_SetInput(void)
{
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin   = DHT11_GPIO_PIN;
    gpio.Mode  = GPIO_MODE_INPUT;
    gpio.Pull  = GPIO_PULLUP;
    HAL_GPIO_Init(DHT11_GPIO_PORT, &gpio);
}

static inline void DHT11_Write(GPIO_PinState s)
{
    HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_GPIO_PIN, s);
}

static inline GPIO_PinState DHT11_Read_Pin(void)
{
    return HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_GPIO_PIN);
}

/* Waits for the pin to reach 'state', up to 'timeout_us'.
 * Returns 0 on success, -1 on timeout. */
static int DHT11_WaitForState(GPIO_PinState state, uint32_t timeout_us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = timeout_us * (SystemCoreClock / 1000000U);
    while (DHT11_Read_Pin() != state)
    {
        if ((DWT->CYCCNT - start) > ticks)
            return -1;
    }
    return 0;
}

/* ===================== Main read routine ===================== */
HAL_StatusTypeDef DHT11_Read(DHT11_Data_t *data)
{
    uint8_t bytes[5] = {0};

    /* --- Host start signal --- */
    DHT11_SetOutput();
    DHT11_Write(GPIO_PIN_RESET);
    osDelay(18);                 /* >= 18 ms low, non time-critical -> can osDelay */
    DHT11_Write(GPIO_PIN_SET);
    DWT_Delay_us(30);
    DHT11_SetInput();

    /* Timing from here on is critical: keep the scheduler from preempting
     * this task mid-bit so we don't mis-sample the line. */
    taskENTER_CRITICAL();

    /* --- Sensor response: 80us low, 80us high --- */
    if (DHT11_WaitForState(GPIO_PIN_RESET, 100) != 0) goto fail;
    if (DHT11_WaitForState(GPIO_PIN_SET,   100) != 0) goto fail;
    if (DHT11_WaitForState(GPIO_PIN_RESET, 100) != 0) goto fail;

    /* --- 40 data bits --- */
    for (int i = 0; i < 40; i++)
    {
        if (DHT11_WaitForState(GPIO_PIN_SET, 65) != 0) goto fail;

        uint32_t t0 = DWT->CYCCNT;
        if (DHT11_WaitForState(GPIO_PIN_RESET, 100) != 0) goto fail;
        uint32_t t1 = DWT->CYCCNT;

        uint32_t high_us = (t1 - t0) / (SystemCoreClock / 1000000U);
        uint8_t bit = (high_us > 40) ? 1 : 0; /* ~26-28us = 0, ~70us = 1 */

        bytes[i / 8] = (bytes[i / 8] << 1) | bit;
    }

    taskEXIT_CRITICAL();

    /* --- Checksum --- */
    uint8_t sum = (uint8_t)(bytes[0] + bytes[1] + bytes[2] + bytes[3]);
    if (sum != bytes[4])
    {
        data->valid = 0;
        data->errorCount++;
        DHT11_SetOutput();
        DHT11_Write(GPIO_PIN_SET); /* idle high */
        return HAL_ERROR;
    }

    data->humidity    = (float)bytes[0] + (float)bytes[1] / 10.0f;
    data->temperature = (float)bytes[2] + (float)bytes[3] / 10.0f;
    data->valid       = 1;

    DHT11_SetOutput();
    DHT11_Write(GPIO_PIN_SET); /* idle high, ready for next request */
    return HAL_OK;

fail:
    taskEXIT_CRITICAL();
    data->valid = 0;
    data->errorCount++;
    DHT11_SetOutput();
    DHT11_Write(GPIO_PIN_SET);
    return HAL_TIMEOUT;
}
