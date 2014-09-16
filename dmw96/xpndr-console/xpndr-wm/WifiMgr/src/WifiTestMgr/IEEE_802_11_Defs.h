#if !defined(__IEEE_802_11_Defs_included__)
#    define  __IEEE_802_11_Defs_included__

//////////////////////////////////////////////////////////////////////////////
/// 802.11 Data MSDU types and subtypes definition.
///
/// Data type (802.11 defines 3 types, Control, Management & Data
///
#define FC_TYPE_DATA                    0x02
//
// Data subtypes.
//
#define FC_SUBTYPE_DATA_SUB             0x0
#define FC_SUBTYPE_DATA_CF_ACK          0x1
#define FC_SUBTYPE_DATA_CF_POLL         0x2
#define FC_SUBTYPE_DATA_CF_ACK_POLL     0x3
#define FC_SUBTYPE_DATA_NULL            0x4
#define FC_SUBTYPE_CF_ACK               0x5
#define FC_SUBTYPE_CF_POLL              0x6
#define FC_SUBTYPE_CF_ACK_POLL          0x7
#define FC_SUBTYPE_QOS_DATA             0x8 // Everything above 0x08 is QoS only. //
#define FC_SUBTYPE_QOS_DATA_CF_ACK      0x9
#define FC_SUBTYPE_QOS_DATA_CF_POLL     0xA
#define FC_SUBTYPE_QOS_DATA_CF_ACK_POLL 0xB
#define FC_SUBTYPE_QOS_NULL             0xC
#define FC_SUBTYPE_QOS_CF_ACK           0xD
#define FC_SUBTYPE_QOS_CF_POLL          0xE
#define FC_SUBTYPE_QOS_CF_ACK_POLL      0xF

//
// MAC header addresses.
// 
#define MH_ADDRESS1_OFFSET              4
#define MH_ADDRESS2_OFFSET              10
#define MH_ADDRESS3_OFFSET              16
#define MH_ADDRESS4_OFFSET              24
#define MH_ADDRESS_SIZE                 6

//
// Access Category types
//
#if !defined(AC_BEST_EFFORT)
#    define AC_BEST_EFFORT              0   // Access Category #0 - Best Effort.    //
#    define AC_BACKGROUND               1   // Access Category #1 - Background.     //
#    define AC_VIDEO                    2   // Access Category #2 - Video           //
#    define AC_VOICE                    3   // Access Category #3 - Voice.          //
#endif // #if !defined(AC_BEST_EFFORT) //

#endif // #if !defined(__IEEE_802_11_Defs_included__) //

