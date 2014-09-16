///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///
/// @file   CRandomPacketGenerator.cpp
///
/// @brief  Contains interface & definition of WIFI Random Packet Generator. (wRPG)
///         It is used for the various tests to create packet with random 
///         variation of attributes.
///

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include "IEEE_802_11_Defs.h"
#include "WifiTestMgr.h"
//#include "WifiTPacketReceiver.h"
#include "CRandomPacketGenerator.h"
#include "WmTypes.h"


/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
/// @var    g_AC0_PhyRates                    
///
/// @brief  Vector of allowed phy rates.                                     
///
//////////////////////////////////////////////////////////////////////////////
#define NUM_PHY_RATES   (sizeof(PhyRatesLUT) / sizeof(PhyRatesLUT[0]))

/*static const uint8 PhyRatesLUT[] =
{
    6, 9, 12, 18, 24, 36, 48, 54
} ;
*/
//RR Change to support B
static const uint8 PhyRatesLUT[] =
{
    1,2,55,6, 9, 11, 12, 18, 24, 36, 48, 54, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x80+2, 0x80+55, 0x80+11 
} ;

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
///
/// @typedef    PayloadHeader 
///
/// @brief      Payload header precedes any payload TXed by this test, and its puropse is 
///             facilitating post-processing.
///
///             -# TSF_Timer - Inserted by MAC H/W - we just leave room for this.  
///             -# count     - Just a count field, counting the packet \#.         
///             -# txRetry   - \# of TX retries used for this packet.                  
///
typedef struct
{
    uint8   id                  ;  //!< Always set to 0x15 to identify packets belonging to the Witch Tests
    uint8   txRetry             ;  //!< \# of TX retries used for this packet.                       
    uint8   type                ;  //!< Announces test boundaries (start/data/end).
    uint8   typeEx              ;  //!< Extended type.
    uint32  acSeqNum            ;  //!< 32-bit sequence number (Little-Endian). For these tests, each AC has a 32-bit sequence counter associated with it. The acSequenceNum is incremented for each packet on that particular AC.
    uint8  TSF_Timer[8]           ;  //!< Inserted by MAC H/W - we just leave room for this.            
    uint32  userDataSize        ;
}
PayloadHeader ;



//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

static const int    MAX_PKT_SIZE           = 1600;

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

CRandomPacketGenerator::CRandomPacketGenerator(void) :
        mInitialized     (true              ) 
{
    memset ( mSourceAddress, 0x00, sizeof(mSourceAddress) ) ;
    memset ( mDestAddress  , 0x00, sizeof(mDestAddress  ) ) ;
    memset ( mBssId        , 0x00, sizeof(mBssId        ) ) ;
    
    mSequenceNum  = 0 ;
    mCount        = 0 ;
    mTid          = 0 ;
    mAIFS         = 0 ;
    meCWmin       = 0 ;
    meCWmax       = 0 ;

    mTestID       = 0 ;

    mBroadcast    = false;

	mPowerLevel   = 0;

	mPayloadType  = VT_FIXED;

    mPhyRateSpec.minRate            = PHY_RATE_6Mbps    ;
    mPhyRateSpec.maxRate            = PHY_RATE_6Mbps    ;
    mPhyRateSpec.variation          = VT_FIXED_AT_MIN   ;

    mPacketSizeSpec.minSize         = TEST_PAYLOAD_MIN_SIZE ;
    mPacketSizeSpec.maxSize         = TEST_PAYLOAD_MIN_SIZE ;
    mPacketSizeSpec.variation       = VT_FIXED_AT_MIN       ;

    mTxRetrySpec.minCount           = MIN_RETRY_COUNT       ;
    mTxRetrySpec.maxCount           = MIN_RETRY_COUNT       ;
    mTxRetrySpec.variation          = VT_FIXED_AT_MIN       ;

} // Default Constructor. //


/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

CRandomPacketGenerator::~CRandomPacketGenerator(void)
{
    //RR OS_CS_Close ( (CritSect *)&mGuard ) ;
} // destructor. //



/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
void CRandomPacketGenerator::setAddresses ( const uint8* sourceAddress ,
                                            const uint8* destAddress   ,
                                            const uint8* bssId         )
{
    memcpy ( mSourceAddress         , // [OUT] -> target.    //
             sourceAddress          , // [IN ] -> source.    //
             sizeof(mSourceAddress) );// [IN ] size.         //

    memcpy ( mDestAddress           , // [OUT] -> target.    //
             destAddress            , // [IN ] -> source.    //
             sizeof(mDestAddress)   );// [IN ] size.         //

    memcpy ( mBssId                 , // [OUT] -> target.    //
             bssId                  , // [IN ] -> source.    //
             sizeof(mBssId)         );// [IN ] size.         //

    return;
} // method CRandomPacketGenerator::setAddresses(). //



/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
void CRandomPacketGenerator::setQoSParms ( const uint8 tid    ,    // [IN ] TID to put in QoS header.
                                           const uint8 aifs   ,    // [IN ] AIFS used in this TID (mapped to AC).
                                           const uint8 eCWmin ,    // [IN ] (log2 of) CWmin used for this TID.
                                           const uint8 eCWmax )    // [IN ] (log2 of) CWmax used for this TID.
{
    mTid    = tid    ;
    mAIFS   = aifs   ;
    meCWmin = eCWmin ;
    meCWmax = eCWmax ;
    return;
} // method CRandomPacketGenerator::setQoSParms(). //



/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
void CRandomPacketGenerator::setTestID    ( const uint8 testID ) 
{
    mTestID = testID ;
    return;
} // method CRandomPacketGenerator::setTestID(). //


/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
void CRandomPacketGenerator::setBroadcast ( const bool  broadcast )
{
    assert((broadcast == true) || (broadcast == false));

    mBroadcast = broadcast;
    
    return;
} // method CRandomPacketGenerator::setBroadcast(). //
                                                                  

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
void CRandomPacketGenerator::setPhyRate ( const ePhyRate          minRate   ,  
                                          const ePhyRate          maxRate   ,  
                                          const eVariationType    variation ) 
{
    assert(minRate <= PHY_RATE_MAX ) ;
    assert(maxRate <= PHY_RATE_MAX ) ;
    assert(minRate <= maxRate      ) ;

    mPhyRateSpec.minRate   = minRate    ;
    mPhyRateSpec.maxRate   = maxRate    ;
    mPhyRateSpec.variation = variation  ;
    return;
} // method CRandomPacketGenerator::setPhyRate(). //



/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
void CRandomPacketGenerator::setPacketSize ( const uint32            minSize   ,  
                                             const uint32            maxSize   ,  
                                             const eVariationType    variation ) 
{
    assert  ( minSize <= TEST_PAYLOAD_MAX_SIZE ) ;
    assert  ( maxSize <= TEST_PAYLOAD_MAX_SIZE ) ;
    assert  ( minSize <= maxSize               ) ;

    mPacketSizeSpec.minSize    = minSize    ;
    mPacketSizeSpec.maxSize    = maxSize    ;
    mPacketSizeSpec.variation  = variation  ;  
    return;
} // method CRandomPacketGenerator::setPacketSize(). //



/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
void CRandomPacketGenerator::setTxRetry    ( const uint32            minCount  ,
                                             const uint32            maxCount  ,
                                             const eVariationType    variation )
{
    assert  ( minCount <= MAX_RETRY_COUNT ) ;
    assert  ( maxCount <= MAX_RETRY_COUNT ) ;
    assert  ( minCount <= maxCount        ) ;

    mTxRetrySpec.minCount      = minCount   ;
    mTxRetrySpec.maxCount      = maxCount   ;
    mTxRetrySpec.variation     = variation  ;
    return;
} // method CRandomPacketGenerator::setPacketSize(). //


/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
void CRandomPacketGenerator::resetVariants ( void ) 
{
    mPhyRateIndex = 0;
    mPacketSize   = 0;
    mTxRetryCount = 0;
} // function CRandomPacketGenerator::resetVariants(). //




/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
void CRandomPacketGenerator::setPowerLevel ( uint8 powerLevel ) 
{
	mPowerLevel = powerLevel;
} // 

void CRandomPacketGenerator::setPayloadType(const ePayloadFillType payloadType		)
{
	mPayloadType = payloadType;
}


/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
// Internal functions ///////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
static void rotateAndShiftAddrToAddr( uint8 out_addr[6], const uint8 in_addr[6]) 
{
    out_addr[0] = (in_addr[5] >> 1) | ( (in_addr[5] << 7) & 0xFF)  ;
    out_addr[1] = (in_addr[4] >> 2) | ( (in_addr[4] << 6) & 0xFF)  ;
    out_addr[2] = (in_addr[3] >> 3) | ( (in_addr[3] << 5) & 0xFF)  ;
    out_addr[3] = (in_addr[2] >> 4) | ( (in_addr[2] << 4) & 0xFF)  ;
    out_addr[4] = (in_addr[1] >> 5) | ( (in_addr[1] << 3) & 0xFF)  ;
    out_addr[5] = (in_addr[0] >> 6) | ( (in_addr[0] << 2) & 0xFF)  ;
}


/*
==============================================================================
SetFrameControl

fc	-	pointer to 2 byte frame control field of 802.11 mac header


proto	-	2 bit version field
type 	-	2 bit type field 	
subtype	- 	4 bit subtype field

toDS/fromDS/moreFrag/retry/pwrMgnt/moreData/wep/order - if zero, bit in fc 
is set off/zero. Otherwise the bit is on/one


==============================================================================
*/
static void SetFrameControl(	uint8 fc[2],	// [IN/OUT]
								uint8 proto,	// [IN]
								uint8 type,		// [IN]
								uint8 subtype,	// [IN]
								uint8 toDS,		// [IN]
								uint8 fromDS,	// [IN]
								uint8 moreFrag,	// [IN]
								uint8 retry,	// [IN]
								uint8 pwrMgnt,	// [IN]
								uint8 moreData,	// [IN]
								uint8 wep,		// [IN]
								uint8 order		// [IN]
							)
{
	assert( fc );
	assert( proto < 4 );
	assert( type < 4 );
	assert( subtype < 16 );

	fc[0]  = 0x03 & proto;
	fc[0] |= 0x0C & (type 	<< 2);
	fc[0] |= 0xF0 & (subtype << 4);


	fc[1]  = (order 	? 1 : 0)	<< 7;
	fc[1] |= (wep   	? 1 : 0)	<< 6;
	fc[1] |= (moreData 	? 1 : 0) 	<< 5;
	fc[1] |= (pwrMgnt 	? 1 : 0)	<< 4;
	fc[1] |= (retry 	? 1 : 0)	<< 3;
	fc[1] |= (moreFrag 	? 1 : 0)	<< 2;
	fc[1] |= (fromDS 	? 1 : 0)	<< 1;
	fc[1] |= (toDS 		? 1 : 0)	<< 0;
}


//////////////////////////////////////////////////////////////////////////////////////////////
/*** 
  @fn               static int setupMacHeader(...) 

  @brief            Constructs an IEEE802.11 MAC header according to parameters and 
                    some simple internal rules.

                    MAC header is constructed on the first 24-32 bytes of the MAC 
                    header. At present, its size is fixed for 26 bytes 
                    (BSS MAC header + QoS header).

                    Parameters include variant parts of the header. The remainder is 
                    determined locally in the function.

  @param[out]       pkt          Array of bytes to contain the constructed MAC header.
  @param[in]        type         Should be Data (use FC_TYPE_DATA \#define...).
  @param[in]        subtype      Data subtype. QoS subtypes need extra 2 bytes in the MAC
                                 header.
  @param[in]        protoVersion Protocol version - the 1st 2 bits of the MAC header.  
  @param[in]        seqNum       Actually, the Sequence field in the MAC header.
  @param[in]        srcAddr      MAC address to be used in the SA field.        
  @param[in]        destAddr     MAC address to be used in the DA field.        
  @param[in]        bssidAddr    MAC address to be used in the BSSID field.     
  
  @return           Size of MAC header used. This indicates where to start putting the payload.

***/
static int setupMacHeader(  uint8   pkt[]           ,
                            uint8   type            , 
                            uint8   subtype         ,
                            uint8   protoVersion    ,
                            uint16  seqNum          ,
                            uint8   srcAddr[6]      ,
                            uint8   destAddr[6]     ,
                            uint8   bssidAddr[6]    ,
                            uint8   TID             ,
                            bool    broadcast   
                         )
{
    uint8   ToDS          ;
    uint8   FromDS        ;
    uint8*  pSA    = NULL ;
    uint8*  pDA    = NULL ;
    uint8*  pBSSID = NULL ;

    if (broadcast == true) 
    {
        //
        // We TX a broadcast/multicast packet as a packet originated from an AP to the BSS.
        // In another place - the retry count should be set to 0, which tells the MAC H/W
        // not to expect an ACK for this packet.
        //
        ToDS    = 0;
        FromDS  = 1;
    } // if this is a broadcast packet then... //
    else
    {
        //
        // Normal non-broadcast packet - we TX from a STA to an AP in a BSS.
        //
        ToDS    = 1;
        FromDS  = 0;
    } // ELSE --> this is a unicast packet.    //

    // 
    // Construct a Frame-Control field of the MAC header (2 bytes).
    //
    SetFrameControl(    pkt           ,  // [OUT] Output field - where the Frame-Control would sit.
                            protoVersion  ,  // [IN ] Protocol version field.
                            type          ,  // [IN ] MSDU type (Control/Management/Data).
                            subtype       ,  // [IN ] MSDU subtype.
                            ToDS          ,  // [IN ] TO-DS indication.
                            FromDS        ,  // [IN ] FROM-DS.
                            0             ,  // [IN ] More flag.
                            0             ,  // [IN ] Retry.
                            0             ,  // [IN ] Power management.
                            0             ,  // [IN ] More data flag.
                            0             ,  // [IN ] WEP protected payload.
                            0                // [IN ] Order flag.
                        );

    // 
    // Duration is just a stub. We're not conducting multiple transfers here.
    //
    uint16   duration ; 
    
    if ( broadcast )    duration = 0    ;
    else                duration = 56   ; // ACK = 40usec @ 6Mbps + SIFS(of 11A) = 56usec 
    
    pkt[2] = (0x000000FF & duration);
    pkt[3] = (0x0000FF00 & duration) >> 8;

    // 
    // Construct MAC addresses.
    //
    if      ( (FromDS == 0) && (ToDS == 0) ) 
    {
        //
        // Packet from and to the BSS itself - doesn't go to DS (Distribution System).
        //
        // Bytes[ 9: 4] = Address1 = DA = Destination Address.
        // Bytes[15:10] = Address2 = SA = Source Address.
        // Bytes[21:16] = Address3 = BSS ID.  
        pDA     = & pkt[MH_ADDRESS1_OFFSET];
        pSA     = & pkt[MH_ADDRESS2_OFFSET];
        pBSSID  = & pkt[MH_ADDRESS3_OFFSET];
    } // if packet is destined to within the BSS (no DS) then... //
    else if ( (FromDS == 0) && (ToDS == 1) ) 
    {
        //
        // Packet is from the BSS (STA) and to the AP for distribution (DS).
        //
        // Bytes[ 9: 4] = Address1 = BSSID.
        // Bytes[15:10] = Address2 = SA.
        // Bytes[21:16] = Address3 = DA.
        pDA     = & pkt[MH_ADDRESS3_OFFSET];
        pSA     = & pkt[MH_ADDRESS2_OFFSET];
        pBSSID  = & pkt[MH_ADDRESS1_OFFSET];
    } // ELSE if packet is from STA to AP (DS) then... //
    else if ( (FromDS == 1) && (ToDS == 0) ) 
    {
        //
        // Packet is from the DS (AP) to the STA.
        //
        // Bytes[ 9: 4] = Address1 = DA.
        // Bytes[15:10] = Address2 = BSSID.
        // Bytes[21:16] = Address3 = SA.
        pDA     = & pkt[MH_ADDRESS1_OFFSET];
        pSA     = & pkt[MH_ADDRESS3_OFFSET];
        pBSSID  = & pkt[MH_ADDRESS2_OFFSET];
    } // ELSE if packet is from AP (DS) to a STA then... //
    else if ( (FromDS == 1) && (ToDS == 1) ) 
    {
        //
        // Packet is from the DS and to the DS (backplane AP's conversing).
        //
        // Bytes[ 9: 4] = Address1 = RA (destination AP).
        // Bytes[15:10] = Address2 = TA (source AP).
        // Bytes[21:16] = Address3 = DA (Destination STA).
        // Bytes[29:24] = Address3 = SA (source STA).
        pDA     = & pkt[MH_ADDRESS1_OFFSET];
        pSA     = & pkt[MH_ADDRESS3_OFFSET];
        pBSSID  = & pkt[MH_ADDRESS2_OFFSET];
    } // ELSE packet is from AP to AP (WDS) ... //
    else
    {
        assert(0);
    }  // ELSE - BAD SHIT!! //

    assert(pDA   );
    assert(pSA   );
    assert(pBSSID);

    //
    // Copy to proper fields.
    //
    memcpy ( pBSSID , bssidAddr, MH_ADDRESS_SIZE ) ;  
    memcpy ( pSA    , srcAddr  , MH_ADDRESS_SIZE ) ;  
    memcpy ( pDA    , destAddr , MH_ADDRESS_SIZE ) ;  

    // 
    // Make sure BSSID and SA do not contain BC/MC bit - LSBit of [0] byte MUST'NT be 1 !!!
    //
    if (broadcast == false) 
    {
        //
        // Not a broadcast - clear LSB of BSSID and SA.
        // (otherwise, we leave unchanged).
        // 
        pkt[ 4] &= ~1; // Not BC/MC in BSSID.
        pkt[10] &= ~1; // Not BC/MC in SA.
    } // if this is not a broadcast packet then (unicast)... //
	else
	{
		// check if the broadcast bit is set
		if ( (pDA[0] & 0x1) != 0x1 )
		{
			pDA[0] |= 0x1;
		} 
	}



    // Sequence & fragmentation number
    uint16  seqFrag = seqNum << 4;

    pkt[22] = (0x000000FF & seqFrag);
    pkt[23] = (0x0000FF00 & seqFrag) >> 8;

    // Minimal MAC header length is 24 bytes.
    int macHeaderLen = 24;

    // ToDS and FromDS are both set
    if ( FromDS && ToDS )
    {
        //
        // In a WDS (2 AP's talking together) - we need the RA of the second AP as well.
        // It is not indtended at the moment, and this code is placed in the slight chance
        // that it would be...
        // 
        macHeaderLen += 6;
        rotateAndShiftAddrToAddr ( & pkt[24], bssidAddr ) ;
    } // if this packet is in a WDS (i.e., 2 AP's conversing together). //
                                   
    if ((type == FC_TYPE_DATA) && (subtype >= 0x08)) // subtype 0x08 and above are used only for QOS... //
    {
        // 
        // Construction of the QoS field.
        // 
        // Byte\#1 Bits[15:08] = TXOP limit / QAP PS Buffer state / TXOP duration required/ queue size... //
        // Byte\#0 Bits[ 3: 0] = TID  (TID <= 7 is for EDCA, and the others for HCCA). 
        //         Bits[    4] = EOSP (End Of Service Period).
        //         Bits[ 6: 5] = ACK policy
        // 
        
        // Byte\#0 setting. //
        pkt[macHeaderLen  ] = TID  ; // TID = 0, EOSP = 0 and ACK policy = 0.

        // Byte\#1 setting. //
        pkt[macHeaderLen+1] = 0x00 ; // TXOP limit etc. //

        macHeaderLen += 2 ;
    } // if this is a QOS packet... //

    return ( macHeaderLen ) ;
}



///////////////////////////////////////////////////////////////////////////////////////////////
/*** 
  @fn               static int setupFrameBody( uint8 frameBody[], int pktSize)

  @brief            Fill body of a MAC  MSDU frame, including additional payload header for 
                    facilitatting post-processing.

                    Payload structure goes as follows:

                    0             1            2            3             4            5            6             7            8                                      
                    +----------------------------------------------------------------------------------------------------------+
                    | ID = 0x15   |     txRetry   |    type    |    typeEx   |                 Per AC Sequence Number                   |
                    |      (1B)      |   (1B)     |    (1B)    |    (1B)     |                       (4B)                         |
                    +----------------------------------------------------------------------------------------------------------+
                    |                                    8 bytes of TSF timer (inserted by MAC H/W).                             |
                    +----------------------------------------------------------------------------------------------------------+
                    |                                                                                                            |
                    |                                         R a n d o m    P a y l o a d                                       |
                    |                                         (unspecified at the moment)                                        |
                    |                                                                                                            |
                    |                                                                                                            |
                    |                                                                                                            |
                    |                                                                                                            |
                    |                                                                                                            |
                    +----------------------------------------------------------------------------------------------------------+

  @note             Payload header is created in this function.

  @param[out]       frameBody    Array of bytes to contain the constructed MAC MSDU payload.
  @param[in]        pktSize      Size of payload to construct. Actual payload size would include 
                                 additional 20 bytes (or so) for payload header. 
  @param[in]        count        Sequence counter to be inserted to payload header.
  @param[in]        txRetry         Number of TX attempts to be used for this packet. (Used during Post Processing)
  @param[in]        type         WITCH test packet type (Start/End/Data).
  @param[in]        testID       Identifies the running test.
  
  @return           Actual size of payload (including payload header).
  
***/


static int setupFrameBody( uint8  frameBody[MAX_PKT_SIZE]    ,   // [OUT] Payload constructed.                                   //
                           uint16 pktSize                    ,   // [IN ] Packet size excluding payload header we're adding.     //
                           uint32 count                      ,   // [IN ] Counter value to be inserted to payload header.        //
                           uint8  txRetry                    ,   // [IN ] \# number of TX attempts to be used for this packet.   //
                           uint8  type                       ,   // [IN ] WITCH packet type (Start/End/Data).                    //
                           uint8  testID                         // [IN ] Test identifier.                                       //
                         )
{
    PayloadHeader       payloadHeader   ;
    uint8*              pPayload        ;
    uint32              payloadSize     ;
    assert( pktSize <= MAX_PKT_SIZE );

    // Set up Witch Payload header
    {
        // Always set this to 0x15 to identify packets belonging to the Witch Tests
        payloadHeader.id            = 0x15     ;   //!< Indicates this is a WITCH packet (matches LLC 1st byte also...).
        payloadHeader.type          = type     ;   //!< WITCH DATA packet (as opposed to Start/End test).
        payloadHeader.typeEx        = testID   ;   //!< Indicates test #1.
        payloadHeader.txRetry       = txRetry  ;
        payloadHeader.acSeqNum      = count    ;
        payloadHeader.userDataSize     = 0x0       ;   // just gibrish data for compatability with post proc
    //  payloadHeader.TSF_Timer = <Inserted by H/W)
    }
    
	//
    // OK, payload-header is ready - copy it to byte aligned payload body.
    //
    memcpy ( frameBody             ,    // -> Target. //
             & payloadHeader       ,    // -> Source. //
             sizeof(payloadHeader) ) ;  // Size.      //

    payloadSize = sizeof(payloadHeader);

	pPayload = & frameBody[sizeof(payloadHeader)] ;

    payloadSize += pktSize ;

    return (payloadSize); 
} // function setupFrameBody(). //





/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
uint32  CRandomPacketGenerator::generateInto 
                         (WmTxBatchPacket*		   pWifiDescriptor ,    // [IN/OUT] -> WIFI API TX descriptor. Packet body must be set 
																			//             to an area allocated by caller!!
                           const void*             testInfo        ,    // [IN    ] - Test Info Structure for post processor
                           const uint32            testInfoSize    )    // [IN]      ] - Size of test info struct
{
    //RRL uint8*  pPacketBuffer = pWifiDescriptor->packet ;    
	uint8*  pPacketBuffer = pWifiDescriptor->pkt;    
    uint32  packetSize    = 0                       ; // Starting from zero.
    
    assert ( pWifiDescriptor ) ;
    assert ( pPacketBuffer   ) ;

	
    //
    // Increment accountancy.
    //
    mSequenceNum = (mSequenceNum + 1) & 0xFFF ; // Increment sequence control field modulo 4096. //            

    // A seperate counter for each AC needs to be tracked and updated
    mCount++;

    //
    // Construct MAC header - mostly static fields.
    //
    uint32 macHeaderSize = setupMacHeader ( pPacketBuffer       ,       // [OUT] -> packet body. 
                                            FC_TYPE_DATA        ,       // [IN ] MAC frame type = DATA.
                                            0,//FC_SUBTYPE_QOS_DATA ,       // [IN ] MAC frame subtype = QoS_DATA.
                                            0                   ,       // [IN ] Protocol version.
                                            mSequenceNum        ,       // [IN ] Sequence control field.
                                            mSourceAddress      ,       // [IN ] source MAC address.
                                            mDestAddress        ,       // [IN ] dest MAC address.
                                            mBssId              ,       // [IN ] BSS ID field.
                                            mTid                ,       // [IN ] TSPEC ID (AC/HCCA stream ID).
                                            mBroadcast          ) ;     // [IN ] Broadcast? (otherwise, unicast).

    packetSize = macHeaderSize; 

	//------------------------------------------------
    //
    // Variation of parameters:
    // ========================
    //
    // Begin with PHY rate.
    //
    switch ( mPhyRateSpec.variation ) 
    {
    case VT_FIXED_AT_MIN:
        mPhyRateIndex = mPhyRateSpec.minRate ;
        break;
    
    case VT_RANDOM      :
        uint8   phyRateIndexSpan ;
         
        phyRateIndexSpan = mPhyRateSpec.maxRate - mPhyRateSpec.minRate ;

        mPhyRateIndex = (rand() % phyRateIndexSpan) + mPhyRateSpec.minRate;
        break;
    
    case VT_ROUND_ROBIN :
        mPhyRateIndex += 1 ;
        if (mPhyRateIndex >= NUM_PHY_RATES) 
        {
            mPhyRateIndex = 0;
        } // if passed the maximal PHY rate index, wrap around... //
        break;

    default:
        //
        // Should not get here in the first place.
        //
        mPhyRateIndex = 0;
    } // switch on PHY rate variation... //
    

    //
    // Packet size is next.
    //
    switch ( mPacketSizeSpec.variation ) 
    {
    case VT_FIXED_AT_MIN:
    {
        mPacketSize = mPacketSizeSpec.minSize ;
    }
    break; // case VT_FIXED_AT_MIN: //
    
    case VT_RANDOM      :
    {
        uint32  sizeSpan = mPacketSizeSpec.maxSize - mPacketSizeSpec.minSize ;

        mPacketSize = (rand() % sizeSpan) + mPacketSizeSpec.minSize; 
    }
    break; // case VT_RANDOM: //
    
    case VT_ROUND_ROBIN :
    {
        mPacketSize += 1 ;
        if (mPacketSize >= TEST_PAYLOAD_MAX_SIZE ) 
        {
            mPacketSize = 0;
        } // if passed the maximal packet size, wrap around... //
    }
    break; // case VT_ROUND_ROBIN: //

    default:
        //
        // Should not get here in the first place.
        //
        mPacketSize = 0;
    } // switch on Packet Size... //
    

    //
    // TX Retry count.
    //
    switch ( mTxRetrySpec.variation ) 
    {
    case VT_FIXED_AT_MIN:
    {
        mTxRetryCount = mTxRetrySpec.minCount;
    } // case VT_FIXED_AT_MIN: //
    break;
    
    case VT_RANDOM      :
    {
        uint32  countSpan = mTxRetrySpec.maxCount - mTxRetrySpec.minCount;

        mTxRetryCount = (rand() % countSpan) + mTxRetrySpec.minCount; 
    } // case VT_RANDOM: //
    break;
    
    case VT_ROUND_ROBIN :
    {
        mTxRetryCount += 1 ;
        if (mTxRetryCount >= MAX_RETRY_COUNT ) 
        {
            mTxRetryCount = 0;
        } // if passed the maximal mTxRetryCount, wrap around... //
    } // case VT_ROUND_ROBIN: //
    break;

    default:
        //
        // Should not get here in the first place.
        //
        mTxRetryCount = 0;
    } // switch on TX Retry count... //


    //------------------------------------------------
    //
    // OK - ready to build packet body.
    //
    uint32  phyRate   = PhyRatesLUT[mPhyRateIndex];
    
    //RRL uint8   witchType = convertWitchPacketTypeToPostProcessor ( witchPacketType ) ;

    //
    // Note that <packetSize> returns the full WIFI packet size, include MAC header and the entire payload (+ our payload header). 
    // At the moment, the payload content is wrapping byte value of 0...255.
    //
    uint32    frameBodySize = setupFrameBody ( & pPacketBuffer[packetSize] , // [OUT] -> 1st byte after MAC header is the start of the payload. //
                                    mPacketSize               , // [IN ] Packet size (not including payload header).               //
                                    mCount                    , // [IN ] Count.                                                    //
                                    mTxRetryCount             , // [IN ] TX Retry count.                                           //
                                    0                 , 		// [IN ] Generating a WITCH packet type as requested.              //
                                    mTestID                     // [IN ] Defined by caller.                                           //
                                 );

	packetSize += frameBodySize ;

	

    uint32    testInfoDest = ((frameBodySize - mPacketSize)+ macHeaderSize);

	//
    // Copy the test info data to the packet payload after the mac and witch header
    memcpy ((pPacketBuffer+testInfoDest), testInfo, testInfoSize);

	uint32    addPayloadDest = (uint32)pPacketBuffer+testInfoDest + testInfoSize;
	uint32	  addPayloadSize = frameBodySize - testInfoSize;

	switch ( mPayloadType ) 
	{
	case VT_FIXED:
	{
		memset ( (void*)addPayloadDest, 0xEE, addPayloadSize ) ;
	} // case VT_FIXED: //
	break;

	case VT_RANDOMFILL      :
	{
		for(int i=0; i<addPayloadSize; i++) {
			pPacketBuffer[testInfoDest + testInfoSize + i] = (uint8_t)(rand()%256);
		}

	} // case VT_RANDOM: //
	break;

	case VT_BYTEINCR :
	{
		for(int i=0; i<addPayloadSize; i++) {
			pPacketBuffer[testInfoDest + testInfoSize + i] = (uint8_t)(i%256);
		}
	} // case VT_BYTEINCR: //
	break;

	default:
		break;
	} // switch mPayloadType //

    //------------------------------------------------
    //
    // Update fields in the WIFI descriptor.
	//RRL pWifiDescriptor->length             = packetSize    ;
    //RRL pWifiDescriptor->phyDataRate        = phyRate       ;

	pWifiDescriptor->packetSize         = packetSize    ;
    pWifiDescriptor->phyRate 	    = phyRate       ;
    pWifiDescriptor->txPowerLevel 	= mPowerLevel	; 

    //------------------------------------------------
    //
    // Broadcast/unicast handling:
    // If unicast   - retry count is taken from field.
    // If broadcast - WE OVERRIDE THE RETRY COUNT TO 0!!
    //
    if (mBroadcast)
    {
        //
        // Broadcast/multicast packet - force retries = 0.
        //
        //RRL pWifiDescriptor->retries = 0             ;
		//pWifiDescriptor->numTransmits = 0             ;
		
    } // if broadcast then... //
    else
    {
        //
        // Unicast packet - set retries to user specification (could also be 0).
        // 
        //RRL pWifiDescriptor->numTransmits = mTxRetryCount ;
    } // ELSE --> this is a unicast... //


/*RRL    //
    // Statically - insert the TSF timestamp offset (macHeaderSize + 8) from the beginning of the packet payload:
    //
    // The WitchPayloadHeader is offset 'macHeaderSize' bytes from the beginning of the packet. 
    // The TSF Timestamp is offset 8 bytes from the beginning of the WitchPayloadHeader.
    //
    pWifiDescriptor->tsfTimestampOffset = macHeaderSize + 8        ; // Tells MAC H/W to insert TSF timer into the payload header... //
    pWifiDescriptor->flags              = WIFI_PACKET_API_80211_TXFLAGS_USE_TSF_TIMESTAMP;
*/
    return ( packetSize ) ;
} // method CRandomPacketGenerator::generateInto(). //


///////////////////////////////////////////////////////////////////////////////////////////////
/*** 
  @fn               ePhyRate CRandomPacketGenerator::convertPhyRateToEnum ( const uint32    numericPhyRate ) const 

  @brief            Converts a numeric phy-rate designation (6,9,...) to enum used here.
                    It is the inverse LUT of PhyRatesLUT[] above.

  @param[in]        numericPhyRate - a number deisgnating one of:
                                     6,9,12,18,24,36,38,54

  @return           Enum of type CRandomPacketGenerator::ePhyRate.
  @retval           CRandomPacketGenerator::PHY_RATE_6Mbps 
  @retval           CRandomPacketGenerator::PHY_RATE_9Mbps 
  @retval           CRandomPacketGenerator::PHY_RATE_12Mbps
  @retval           CRandomPacketGenerator::PHY_RATE_18Mbps
  @retval           CRandomPacketGenerator::PHY_RATE_24Mbps
  @retval           CRandomPacketGenerator::PHY_RATE_36Mbps
  @retval           CRandomPacketGenerator::PHY_RATE_38Mbps
  @retval           CRandomPacketGenerator::PHY_RATE_54Mbps

***/
CRandomPacketGenerator::ePhyRate CRandomPacketGenerator::convertPhyRateToEnum ( const uint32    numericPhyRate ) const 
{
    ePhyRate    phyRate = PHY_RATE_1Mbps;

    switch ( numericPhyRate ) 
    {

	case  1:
		phyRate = PHY_RATE_1Mbps    ;
		break;

	case  2:
		phyRate = PHY_RATE_2Mbps    ;
		break;

	case  55:
		phyRate = PHY_RATE_5d5Mbps    ;
		break;

	case  6:
        phyRate = PHY_RATE_6Mbps    ;
        break;

    case  9:
        phyRate = PHY_RATE_9Mbps    ;
        break;

	case  11:
		phyRate = PHY_RATE_11Mbps    ;
		break;

    case 12:
        phyRate = PHY_RATE_12Mbps   ;
        break;

    case 18:
        phyRate = PHY_RATE_18Mbps   ;
        break;

    case 24:
        phyRate = PHY_RATE_24Mbps   ;
        break;

    case 36:
        phyRate = PHY_RATE_36Mbps   ;
        break;

    case 48:
        phyRate = PHY_RATE_48Mbps   ;
        break;

    case 54:
        phyRate = PHY_RATE_54Mbps   ;
        break;

    case 0x40:
        phyRate = PHY_RATE_MCS_IDX_0   ;
        break;
    case 0x41:
        phyRate = PHY_RATE_MCS_IDX_1   ;
        break;
    case 0x42:
        phyRate = PHY_RATE_MCS_IDX_2   ;
        break;
    case 0x43:
        phyRate = PHY_RATE_MCS_IDX_3   ;
        break;
    case 0x44:
        phyRate = PHY_RATE_MCS_IDX_4   ;
        break;
    case 0x45:
        phyRate = PHY_RATE_MCS_IDX_5   ;
        break;
    case 0x46:
        phyRate = PHY_RATE_MCS_IDX_6   ;
        break;
    case 0x47:
        phyRate = PHY_RATE_MCS_IDX_7   ;
        break;

    case (0x80+2):
        phyRate = PHY_RATE_2Mbps_LONG   ;
        break;
		
    case (0x80+55):
        phyRate = PHY_RATE_5d5Mbps_LONG   ;
        break;

    case (0x80+11):
        phyRate = PHY_RATE_11Mbps_LONG   ;
        break;
		
		
		
	assert(0);
    } // switch on given numeric phy rate //        

    return ( phyRate ) ;
} // method CRandomPacketGenerator::convertPhyRateToEnum(). //

