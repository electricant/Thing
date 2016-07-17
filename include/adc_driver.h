/**
 * This driver is responsible for controlling the ADC and providing the various
 * readings to the other modules.
 *
 * It is based on prrevious work by Atmel Corporation (http://www.atmel.com)
 * and it has been adapted to suit this application
 *
 * Copyright (C) 2008 Atmel Corporation <avr@atmel.com>
 * Copyright (C) 2016 Paolo Scaramuzza <paolo.scaramuzza@ipol.gq>
 */
#ifndef ADC_DRIVER_H
#define ADC_DRIVER_H
#include "board.h"
#include "avr_compiler.h"

/* Defines */

// settling time in clock cycles for the ADC
#define COMMON_MODE_CYCLES 16
// gain used to meaure currents
#define CURRENT_GAIN ADC_CH_GAIN_4X_gc
// gain used to measure angles
#define ANGLE_GAIN ADC_CH_GAIN_1X_gc
// gain used to measure the battery voltage
#define BATTERY_GAIN ADC_CH_GAIN_1X_gc
// offset on the current measurement (mA)
#define CURRENT_OFFSET 1
// offset on the angle measurement (degrees)
#define ANGLE_OFFSET   32

/* Macros */

/*! \brief This macro enables the selected adc.
 *
 *  Before the ADC is enabled the first time the function
 *  ADC_CalibrationValues_Set should be used to reduce the gain error in the
 *  ADC.
 *
 *  \note After the ADC is enabled the commen mode voltage in the ADC is ready
 *        after 12 ADC clock cycels. Do one dummy conversion or wait the required
 *        number of clock cycles to reasure correct conversion.
 *
 *  \param  _adc          Pointer to ADC module register section.
 */
#define ADC_Enable(_adc) ((_adc)->CTRLA |= ADC_ENABLE_bm)

/*! \brief This macro disables the selected adc.
 *
 *  \param  _adc  Pointer to ADC module register section
 */
#define ADC_Disable(_adc) ((_adc)->CTRLA = (_adc)->CTRLA & (~ADC_ENABLE_bm))

/*! \brief This macro flushes the pipline in the selected adc.
 *
 *  \param  _adc  Pointer to ADC module register section
 */
#define ADC_Pipeline_Flush(_adc) ((_adc)->CTRLA |= ADC_FLUSH_bm)


/*! \brief This macro set the conversion mode and resolution in the selected adc.
 *
 *  This macro configures the conversion mode to signed or unsigned and set
 *  the resolution in and the way the results are put in the result
 *  registers.
 *
 *  \param  _adc          Pointer to ADC module register section
 *  \param  _signedMode   Selects conversion mode: signed (true)
 *                        or unsigned (false). USE bool type.
 *  \param  _resolution   Resolution and presentation selection.
 *                        Use ADC_RESOLUTION_t type.
 */

#define ADC_ConvMode_and_Resolution_Config(_adc, _signedMode, _resolution)     \
	((_adc)->CTRLB = ((_adc)->CTRLB & (~(ADC_RESOLUTION_gm|ADC_CONMODE_bm)))|  \
		(_resolution| ( _signedMode? ADC_CONMODE_bm : 0)))

/*! \brief Helper macro for readability for ADC_ConvMode_and_Resolution_Config
 *
 *  \sa  ADC_ConvMode_and_Resolution_Config
 */
#define ADC_ConvMode_Signed true

/*! \brief Helper macro for readability for ADC_ConvMode_and_Resolution_Config
 *
 *  \sa  ADC_ConvMode_and_Resolution_Config
 */
#define ADC_ConvMode_Unsigned false


/*! \brief This macro set the prescaler factor in the selected adc.
 *
 *  This macro configures the division factor between the XMEGA
 *  IO-clock and the ADC clock. Given a certain IO-clock, the prescaler
 *  must be configured so the the ADC clock is within recommended limits.
 *  A faster IO-clock required higher division factors.
 *
 *  \note  The maximum ADC sample rate is always one fourth of the IO clock.
 *
 *  \param  _adc  Pointer to ADC module register section.
 *  \param  _div  ADC prescaler division factor setting. Use ADC_PRESCALER_t type
 */
#define ADC_Prescaler_Config(_adc, _div)                                       \
	((_adc)->PRESCALER = ((_adc)->PRESCALER & (~ADC_PRESCALER_gm)) | _div)


/*! \brief This macro set the conversion reference in the selected adc.
 *
 *  \param  _adc      Pointer to ADC module register section.
 *  \param  _convRef  Selects reference voltage for all conversions.
 *                    Use ADC_REFSEL_t type.
 */
#define ADC_Reference_Config(_adc, _convRef)                                   \
	((_adc)->REFCTRL = ((_adc)->REFCTRL & ~(ADC_REFSEL_gm)) | _convRef)


/*! \brief This macro sets the sweep channel settings.
 *
 *  \param  _adc            Pointer to ADC module register section.
 *  \param  _sweepChannels  Sweep channel selection. Use ADC_SWEEP_t type
 */
#define ADC_SweepChannels_Config(_adc, _sweepChannels)                         \
	((_adc)->EVCTRL = ((_adc)->EVCTRL & (~ADC_SWEEP_gm)) | _sweepChannels)


/*! \brief This macro configures the event channels used and the event mode.
 *
 *  This macro configures the way events are used to trigger conversions for
 *  the virtual channels. Use the eventChannels parameter to select which event
 *  channel to associate with virtual channel 0 or to trigger a conversion sweep,
 *  depending on the selected eventMode parameter.
 *
 *  \param  _adc            Pointer to ADC module register section.
 *  \param  _eventChannels  The first event channel to be used for triggering.
 *                          Use ADC_EVSEL_t type.
 *  \param  _eventMode      Select event trigger mode.
 *                          Use ADC_EVACT_t type.
 */
#define ADC_Events_Config(_adc, _eventChannels, _eventMode)                \
	(_adc)->EVCTRL = ((_adc)->EVCTRL & (~(ADC_EVSEL_gm | ADC_EVACT_gm))) | \
	                 ((uint8_t) _eventChannels | _eventMode)


/*! \brief  This macro configures the interrupt mode and level for one channel.
 *
 *  The interrupt mode affects the interrupt flag for the virtual channel,
 *  and thus also affects code that polls this flag instead of using interrupts.
 *
 *  \note  When using the result comparator function, the compare value must be
 *         set using the ADC_SetCompareValue function.
 *
 *  \param  _adc_ch          Pointer to ADC channel register section.
 *  \param  _interruptMode   Interrupt mode, flag on complete or above/below
 *                           compare value. Use ADC_CH_INTMODE_t type.
 *  \param  _interruptLevel  Disable or set low/med/high priority for this
 *                           virtual channel. Use ADC_CH_INTLVL_t type.
 */
#define ADC_Ch_Interrupts_Config(_adc_ch, _interruptMode, _interruptLevel)     \
	(_adc_ch)->INTCTRL = (((_adc_ch)->INTCTRL &                            \
	                      (~(ADC_CH_INTMODE_gm | ADC_CH_INTLVL_gm))) |     \
	                      ((uint8_t) _interruptMode | _interruptLevel))


/*! \brief This macro configures the input mode and gain to a specific virtual channel.
 *
 *  \param  _adc_ch         Pointer to ADC channel register section.
 *  \param  _inputMode      Input mode for this channel, differential,
 *                         single-ended, gain etc. Use ADC_CH_INPUTMODE_t type.
 *  \param  _gain           The preamplifiers gain value.
 *                         Use ADC_CH_GAINFAC_t type.
 *
 */
#define ADC_Ch_InputMode_and_Gain_Config(_adc_ch, _inputMode, _gain)           \
	(_adc_ch)->CTRL = ((_adc_ch)->CTRL &                                   \
	                  (~(ADC_CH_INPUTMODE_gm|ADC_CH_GAIN_gm))) |        \
	                  ((uint8_t) _inputMode|_gain)

/*!  \brief This macro configures the Positiv and negativ inputs.
 *
 *  \param  _adc_ch    Which ADC channel to configure.
 *  \param  _posInput  Which pin (or internal signal) to connect to positive
 *                     ADC input. Use ADC_CH_MUXPOS_enum type.
 *  \param  _negInput  Which pin to connect to negative ADC input.
 *                     Use ADC_CH_MUXNEG_t type.
 *
 *  \note  The negative input is connected to GND for single-ended and internal input modes.
 */
#define ADC_Ch_InputMux_Config(_adc_ch, _posInput, _negInput)                  \
	((_adc_ch)->MUXCTRL = (uint8_t) _posInput | _negInput)


/*! \brief This macro returns the channel conversion complete flag..
 *
 *  \param  _adc_ch  Pointer to ADC Channel register section.
 *
 *  \return value of channels conversion complete flag.
 */
#define ADC_Ch_Conversion_Complete(_adc_ch)                                    \
	(((_adc_ch)->INTFLAGS & ADC_CH_CHIF_bm) != 0x00)


/*! \brief This macro sets the value in the ADC compare register.
 *
 *  The value in the ADC compare register is used by the result comparator for
 *  channels that are configured to notify when result is above or below this
 *  value. Even if the ADC compare value register is always left adjusted, the input
 *  to this function is adjusted according to the result presentation setup
 *  for the ADC. This means that the value will be right adjusted unless the
 *  "12-bit left adjust" result mode is selected with
 *  ADC_ConvMode_and_Resolution_Config.
 *
 *  \param  _adc    Pointer to ADC module register section.
 *  \param  _value  12-bit value used by the result comparator. Use uint16_t type.
 */
#define ADC_CompareValue_Set(_adc, _value) ((_adc)->CMP = _value)


/*! \brief This macro enables the Free Running mode in the selected adc.
 *
 *  \param  _adc   Pointer to ADC module register section.
 */
#define ADC_FreeRunning_Enable(_adc)  ((_adc)->CTRLB |= ADC_FREERUN_bm)


/*! \brief This macro disables the Free Running mode in the selected adc.
 *
 *  \param  _adc  Pointer to ADC module register section.
 */
#define ADC_FreeRunning_Disable(_adc)                                          \
	((_adc)->CTRLB = (_adc)->CTRLB & (~ADC_FREERUN_bm))


/*! \brief This macro start one channel conversion
 *
 *  Use the ADC_GetWordResultCh or ADC_GetByteResultCh functions to
 *  retrieve the conversion result. This macro is not to be used
 *  when the ADC is running in free-running mode.
 *
 *  \param  _adc_ch  Pointer to ADC Channel module register section.
 */
#define ADC_Ch_Conversion_Start(_adc_ch) ((_adc_ch)->CTRL |= ADC_CH_START_bm)


/*! \brief This macro start multiple channel conversions
 *
 *  This macro starts a conversion for the channels selected by
 *  the channel mask parameter. Use the bit mask defines for each
 *  channel and combine them into one byte using bitwise OR.
 *  The available masks are ADC_CH0START_bm, ADC_CH1START_bm,
 *  ADC_CH2START_bm and ADC_CH3START_bm.
 *
 *  \param  _adc          Pointer to ADC module register section.
 *  \param  _channelMask  A bitmask selecting which channels to check.
 */
#define ADC_Conversions_Start(_adc, _channelMask)                         \
	(_adc)->CTRLA |= _channelMask &                                   \
	              (ADC_CH0START_bm | ADC_CH1START_bm |                \
	               ADC_CH2START_bm | ADC_CH3START_bm)


/*! \brief This macro pre enables the Bandgap Reference.
 *
 *  \note  If the ADC is enabled the Bandgap Reference is automaticly enabled.
 *
 *  \param  _adc  Pointer to ADC module register section.
 */
#define ADC_BandgapReference_Enable(_adc) ((_adc)->REFCTRL |= ADC_BANDGAP_bm)


/*! \brief This macro disables the pre enabled the Bandgap Reference.
 *
 *  \param  _adc  Pointer to ADC module register section.
 */
#define ADC_BandgapReference_Disable(_adc) ((_adc)->REFCTRL &= ~ADC_BANDGAP_bm)


/*! \brief This macro makes sure that the temperature reference circuitry is enabled.
 *
 *  \note  Enabling the temperature reference automatically enables the bandgap reference.
 *
 *  \param  _adc  Pointer to ADC module register section.
 */
#define ADC_TempReference_Enable(_adc) ((_adc)->REFCTRL |= ADC_TEMPREF_bm)


/*! \brief This macro disables the temperature reference.
 *
 *  \param  _adc  Pointer to ADC module register section.
 */
#define ADC_TempReference_Disable(_adc)                                        \
	((_adc)->REFCTRL = (_adc)->REFCTRL & (~ADC_TEMPREF_bm))

/* Data structures */

struct ADC_Conversion_t {
	uint8_t gain : 4;
	uint8_t muxposPin;
	int16_t result : 12;
};

/* Prototypes for functions. */
void ADC_init();

uint8_t ADC_getServoCurrent(uint8_t servo_num);

uint8_t ADC_getServoAngle(uint8_t servo_num);

uint8_t ADC_getBatteryVoltage();

/*! \brief This function get the calibration data from the production calibration.
 *
 *  The calibration data is loaded from flash and stored in the calibration
 *  register. The calibration data reduces the non-linearity error in the adc.
 *
 *  \param  adc          Pointer to ADC module register section.
 */
void ADC_loadCalibrationValues(ADC_t * adc);

/*! \brief This function waits until the adc common mode is settled.
 *
 *  After the ADC clock has been turned on, the common mode voltage in the ADC
 *  need some time to settle. The time it takes equals one dummy conversion.
 *  Instead of doing a dummy conversion this function waits until the common
 *  mode is settled.
 *
 *  \note The function sets the prescaler to the minimum value possible when the
 *        clock speed is larger than 8 MHz to minimize the time it takes the
 *        common mode to settle.
 *
 *  \note The ADC clock is turned off every time the ADC is disabled or the
 *        device goes into sleep (not Idle sleep mode).
 *
 *  \param  adc Pointer to ADC module register section.
 */
void ADC_waitSettle(ADC_t * adc);

/* Offset addresses for production signature row on GCC */
#ifndef ADCACAL0_offset

#define ADCACAL0_offset 0x20
#define ADCACAL1_offset 0x21
#define ADCBCAL0_offset 0x24
#define ADCBCAL1_offset 0x25

#endif
#endif
