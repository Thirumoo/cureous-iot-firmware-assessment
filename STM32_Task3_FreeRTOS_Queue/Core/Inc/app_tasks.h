#ifndef APP_TASKS_H
#define APP_TASKS_H

#include "main.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

typedef struct
{
    uint8_t button_id;
    uint32_t timestamp;
} ButtonEvent_t;

void App_Tasks_Init(void);
void App_ButtonInterrupt(uint16_t GPIO_Pin);

#endif
