/************************************************************************************
 * Copyright (C) 2016                                                               *
 * TETCOS, Bangalore. India                                                         *
 *                                                                                  *
 * Tetcos owns the intellectual property rights in the Product and its content.     *
 * The copying, redistribution, reselling or publication of any or all of the       *
 * Product or its content without express prior written consent of Tetcos is        *
 * prohibited. Ownership and / or any other right relating to the software and all  *
 * intellectual property rights therein shall remain at all times with Tetcos.      *
 *                                                                                  *
 * Author:    Shashi Kant Suman                                                     *
 *                                                                                  *
 * ---------------------------------------------------------------------------------*/
#include "main.h"
#include "IEEE802_11_Phy.h"
#include "../BatteryModel/BatteryModel.h"

PHY_TX_STATUS get_radio_state(PIEEE802_11_PHY_VAR phy)
{
	return phy->radio.radioState;
}

static bool isChangeRadioIsPermitted(PIEEE802_11_MAC_VAR mac,
									 PIEEE802_11_PHY_VAR phy,
									 PHY_TX_STATUS newState,
									 NETSIM_ID peerId,
									 UINT64 transmissionId)
{
	PHY_TX_STATUS oldState = phy->radio.radioState;

	if (newState == RX_ON_BUSY && isMacTransmittingState(mac))
		return false;

	if (oldState == RX_ON_IDLE)
		return true; // Radio is idle.

	if (phy->radio.transmissionId == transmissionId)
		return true;

	if (phy->radio.peerId != peerId)
		return false;

	if (phy->radio.transmissionId != transmissionId)
		return false;

	if (oldState == RX_ON_BUSY && newState != RX_ON_IDLE)
		return false;

	if (oldState == TRX_ON_BUSY && newState != RX_ON_IDLE)
		return false;

	return true;
}

bool set_radio_state(NETSIM_ID d,
					 NETSIM_ID in,
					 PHY_TX_STATUS state,
					 NETSIM_ID peerId,
					 UINT64 transmissionId)
{
	PIEEE802_11_PHY_VAR phy = IEEE802_11_PHY(d, in);
	PIEEE802_11_MAC_VAR mac = IEEE802_11_MAC(d, in);
	if (phy->radio.radioState == RX_OFF)
		return false;
	
	if (!isChangeRadioIsPermitted(mac, phy, state, peerId, transmissionId))
		return false;

	ptrBATTERY battery = phy->battery;
	bool isChange = true;
	if (battery)
	{
		isChange = battery_set_mode(battery, state, pstruEventDetails->dEventTime);
	}

	if (isChange)
	{
		phy->radio.radioState = state;
		phy->radio.peerId = peerId;
		phy->radio.transmissionId = transmissionId;
	}
	else
		phy->radio.radioState = RX_OFF;
	return isChange;
}

bool is_radio_idle(PIEEE802_11_PHY_VAR phy)
{
	return (phy->radio.radioState==RX_ON_IDLE);
}
