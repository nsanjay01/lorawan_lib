/*!
 * \file      radio.h
 *
 * \brief     Radio driver API definition
 *
 * \copyright Revised BSD License, see file LICENSE.
 *
 * \code
 *                ______                              _
 *               / _____)             _              | |
 *              ( (____  _____ ____ _| |_ _____  ____| |__
 *               \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 *               _____) ) ____| | | || |_| ____( (___| | | |
 *              (______/|_____)_|_|_| \__)_____)\____)_| |_|
 *              (C)2013-2017 Semtech
 *
 * \endcode
 *
 * \author    Miguel Luis ( Semtech )
 *
 * \author    Gregory Cristian ( Semtech )
 */
#ifndef __RADIO_H__
#define __RADIO_H__

#if defined ARDUINO_RAKWIRELESS_RAK11300
#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>
#define TASK_PRIO_NORMAL 1
#endif


#include "stm32f4xx_hal.h"
#include "sx126x.h"


#ifndef RXTIMEOUT_LORA_MAX
#define RXTIMEOUT_LORA_MAX 0x380
#endif

#define RADIO_TCXO_SETUP_TIME 50 // [ms]

/*!
 * Radio complete Wake-up Time with margin for temperature compensation
 */
#define RADIO_WAKEUP_TIME 3 // [ms]



/*!
 * Radio driver supported modems
 */
typedef enum
{
	MODEM_FSK = 0,
	MODEM_LORA 
} RadioModems_t;

/*!
 * Radio driver internal state machine states definition
 */
typedef enum
{
	RF_IDLE = 0,   //!< The radio is idle
	RF_RX_RUNNING, //!< The radio is in reception state
	RF_TX_RUNNING, //!< The radio is in transmission state
	RF_CAD,		   //!< The radio is doing channel activity detection
} RadioState_t;


/*!
 * Radio hardware and global parameters
 */
typedef struct radio_context_s
{
    SPI_HandleTypeDef  spi;
    // GPIO_TypeDef reset;

    struct
    {
    GPIO_TypeDef *GPIO_PORT;
    uint16_t pin;
    }nss;

    struct 
    {
    GPIO_TypeDef *GPIO_PORT;
    uint16_t pin;
    }busy;

    struct
    {
    GPIO_TypeDef *GPIO_PORT;
    uint16_t pin;
    }reset;

} radio_context_t;





/*!
 * \brief Radio driver callback functions
 */
typedef struct
{
	/*!
     * \brief  Tx Done callback prototype.
     */
	void (*TxDone)(void);
	/*!
     * \brief  Tx Timeout callback prototype.
     */
	void (*TxTimeout)(void);
	/*!
     * \brief Rx Done callback prototype.
     *
     * \param  payload Received buffer pointer
     * \param  size    Received buffer size
     * \param  rssi    RSSI value computed while receiving the frame [dBm]
     * \param  snr     SNR value computed while receiving the frame [dB]
     *                     FSK : N/A ( set to 0 )
     *                     LoRa: SNR value in dB
     */
	void (*RxDone)(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);
	/*!
     * \brief  Rx Timeout callback prototype.
     */
	void (*RxTimeout)(void);
	/*!
     * \brief Rx Error callback prototype.
     */
	void (*RxError)(void);
	/*!
     * \brief Preamble detected callback prototype.
     */
	void (*PreAmpDetect)(void);
	/*!
     * \brief  FHSS Change Channel callback prototype.
     *
     * \param  currentChannel   Index number of the current channel
     */
	void (*FhssChangeChannel)(uint8_t currentChannel);

	/*!
     * \brief CAD Done callback prototype.
     *
     * \param  channelDetected    Channel Activity detected during the CAD
     */
	void (*CadDone)(bool channelActivityDetected);
} RadioEvents_t;

/*!
 * \brief Radio driver definition
 */
struct Radio_s
{
	/*!
     * \brief Initializes the radio
     *
     * \param  events Structure containing the driver callback functions
     */
	void (*Init)(RadioEvents_t *events);
	/*!
     * \brief Re-Initializes the radio after CPU wakeup from deep sleep
     *
     * \param  events Structure containing the driver callback functions
     */
	void (*ReInit)(RadioEvents_t *events);
	/*!
     * Return current radio status
     *
     * \param status Radio status.[RF_IDLE, RF_RX_RUNNING, RF_TX_RUNNING]
     */
	RadioState_t (*GetStatus)(void);
	/*!
     * \brief Configures the radio with the given modem
     *
     * \param  modem Modem to be used [0: FSK, 1: LoRa]
     */
	void (*SetModem)(RadioModems_t modem);
	/*!
     * \brief Sets the channel frequency
     *
     * \param  freq         Channel RF frequency
     */
	void (*SetChannel)(uint32_t freq);
	/*!
     * \brief Checks if the channel is free for the given time
     *
     * \param  modem      Radio modem to be used [0: FSK, 1: LoRa]
     * \param  freq       Channel RF frequency
     * \param  rssiThresh RSSI threshold
     * \param  maxCarrierSenseTime Max time while the RSSI is measured
     *
     * \retval isFree         [true: Channel is free, false: Channel is not free]
     */
	bool (*IsChannelFree)(RadioModems_t modem, uint32_t freq, int16_t rssiThresh, uint32_t maxCarrierSenseTime);
	/*!
     * \brief Generates a 32 bits random value based on the RSSI readings
     *
     * \remark This function sets the radio in LoRa modem mode and disables
     *         all interrupts.
     *         After calling this function either Radio.SetRxConfig or
     *         Radio.SetTxConfig functions must be called.
     *
     * \retval randomValue    32 bits random value
     */
	uint32_t (*Random)(void);
	/*!
     * \brief Sets the reception parameters
     *
     * \param  modem        Radio modem to be used [0: FSK, 1: LoRa]
     * \param  bandwidth    Sets the bandwidth
     *                          FSK : >= 2600 and <= 250000 Hz
     *                          LoRa: [0: 125 kHz, 1: 250 kHz,
     *                                 2: 500 kHz, 3: Reserved]
     * \param  datarate     Sets the Datarate
     *                          FSK : 600..300000 bits/s
     *                          LoRa: [6: 64, 7: 128, 8: 256, 9: 512,
     *                                10: 1024, 11: 2048, 12: 4096  chips]
     * \param  coderate     Sets the coding rate (LoRa only)
     *                          FSK : N/A ( set to 0 )
     *                          LoRa: [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
     * \param  bandwidthAfc Sets the AFC Bandwidth (FSK only)
     *                          FSK : >= 2600 and <= 250000 Hz
     *                          LoRa: N/A ( set to 0 )
     * \param  preambleLen  Sets the Preamble length
     *                          FSK : Number of bytes
     *                          LoRa: Length in symbols (the hardware adds 4 more symbols)
     * \param  symbTimeout  Sets the RxSingle timeout value
     *                          FSK : timeout in number of bytes
     *                          LoRa: timeout in symbols
     * \param  fixLen       Fixed length packets [0: variable, 1: fixed]
     * \param  payloadLen   Sets payload length when fixed length is used
     * \param  crcOn        Enables/Disables the CRC [0: OFF, 1: ON]
     * \param  freqHopOn    Enables disables the intra-packet frequency hopping
     *                          FSK : N/A ( set to 0 )
     *                          LoRa: [0: OFF, 1: ON]
     * \param  hopPeriod    Number of symbols between each hop
     *                          FSK : N/A ( set to 0 )
     *                          LoRa: Number of symbols
     * \param  iqInverted   Inverts IQ signals (LoRa only)
     *                          FSK : N/A ( set to 0 )
     *                          LoRa: [0: not inverted, 1: inverted]
     * \param  rxContinuous Sets the reception in continuous mode
     *                          [false: single mode, true: continuous mode]
     */
	void (*SetRxConfig)(RadioModems_t modem, uint32_t bandwidth,
						uint32_t datarate, uint8_t coderate,
						uint32_t bandwidthAfc, uint16_t preambleLen,
						uint16_t symbTimeout, bool fixLen,
						uint8_t payloadLen,
						bool crcOn, bool freqHopOn, uint8_t hopPeriod,
						bool iqInverted, bool rxContinuous);
	/*!
     * \brief Sets the transmission parameters
     *
     * \param  modem        Radio modem to be used [0: FSK, 1: LoRa]
     * \param  power        Sets the output power [dBm]
     * \param  fdev         Sets the frequency deviation (FSK only)
     *                          FSK : [Hz]
     *                          LoRa: 0
     * \param  bandwidth    Sets the bandwidth (LoRa only)
     *                          FSK : 0
     *                          LoRa: [0: 125 kHz, 1: 250 kHz,
     *                                 2: 500 kHz, 3: Reserved]
     * \param  datarate     Sets the Datarate
     *                          FSK : 600..300000 bits/s
     *                          LoRa: [6: 64, 7: 128, 8: 256, 9: 512,
     *                                10: 1024, 11: 2048, 12: 4096  chips]
     * \param  coderate     Sets the coding rate (LoRa only)
     *                          FSK : N/A ( set to 0 )
     *                          LoRa: [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
     * \param  preambleLen  Sets the preamble length
     *                          FSK : Number of bytes
     *                          LoRa: Length in symbols (the hardware adds 4 more symbols)
     * \param  fixLen       Fixed length packets [0: variable, 1: fixed]
     * \param  crcOn        Enables disables the CRC [0: OFF, 1: ON]
     * \param  freqHopOn    Enables disables the intra-packet frequency hopping
     *                          FSK : N/A ( set to 0 )
     *                          LoRa: [0: OFF, 1: ON]
     * \param  hopPeriod    Number of symbols between each hop
     *                          FSK : N/A ( set to 0 )
     *                          LoRa: Number of symbols
     * \param  iqInverted   Inverts IQ signals (LoRa only)
     *                          FSK : N/A ( set to 0 )
     *                          LoRa: [0: not inverted, 1: inverted]
     * \param  timeout      Transmission timeout [ms]
     */
	void (*SetTxConfig)(RadioModems_t modem, int8_t power, uint32_t fdev,
						uint32_t bandwidth, uint32_t datarate,
						uint8_t coderate, uint16_t preambleLen,
						bool fixLen, bool crcOn, bool freqHopOn,
						uint8_t hopPeriod, bool iqInverted, uint32_t timeout);
	/*!
     * \brief Checks if the given RF frequency is supported by the hardware
     *
     * \param  frequency RF frequency to be checked
     * \retval isSupported [true: supported, false: unsupported]
     */
	bool (*CheckRfFrequency)(uint32_t frequency);
	/*!
     * \brief Computes the packet time on air in ms for the given payload
     *
     * \remark Can only be called once SetRxConfig or SetTxConfig have been called
     *
     * \param  modem      Radio modem to be used [0: FSK, 1: LoRa]
     * \param  pktLen     Packet payload length
     *
     * \retval airTime        Computed airTime (ms) for the given packet payload length
     */
	uint32_t (*TimeOnAir)(RadioModems_t modem, uint8_t pktLen);
	/*!
     * \brief Sends the buffer of size. Prepares the packet to be sent and sets
     *        the radio in transmission
     *
     * \param buffer     Buffer pointer
     * \param size       Buffer size
     */
	void (*Send)(uint8_t *buffer, uint8_t size);
	/*!
     * \brief Sets the radio in sleep mode
     */
	// void (*Sleep)(const void* context, const sx126x_sleep_cfgs_t cfg);
     void (*Sleep)(void);
	/*!
     * \brief Sets the radio in standby mode
     */
	void (*Standby)(void);
	/*!
     * \brief Sets the radio in reception mode for the given time
     * \param  timeout Reception timeout [ms]
     *                     [0: continuous, others timeout]
     */
	void (*Rx)(uint32_t timeout);
	/*!
     * \brief Set Channel Activity Detection parameters
     */
	void (*SetCadParams)(uint8_t cadSymbolNum, uint8_t cadDetPeak, uint8_t cadDetMin, uint8_t cadExitMode, uint32_t cadTimeout);
	/*!
     * \brief Start a Channel Activity Detection
     */
	void (*StartCad)(void);
	/*!
     * \brief Sets the radio in continuous wave transmission mode
     *
     * \param freq       Channel RF frequency
     * \param power      Sets the output power [dBm]
     * \param time       Transmission mode timeout [s]
     */
	void (*SetTxContinuousWave)(uint32_t freq, int8_t power, uint16_t time);
	/*!
     * \brief Reads the current RSSI value
     *
     * \retval rssiValue Current RSSI value in [dBm]
     */
	int16_t (*Rssi)(RadioModems_t modem);
	/*!
     * \brief Writes the radio register at the specified address
     *
     * \param  addr Register address
     * \param  data New register value
     */
	void (*Write)(uint16_t addr, uint8_t data);
	/*!
     * \brief Reads the radio register at the specified address
     *
     * \param  addr Register address
     * \retval data Register value
     */
	uint8_t (*Read)(uint16_t addr);
	/*!
     * \brief Writes multiple radio registers starting at address
     *
     * \param  addr   First Radio register address
     * \param  buffer Buffer containing the new register's values
     * \param  size   Number of registers to be written
     */
	void (*WriteBuffer)(uint16_t addr, uint8_t *buffer, uint8_t size);
	/*!
     * \brief Reads multiple radio registers starting at address
     *
     * \param  addr First Radio register address
     * \param  buffer Buffer where to copy the registers data
     * \param  size Number of registers to be read
     */
	void (*ReadBuffer)(uint16_t addr, uint8_t *buffer, uint8_t size);
	/*!
     * \brief Sets the maximum payload length.
     *
     * \param  modem      Radio modem to be used [0: FSK, 1: LoRa]
     * \param  max        Maximum payload length in bytes
     */
	void (*SetMaxPayloadLength)(RadioModems_t modem, uint8_t max);
	/*!
     * \brief Sets the network to public or private. Updates the sync byte.
     *
     * \remark Applies to LoRa modem only
     *
     * \param  enable if true, it enables a public network
     */
	void (*SetPublicNetwork)(bool enable);
	/*!
	 * \brief Sets a custom Sync-Word. Updates the sync byte.
	 *
	 * \remark ATTENTION, changes the LoRaWAN sync word as well. Use with care.
	 *
	 * \param  syncword 2 byte custom Sync-Word to be used
	 */
	void (*SetCustomSyncWord)(uint16_t syncword);
	/*!
	 * \brief Get Sync-Word.
	 *
	 * \retval  syncword current 2 byte custom Sync-Word
	 */
	uint16_t (*GetSyncWord)(void);
	/*!
	 * \brief Gets the time required for the board plus radio to get out of sleep.[ms]
	 *
	 * \retval time Radio plus board wakeup time in ms.
	 */
	uint32_t (*GetWakeupTime)(void);
	/*!
     * \brief Process radio irq in background task (nRF52 & ESP32)
     */
	void (*BgIrqProcess)(void);
	/*!
     * \brief Process radio irq
     */
	void (*IrqProcess)(void);
	/*
      * \brief Process radio irq after CPU wakeup from deep sleep
     */
	void (*IrqProcessAfterDeepSleep)(void);
	/*
     * The next functions are available only on SX126x radios.
     */
	/*!
     * \brief Sets the radio in reception mode with Max LNA gain for the given time
     *
     * \remark Available on SX126x radios only.
     *
     * \param  timeout Reception timeout [ms]
     *                     [0: continuous, others timeout]
     */
	void (*RxBoosted)(uint32_t timeout);
	/*!
	 * \brief Enforce use of Low Datarate Optimization
	 *
	 * \remark Available on SX126x radios only.
	 *
	 * \param   enforce       True = Enforce Low DR optimization
	 */
	void (*EnforceLowDRopt)(bool enforce);
	/*!
	 * \brief Sets the Rx duty cycle management parameters
	 *
	 * \remark Available on SX126x radios only.
	 *
	 * \param   rxTime        Structure describing reception timeout value
	 * \param   sleepTime     Structure describing sleep timeout value
	 */
	void (*SetRxDutyCycle)(uint32_t rxTime, uint32_t sleepTime);
};

/*!
 * \brief Radio driver
 *
 * \remark This variable is defined and initialized in the specific radio
 *         board implementation
 */
extern const struct Radio_s Radio;

// defined sx126x_pkt_params_lora_t here
extern sx126x_pkt_params_lora_t lora_pkt_params;

// defined sx126x_mod_params_lora_t here
extern sx126x_mod_params_lora_t lora_mod_params;
#endif // __RADIO_H__
