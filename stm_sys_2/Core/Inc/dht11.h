#ifndef DHT11_H
#define DHT11_H

#include "main.h"
#include <stdint.h>

/* ---- Pin configuration ---- */
#define DHT11_GPIO_PORT   GPIOA
#define DHT11_GPIO_PIN    GPIO_PIN_1

typedef struct {
    float   temperature;   /* degrees C            */
    float   humidity;      /* % RH                  */
    uint8_t valid;         /* 1 = last read succeeded */
    uint32_t errorCount;   /* running count of failed reads (diagnostics) */
} DHT11_Data_t;

/* DWT cycle-counter based microsecond delay, used by dht11.c and shared
 * with other drivers that need tight timing. */
void     DWT_Delay_Init(void);
void     DWT_Delay_us(uint32_t us);

/* Blocking single-shot read of the DHT11. Call at most once per second
 * (sensor limitation). Returns HAL_OK on success. On failure the previous
 * *data content is left untouched except 'valid' is cleared. */
HAL_StatusTypeDef DHT11_Read(DHT11_Data_t *data);

#endif /* DHT11_H */
