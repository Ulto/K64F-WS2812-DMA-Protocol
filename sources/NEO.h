/*
 * EOS.h
 *
 *  Created on: Mar 22, 2017
 *      Author: Greg
 */

#ifndef SOURCES_NEO_H_
#define SOURCES_NEO_H_

#include "board.h"
#include "fsl_ftm.h"
#include "fsl_edma.h"
#include "fsl_dmamux.h"
#include <fsl_port.h>

#include "Color.h"


/*******************************************************************************
 * Definitions
 ******************************************************************************/
/* Get source clock for FTM driver */
#define FTM_SOURCE_CLOCK CLOCK_GetFreq(kCLOCK_BusClk)

#define NOF_FTM_CHANNELS  3 		/* using three FTM0 channels, running with 60 MHz system clock */
#define FTM_CH0_TICKS     (0x10)  	/* delay until 0xFF */
#define FTM_CH1_TICKS     (0x2A)  	/* at 0.4us write data */
#define FTM_CH2_TICKS     (0x40)  	/* at 0.8us clear bits  */
#define FTM_PERIOD_TICKS  (0x4B)  	/* 1.25 us period */

#define NEO_NOF_PIXEL		150
#define NEO_NOF_BITS_PIXEL 	24

#define PIN0_IDX						 0u
#define PIN1_IDX                         1u   /*!< Pin number for pin 1 in a port */
#define PIN2_IDX                         2u   /*!< Pin number for pin 2 in a port */
#define PIN3_IDX                         3u   /*!< Pin number for pin 3 in a port */
#define PIN4_IDX                         4u   /*!< Pin number for pin 4 in a port */
#define PIN5_IDX                         5u   /*!< Pin number for pin 5 in a port */
#define PIN6_IDX                         6u   /*!< Pin number for pin 6 in a port */
#define PIN7_IDX                         7u   /*!< Pin number for pin 7 in a port */


/*******************************************************************************
 * Prototypes
 ******************************************************************************/

void StartStopFTM(FTM_Type *base, bool startIt);

void ResetFTM(FTM_Type *base);

void initBuffer(void);

void delay(uint32_t ms);


void colorWipe(uint8_t strip_N, Color c, uint32_t SpeedDelay);


void setPixel(uint8_t strip_N, uint8_t Pixel, Color c);


#endif /* SOURCES_NEO_H_ */
