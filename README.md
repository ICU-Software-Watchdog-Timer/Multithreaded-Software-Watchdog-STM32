🏥 Multithreaded Software Watchdog Architecture on STM32 with FreeRTOS

A real-time ICU patient monitoring simulation built on the STM32F4 Discovery board using FreeRTOS — featuring a priority-based multitasking architecture and a software watchdog for fault detection.


📌 Project Overview
This project implements a multithreaded software watchdog on the STM32F4 Discovery board using FreeRTOS. It simulates an ICU patient monitoring system where four sensor tasks run concurrently, each checking in with a central watchdog supervisor. If any task fails or freezes, the watchdog detects it within a bounded deadline and raises a fault alert.
The system is modeled after real ICU requirements, with task priorities reflecting clinical urgency.

🧠 System Architecture
┌────────────────────────────────────────────────────────┐
│                  FreeRTOS Scheduler                    │
│                                                        │
│  P5 ── WatchdogTask  ◄── supervises all tasks          │
│  P4 ── ADCTask       ◄── vitals (heart rate, SpO2)     │
│  P3 ── UARTTask      ◄── data transmission             │
│  P2 ── AccelTask     ◄── fall/seizure detection        │
│  P1 ── MicTask       ◄── breathing/sound monitoring    │
└────────────────────────────────────────────────────────┘
         │
         ▼
   xEventGroupWaitBits()  ← watchdog checks all bits
   Timeout: 3200 ms       ← worst-case deadline (UART full cycle)
Each sensor task:

Toggles an onboard LED (visual heartbeat)
Sets its corresponding event group bit (xEventGroupSetBits)
Delays for its assigned period (osDelay)

The WatchdogTask waits for all four bits to be set within 3200 ms. If any task misses the deadline, the watchdog triggers a fault response (blue LED alert).

⚡ Task Priority & Clinical Justification
PriorityTaskLEDPeriodClinical ReasonP5WatchdogTaskBlue3200ms windowMust always preempt all tasksP4ADCTaskRed1100msVitals (SpO2, HR) — life-criticalP3UARTTaskOrange1600msData must reach central stationP2AccelTaskGreen700msFall/seizure detectionP1MicTaskBlue400msBreathing sounds — slowest signal

Why does ADC (P4) blink slower than Accel (P2)?
Priority and sampling period are independent concepts. Priority determines CPU scheduling when tasks compete. Sampling period reflects clinical need — ADC doesn't require sub-second LED updates for a demo, while Accel's faster blink confirms it's alive.


⏱️ Watchdog Timeout: 3200 ms
The timeout is derived from the slowest task's full cycle:
UART period = 1600ms  →  full cycle = 1600ms × 2 = 3200ms
TaskFull CycleMicTask800 msAccelTask1400 msADCTask2200 msUARTTask3200 ms ← determines timeout
Too short → false alarms on healthy slow tasks
Too long → real failures go undetected
3200 ms → tightest safe deadline

🔧 FreeRTOS Initialization
cvoid MX_FREERTOS_Init(void) {
    xTaskEventGroup = xEventGroupCreate();

    xTaskCreate(MicTask,      "__Mic__",   128, NULL, 1, NULL);
    xTaskCreate(AccelTask,    "__Accel__", 128, NULL, 2, NULL);
    xTaskCreate(UARTTask,     "UART",      128, NULL, 3, NULL);
    xTaskCreate(ADCTask,      "ADC",       128, NULL, 4, NULL);
    xTaskCreate(WatchdogTask, "WDG",       256, NULL, 5, NULL);
}

💻 Core Task Implementations
cvoid MicTask(void *argument) {           // P1 — 400ms
    for(;;) {
        HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_12);
        xEventGroupSetBits(xTaskEventGroup, BIT_MIC_TASK);
        osDelay(400);
    }
}

void AccelTask(void *argument) {         // P2 — 700ms
    for(;;) {
        HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_13);
        xEventGroupSetBits(xTaskEventGroup, BIT_ACCEL_TASK);
        osDelay(700);
    }
}

void ADCTask(void *argument) {           // P4 — 1100ms
    for(;;) {
        HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_14);
        xEventGroupSetBits(xTaskEventGroup, BIT_ADC_TASK);
        osDelay(1100);
    }
}

void UARTTask(void *argument) {          // P3 — 1600ms
    for(;;) {
        HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_15);
        xEventGroupSetBits(xTaskEventGroup, BIT_UART_TASK);
        osDelay(1600);
    }
}
Watchdog Task
cvoid WatchdogTask(void *argument) {      // P5 — supervisor
    for(;;) {
        EventBits_t bits = xEventGroupWaitBits(
            xTaskEventGroup,
            ALL_BITS,
            pdTRUE,           // clear on exit
            pdTRUE,           // wait for ALL bits
            pdMS_TO_TICKS(3200)
        );

        if ((bits & ALL_BITS) == ALL_BITS) {
            // All tasks healthy — normal operation
        } else {
            // Fault detected — trigger alert
            HAL_GPIO_WritePin(FAULT_LED_PORT, FAULT_LED_PIN, GPIO_PIN_SET);
        }
    }
}

🚨 Fault Simulation
To simulate a task failure, replace its body with a blocking call:
c// Option A — infinite loop (causes priority starvation of lower tasks)
while(1);

// Option B — self-suspend (recommended: clean failure without starving others)
vTaskSuspend(NULL);

Note on while(1) vs vTaskSuspend:
Using while(1) in a P3 task starves all P1 and P2 tasks (they never get CPU time). vTaskSuspend(NULL) is cleaner — the task blocks without consuming CPU, so lower-priority tasks continue blinking normally and only the faulty task's LED goes dark.


🛠️ Hardware & Tools
ComponentDetailBoardSTM32F4 Discovery (STM32F407VGTx)RTOSFreeRTOS via STM32CubeIDE CMSIS-RTOS v2IDESTM32CubeIDELanguageC (HAL drivers)Sync primitiveEventGroup (xEventGroupWaitBits)LEDs usedPD12 (Green), PD13 (Orange), PD14 (Red), PD15 (Blue)

📁 Project Structure
├── Core/
│   ├── Src/
│   │   ├── main.c
│   │   ├── freertos.c        ← MX_FREERTOS_Init + all task definitions
│   │   └── stm32f4xx_it.c
│   └── Inc/
│       └── main.h
├── Middlewares/
│   └── Third_Party/FreeRTOS/
└── README.md

🎓 Key Concepts Demonstrated

Preemptive multitasking with FreeRTOS priority scheduling
Software watchdog using xEventGroupWaitBits with timeout
Priority starvation — observable when a mid-priority task loops forever
Worst-case deadline analysis — watchdog timeout = max task cycle time
Clinical priority mapping — RTOS priorities aligned with ICU sensor criticality


👤 Author
Riddhi Neelesh Dabadghao
Anuja Mohan Tiwaskar
RKNEC (Shri Ramdeobaba College of Engineering and Management)
B.E. Biomedical Engineering
