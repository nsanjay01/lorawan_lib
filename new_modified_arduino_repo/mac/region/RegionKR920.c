/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2013 Semtech
 ___ _____ _   ___ _  _____ ___  ___  ___ ___
/ __|_   _/_\ / __| |/ / __/ _ \| _ \/ __| __|
\__ \ | |/ _ \ (__| ' <| _| (_) |   / (__| _|
|___/ |_/_/ \_\___|_|\_\_| \___/|_|_\\___|___|
embedded.connectivity.solutions===============

Description: LoRa MAC region KR920 implementation

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis ( Semtech ), Gregory Cristian ( Semtech ) and Daniel Jaeckle ( STACKFORCE )
*/
#include "Commissioning.h"

#ifdef REGION_KR920

#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

// #include "boards/mcu/board.h"
#include "LoRaMac.h"

#include "utilities.h"

#include "Region.h"
#include "RegionCommon.h"
#include "RegionKR920.h"
#include "radio.h"

// Definitions
#define CHANNELS_MASK_SIZE 1

// Global attributes
/*!
 * LoRaMAC channels
 */
static ChannelParams_t Channels[KR920_MAX_NB_CHANNELS];

/*!
 * LoRaMac bands
 */
static Band_t Bands[KR920_MAX_NB_BANDS] =
	{
		KR920_BAND0};

/*!
 * LoRaMac channels mask
 */
extern uint16_t ChannelsMask[6];

/*!
 * LoRaMac channels remaining
 */
extern uint16_t ChannelsMaskRemaining[];

/*!
 * LoRaMac channels default mask
 */
extern uint16_t ChannelsDefaultMask[];

// Static functions
static int8_t GetNextLowerTxDr(int8_t dr, int8_t minDr)
{
	uint8_t nextLowerDr = 0;

	if (dr == minDr)
	{
		nextLowerDr = minDr;
	}
	else
	{
		nextLowerDr = dr - 1;
	}
	return nextLowerDr;
}

static int8_t GetMaxEIRP(uint32_t freq)
{
	if (freq >= 922100000)
	{ // Limit to 14dBm
		return KR920_DEFAULT_MAX_EIRP_HIGH;
	}
	// Limit to 10dBm
	return KR920_DEFAULT_MAX_EIRP_LOW;
}

static uint32_t GetBandwidth(uint32_t drIndex)
{
	switch (BandwidthsKR920[drIndex])
	{
	default:
	case 125000:
		return 0;
	case 250000:
		return 1;
	case 500000:
		return 2;
	}
}

static int8_t LimitTxPower(int8_t txPower, int8_t maxBandTxPower, int8_t datarate, uint16_t *channelsMask)
{
	int8_t txPowerResult = txPower;

	// Limit tx power to the band max
	txPowerResult = T_MAX(txPower, maxBandTxPower);

	return txPowerResult;
}

static bool VerifyTxFreq(uint32_t freq)
{
	uint32_t tmpFreq = freq;

	// Check radio driver support
	if (Radio.CheckRfFrequency(tmpFreq) == false)
	{
		return false;
	}

	// Verify if the frequency is valid. The frequency must be in a specified
	// range and can be set to specific values.
	if ((tmpFreq >= 920900000) && (tmpFreq <= 923300000))
	{
		// Range ok, check for specific value
		tmpFreq -= 920900000;
		if ((tmpFreq % 200000) == 0)
		{
			return true;
		}
	}
	return false;
}

static uint8_t CountNbOfEnabledChannels(bool joined, uint8_t datarate, uint16_t *channelsMask, ChannelParams_t *channels, Band_t *bands, uint8_t *enabledChannels, uint8_t *delayTx)
{
	uint8_t nbEnabledChannels = 0;
	uint8_t delayTransmission = 0;

	for (uint8_t i = 0, k = 0; i < KR920_MAX_NB_CHANNELS; i += 16, k++)
	{
		for (uint8_t j = 0; j < 16; j++)
		{
			if ((channelsMask[k] & (1 << j)) != 0)
			{
				if (channels[i + j].Frequency == 0)
				{ // Check if the channel is enabled
					continue;
				}
				if (joined == false)
				{
					if ((KR920_JOIN_CHANNELS & (1 << j)) == 0)
					{
						continue;
					}
				}
				if (RegionCommonValueInRange(datarate, channels[i + j].DrRange.Fields.Min,
											 channels[i + j].DrRange.Fields.Max) == false)
				{ // Check if the current channel selection supports the given datarate
					continue;
				}
				if (bands[channels[i + j].Band].TimeOff > 0)
				{ // Check if the band is available for transmission
					delayTransmission++;
					continue;
				}
				enabledChannels[nbEnabledChannels++] = i + j;
			}
		}
	}

	*delayTx = delayTransmission;
	return nbEnabledChannels;
}

PhyParam_t RegionKR920GetPhyParam(GetPhyParams_t *getPhy)
{
	PhyParam_t phyParam = {0};

	switch (getPhy->Attribute)
	{
	case PHY_MIN_RX_DR:
	{
		phyParam.Value = KR920_RX_MIN_DATARATE;
		break;
	}
	case PHY_MIN_TX_DR:
	{
		phyParam.Value = KR920_TX_MIN_DATARATE;
		break;
	}
	case PHY_DEF_TX_DR:
	{
		phyParam.Value = KR920_DEFAULT_DATARATE;
		break;
	}
	case PHY_NEXT_LOWER_TX_DR:
	{
		phyParam.Value = GetNextLowerTxDr(getPhy->Datarate, KR920_TX_MIN_DATARATE);
		break;
	}
	case PHY_DEF_TX_POWER:
	{
		phyParam.Value = KR920_DEFAULT_TX_POWER;
		break;
	}
	case PHY_MAX_PAYLOAD:
	{
		phyParam.Value = MaxPayloadOfDatarateKR920[getPhy->Datarate];
		break;
	}
	case PHY_MAX_PAYLOAD_REPEATER:
	{
		phyParam.Value = MaxPayloadOfDatarateRepeaterKR920[getPhy->Datarate];
		break;
	}
	case PHY_DUTY_CYCLE:
	{
		phyParam.Value = KR920_DUTY_CYCLE_ENABLED;
		break;
	}
	case PHY_MAX_RX_WINDOW:
	{
		phyParam.Value = KR920_MAX_RX_WINDOW;
		break;
	}
	case PHY_RECEIVE_DELAY1:
	{
		phyParam.Value = KR920_RECEIVE_DELAY1;
		break;
	}
	case PHY_RECEIVE_DELAY2:
	{
		phyParam.Value = KR920_RECEIVE_DELAY2;
		break;
	}
	case PHY_JOIN_ACCEPT_DELAY1:
	{
		phyParam.Value = KR920_JOIN_ACCEPT_DELAY1;
		break;
	}
	case PHY_JOIN_ACCEPT_DELAY2:
	{
		phyParam.Value = KR920_JOIN_ACCEPT_DELAY2;
		break;
	}
	case PHY_MAX_FCNT_GAP:
	{
		phyParam.Value = KR920_MAX_FCNT_GAP;
		break;
	}
	case PHY_ACK_TIMEOUT:
	{
		phyParam.Value = (KR920_ACKTIMEOUT + randr(-KR920_ACK_TIMEOUT_RND, KR920_ACK_TIMEOUT_RND));
		break;
	}
	case PHY_DEF_DR1_OFFSET:
	{
		phyParam.Value = KR920_DEFAULT_RX1_DR_OFFSET;
		break;
	}
	case PHY_DEF_RX2_FREQUENCY:
	{
		phyParam.Value = KR920_RX_WND_2_FREQ;
		break;
	}
	case PHY_DEF_RX2_DR:
	{
		phyParam.Value = KR920_RX_WND_2_DR;
		break;
	}
	case PHY_CHANNELS_MASK:
	{
		phyParam.ChannelsMask = ChannelsMask;
		break;
	}
	case PHY_CHANNELS_DEFAULT_MASK:
	{
		phyParam.ChannelsMask = ChannelsDefaultMask;
		break;
	}
	case PHY_MAX_NB_CHANNELS:
	{
		phyParam.Value = KR920_MAX_NB_CHANNELS;
		break;
	}
	case PHY_CHANNELS:
	{
		phyParam.Channels = Channels;
		break;
	}
	case PHY_DEF_UPLINK_DWELL_TIME:
	case PHY_DEF_DOWNLINK_DWELL_TIME:
	{
		phyParam.Value = 0;
		break;
	}
	case PHY_DEF_MAX_EIRP:
	{
		// We set the higher maximum EIRP as default value.
		// The reason for this is, that the frequency may
		// change during a channel selection for the next uplink.
		// The value has to be recalculated in the TX configuration.
		phyParam.fValue = KR920_DEFAULT_MAX_EIRP_HIGH;
		break;
	}
	case PHY_DEF_ANTENNA_GAIN:
	{
		phyParam.fValue = KR920_DEFAULT_ANTENNA_GAIN;
		break;
	}
	case PHY_NB_JOIN_TRIALS:
	case PHY_DEF_NB_JOIN_TRIALS:
	{
		phyParam.Value = 48;
		break;
	}
	default:
	{
		break;
	}
	}

	return phyParam;
}

void RegionKR920SetBandTxDone(SetBandTxDoneParams_t *txDone)
{
	RegionCommonSetBandTxDone(txDone->Joined, &Bands[Channels[txDone->Channel].Band], txDone->LastTxDoneTime);
}

void RegionKR920InitDefaults(InitType_t type)
{
	switch (type)
	{
	case INIT_TYPE_INIT:
	{
		// Channels
		Channels[0] = (ChannelParams_t)KR920_LC1;
		Channels[1] = (ChannelParams_t)KR920_LC2;
		Channels[2] = (ChannelParams_t)KR920_LC3;
		Channels[3] = (ChannelParams_t)KR920_LC4;
		Channels[4] = (ChannelParams_t)KR920_LC5;
		Channels[5] = (ChannelParams_t)KR920_LC6;
		Channels[6] = (ChannelParams_t)KR920_LC7;

		// Initialize the channels default mask
		ChannelsDefaultMask[0] = LC(1) + LC(2) + LC(3) + LC(4) + LC(5) + LC(6) + LC(7) + LC(8);
		// Update the channels mask
		RegionCommonChanMaskCopy(ChannelsMask, ChannelsDefaultMask, 1);
		break;
	}
	case INIT_TYPE_RESTORE:
	{
		// Restore channels default mask
		ChannelsMask[0] |= ChannelsDefaultMask[0];
		break;
	}
	case INIT_TYPE_APP_DEFAULTS:
	{
		// Update the channels mask defaults
		RegionCommonChanMaskCopy(ChannelsMask, ChannelsDefaultMask, 1);
		break;
	}
	default:
	{
		break;
	}
	}
}

bool RegionKR920Verify(VerifyParams_t *verify, PhyAttribute_t phyAttribute)
{
	switch (phyAttribute)
	{
	case PHY_TX_DR:
	{
		return RegionCommonValueInRange(verify->DatarateParams.Datarate, KR920_TX_MIN_DATARATE, KR920_TX_MAX_DATARATE);
	}
	case PHY_DEF_TX_DR:
	{
		return RegionCommonValueInRange(verify->DatarateParams.Datarate, DR_0, DR_5);
	}
	case PHY_RX_DR:
	{
		return RegionCommonValueInRange(verify->DatarateParams.Datarate, KR920_RX_MIN_DATARATE, KR920_RX_MAX_DATARATE);
	}
	case PHY_DEF_TX_POWER:
	case PHY_TX_POWER:
	{
		// Remark: switched min and max!
		return RegionCommonValueInRange(verify->TxPower, KR920_MAX_TX_POWER, KR920_MIN_TX_POWER);
	}
	case PHY_DUTY_CYCLE:
	{
		return KR920_DUTY_CYCLE_ENABLED;
	}
	case PHY_NB_JOIN_TRIALS:
	{
		if (verify->NbJoinTrials < 48)
		{
			return false;
		}
		break;
	}
	default:
		return false;
	}
	return true;
}

void RegionKR920ApplyCFList(ApplyCFListParams_t *applyCFList)
{
	ChannelParams_t newChannel;
	ChannelAddParams_t channelAdd;
	ChannelRemoveParams_t channelRemove;

	// Setup default datarate range
	newChannel.DrRange.Value = (DR_5 << 4) | DR_0;

	// Size of the optional CF list
	if (applyCFList->Size != 16)
	{
		return;
	}

	// Last byte is RFU, don't take it into account
	for (uint8_t i = 0, chanIdx = KR920_NUMB_DEFAULT_CHANNELS; chanIdx < KR920_MAX_NB_CHANNELS; i += 3, chanIdx++)
	{
		if (chanIdx < (KR920_NUMB_CHANNELS_CF_LIST + KR920_NUMB_DEFAULT_CHANNELS))
		{
			// Channel frequency
			newChannel.Frequency = (uint32_t)applyCFList->Payload[i];
			newChannel.Frequency |= ((uint32_t)applyCFList->Payload[i + 1] << 8);
			newChannel.Frequency |= ((uint32_t)applyCFList->Payload[i + 2] << 16);
			newChannel.Frequency *= 100;

			// Initialize alternative frequency to 0
			newChannel.Rx1Frequency = 0;
		}
		else
		{
			newChannel.Frequency = 0;
			newChannel.DrRange.Value = 0;
			newChannel.Rx1Frequency = 0;
		}

		if (newChannel.Frequency != 0)
		{
			channelAdd.NewChannel = &newChannel;
			channelAdd.ChannelId = chanIdx;

			// Try to add all channels
			RegionKR920ChannelAdd(&channelAdd);
		}
		else
		{
			channelRemove.ChannelId = chanIdx;

			RegionKR920ChannelsRemove(&channelRemove);
		}
	}
}

bool RegionKR920ChanMaskSet(ChanMaskSetParams_t *chanMaskSet)
{
	switch (chanMaskSet->ChannelsMaskType)
	{
	case CHANNELS_MASK:
	{
		RegionCommonChanMaskCopy(ChannelsMask, chanMaskSet->ChannelsMaskIn, 1);
		break;
	}
	case CHANNELS_DEFAULT_MASK:
	{
		RegionCommonChanMaskCopy(ChannelsDefaultMask, chanMaskSet->ChannelsMaskIn, 1);
		break;
	}
	default:
		return false;
	}
	return true;
}

bool RegionKR920AdrNext(AdrNextParams_t *adrNext, int8_t *drOut, int8_t *txPowOut, uint32_t *adrAckCounter)
{
	bool adrAckReq = false;
	int8_t datarate = adrNext->Datarate;
	int8_t txPower = adrNext->TxPower;
	GetPhyParams_t getPhy;
	PhyParam_t phyParam;

	// Report back the adr ack counter
	*adrAckCounter = adrNext->AdrAckCounter;

	if (adrNext->AdrEnabled == true)
	{
		if (datarate == KR920_TX_MIN_DATARATE)
		{
			*adrAckCounter = 0;
			adrAckReq = false;
		}
		else
		{
			if (adrNext->AdrAckCounter >= KR920_ADR_ACK_LIMIT)
			{
				adrAckReq = true;
				txPower = KR920_MAX_TX_POWER;
			}
			else
			{
				adrAckReq = false;
			}
			if (adrNext->AdrAckCounter >= (KR920_ADR_ACK_LIMIT + KR920_ADR_ACK_DELAY))
			{
				if ((adrNext->AdrAckCounter % KR920_ADR_ACK_DELAY) == 1)
				{
					// Decrease the datarate
					getPhy.Attribute = PHY_NEXT_LOWER_TX_DR;
					getPhy.Datarate = datarate;
					getPhy.UplinkDwellTime = adrNext->UplinkDwellTime;
					phyParam = RegionKR920GetPhyParam(&getPhy);
					datarate = phyParam.Value;

					if (datarate == KR920_TX_MIN_DATARATE)
					{
						// We must set adrAckReq to false as soon as we reach the lowest datarate
						adrAckReq = false;
						if (adrNext->UpdateChanMask == true)
						{
							// Re-enable default channels
							ChannelsMask[0] |= LC(1) + LC(2) + LC(3);
						}
					}
				}
			}
		}
	}

	*drOut = datarate;
	*txPowOut = txPower;
	return adrAckReq;
}

void RegionKR920ComputeRxWindowParameters(int8_t datarate, uint8_t minRxSymbols, uint32_t rxError, RxConfigParams_t *rxConfigParams)
{
	double tSymbol = 0.0;

	// Get the datarate, perform a boundary check
	rxConfigParams->Datarate = T_MIN(datarate, KR920_RX_MAX_DATARATE);
	rxConfigParams->Bandwidth = GetBandwidth(rxConfigParams->Datarate);

	tSymbol = RegionCommonComputeSymbolTimeLoRa(DataratesKR920[rxConfigParams->Datarate], BandwidthsKR920[rxConfigParams->Datarate]);

	RegionCommonComputeRxWindowParameters(tSymbol, minRxSymbols, rxError, RADIO_WAKEUP_TIME, &rxConfigParams->WindowTimeout, &rxConfigParams->WindowOffset);
}

bool RegionKR920RxConfig(RxConfigParams_t *rxConfig, int8_t *datarate)
{
	int8_t dr = rxConfig->Datarate;
	uint8_t maxPayload = 0;
	int8_t phyDr = 0;
	uint32_t frequency = rxConfig->Frequency;

	if (Radio.GetStatus() != RF_IDLE)
	{
		return false;
	}

	if (rxConfig->Window == 0)
	{
		// Apply window 1 frequency
		frequency = Channels[rxConfig->Channel].Frequency;
		// Apply the alternative RX 1 window frequency, if it is available
		if (Channels[rxConfig->Channel].Rx1Frequency != 0)
		{
			frequency = Channels[rxConfig->Channel].Rx1Frequency;
		}
	}

	// Read the physical datarate from the datarates table
	phyDr = DataratesKR920[dr];

	Radio.SetChannel(frequency);

	// Radio configuration
	// Radio.SetRxConfig(MODEM_LORA, rxConfig->Bandwidth, phyDr, 1, 0, 8, rxConfig->WindowTimeout, false, 0, false, 0, 0, true, rxConfig->RxContinuous);
	// RAKwireless symbTimeout changed after tests done by RAKwireless
	Radio.SetRxConfig(MODEM_LORA, rxConfig->Bandwidth, phyDr, 1, 0, 8, 0, false, 0, false, 0, 0, true, rxConfig->RxContinuous);
	maxPayload = MaxPayloadOfDatarateKR920[dr];
	Radio.SetMaxPayloadLength(MODEM_LORA, maxPayload + LORA_MAC_FRMPAYLOAD_OVERHEAD);

	*datarate = (uint8_t)dr;
	return true;
}

bool RegionKR920TxConfig(TxConfigParams_t *txConfig, int8_t *txPower, TimerTime_t *txTimeOnAir)
{
	int8_t phyDr = DataratesKR920[txConfig->Datarate];
	int8_t txPowerLimited = LimitTxPower(txConfig->TxPower, Bands[Channels[txConfig->Channel].Band].TxMaxPower, txConfig->Datarate, ChannelsMask);
	uint32_t bandwidth = GetBandwidth(txConfig->Datarate);
	float maxEIRP = GetMaxEIRP(Channels[txConfig->Channel].Frequency);
	int8_t phyTxPower = 0;

	// Take the minimum between the maxEIRP and txConfig->MaxEirp.
	// The value of txConfig->MaxEirp could have changed during runtime, e.g. due to a MAC command.
	maxEIRP = T_MIN(txConfig->MaxEirp, maxEIRP);

	// Calculate physical TX power
	phyTxPower = RegionCommonComputeTxPower(txPowerLimited, maxEIRP, txConfig->AntennaGain);

	// Setup the radio frequency
	Radio.SetChannel(Channels[txConfig->Channel].Frequency);

	Radio.SetTxConfig(MODEM_LORA, phyTxPower, 0, bandwidth, phyDr, 1, 8, false, true, 0, 0, false, 3000);

	// Setup maximum payload lenght of the radio driver
	Radio.SetMaxPayloadLength(MODEM_LORA, txConfig->PktLen);
	// Get the time-on-air of the next tx frame
	*txTimeOnAir = Radio.TimeOnAir(MODEM_LORA, txConfig->PktLen);

	*txPower = txPowerLimited;
	return true;
}

uint8_t RegionKR920LinkAdrReq(LinkAdrReqParams_t *linkAdrReq, int8_t *drOut, int8_t *txPowOut, uint8_t *nbRepOut, uint8_t *nbBytesParsed)
{
	uint8_t status = 0x07;
	RegionCommonLinkAdrParams_t linkAdrParams;
	uint8_t nextIndex = 0;
	uint8_t bytesProcessed = 0;
	uint16_t chMask = 0;
	GetPhyParams_t getPhy;
	PhyParam_t phyParam;
	RegionCommonLinkAdrReqVerifyParams_t linkAdrVerifyParams;

	while (bytesProcessed < linkAdrReq->PayloadSize)
	{
		// Get ADR request parameters
		nextIndex = RegionCommonParseLinkAdrReq(&(linkAdrReq->Payload[bytesProcessed]), &linkAdrParams);

		if (nextIndex == 0)
			break; // break loop, since no more request has been found

		// Update bytes processed
		bytesProcessed += nextIndex;

		// Revert status, as we only check the last ADR request for the channel mask KO
		status = 0x07;

		// Setup temporary channels mask
		chMask = linkAdrParams.ChMask;

		// Verify channels mask
		if ((linkAdrParams.ChMaskCtrl == 0) && (chMask == 0))
		{
			status &= 0xFE; // Channel mask KO
		}
		else if (((linkAdrParams.ChMaskCtrl >= 1) && (linkAdrParams.ChMaskCtrl <= 5)) ||
				 (linkAdrParams.ChMaskCtrl >= 7))
		{
			// RFU
			status &= 0xFE; // Channel mask KO
		}
		else
		{
			for (uint8_t i = 0; i < KR920_MAX_NB_CHANNELS; i++)
			{
				if (linkAdrParams.ChMaskCtrl == 6)
				{
					if (Channels[i].Frequency != 0)
					{
						chMask |= 1 << i;
					}
				}
				else
				{
					if (((chMask & (1 << i)) != 0) &&
						(Channels[i].Frequency == 0))
					{					// Trying to enable an undefined channel
						status &= 0xFE; // Channel mask KO
					}
				}
			}
		}
	}

	// Get the minimum possible datarate
	getPhy.Attribute = PHY_MIN_TX_DR;
	getPhy.UplinkDwellTime = linkAdrReq->UplinkDwellTime;
	phyParam = RegionKR920GetPhyParam(&getPhy);

	linkAdrVerifyParams.Status = status;
	linkAdrVerifyParams.AdrEnabled = linkAdrReq->AdrEnabled;
	linkAdrVerifyParams.Datarate = linkAdrParams.Datarate;
	linkAdrVerifyParams.TxPower = linkAdrParams.TxPower;
	linkAdrVerifyParams.NbRep = linkAdrParams.NbRep;
	linkAdrVerifyParams.CurrentDatarate = linkAdrReq->CurrentDatarate;
	linkAdrVerifyParams.CurrentTxPower = linkAdrReq->CurrentTxPower;
	linkAdrVerifyParams.CurrentNbRep = linkAdrReq->CurrentNbRep;
	linkAdrVerifyParams.NbChannels = KR920_MAX_NB_CHANNELS;
	linkAdrVerifyParams.ChannelsMask = &chMask;
	linkAdrVerifyParams.MinDatarate = (int8_t)phyParam.Value;
	linkAdrVerifyParams.MaxDatarate = KR920_TX_MAX_DATARATE;
	linkAdrVerifyParams.Channels = Channels;
	linkAdrVerifyParams.MinTxPower = KR920_MIN_TX_POWER;
	linkAdrVerifyParams.MaxTxPower = KR920_MAX_TX_POWER;

	// Verify the parameters and update, if necessary
	status = RegionCommonLinkAdrReqVerifyParams(&linkAdrVerifyParams, &linkAdrParams.Datarate, &linkAdrParams.TxPower, &linkAdrParams.NbRep);

	// Update channelsMask if everything is correct
	if (status == 0x07)
	{
		// Set the channels mask to a default value
		memset(ChannelsMask, 0, sizeof(ChannelsMask));
		// Update the channels mask
		ChannelsMask[0] = chMask;
	}

	// Update status variables
	*drOut = linkAdrParams.Datarate;
	*txPowOut = linkAdrParams.TxPower;
	*nbRepOut = linkAdrParams.NbRep;
	*nbBytesParsed = bytesProcessed;

	return status;
}

uint8_t RegionKR920RxParamSetupReq(RxParamSetupReqParams_t *rxParamSetupReq)
{
	uint8_t status = 0x07;

	// Verify radio frequency
	if (Radio.CheckRfFrequency(rxParamSetupReq->Frequency) == false)
	{
		status &= 0xFE; // Channel frequency KO
	}

	// Verify datarate
	if (RegionCommonValueInRange(rxParamSetupReq->Datarate, KR920_RX_MIN_DATARATE, KR920_RX_MAX_DATARATE) == false)
	{
		status &= 0xFD; // Datarate KO
	}

	// Verify datarate offset
	if (RegionCommonValueInRange(rxParamSetupReq->DrOffset, KR920_MIN_RX1_DR_OFFSET, KR920_MAX_RX1_DR_OFFSET) == false)
	{
		status &= 0xFB; // Rx1DrOffset range KO
	}

	return status;
}

uint8_t RegionKR920NewChannelReq(NewChannelReqParams_t *newChannelReq)
{
	uint8_t status = 0x03;
	ChannelAddParams_t channelAdd;
	ChannelRemoveParams_t channelRemove;

	if (newChannelReq->NewChannel->Frequency == 0)
	{
		channelRemove.ChannelId = newChannelReq->ChannelId;

		// Remove
		if (RegionKR920ChannelsRemove(&channelRemove) == false)
		{
			status &= 0xFC;
		}
	}
	else
	{
		channelAdd.NewChannel = newChannelReq->NewChannel;
		channelAdd.ChannelId = newChannelReq->ChannelId;

		switch (RegionKR920ChannelAdd(&channelAdd))
		{
		case LORAMAC_STATUS_OK:
		{
			break;
		}
		case LORAMAC_STATUS_FREQUENCY_INVALID:
		{
			status &= 0xFE;
			break;
		}
		case LORAMAC_STATUS_DATARATE_INVALID:
		{
			status &= 0xFD;
			break;
		}
		case LORAMAC_STATUS_FREQ_AND_DR_INVALID:
		{
			status &= 0xFC;
			break;
		}
		default:
		{
			status &= 0xFC;
			break;
		}
		}
	}

	return status;
}

int8_t RegionKR920TxParamSetupReq(TxParamSetupReqParams_t *txParamSetupReq)
{
	return -1;
}

uint8_t RegionKR920DlChannelReq(DlChannelReqParams_t *dlChannelReq)
{
	uint8_t status = 0x03;

	// Verify if the frequency is supported
	if (VerifyTxFreq(dlChannelReq->Rx1Frequency) == false)
	{
		status &= 0xFE;
	}

	// Verify if an uplink frequency exists
	if (Channels[dlChannelReq->ChannelId].Frequency == 0)
	{
		status &= 0xFD;
	}

	// Apply Rx1 frequency, if the status is OK
	if (status == 0x03)
	{
		Channels[dlChannelReq->ChannelId].Rx1Frequency = dlChannelReq->Rx1Frequency;
	}

	return status;
}

int8_t RegionKR920AlternateDr(AlternateDrParams_t *alternateDr)
{
	int8_t datarate = 0;

	if ((alternateDr->NbTrials % 48) == 0)
	{
		datarate = DR_0;
	}
	else if ((alternateDr->NbTrials % 32) == 0)
	{
		datarate = DR_1;
	}
	else if ((alternateDr->NbTrials % 24) == 0)
	{
		datarate = DR_2;
	}
	else if ((alternateDr->NbTrials % 16) == 0)
	{
		datarate = DR_3;
	}
	else if ((alternateDr->NbTrials % 8) == 0)
	{
		datarate = DR_4;
	}
	else
	{
		datarate = DR_5;
	}
	return datarate;
}

void RegionKR920CalcBackOff(CalcBackOffParams_t *calcBackOff)
{
	RegionCommonCalcBackOffParams_t calcBackOffParams;

	calcBackOffParams.Channels = Channels;
	calcBackOffParams.Bands = Bands;
	calcBackOffParams.LastTxIsJoinRequest = calcBackOff->LastTxIsJoinRequest;
	calcBackOffParams.Joined = calcBackOff->Joined;
	calcBackOffParams.DutyCycleEnabled = calcBackOff->DutyCycleEnabled;
	calcBackOffParams.Channel = calcBackOff->Channel;
	calcBackOffParams.ElapsedTime = calcBackOff->ElapsedTime;
	calcBackOffParams.TxTimeOnAir = calcBackOff->TxTimeOnAir;

	RegionCommonCalcBackOff(&calcBackOffParams);
}

bool RegionKR920NextChannel(NextChanParams_t *nextChanParams, uint8_t *channel, TimerTime_t *time, TimerTime_t *aggregatedTimeOff)
{
	uint8_t nbEnabledChannels = 0;
	uint8_t delayTx = 0;
	uint8_t enabledChannels[KR920_MAX_NB_CHANNELS] = {0};
	TimerTime_t nextTxDelay = 0;

	if (RegionCommonCountChannels(ChannelsMask, 0, 1) == 0)
	{	// Reactivate default channels
		//ChannelsMask[0] |= LC(1) + LC(2) + LC(3);
	}

	if (nextChanParams->AggrTimeOff <= TimerGetElapsedTime(nextChanParams->LastAggrTx))
	{
		// Reset Aggregated time off
		*aggregatedTimeOff = 0;

		// Update bands Time OFF
		nextTxDelay = RegionCommonUpdateBandTimeOff(nextChanParams->Joined, nextChanParams->DutyCycleEnabled, Bands, KR920_MAX_NB_BANDS);

		// Search how many channels are enabled
		nbEnabledChannels = CountNbOfEnabledChannels(nextChanParams->Joined, nextChanParams->Datarate,
													 ChannelsMask, Channels,
													 Bands, enabledChannels, &delayTx);
	}
	else
	{
		delayTx++;
		nextTxDelay = nextChanParams->AggrTimeOff - TimerGetElapsedTime(nextChanParams->LastAggrTx);
	}

	if (nbEnabledChannels > 0)
	{
		*channel = enabledChannels[randr(0, nbEnabledChannels - 1)];
		*time = 0;

		return true;
#if 0
			uint8_t channelNext = 0;
			
            for (uint8_t i = 0, j = randr(0, nbEnabledChannels - 1); i < KR920_MAX_NB_CHANNELS; i++)
            {
                channelNext = enabledChannels[j];
                j = (j + 1) % nbEnabledChannels;

                // Perform carrier sense for KR920_CARRIER_SENSE_TIME
                // If the channel is free, we can stop the LBT mechanism
                if (Radio.IsChannelFree(MODEM_LORA, Channels[channelNext].Frequency, KR920_RSSI_FREE_TH, KR920_CARRIER_SENSE_TIME) == true)
                {
                    // Free channel found
                    *channel = channelNext;
                    *time = 0;
                    return true;
                }
            }

            return false;
#endif
	}
	else
	{
		if (delayTx > 0)
		{
			// Delay transmission due to AggregatedTimeOff or to a band time off
			*time = nextTxDelay;
			return true;
		}
		// Datarate not supported by any channel, restore defaults
		//ChannelsMask[0] |= LC(1) + LC(2) + LC(3);
		*time = 0;
		return false;
	}
}

LoRaMacStatus_t RegionKR920ChannelAdd(ChannelAddParams_t *channelAdd)
{
	uint8_t band = 0;
	bool drInvalid = false;
	bool freqInvalid = false;
	uint8_t id = channelAdd->ChannelId;

	if (id >= KR920_MAX_NB_CHANNELS)
	{
		return LORAMAC_STATUS_PARAMETER_INVALID;
	}

	// Validate the datarate range
	if (RegionCommonValueInRange(channelAdd->NewChannel->DrRange.Fields.Min, KR920_TX_MIN_DATARATE, KR920_TX_MAX_DATARATE) == false)
	{
		drInvalid = true;
	}
	if (RegionCommonValueInRange(channelAdd->NewChannel->DrRange.Fields.Max, KR920_TX_MIN_DATARATE, KR920_TX_MAX_DATARATE) == false)
	{
		drInvalid = true;
	}
	if (channelAdd->NewChannel->DrRange.Fields.Min > channelAdd->NewChannel->DrRange.Fields.Max)
	{
		drInvalid = true;
	}

	// Default channels don't accept all values
	if (id < KR920_NUMB_DEFAULT_CHANNELS)
	{
		// All datarates are supported
		// We are not allowed to change the frequency
		if (channelAdd->NewChannel->Frequency != Channels[id].Frequency)
		{
			freqInvalid = true;
		}
	}

	// Check frequency
	if (freqInvalid == false)
	{
		if (VerifyTxFreq(channelAdd->NewChannel->Frequency) == false)
		{
			freqInvalid = true;
		}
	}

	// Check status
	if ((drInvalid == true) && (freqInvalid == true))
	{
		return LORAMAC_STATUS_FREQ_AND_DR_INVALID;
	}
	if (drInvalid == true)
	{
		return LORAMAC_STATUS_DATARATE_INVALID;
	}
	if (freqInvalid == true)
	{
		return LORAMAC_STATUS_FREQUENCY_INVALID;
	}

	memcpy(&(Channels[id]), channelAdd->NewChannel, sizeof(Channels[id]));
	Channels[id].Band = band;
	ChannelsMask[0] |= (1 << id);
	return LORAMAC_STATUS_OK;
}

bool RegionKR920ChannelsRemove(ChannelRemoveParams_t *channelRemove)
{
	uint8_t id = channelRemove->ChannelId;

	if (id < KR920_NUMB_DEFAULT_CHANNELS)
	{
		return false;
	}

	// Remove the channel from the list of channels
	Channels[id] = (ChannelParams_t){0, 0, {0}, 0};

	return RegionCommonChanDisable(ChannelsMask, id, KR920_MAX_NB_CHANNELS);
}

void RegionKR920SetContinuousWave(ContinuousWaveParams_t *continuousWave)
{
	int8_t txPowerLimited = LimitTxPower(continuousWave->TxPower, Bands[Channels[continuousWave->Channel].Band].TxMaxPower, continuousWave->Datarate, ChannelsMask);
	float maxEIRP = GetMaxEIRP(Channels[continuousWave->Channel].Frequency);
	int8_t phyTxPower = 0;
	uint32_t frequency = Channels[continuousWave->Channel].Frequency;

	// Take the minimum between the maxEIRP and continuousWave->MaxEirp.
	// The value of continuousWave->MaxEirp could have changed during runtime, e.g. due to a MAC command.
	maxEIRP = T_MIN(continuousWave->MaxEirp, maxEIRP);

	// Calculate physical TX power
	phyTxPower = RegionCommonComputeTxPower(txPowerLimited, maxEIRP, continuousWave->AntennaGain);

	Radio.SetTxContinuousWave(frequency, phyTxPower, continuousWave->Timeout);
}

uint8_t RegionKR920ApplyDrOffset(uint8_t downlinkDwellTime, int8_t dr, int8_t drOffset)
{
	int8_t datarate = dr - drOffset;

	if (datarate < 0)
	{
		datarate = DR_0;
	}
	return datarate;
}

#endif