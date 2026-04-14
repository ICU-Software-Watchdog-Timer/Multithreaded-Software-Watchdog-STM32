#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "event_groups.h"

#define BIT_MIC_TASK    (1 << 0)
#define BIT_ACCEL_TASK  (1 << 1)
#define BIT_ADC_TASK    (1 << 2)
#define BIT_UART_TASK   (1 << 3)
#define ALL_BITS (BIT_MIC_TASK | BIT_ACCEL_TASK | BIT_ADC_TASK | BIT_UART_TASK)

EventGroupHandle_t xTaskEventGroup;

void MicTask(void *argument) {
    for(;;) {
        HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_12);
        xEventGroupSetBits(xTaskEventGroup, BIT_MIC_TASK);
        osDelay(400);
    }
}

void AccelTask(void *argument) {
    for(;;) {
//         while(1);  // uncomment to simulate failure
        HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_13);
        xEventGroupSetBits(xTaskEventGroup, BIT_ACCEL_TASK);
        osDelay(700);
    }
}

void ADCTask(void *argument) {
    for(;;) {
//    	vTaskDelay(portMAX_DELAY);
        HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_14);
        xEventGroupSetBits(xTaskEventGroup, BIT_ADC_TASK);
        osDelay(1100);
    }
}

void UARTTask(void *argument) {
    for(;;) {
        HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_15);
        xEventGroupSetBits(xTaskEventGroup, BIT_UART_TASK);
        osDelay(1600);
    }
}

void WatchdogTask(void *argument) {
    EventBits_t bits;
    osDelay(2000);
    for(;;) {
        xEventGroupClearBits(xTaskEventGroup, ALL_BITS);
        bits = xEventGroupWaitBits(xTaskEventGroup,
                                   ALL_BITS, pdTRUE, pdTRUE,
                                   pdMS_TO_TICKS(2000));
        if ((bits & ALL_BITS) != ALL_BITS) {
            uint16_t failedPins = 0;
            if (!(bits & BIT_MIC_TASK))   failedPins |= GPIO_PIN_12;
            if (!(bits & BIT_ACCEL_TASK)) failedPins |= GPIO_PIN_13;
            if (!(bits & BIT_ADC_TASK))   failedPins |= GPIO_PIN_14;
            if (!(bits & BIT_UART_TASK))  failedPins |= GPIO_PIN_15;
            for(int i = 0; i < 10; i++) {
                HAL_GPIO_WritePin(GPIOD, failedPins, GPIO_PIN_SET);
                osDelay(100);
                HAL_GPIO_WritePin(GPIOD, failedPins, GPIO_PIN_RESET);
                osDelay(100);
            }
        }
        osDelay(500);
    }
}

// NO vTaskStartScheduler() here — main.c handles it via osKernelStart()
void MX_FREERTOS_Init(void) {
    xTaskEventGroup = xEventGroupCreate();
    xTaskCreate(MicTask,      "Mic",   128, NULL, 1, NULL);
    xTaskCreate(AccelTask,    "Accel", 128, NULL, 2, NULL);
    xTaskCreate(ADCTask,      "ADC",   128, NULL, 4, NULL);
    xTaskCreate(UARTTask,     "UART",  128, NULL, 3, NULL);
    xTaskCreate(WatchdogTask, "WDG",   256, NULL, 5, NULL);
}
