#include "app_tasks.h"

#include <stdio.h>
#include <string.h>

/*
 * Queue handle used to transfer button events
 * from interrupt/event generation side to UART logger task.
 */
static QueueHandle_t buttonEventQueue = NULL;


/*
 * UART handle is defined in main.c by CubeMX.
 */
extern UART_HandleTypeDef huart2;


/*
 * FreeRTOS task prototypes.
 */
static void EventProducerTask(void *argument);
static void UartLoggerTask(void *argument);


/*
 * Application initialization.
 *
 * Creates:
 *   1. Button event queue
 *   2. Event producer task
 *   3. UART logger task
 */
void App_Tasks_Init(void)
{
    /*
     * Queue can hold 10 ButtonEvent_t structures.
     *
     * The queue provides the synchronization boundary between
     * event generation and UART transmission.
     */
    buttonEventQueue = xQueueCreate(
        10,
        sizeof(ButtonEvent_t)
    );

    /*
     * Defensive error handling:
     * If queue creation fails, stop execution.
     *
     * In a production system, this could instead
     * enter a controlled error/recovery state.
     */
    if (buttonEventQueue == NULL)
    {
        Error_Handler();
    }


    /*
     * Create Event Producer task.
     *
     * This task is responsible for monitoring application-level
     * event processing. The actual GPIO edge detection occurs
     * in the EXTI interrupt.
     */
    if (xTaskCreate(
            EventProducerTask,
            "EventProducer",
            256,
            NULL,
            tskIDLE_PRIORITY + 2,
            NULL) != pdPASS)
    {
        Error_Handler();
    }


    /*
     * Create UART Consumer/Logger task.
     *
     * This task blocks on the queue and wakes only when
     * a button event becomes available.
     */
    if (xTaskCreate(
            UartLoggerTask,
            "UartLogger",
            512,
            NULL,
            tskIDLE_PRIORITY + 1,
            NULL) != pdPASS)
    {
        Error_Handler();
    }
}


/*
 * GPIO EXTI event handler.
 *
 * This function is called from HAL_GPIO_EXTI_Callback().
 *
 * IMPORTANT:
 *
 * This function runs in interrupt context.
 *
 * Therefore:
 *   - Do NOT use printf()
 *   - Do NOT use HAL_UART_Transmit()
 *   - Do NOT use xQueueSend()
 *
 * Instead we use xQueueSendFromISR().
 */
void App_ButtonInterrupt(uint16_t GPIO_Pin)
{
    ButtonEvent_t event;

    BaseType_t higherPriorityTaskWoken = pdFALSE;


    /*
     * Identify which button generated the interrupt.
     */
    if (GPIO_Pin == GPIO_PIN_0)
    {
        event.button_id = 1;
    }
    else if (GPIO_Pin == GPIO_PIN_1)
    {
        event.button_id = 2;
    }
    else
    {
        /*
         * Unknown interrupt source.
         */
        return;
    }


    /*
     * HAL_GetTick() returns the system tick in milliseconds.
     *
     * Example:
     *
     * 12500 means the button event occurred
     * approximately 12.5 seconds after system startup.
     */
    event.timestamp = HAL_GetTick();


    /*
     * Send event to the FreeRTOS queue.
     *
     * xQueueSendFromISR() is specifically designed for
     * interrupt context.
     *
     * The timeout is NOT used because an ISR must never block.
     *
     * If the queue is full, the event is dropped.
     */
    if (buttonEventQueue != NULL)
    {
        if (xQueueSendFromISR(
                buttonEventQueue,
                &event,
                &higherPriorityTaskWoken) != pdPASS)
        {
            /*
             * Queue full.
             *
             * We intentionally do not block here.
             *
             * A production system could increment an
             * overflow counter or set an error flag.
             */
        }
    }


    /*
     * Request a context switch if the UART logger task
     * was waiting for this queue and has higher priority.
     */
    portYIELD_FROM_ISR(higherPriorityTaskWoken);
}


/*
 * Event Producer Task
 *
 * The actual GPIO edge detection occurs through EXTI.
 *
 * This task intentionally does not perform UART operations.
 *
 * It simply remains available as the application-level
 * producer side of the architecture.
 */
static void EventProducerTask(void *argument)
{
    (void)argument;

    for (;;)
    {
        /*
         * EXTI interrupts generate the events.
         *
         * The task does not need to poll the GPIO pins.
         *
         * Sleeping allows other FreeRTOS tasks to execute.
         */
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


/*
 * UART Logger / Consumer Task
 *
 * This is the consumer side of the Producer-Consumer design.
 *
 * It waits efficiently for an event instead of continuously
 * polling the queue.
 */
static void UartLoggerTask(void *argument)
{
    ButtonEvent_t event;

    char jsonMessage[128];

    int messageLength;

    HAL_StatusTypeDef uartStatus;


    (void)argument;


    for (;;)
    {
        /*
         * Block until an event arrives.
         *
         * portMAX_DELAY means the task sleeps efficiently
         * while the queue is empty.
         *
         * This is much better than continuously polling.
         */
        if (xQueueReceive(
                buttonEventQueue,
                &event,
                portMAX_DELAY) == pdPASS)
        {
            /*
             * Convert the event structure into JSON.
             *
             * Example:
             *
             * {"button_id":1,"timestamp":12500}
             */
            messageLength = snprintf(
                jsonMessage,
                sizeof(jsonMessage),
                "{\"button_id\":%u,\"timestamp\":%lu}\r\n",
                event.button_id,
                (unsigned long)event.timestamp
            );


            /*
             * Defensive check:
             *
             * snprintf() returns the number of characters
             * that would have been written.
             */
            if (messageLength < 0)
            {
                continue;
            }


            /*
             * Prevent transmission beyond our buffer.
             */
            if ((size_t)messageLength >= sizeof(jsonMessage))
            {
                messageLength = sizeof(jsonMessage) - 1;
            }


            /*
             * UART transmission is performed here,
             * NOT inside the interrupt.
             *
             * Because this is a FreeRTOS task, blocking
             * UART operation does not block the GPIO ISR.
             */
            uartStatus = HAL_UART_Transmit(
                &huart2,
                (uint8_t *)jsonMessage,
                (uint16_t)messageLength,
                100
            );


            /*
             * Defensive UART error handling.
             */
            if (uartStatus != HAL_OK)
            {
                /*
                 * UART transmission failed.
                 *
                 * For the assessment we simply continue.
                 *
                 * A production implementation could:
                 *   - increment an error counter
                 *   - restart UART
                 *   - report the error
                 */
            }
        }
    }
}
