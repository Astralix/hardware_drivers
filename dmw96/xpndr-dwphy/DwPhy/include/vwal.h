////////////////////////////////////////////////////////////////////////////////
///	@file		lwam.c
///	@brief		TODO	
///
/// Applications that use the VWAL must include this header file. WifiApi.h is
/// included by this file because VWAL implements the WifiAPI's functionality.
//
///	@version	$Revision: 1.5 $
///	@note
///	@internal
///	@author		$Author: jyoung $
///	@date		$Date: 2007-08-17 19:19:15 $
///	@attention
///			Copyright (C) 2001-2007 DSP GROUP, INC.   All Rights Reserved
////////////////////////////////////////////////////////////////////////////////


#ifndef _VIRTUAL_WIFI_API_LIBRARY_H
#define _VIRTUAL_WIFI_API_LIBRARY_H

#include "WifiApi.h"

/// \brief Initialize the Virtual WifiAPI Library
/// 
/// \retval 1 VWAL Initialized successfully
/// \retval 0 VWAL could not be initialized.
WIFIAPI int VWAL_Init();

/// \brief Shutdown the Virtual WifiAPI Library
WIFIAPI void VWAL_Shutdown();

/// @brief Read a MIBOID value
/// @param[in] device Network Interface Name
/// @param[in] id MIBOID identification number
/// @param[io] buf Buffer where read MIBOID data is stored.
/// @return WIFI_*
WIFIAPI uint32 VWAL_MibRead(void* device, uint16 id, void* buf);

/// @brief Write a MIBOID value
/// @param[in] device Network Interface Name
/// @param[in] id MIBOID identification number
/// @param[io] buf pointer to MIBOID data that is to be written.
/// @return WIFI_*
WIFIAPI uint32 VWAL_MibWrite(void* device, uint16 id, void* buf);

#endif
