# Pralay2_stm2_sys2
This system consists of INA260+LEAK_SENSOR+DHT11.
# Multi-Sensor Telemetry System - STM32F103C8T6

## 📋 Table of Contents
1. [System Overview](#system-overview)
2. [Hardware Requirements](#hardware-requirements)
3. [Pin Configuration](#pin-configuration)
4. [Software Architecture](#software-architecture)
5. [RTOS Configuration](#rtos-configuration)
6. [Task Details](#task-details)
7. [Communication Protocols](#communication-protocols)
8. [Data Structures](#data-structures)
9. [Telemetry Packet Format](#telemetry-packet-format)
10. [Build & Flash Instructions](#build--flash-instructions)
11. [Debugging](#debugging)
12. [Troubleshooting](#troubleshooting)

---

## System Overview

This is a **real-time multi-sensor telemetry system** running on an STM32F103C8T6 (Blue Pill) microcontroller. It reads data from three different sensors concurrently using FreeRTOS and transmits the data via UART using DMA for efficient communication.

### Key Features
- ✅ **True multitasking** with FreeRTOS (CMSIS-RTOS v2)
- ✅ **Concurrent sensor reading** (DHT11, INA260, Leak Sensor)
- ✅ **Non-blocking UART** transmission with DMA
- ✅ **Thread-safe data sharing** using mutexes
- ✅ **Real-time telemetry** packet transmission
- ✅ **Low CPU usage** with power-efficient design
- ✅ **Industrial-grade** synchronization mechanisms

### System Block Diagram
```
┌─────────────────────────────────────────────────────────────────────┐
│                        STM32F103C8T6                               │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐         │
│  │   DHT11Task   │  │  INA260Task   │  │   LeakTask    │         │
│  │   (1000ms)    │  │   (200ms)     │  │   (100ms)     │         │
│  │  Priority: N  │  │  Priority: N  │  │  Priority: N  │         │
│  └───────┬───────┘  └───────┬───────┘  └───────┬───────┘         │
│          │                  │                  │                  │
│          ▼                  ▼                  ▼                  │
│  ┌────────────────────────────────────────────────────────┐      │
│  │           Shared Data (Protected by Mutexes)           │      │
│  │  ┌───────────┐  ┌───────────┐  ┌───────────┐        │      │
│  │  │ g_dhtData │  │ g_inaData │  │g_leakData │        │      │
│  │  │ (MUTEX 1) │  │ (MUTEX 2) │  │ (MUTEX 3) │        │      │
│  │  └───────────┘  └───────────┘  └───────────┘        │      │
│  └────────────────────────────────────────────────────────┘      │
│                            │                                      │
│                            ▼                                      │
│  ┌────────────────────────────────────────────────────────┐      │
│  │         UARTTask (Priority: AboveNormal)               │      │
│  │               (500ms Period)                           │      │
│  │  - Snapshot all sensor data                           │      │
│  │  - Build telemetry packet                             │      │
│  │  - Transmit via UART DMA                             │      │
│  └────────────────────┬───────────────────────────────────┘      │
│                       │                                          │
│                       ▼                                          │
│  ┌────────────────────────────────────────────────────────┐      │
│  │         UART DMA Transmission (Background)             │      │
│  │  - Non-blocking transmission                          │      │
│  │  - CPU free for other tasks                          │      │
│  └────────────────────────────────────────────────────────┘      │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

---

## Hardware Requirements

### Required Components

| Component | Quantity | Specification |
|-----------|----------|---------------|
| STM32F103C8T6 Blue Pill | 1 | ARM Cortex-M3 @72MHz, 20KB RAM, 64KB Flash |
| DHT11 Sensor | 1 | Temperature & Humidity sensor |
| INA260 Sensor | 1 | Voltage/Current/Power sensor (I2C) |
| Leak Sensor | 1 | Analog water leak detector |
| USB-to-Serial Adapter | 1 | For UART communication (e.g., CP2102, FTDI) |
| Breadboard & Jumper Wires | - | For connections |
| 3.3V/5V Power Supply | 1 | For powering the system |

### Optional Components
- LED for debugging (PC13 - onboard LED on Blue Pill)
- Logic Analyzer for debugging protocols
- Oscilloscope for signal verification

### Power Requirements
- **Supply Voltage**: 5V (via USB or external) or 3.3V
- **Current Consumption**: ~50mA typical
- **DHT11**: 2.5-5.5V, 0.5mA typical
- **INA260**: 3.0-5.5V, 340µA typical

---

## Pin Configuration

### Complete Pinout Table

| **Pin Name** | **Function** | **Direction** | **Description** |
|--------------|--------------|---------------|-----------------|
| **PA9** | USART1_TX | Output | UART transmit (to USB-serial) |
| **PA10** | USART1_RX | Input | UART receive (from USB-serial) |
| **PA1** | DHT11_DATA | Bidirectional | DHT11 single-wire data line |
| **PA0** | ADC1_IN0 | Input | Leak sensor analog input |
| **PB6** | I2C1_SCL | Output | I2C clock for INA260 |
| **PB7** | I2C1_SDA | Bidirectional | I2C data for INA260 |
| **PC13** | LED | Output | Onboard Blue Pill LED (Active Low) |
| **3.3V** | VDD | Output | 3.3V output for sensors |
| **GND** | GND | Ground | Common ground |

### Detailed Pin Configuration

```
┌─────────────────────────────────────────────────────────────────────────┐
│                       STM32F103C8T6 (Blue Pill)                        │
│                                                                         │
│  ┌───────────────────────────────────────────────────────────┐         │
│  │                                                           │         │
│  │   PB9 ───┐                                         ┌─────PA0    Leak│
│  │   PB8 ───┤                                         ├─────PA1    DHT11│
│  │   BOOT0──┤                                         ├─────PA2         │
│  │   PB7 ───┼─ I2C1 SDA (INA260)                      ├─────PA3         │
│  │   PB6 ───┼─ I2C1 SCL (INA260)                      ├─────PA4         │
│  │   PB5 ───┤                                         ├─────PA5         │
│  │   PB4 ───┤                                         ├─────PA6         │
│  │   PB3 ───┤                                         ├─────PA7         │
│  │   PA15───┤                                         ├─────PB0         │
│  │   PA14───┤                                         ├─────PB1         │
│  │   PA13───┤                                         ├─────PB10        │
│  │   PA12───┤                                         ├─────PB11        │
│  │   PA11───┤                                         ├─────PB12        │
│  │   PA10───┼─ USART1_RX                              ├─────PB13        │
│  │   PA9 ───┼─ USART1_TX                              ├─────PB14        │
│  │   PA8 ───┤                                         ├─────PB15        │
│  │   PC13───┼─ LED (Onboard)                          ├─────PC14        │
│  │                                                           │         │
│  └───────────────────────────────────────────────────────────┘         │
│                                                                         │
│  3.3V ───┬─ Power for sensors                                        │
│  5V  ────┴─ Main power input                                         │
│  GND ────┬─ Common ground                                           │
└─────────────────────────────────────────────────────────────────────────┘
```

### Connection Diagram

```
┌──────────────────┐              ┌─────────────────────────────────┐
│   STM32F103C8T6  │              │         Sensors & Peripherals   │
├──────────────────┤              ├─────────────────────────────────┤
│                  │              │                                 │
│   PA9 (TX)       ├──────────────┼──► USB-Serial Adapter (RX)    │
│   PA10 (RX)      ├──────────────┼──► USB-Serial Adapter (TX)    │
│                  │              │                                 │
│   PA1 (DHT11)    ├──────────────┼──► DHT11 Sensor (DATA)        │
│                  │              │    ├── 3.3V                    │
│                  │              │    ├── GND                     │
│                  │              │                                 │
│   PA0 (ADC)      ├──────────────┼──► Leak Sensor (Analog Out)   │
│                  │              │    ├── 3.3V                    │
│                  │              │    ├── GND                     │
│                  │              │                                 │
│   PB6 (SCL)      ├──────────────┼──► INA260 (SCL)              │
│   PB7 (SDA)      ├──────────────┼──► INA260 (SDA)              │
│                  │              │    ├── 3.3V                    │
│                  │              │    ├── GND                     │
│                  │              │                                 │
│   PC13 (LED)     ├──────────────┼──► Onboard LED                │
│                  │              │                                 │
│   3.3V           ├──────────────┼──► Power to all sensors       │
│   GND            ├──────────────┼──► Common ground              │
│                  │              │                                 │
└──────────────────┘              └─────────────────────────────────┘
```

---

## Software Architecture

### File Structure
```
📁 Project Root
├── 📁 Core
│   ├── 📁 Inc
│   │   ├── main.h
│   │   ├── dht11.h
│   │   ├── ina260.h
│   │   ├── leak_sensor.h
│   │   └── FreeRTOSConfig.h
│   └── 📁 Src
│       ├── main.c
│       ├── dht11.c
│       ├── ina260.c
│       ├── leak_sensor.c
│       ├── system_stm32f1xx.c
│       └── stm32f1xx_it.c
├── 📁 Drivers
│   └── 📁 STM32F1xx_HAL_Driver
├── 📁 Middlewares
│   └── 📁 Third_Party
│       └── 📁 FreeRTOS
├── 📁 .vscode
│   └── (IDE configuration files)
├── 📁 Debug
├── 📄 .ioc (STM32CubeMX project file)
├── 📄 Makefile
└── 📄 README.md
```

### Module Dependencies
```
┌─────────────────────────────────────────────────────────────────────┐
│                         APPLICATION LAYER                          │
├─────────────────────────────────────────────────────────────────────┤
│  main.c                                                             │
│  ├── DHT11Task()                                                   │
│  ├── INA260Task()                                                  │
│  ├── LeakTask()                                                    │
│  └── UARTTask()                                                    │
├─────────────────────────────────────────────────────────────────────┤
│                         SENSOR DRIVERS                              │
├─────────────────────────────────────────────────────────────────────┤
│  dht11.c          │  ina260.c        │  leak_sensor.c              │
│  ├── DHT11_Read() │  ├── INA260_Read()│  ├── LeakSensor_Read()     │
│  └── DHT11_Init() │  └── INA260_Init()│  └── LeakSensor_Init()     │
├─────────────────────────────────────────────────────────────────────┤
│                      CMSIS-RTOS v2 (FreeRTOS)                      │
├─────────────────────────────────────────────────────────────────────┤
│  │  osMutexNew()  │  osSemaphoreNew()│  osThreadNew()             │
│  │  osMutexAcquire│  osSemaphoreAcq  │  osThreadGetId()           │
│  │  osMutexRelease│  osSemaphoreRel  │  osDelay()                 │
├─────────────────────────────────────────────────────────────────────┤
│                    STM32F1xx HAL Drivers                           │
├─────────────────────────────────────────────────────────────────────┤
│  GPIO  │  I2C  │  ADC  │  UART  │  DMA  │  NVIC                   │
├─────────────────────────────────────────────────────────────────────┤
│                        HARDWARE                                     │
├─────────────────────────────────────────────────────────────────────┤
│  STM32F103C8T6 │ DHT11 │ INA260 │ Leak │ USB-Serial               │
└─────────────────────────────────────────────────────────────────────┘
```

---

## RTOS Configuration

### FreeRTOS Configuration (`FreeRTOSConfig.h`)

```c
/*-----------------------------------------------------------
 * Application specific definitions.
 *----------------------------------------------------------*/

#define configUSE_PREEMPTION                    1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1
#define configUSE_TICKLESS_IDLE                 0
#define configCPU_CLOCK_HZ                      72000000
#define configTICK_RATE_HZ                      1000
#define configMAX_PRIORITIES                    5
#define configMINIMAL_STACK_SIZE                128
#define configMAX_TASK_NAME_LEN                 16
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_TASK_NOTIFICATIONS            1
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             0
#define configUSE_COUNTING_SEMAPHORES           1
#define configUSE_ALTERNATIVE_API               0
#define configQUEUE_REGISTRY_SIZE               8
#define configUSE_QUEUE_SETS                    0
#define configUSE_TIME_SLICING                  1
#define configUSE_NEWLIB_REENTRANT              0
#define configENABLE_BACKWARD_COMPATIBILITY     0
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 5

/* Memory allocation */
#define configSUPPORT_DYNAMIC_ALLOCATION        1
#define configSUPPORT_STATIC_ALLOCATION         0

/* Timer */
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               2
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            128

/* Interrupt nesting */
#define configKERNEL_INTERRUPT_PRIORITY         1
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    3

/* Hook functions */
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configCHECK_FOR_STACK_OVERFLOW          2
#define configUSE_MALLOC_FAILED_HOOK            1

/* Heap */
#define configTOTAL_HEAP_SIZE                   8192
```

### CMSIS-RTOS v2 Configuration

```c
// System Configuration
#define osCMSIS                 20020000  // CMSIS-RTOS v2 version
#define osKernelSystemId        0x52425441  // "RBTA"
#define osFeature_MainThread    1
#define osFeature_Signals       0
#define osFeature_Semaphore     1
#define osFeature_Wait          0
#define osFeature_SysTick       1
#define osFeature_Pool          0
#define osFeature_MessageQ      1
#define osFeature_MemoryPool    0
#define osFeature_EventFlags    1
#define osFeature_Timer         1
#define osFeature_Thread        1
#define osFeature_Mutex         1
```

---

## Task Details

### Task Summary

| Task Name | Function | Priority | Period | Stack Size | Description |
|-----------|----------|----------|--------|------------|-------------|
| **DHT11Task** | `DHT11Task()` | Normal | 1000ms | 1024 bytes | Reads temperature & humidity |
| **INA260Task** | `INA260Task()` | Normal | 200ms | 1024 bytes | Reads voltage, current, power |
| **LeakTask** | `LeakTask()` | Normal | 100ms | 768 bytes | Reads leak sensor (ADC) |
| **UARTTask** | `UARTTask()` | AboveNormal | 500ms | 1024 bytes | Transmits telemetry packet |

### Detailed Task Specifications

#### 1. DHT11Task
```c
static void DHT11Task(void *argument)
{
    DHT11_Data_t local = {0};
    
    for (;;)
    {
        // Read sensor (takes ~40ms)
        DHT11_Read(&local);
        
        // Update shared data (protected by mutex)
        osMutexAcquire(g_dhtMutex, osWaitForever);
        g_dhtData.temperature = local.temperature;
        g_dhtData.humidity    = local.humidity;
        g_dhtData.valid       = local.valid;
        g_dhtData.errorCount  = local.errorCount;
        osMutexRelease(g_dhtMutex);
        
        // Wait 1000ms
        osDelay(pdMS_TO_TICKS(DHT11_PERIOD_MS));
    }
}
```

**Memory Usage:**
- Stack: 1024 bytes
- Local variables: ~20 bytes
- Function calls: DHT11_Read() uses additional stack

**Timing:**
- Execution time: ~40ms (DHT11 read)
- Sleep time: 1000ms
- CPU utilization: ~4%

#### 2. INA260Task
```c
static void INA260Task(void *argument)
{
    INA260_Data_t local = {0};
    
    for (;;)
    {
        // Read sensor (I2C communication)
        INA260_Read(&local);
        
        // Update shared data (protected by mutex)
        osMutexAcquire(g_inaMutex, osWaitForever);
        g_inaData.voltage_V  = local.voltage_V;
        g_inaData.current_A  = local.current_A;
        g_inaData.power_W    = local.power_W;
        g_inaData.valid      = local.valid;
        g_inaData.errorCount = local.errorCount;
        osMutexRelease(g_inaMutex);
        
        // Wait 200ms
        osDelay(pdMS_TO_TICKS(INA260_PERIOD_MS));
    }
}
```

**Memory Usage:**
- Stack: 1024 bytes
- Local variables: ~30 bytes
- I2C buffers: ~32 bytes

**Timing:**
- Execution time: ~5-10ms (I2C read)
- Sleep time: 200ms
- CPU utilization: ~2.5-5%

#### 3. LeakTask
```c
static void LeakTask(void *argument)
{
    Leak_Data_t local = {0};
    
    for (;;)
    {
        // Read ADC
        LeakSensor_Read(&local);
        
        // Update shared data (protected by mutex)
        osMutexAcquire(g_leakMutex, osWaitForever);
        g_leakData.raw          = local.raw;
        g_leakData.voltage      = local.voltage;
        g_leakData.leakDetected = local.leakDetected;
        g_leakData.valid        = local.valid;
        g_leakData.errorCount   = local.errorCount;
        osMutexRelease(g_leakMutex);
        
        // Wait 100ms
        osDelay(pdMS_TO_TICKS(LEAK_PERIOD_MS));
    }
}
```

**Memory Usage:**
- Stack: 768 bytes
- Local variables: ~16 bytes

**Timing:**
- Execution time: ~1ms (ADC read)
- Sleep time: 100ms
- CPU utilization: ~1%

#### 4. UARTTask
```c
static void UARTTask(void *argument)
{
    TelemetryPacket_t pkt;
    
    for (;;)
    {
        // Build packet (snapshot all sensor data)
        Telemetry_BuildPacket(&pkt);
        
        // Wait for semaphore (DMA free)
        osSemaphoreAcquire(s_txDonesem, osWaitForever);
        
        // Start DMA transmission
        HAL_UART_Transmit_DMA(&huart1, (uint8_t*)&pkt, sizeof(pkt));
        
        // Wait 500ms
        osDelay(pdMS_TO_TICKS(UART_PERIOD_MS));
    }
}
```

**Memory Usage:**
- Stack: 1024 bytes
- Packet buffer: 33 bytes
- Local variables: ~50 bytes

**Timing:**
- Execution time: ~1ms (building packet)
- DMA transfer: ~3ms @ 115200 baud
- Sleep time: 500ms
- CPU utilization: ~1%

---

## Communication Protocols

### 1. UART (USART1) Configuration

```c
// USART1 Settings
Baud Rate:      115200
Data Bits:      8
Stop Bits:      1
Parity:         None
Flow Control:   None
Mode:           TX/RX
Oversampling:   16

// DMA Settings
Channel:        DMA1 Channel4 (TX)
Direction:      Memory to Peripheral
Mode:           Normal (not circular)
Data Width:     Byte (8-bit)
Priority:       High
```

**UART Pinout:**
```
┌─────────┬─────────────┬──────────────────┐
│ Pin     │ Signal      │ Connection       │
├─────────┼─────────────┼──────────────────┤
│ PA9     │ USART1_TX   │ USB-Serial (RX)  │
│ PA10    │ USART1_RX   │ USB-Serial (TX)  │
└─────────┴─────────────┴──────────────────┘
```

**UART Frame Format:**
```
┌──────┬──────┬──────┬──────┬──────┐
│ START│ DATA │ PARITY│ STOP │  IDLE│
│ Bit  │ 8-bit│ (none)│ 1-bit│      │
└──────┴──────┴──────┴──────┴──────┘

Timing: 1 bit = 1/115200 = 8.68µs
Total frame time: 10 bits = 86.8µs
```

### 2. I2C (I2C1) Configuration

```c
// I2C1 Settings
Mode:           Standard Mode
Clock Speed:    100 kHz
Duty Cycle:     2
Addressing:     7-bit
Own Address:    0x00

// INA260 I2C Address
Device Address: 0x40 (7-bit)
Write Address:  0x40
Read Address:   0x41
```

**I2C Pinout:**
```
┌─────────┬─────────────┬──────────────────┐
│ Pin     │ Signal      │ Connection       │
├─────────┼─────────────┼──────────────────┤
│ PB6     │ I2C1_SCL    │ INA260 (SCL)     │
│ PB7     │ I2C1_SDA    │ INA260 (SDA)     │
└─────────┴─────────────┴──────────────────┘
```

**INA260 Registers:**
```
┌────────────────┬────────┬──────────────────────────┐
│ Register       │ Address│ Description               │
├────────────────┼────────┼──────────────────────────┤
│ Configuration  │ 0x00   │ Operating mode            │
│ Current        │ 0x01   │ Current reading           │
│ Voltage        │ 0x02   │ Bus voltage reading       │
│ Power          │ 0x03   │ Power reading             │
└────────────────┴────────┴──────────────────────────┘
```

### 3. DHT11 Protocol

```c
// Single-wire protocol
Timing Requirements:
- Start signal:  18ms LOW, then HIGH
- Response:      80µs LOW, 80µs HIGH
- Bit '0':       50µs LOW, 26-28µs HIGH
- Bit '1':       50µs LOW, 70µs HIGH

Data Format:
Total bits: 40 bits (5 bytes)
Byte 0: Humidity integer part
Byte 1: Humidity decimal part
Byte 2: Temperature integer part
Byte 3: Temperature decimal part
Byte 4: Checksum (sum of bytes 0-3)
```

**DHT11 Pinout:**
```
┌─────────┬─────────────┬──────────────────┐
│ Pin     │ Signal      │ Connection       │
├─────────┼─────────────┼──────────────────┤
│ PA1     │ DATA        │ DHT11 (DATA)     │
│ 3.3V    │ VCC         │ DHT11 (VCC)      │
│ GND     │ GND         │ DHT11 (GND)      │
└─────────┴─────────────┴──────────────────┘
```

### 4. ADC (ADC1) Configuration

```c
// ADC Settings
Channel:        ADC_CHANNEL_0 (PA0)
Resolution:     12-bit (0-4095)
Conversion Mode: Single
Sampling Time:  55.5 cycles
Trigger:        Software
Data Alignment: Right
```

**ADC Conversion Formula:**
```
Voltage = (ADC_Value / 4096) * 3.3V
Leak Threshold: > 1.5V = Leak Detected
```

**ADC Pinout:**
```
┌─────────┬─────────────┬──────────────────┐
│ Pin     │ Signal      │ Connection       │
├─────────┼─────────────┼──────────────────┤
│ PA0     │ ADC1_IN0    │ Leak Sensor Out  │
│ 3.3V    │ VREF        │ Reference        │
└─────────┴─────────────┴──────────────────┘
```

---

## Data Structures

### 1. DHT11 Data Structure
```c
typedef struct {
    float temperature;    // Temperature in °C
    float humidity;       // Humidity in %
    uint8_t valid;        // 1 = data valid, 0 = error
    uint8_t errorCount;   // Number of consecutive errors
} DHT11_Data_t;
```

### 2. INA260 Data Structure
```c
typedef struct {
    float voltage_V;      // Bus voltage in Volts
    float current_A;      // Current in Amperes
    float power_W;        // Power in Watts
    uint8_t valid;        // 1 = data valid, 0 = error
    uint8_t errorCount;   // Number of consecutive errors
} INA260_Data_t;
```

### 3. Leak Sensor Data Structure
```c
typedef struct {
    uint16_t raw;         // Raw ADC value (0-4095)
    float voltage;        // Voltage in Volts
    uint8_t leakDetected; // 1 = leak detected, 0 = no leak
    uint8_t valid;        // 1 = data valid, 0 = error
    uint8_t errorCount;   // Number of consecutive errors
} Leak_Data_t;
```

### 4. Telemetry Packet Structure
```c
#pragma pack(push, 1)
typedef struct {
    uint8_t  header;         // 0xAA - Start of packet
    
    // DHT11 Data
    float    temperature;    // °C
    float    humidity;       // %
    uint8_t  dhtValid;       // 1 = valid
    
    // INA260 Data
    float    voltage;        // Volts
    float    current;        // Amperes
    float    power;          // Watts
    uint8_t  inaValid;       // 1 = valid
    
    // Leak Sensor Data
    uint16_t leakRaw;        // ADC raw value
    uint8_t  leakDetected;   // 1 = leak detected
    uint8_t  leakValid;      // 1 = valid
    
    // Packet Info
    uint32_t seqNumber;      // Sequence number (increments each packet)
    uint8_t  checksum;       // XOR checksum of all previous bytes
    uint8_t  footer;         // 0x55 - End of packet
} TelemetryPacket_t;
#pragma pack(pop)

// Total size: 33 bytes
```

---

## Telemetry Packet Format

### Packet Layout
```
┌──────┬─────────┬─────────┬─────────┬─────────┬─────────┬─────────┬─────────┐
│ Byte │ Offset  │ Size    │ Field   │ Type    │ Example │ Description       │
├──────┼─────────┼─────────┼─────────┼─────────┼─────────┼───────────────────┤
│ 0    │ 0       │ 1       │ header  │ uint8_t │ 0xAA    │ Start of packet   │
├──────┼─────────┼─────────┼─────────┼─────────┼─────────┼───────────────────┤
│ 1-4  │ 1       │ 4       │ temp    │ float   │ 25.5    │ Temperature (°C)  │
├──────┼─────────┼─────────┼─────────┼─────────┼─────────┼───────────────────┤
│ 5-8  │ 5       │ 4       │ hum     │ float   │ 60.0    │ Humidity (%)      │
├──────┼─────────┼─────────┼─────────┼─────────┼─────────┼───────────────────┤
│ 9    │ 9       │ 1       │ dhtVal  │ uint8_t │ 1       │ DHT11 valid flag  │
├──────┼─────────┼─────────┼─────────┼─────────┼─────────┼───────────────────┤
│ 10-13│ 10      │ 4       │ volt    │ float   │ 3.30    │ Voltage (V)       │
├──────┼─────────┼─────────┼─────────┼─────────┼─────────┼───────────────────┤
│ 14-17│ 14      │ 4       │ cur     │ float   │ 0.50    │ Current (A)       │
├──────┼─────────┼─────────┼─────────┼─────────┼─────────┼───────────────────┤
│ 18-21│ 18      │ 4       │ power   │ float   │ 1.65    │ Power (W)         │
├──────┼─────────┼─────────┼─────────┼─────────┼─────────┼───────────────────┤
│ 22   │ 22      │ 1       │ inaVal  │ uint8_t │ 1       │ INA260 valid flag │
├──────┼─────────┼─────────┼─────────┼─────────┼─────────┼───────────────────┤
│ 23-24│ 23      │ 2       │ leakRaw │ uint16_t│ 1024    │ ADC raw value     │
├──────┼─────────┼─────────┼─────────┼─────────┼─────────┼───────────────────┤
│ 25   │ 25      │ 1       │ leakDet │ uint8_t │ 0       │ Leak detected?    │
├──────┼─────────┼─────────┼─────────┼─────────┼─────────┼───────────────────┤
│ 26   │ 26      │ 1       │ leakVal │ uint8_t │ 1       │ Leak valid flag   │
├──────┼─────────┼─────────┼─────────┼─────────┼─────────┼───────────────────┤
│ 27-30│ 27      │ 4       │ seq     │ uint32_t│ 42      │ Sequence number   │
├──────┼─────────┼─────────┼─────────┼─────────┼─────────┼───────────────────┤
│ 31   │ 31      │ 1       │ checksum│ uint8_t │ 0xFA    │ XOR checksum      │
├──────┼─────────┼─────────┼─────────┼─────────┼─────────┼───────────────────┤
│ 32   │ 32      │ 1       │ footer  │ uint8_t │ 0x55    │ End of packet     │
└──────┴─────────┴─────────┴─────────┴─────────┴─────────┴───────────────────┘
```

### Checksum Calculation
```c
// Checksum covers all bytes from header to seqNumber (bytes 0-30)
uint8_t checksum = 0;
for (int i = 0; i < 31; i++) {
    checksum ^= packet[i];
}
// Result stored at byte 31
```

### Example Packet Hex Dump
```
AA 00 00 CC 41 00 00 70 42 01 33 33 53 40 00 00 00 3F 33 33 D3 3F 01 00 04 00 01 00 00 00 2A FA 55
│  └─────────────────┘ └─────────────────┘ └──────────┘ └──────────────┘ └──────────┘ └──────┘ └─┘
│         TEMP              HUMIDITY        DHT VALID       VOLTAGE         CURRENT      POWER
│                                                                                     INA VALID
│                                                                                       LEAK RAW
│                                                                                        LEAK DET
│                                                                                         LEAK VAL
│                                                                                          SEQ NUM
│                                                                                           CHECK
│                                                                                             FOOTER
```

### Packet Parsing (Python Example)
```python
import struct

def parse_packet(data):
    # data should be 33 bytes
    header = data[0]
    temp = struct.unpack('<f', data[1:5])[0]
    hum = struct.unpack('<f', data[5:9])[0]
    dht_valid = data[9]
    volt = struct.unpack('<f', data[10:14])[0]
    current = struct.unpack('<f', data[14:18])[0]
    power = struct.unpack('<f', data[18:22])[0]
    ina_valid = data[22]
    leak_raw = struct.unpack('<H', data[23:25])[0]
    leak_det = data[25]
    leak_valid = data[26]
    seq = struct.unpack('<I', data[27:31])[0]
    checksum = data[31]
    footer = data[32]
    
    # Verify packet
    if header != 0xAA or footer != 0x55:
        return None
    
    # Verify checksum
    calc = 0
    for i in range(31):
        calc ^= data[i]
    if calc != checksum:
        return None
    
    return {
        'temperature': temp,
        'humidity': hum,
        'voltage': volt,
        'current': current,
        'power': power,
        'leak_detected': leak_det,
        'sequence': seq
    }
```

---