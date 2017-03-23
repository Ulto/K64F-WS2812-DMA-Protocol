
#include "NEO.h"
#include "Color.h"

/*******************************************************************************
 * Variables
 ******************************************************************************/
volatile bool ftmIsrFlag = false;


/* DMA related */
#define NOF_EDMA_CHANNELS  3 /* using three DMA channels */
//static edma_chn_state_t chnStates[NOF_EDMA_CHANNELS]; /* array of DMA channel states */
static volatile bool dmaDone = false; /* set by DMA complete interrupt on DMA channel 3 */
static const uint8_t OneValue = 0xFF; /* value to clear or set the port bit(s) */

const uint8_t gamma8[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  2,  3,  3,  3,
    3,  3,  3,  4,  4,  4,  4,  4,  4,  5,  5,  5,  5,  6,  6,  6,
    6,  6,  7,  7,  7,  8,  8,  8,  8,  9,  9,  9, 10, 10, 10, 11,
   11, 12, 12, 12, 13, 13, 14, 14, 14, 15, 15, 16, 16, 17, 17, 18,
   18, 19, 19, 20, 21, 21, 22, 22, 23, 23, 24, 25, 25, 26, 27, 27,
   28, 29, 30, 30, 31, 32, 33, 33, 34, 35, 36, 37, 37, 38, 39, 40,
   41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56,
   57, 59, 60, 61, 62, 63, 65, 66, 67, 68, 70, 71, 72, 74, 75, 76,
   78, 79, 81, 82, 84, 85, 87, 88, 90, 91, 93, 95, 96, 98, 99,101,
  103,105,106,108,110,112,113,115,117,119,121,123,125,127,129,131,
  133,135,137,139,141,143,146,148,150,152,154,157,159,161,164,166,
  168,171,173,176,178,181,183,186,188,191,194,196,199,202,204,207,
  210,213,216,219,221,224,227,230,233,236,239,242,246,249,252,255
 };

static uint8_t transmitBuf[NEO_NOF_PIXEL*NEO_NOF_BITS_PIXEL] ={};




/*******************************************************************************
 * Code
 ******************************************************************************/

void EDMA_Callback(edma_handle_t *handle, void *userData, bool transferDone, uint32_t tcds) {
  (void) userData;
  (void) tcds;

  dmaDone = transferDone;
}

void InitDMADriver() {
  edma_config_t edma_config = {0};

  // initialize EDMA
  EDMA_GetDefaultConfig(&edma_config);
  edma_config.enableDebugMode = false;
  edma_config.enableHaltOnError = false;
  edma_config.enableRoundRobinArbitration = true;
  EDMA_Init(DMA0, &edma_config);

  // configure DMAMUX
  DMAMUX_Init(DMAMUX0);
  for (uint32_t c = 0; c < 3; c++) {
    DMAMUX_DisableChannel(DMAMUX0, c);
    DMAMUX_DisablePeriodTrigger(DMAMUX0, c);
    DMAMUX_SetSource(DMAMUX0, c, (kDmaRequestMux0FTM0Channel0 + c) & 0xff);
  }

}
/*
void dma_deinit() {
  for (uint32_t c = 0; c < 3; c++) {
    DMAMUX_DisableChannel(DMAMUX0, c);
    DMAMUX_SetSource(DMAMUX0, c, kDmaRequestMux0Disable & 0xff);
  }

  DMAMUX_Deinit(DMAMUX0);
  EDMA_Deinit(DMA0);
}


*/
static void StopFTMDMA(FTM_Type *instance) {
	uint8_t channel;

	StartStopFTM(instance, false); /* stop FTM timer */
	/* reset all values */
	FTM0->CNT = 0;
	FTM_ClearStatusFlags(instance, kFTM_TimeOverflowFlag ); /* clear timer overflow flag (if any) */

	for(channel = 0; channel < NOF_FTM_CHANNELS; channel++)
		instance->CONTROLS[channel].CnSC |= FTM_CnSC_DMA(0); /* disable DMA request */

  	FTM_ClearStatusFlags(instance, kFTM_Chnl0Flag | kFTM_Chnl1Flag | kFTM_Chnl2Flag); /* clear channel flag */
  	FTM_DisableInterrupts(FTM0, kFTM_Chnl0InterruptEnable | kFTM_Chnl1InterruptEnable | kFTM_Chnl2InterruptEnable); // disable channel interrupt

}

void DMA_Transfer(uint8_t *transmitBuf, uint32_t nofBytes) {
  edma_transfer_config_t transfer[3];
  edma_handle_t handle[3];
  edma_tcd_t __aligned(32) tcd[3];

  //Clear array memory
  memset(&transfer, 0, sizeof(transfer));
  memset(&handle, 0, sizeof(handle));
  memset(&tcd, 0, sizeof(tcd));

  ResetFTM(FTM0);

  for (uint32_t c = 0; c < 3; c++) {
    DMAMUX_EnableChannel(DMAMUX0, c);
  }

  EDMA_CreateHandle(&handle[0], DMA0, 0);
  EDMA_InstallTCDMemory(&handle[0], &tcd[0], 1);
  EDMA_ResetChannel(handle[0].base, handle[0].channel);
  EDMA_SetModulo(DMA0, 0, kEDMA_ModuloDisable, kEDMA_ModuloDisable);
  EDMA_PrepareTransfer(&transfer[0], (void *) &OneValue, 1, (void *) &(GPIOD->PSOR), 1, 1, nofBytes, kEDMA_MemoryToPeripheral);
  transfer[0].srcOffset = 0;
  EDMA_SubmitTransfer(&handle[0], &transfer[0]);

  EDMA_CreateHandle(&handle[1], DMA0, 1);
  EDMA_InstallTCDMemory(&handle[1], &tcd[1], 1);
  EDMA_ResetChannel(handle[1].base, handle[1].channel);
  EDMA_SetModulo(DMA0, 1, kEDMA_ModuloDisable, kEDMA_ModuloDisable);
  EDMA_PrepareTransfer(&transfer[1], (void *) transmitBuf, 1, (void *) &(GPIOD->PDOR), 1, 1, nofBytes, kEDMA_MemoryToPeripheral);
  EDMA_SubmitTransfer(&handle[1], &transfer[1]);

  EDMA_CreateHandle(&handle[2], DMA0, 2);
  EDMA_InstallTCDMemory(&handle[2], &tcd[2], 1);
  EDMA_ResetChannel(handle[2].base, handle[2].channel);
  EDMA_SetModulo(DMA0, 2, kEDMA_ModuloDisable, kEDMA_ModuloDisable);
  EDMA_PrepareTransfer(&transfer[2], (void *) &OneValue, 1, (void *) &(GPIOD->PCOR), 1, 1, nofBytes, kEDMA_MemoryToPeripheral);
  transfer[2].srcOffset = 0;
  EDMA_SetCallback(&handle[2], EDMA_Callback, NULL);
  EDMA_SubmitTransfer(&handle[2], &transfer[2]);

  dmaDone = false;

  EDMA_StartTransfer(&handle[0]);
  EDMA_StartTransfer(&handle[1]);
  EDMA_StartTransfer(&handle[2]);


  FTM_StartTimer(FTM0, kFTM_SystemClock);

  do {} while (!dmaDone);

  /* stop FTM DMA transfers */
  StopFTMDMA(FTM0);

  /* stop DMA channel */
  EDMA_StopTransfer(&handle[0]);
  EDMA_StopTransfer(&handle[1]);
  EDMA_StopTransfer(&handle[2]);

  /* Release EDMA channel request trough DMAMUX, otherwise events might still be latched! */
  DMAMUX_DisableChannel(DMAMUX0, 0);
  DMAMUX_DisableChannel(DMAMUX0, 1);
  DMAMUX_DisableChannel(DMAMUX0, 2);

}


void FTM_initialize(void)
{

	ftm_config_t ftmInfo;
	ftm_chnl_pwm_signal_param_t ftmParam[3];

	// Configure ftm params with frequency 800kHZ
	ftmParam[0].chnlNumber = kFTM_Chnl_0;
	ftmParam[0].level = kFTM_HighTrue;
	ftmParam[0].dutyCyclePercent = 10U;
	ftmParam[0].firstEdgeDelayPercent = 0U;

	ftmParam[1].chnlNumber = kFTM_Chnl_1;
    ftmParam[1].level = kFTM_HighTrue;
    ftmParam[1].dutyCyclePercent = 10U;
    ftmParam[1].firstEdgeDelayPercent = 0U;

    ftmParam[2].chnlNumber = kFTM_Chnl_2;
    ftmParam[2].level = kFTM_HighTrue;
    ftmParam[2].dutyCyclePercent = 10U;
    ftmParam[2].firstEdgeDelayPercent = 0U;


    FTM_GetDefaultConfig(&ftmInfo);

    // Initialize FTM module
    FTM_Init(FTM0, &ftmInfo);

    // Disable Overflow and Fault interrupts
    FTM_DisableInterrupts(FTM0, kFTM_TimeOverflowInterruptEnable | kFTM_FaultInterruptEnable);

    // Clear timer overflow
    FTM_ClearStatusFlags(FTM0, kFTM_TimeOverflowFlag );


    //Enable PWM mode for all channels
    // Setup 3 channels of PWM for 800kHz
    // Clock source is 60MHz
    FTM_SetupPwm(FTM0, ftmParam, 3, kFTM_EdgeAlignedPwm, 800000, FTM_SOURCE_CLOCK);

    // CNTIN is to be set 0
    FTM0->CNTIN = 0;

    // Set the module counters
    FTM_SetChnCountVal(FTM0, 0, FTM_CH0_TICKS);
    FTM_SetChnCountVal(FTM0, 1, FTM_CH1_TICKS);
    FTM_SetChnCountVal(FTM0, 2, FTM_CH2_TICKS);

}

void initBuffer(void)
{
	for(uint32_t i=0;i<NEO_NOF_PIXEL*NEO_NOF_BITS_PIXEL;i++){
		transmitBuf[i] = 0;
	}
}

void ResetFTM(FTM_Type *base)
{
	uint8_t channel;

	base->CNT = 0; /* reset FTM counter */
	FTM_ClearStatusFlags(base, kFTM_TimeOverflowFlag ); /* clear timer overflow flag (if any) */

	for(channel = 0; channel < NOF_FTM_CHANNELS; channel++)
		base->CONTROLS[channel].CnSC |= FTM_CnSC_DMA(1); /* enable DMA request */

	FTM_ClearStatusFlags(base, kFTM_Chnl0Flag | kFTM_Chnl1Flag | kFTM_Chnl2Flag); /* clear channel flag */
	FTM_EnableInterrupts(FTM0, kFTM_Chnl0InterruptEnable | kFTM_Chnl1InterruptEnable | kFTM_Chnl2InterruptEnable); /* enable channel interrupt: need to have both DMA and CHnIE set for DMA transfers! */

}

void init_Hardware(void)
{
	FTM_initialize();

	CLOCK_EnableClock(kCLOCK_PortC);    /* Port C Clock Gate Control: Clock enabled */
	CLOCK_EnableClock(kCLOCK_PortD);	/* Port D Clock Gate Control: Clock enabled */


	PORT_SetPinMux(PORTC, PIN1_IDX, kPORT_MuxAlt4);            	/* PORTC1 (pin B11) is configured as FTM0_CH0 */
	PORT_SetPinMux(PORTC, PIN2_IDX, kPORT_MuxAlt4);            	/* PORTC2 (pin A12) is configured as FTM0_CH1 */
	PORT_SetPinMux(PORTC, PIN3_IDX, kPORT_MuxAlt4);            	/* PORTC2 (pin A12) is configured as FTM0_CH2 */


	// Setup first 8 bits of PORTD for output
	// 8 bits is the limit here as the transmit buffer is uint8_t
	PORT_SetPinMux(PORTD, PIN0_IDX, kPORT_MuxAsGpio);
	PORT_SetPinMux(PORTD, PIN1_IDX, kPORT_MuxAsGpio);
	PORT_SetPinMux(PORTD, PIN2_IDX, kPORT_MuxAsGpio);
	PORT_SetPinMux(PORTD, PIN3_IDX, kPORT_MuxAsGpio);
	PORT_SetPinMux(PORTD, PIN4_IDX, kPORT_MuxAsGpio);
	PORT_SetPinMux(PORTD, PIN5_IDX, kPORT_MuxAsGpio);
	PORT_SetPinMux(PORTD, PIN6_IDX, kPORT_MuxAsGpio);
	PORT_SetPinMux(PORTD, PIN7_IDX, kPORT_MuxAsGpio);


	GPIOD->PSOR |= 0xFF;
	GPIOD->PDDR |= 0xFF;


	InitDMADriver();
}


void StartStopFTM(FTM_Type *base, bool startIt)
{
	if(startIt)
		FTM_StartTimer(base, kFTM_SystemClock);
	else
		FTM_StopTimer(base);

}

void NEO_initialize(void)
{

	init_Hardware();
	ResetFTM(FTM0);
	StartStopFTM(FTM0, true);

}

void colorWipe(uint8_t strip_N, Color c, uint32_t SpeedDelay) {
  for(uint8_t i=0; i<NEO_NOF_PIXEL; i++) {
      setPixel(strip_N, i, c);
      DMA_Transfer(transmitBuf, sizeof(transmitBuf));
      delay(SpeedDelay);
  }

}

/*!brief Delay routine */
void delay(uint32_t ms)
{
  for(uint32_t i=0; i<1000*ms; i++){
      __asm__("nop");
  }
}

void setPixel(uint8_t strip_N, uint8_t Pixel, Color c) {
	RGB cc = packColor(c);
	for(uint32_t j=(Pixel*NEO_NOF_BITS_PIXEL);j<(Pixel*NEO_NOF_BITS_PIXEL)+NEO_NOF_BITS_PIXEL;j++){
		if((cc & 0x01) == 1)
			transmitBuf[j] |= ((cc & 0x1) << strip_N);
		else
			transmitBuf[j] &= ~((0x1) << strip_N);
		cc=cc>>1;
	}
}
