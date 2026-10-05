#include "adc_filter.h"
#include <stddef.h>
/*
 * Circular buffer containing the latest
 * MOVING_AVG_WINDOW ADC samples.
 */
static uint16_t sample_buffer[MOVING_AVG_WINDOW];

/*
 * Running sum of all samples currently
 * stored in the buffer.
 *
 * uint32_t is used because:
 *
 * Maximum ADC value = 4095
 * Maximum sum = 4095 * 10 = 40950
 */
static uint32_t sample_sum = 0U;

/*
 * Index of the oldest sample in the circular buffer.
 */
static uint8_t buffer_index = 0U;

/*
 * Number of valid samples currently stored.
 */
static uint8_t sample_count = 0U;


/*
 * Initialize the moving-average filter.
 */
void MovingAverage_Init(void)
{
    sample_sum = 0U;
    buffer_index = 0U;
    sample_count = 0U;

    /*
     * Clear the sample buffer.
     */
    for (uint8_t i = 0U; i < MOVING_AVG_WINDOW; i++)
    {
        sample_buffer[i] = 0U;
    }
}


/*
 * Add one new ADC sample and calculate
 * the average of the latest 10 samples.
 */
bool MovingAverage_AddSample(uint16_t sample,
                             uint16_t *average)
{
    /*
     * Defensive programming:
     * Make sure the output pointer is valid.
     */
    if (average == NULL)
    {
        return false;
    }


    /*
     * First 10 samples:
     *
     * The buffer is not full yet, so simply
     * add each sample to the running sum.
     */
    if (sample_count < MOVING_AVG_WINDOW)
    {
        sample_buffer[buffer_index] = sample;

        sample_sum += sample;

        sample_count++;

        /*
         * Move to the next buffer position.
         */
        buffer_index++;

        if (buffer_index >= MOVING_AVG_WINDOW)
        {
            buffer_index = 0U;
        }


        /*
         * Do not calculate a 10-sample moving
         * average until all 10 samples exist.
         */
        if (sample_count < MOVING_AVG_WINDOW)
        {
            return false;
        }
    }
    else
    {
        /*
         * Buffer is already full.
         *
         * Remove the oldest sample from the sum.
         */
        sample_sum -= sample_buffer[buffer_index];


        /*
         * Add the newest sample.
         */
        sample_sum += sample;


        /*
         * Replace the oldest sample with
         * the newest sample.
         */
        sample_buffer[buffer_index] = sample;


        /*
         * Move circular-buffer index.
         */
        buffer_index++;

        if (buffer_index >= MOVING_AVG_WINDOW)
        {
            buffer_index = 0U;
        }
    }


    /*
     * Calculate the average.
     *
     * Example:
     *
     * 1000 + 1010 + ... + 990
     * -------------------------
     *             10
     */
    *average = (uint16_t)(sample_sum / MOVING_AVG_WINDOW);


    return true;
}
