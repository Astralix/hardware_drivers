///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if !defined(__CRandomPacketGenerator_included__)
#    define  __CRandomPacketGenerator_included__
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///
/// @file   CRandomPacketGenerator.h
///
/// @brief  Contains interface & definition of WIFI Random Packet Generator. (wRPG)
///         It is used for the various tests to create packet with random 
///         variation of attributes.
///
///         RPG allows dynamica variation of the following attributes:
///         -# Packet size.
///         -# Payload (includes payload header).
///         -# Phy rate.                           
///         -# Number of TX retries.
///
///         It constructs a full WIFI packet, to be TXed down the WIFI IEEE802.11 
///         packet API, i.e., it includes:
///         -# a 802.11 MAC header (should be 26 bytes for QoS between QSTA and QAP).
///         -# a 802.11 MAC payload.
///            As part of the MAC payload, it logically assigns the 1st 20 bytes (or so)
///            of the payload to a \em \b Payload-Header .
///
///         In order to construct a valid MAC header, the following parameters must be 
///         supplied by the caller:
///         -# Addresses (Source, Destination and AP's (SA, DA, BSSID)).
///         -# Stream ID - this is set in the QoS header field (bytes 24-25 in the MAC header.
///         -# Data subtype - this usually should be QoS_Data, but it could be other types
///                           (Data + ACK, etc).
///
///
/// @note   This is a passive function - it does not take care of the following:
///         -# Throughput (packet rate). This is left to the caller.
///         -# Stream ID (HCCA or EDCA is leftout to the caller).
///

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
///
/// @define     WITCH_TYPE_TEST_DATA, WITCH_TYPE_TEST_START, WITCH_TYPE_TEST_END
///
/// @brief      Identifies type of packets.
///             Randomally generated packets all have the data type.
///
#define WITCH_TYPE_TEST_START	0x01
#define WITCH_TYPE_TEST_END		0x02
#define WITCH_TYPE_TEST_DATA	0x03

///////////////////////////////////////////////////////////////////////////////
///
/// @class  CRandomPacketGenerator
///
/// @brief  The RPG class discussed at the file comment.
///
class CRandomPacketGenerator 
{
public:
    
    enum { TEST_PAYLOAD_HEADER_LENGTH = 20   };   //!< size of payload header the test reserves for itself. //
    enum { TEST_PAYLOAD_MIN_SIZE      = 0    };   //!< Not including payload header.                        //
    enum { TEST_PAYLOAD_MAX_SIZE      = 1500 };   //!< Not including payload header.                        //
    enum { MIN_RETRY_COUNT            = 0    };   //!< As defined in the 802.11 spec.                       //
    enum { MAX_RETRY_COUNT            = 15   };   //!< As defined in the 802.11 spec.                       //

    enum  eWitchPacketType
    {
        WITCH_PACKET_TYPE_START = WITCH_TYPE_TEST_START    ,
        WITCH_PACKET_TYPE_END   = WITCH_TYPE_TEST_END      ,
        WITCH_PACKET_TYPE_DATA  = WITCH_TYPE_TEST_DATA
    } ;

/*    enum  ePhyRate
    {
        PHY_RATE_6Mbps   = 0 ,
        PHY_RATE_9Mbps   = 1 ,
        PHY_RATE_12Mbps  = 2 ,
        PHY_RATE_18Mbps  = 3 ,
        PHY_RATE_24Mbps  = 4 ,
        PHY_RATE_36Mbps  = 5 ,
        PHY_RATE_48Mbps  = 6 ,
        PHY_RATE_54Mbps  = 7 ,
        PHY_RATE_MAX     = 8 
    } ;
*/
	//CHANGE TO SUPPORT B/G
    enum  ePhyRate
    {
		PHY_RATE_1Mbps	 = 0,
		PHY_RATE_2Mbps	 = 1,
		PHY_RATE_5d5Mbps = 2,
		PHY_RATE_6Mbps   = 3 ,
		PHY_RATE_9Mbps   = 4 ,
		PHY_RATE_11Mbps	 = 5,
		PHY_RATE_12Mbps  = 6 ,
		PHY_RATE_18Mbps  = 7 ,
		PHY_RATE_24Mbps  = 8 ,
		PHY_RATE_36Mbps  = 9 ,
		PHY_RATE_48Mbps  = 10 ,
		PHY_RATE_54Mbps  = 11 ,
		PHY_RATE_MCS_IDX_0  = 12, 
		PHY_RATE_MCS_IDX_1  = 13, 
		PHY_RATE_MCS_IDX_2  = 14, 
		PHY_RATE_MCS_IDX_3  = 15, 
		PHY_RATE_MCS_IDX_4  = 16, 
		PHY_RATE_MCS_IDX_5  = 17, 
		PHY_RATE_MCS_IDX_6  = 18, 
		PHY_RATE_MCS_IDX_7  = 19, 
		PHY_RATE_2Mbps_LONG = 	20,
		PHY_RATE_5d5Mbps_LONG = 21,
		PHY_RATE_11Mbps_LONG =	22,

		PHY_RATE_MAX     = 		23 
    } ;

    enum  eVariationType
    {
        
        VT_FIXED_AT_MIN = 0 , //!< No variation, fixed at minimal value.              //
        VT_RANDOM       = 1 , //!< Variate randomally around possible values.         //
        VT_ROUND_ROBIN  = 2   //!< variate around all possible values incrementally.  //
    } ;

	enum  ePayloadFillType
	{
		VT_FIXED     = 0 ,
		VT_RANDOMFILL    = 1 ,
		VT_BYTEINCR  = 2 
	} ;


	///////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////
    //
    // Default constructor/destructor pair.
    //
    CRandomPacketGenerator() ;
    ~CRandomPacketGenerator() ;

    //
    // Static parameters specification.
    //
    void setAddresses ( const uint8* sourceAddress            ,    // [IN ] Source address to put in MAC header (MUST NOT Havre BC/MC flag on!) //
                        const uint8* destAddress              ,    // [IN ] Dest address to put in MAC header - can be MC/BC.
                        const uint8* bssId                    ) ;  // [IN ] BSSID (MAC of AP) - cannot have MC/BC.


    void setQoSParms  ( const uint8 tid                         ,    // [IN ] TID to put in QoS header.
                        const uint8 aifs                        ,    // [IN ] AIFS used in this TID (mapped to AC).
                        const uint8 eCWmin                      ,    // [IN ] (log2 of) CWmin used for this TID.
                        const uint8 eCWmax                      ) ;  // [IN ] (log2 of) CWmax used for this TID.

    void setTestID    ( const uint8 testID                      ) ;  // [IN ] Test ID is defined by the caller. We just carry this to WITCH.
    void setBroadcast ( const bool  broadcast                   ) ;  // [IN ] true  - Generate broadcast packets.
                                                                     //       false - Generate unicast packets.

    //
    // Variating parameter specification.
    //
    void setPhyRate   ( const ePhyRate          minRate         ,    // [IN ] Minimal Phy rate to set.
                        const ePhyRate          maxRate         ,    // [IN ] Maximal Phy rate.
                        const eVariationType    variation       ) ;  // [IN ] Variation scheme (Random/Round-Robin...).

    void setPacketSize( const uint32            minSize         ,    // Ditto...
                        const uint32            maxSize         ,
                        const eVariationType    variation       ) ;

    void setTxRetry   ( const uint32            minCount        ,    
                        const uint32            maxCount        ,
                        const eVariationType    variation       ) ;

	void setPowerLevel ( const uint8           powerLevel) ;

	void setPayloadType(const ePayloadFillType payloadType		);

    void resetVariants( void                                    ) ;

    //
    // Pumper function - kicks a random packet into a given descriptor and packet buffer.
    //
    uint32  generateInto (WmTxBatchPacket*		   pWifiDescriptor ,   // [IN/OUT] -> WIFI API TX descriptor. Packet body must be set 
                                                                        //             to an area allocated by caller!!
						   const void*	testInfo     = NULL        ,	// [IN    ] - Test Info Structure for post processor
						   const uint32 testInfoSize = 0   		   );	// [IN]	  ] - Size of test info struct

    ePhyRate convertPhyRateToEnum ( const uint32    numericPhyRate ) const ;

    //
    // Note - we rely on identical values of enum and #define used by the post-processor tool (witch tool, that is).
    // 
    uint8    convertWitchPacketTypeToPostProcessor ( const eWitchPacketType witchPacketType ) 
    {
        return ( (uint8)witchPacketType ) ;
    } // method convertWitchPacketTypeToPostProcessor(). //

private:
    struct SPhyRateSpec
    {
        ePhyRate        minRate             ;
        ePhyRate        maxRate             ;
        eVariationType  variation           ;
    }    ;


    struct SPacketSizeSpec
    {
        uint32          minSize             ;
        uint32          maxSize             ;
        eVariationType  variation           ;
    }    ;


    struct STxRetrySpec
    {
        uint32          minCount            ;
        uint32          maxCount            ;
        eVariationType  variation           ;
    }  ;

    //
    // Header... 
    //
    bool                mInitialized            ;
    //RR1 CCriticalSection    mGuard                  ; //!< Critical section to make this thread-safe.

    //
    // Accountancy.
    //
    uint32              mSequenceNum            ;
    uint32              mCount                  ;

    //
    // MAC header static parameters.
    //
    uint8               mSourceAddress[6]       ;
    uint8               mDestAddress  [6]       ;
    uint8               mBssId        [6]       ;
    uint8               mTestID                 ; //!< Identifies the test - defined by the caller only.

    bool                mBroadcast              ; //!< Prepare a broadcast packet?
    uint8               mTid                    ; //!< Identifies the QoS stream/Access Category.             
                                                  //!< For EDCA tests, this should be in the range  0...7.    
    uint8               mAIFS                   ; //!< IFS used for the Access Category that this TID is mapped to. 
    uint8               meCWmin                 ; //!< (log2 of) Minimal Contention Window size for the AC used by this TID.
    uint8               meCWmax                 ; //!< (log2 of) Maximal Contention Window size for the AC used by this TID.
    //
    // Dynamic parameters (i.e., variatin parms).
    //
    SPhyRateSpec        mPhyRateSpec            ; //!< Specification of Phy rate and its variation.           //
    SPacketSizeSpec     mPacketSizeSpec         ; //!< Specification for Packet size and its var..            //
    STxRetrySpec        mTxRetrySpec            ; //!< Specification for TX Retry count and its variation.    //

    uint8               mPhyRateIndex           ; //!< Last Phy rate index chosen (index of a phy rate LUT)   //
    uint32              mPacketSize             ; //!< Last packet size chosen.                               //
    uint8               mTxRetryCount           ; //!< Last TX retry chosen.                                  //


	uint8               mPowerLevel             ; //!< tx PowerLevel

	ePayloadFillType		mPayloadType			;
    //
    // Prevent dangerous copying/assignments.
    //
    CRandomPacketGenerator(const CRandomPacketGenerator& ) ;
    CRandomPacketGenerator& operator= (const CRandomPacketGenerator &);
} ;





#endif // #if !defined(__CRandomPacketGenerator_included__) //

