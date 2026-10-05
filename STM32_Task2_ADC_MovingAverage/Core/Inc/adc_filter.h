#ifndef ADC_FILTER_H
#define ADC_FILTER_H

#include <stdint.h>
#include <stdbool.h>

/*
 * Number of ADC samples used to calculate
 * the moving average.
 */
#define MOVING_AVG_WINDOW 10U

/*
 * Initialize the moving-average filter.
 */
void MovingAverage_Init(void);

/*
 * Add a new ADC sample to the filter.

 * Parameters:
 *   sample  - New ADC reading.
 *   average - Pointer where calculated average is stored.

 * Return:
 *   true  - A valid 10-sample average is available.
 *   false - The buffer is not yet full or parameter is invalid.
 */
bool MovingAverage_AddSample(uint16_t sample,
                             uint16_t *average);

#endif /* ADC_FILTER_H */
