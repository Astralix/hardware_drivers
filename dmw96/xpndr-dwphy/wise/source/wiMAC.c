//--< wiMAC.c >------------------------------------------------------------------------------------
//=================================================================================================
//
//  Simple 802.11 MAC Emulation for WiSE
//  Copyright 2004 Bermai, 2007-2011 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "wiMAC.h"
#include "wiParse.h"
#include "wiRnd.h"
#include "wiTest.h"
#include "wiUtil.h"

//=================================================================================================
//--  PARAMETERS WITH MODULE (FILE) SCOPE
//=================================================================================================
static wiMAC_State_t State = {0};
static wiUInt BroadcastAddr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

//=================================================================================================
//--  ERROR HANDLING
//=================================================================================================
#define  STATUS(arg)  WICHK((arg))
#define XSTATUS(arg) if (WICHK(arg)<0) return WI_ERROR_RETURNED;

//=================================================================================================
//--  INTERNAL FUNCTION PROTOTYPES
//=================================================================================================
wiStatus wiMAC_ParseLine(wiString text);

// ================================================================================================
// FUNCTION  : wiMAC_State()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiMAC_State_t *wiMAC_State(void)
{
    return &State;
}
// end of wiMAC_State()

// ================================================================================================
// FUNCTION  : wiMAC_SetAddress
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiMAC_SetAddress(wiInt n, wiString hexString)
{
    int i; char buf[3] = {0,0,0}; 
    wiMAC_State_t *pState = wiMAC_State();

    if (InvalidRange(n, 1, 4)) return WI_ERROR_PARAMETER1;
    if (wiProcess_ThreadIndex() ) return WI_ERROR_INVALID_FOR_THREAD;

    n--; // change to zero-based index
    for (i=0; i<6; i++)
    {
        buf[0] = *(hexString++);
        buf[1] = *(hexString++);
        pState->Address[n][i] = strtoul(buf, NULL, 16);
    }
    return WI_SUCCESS;
}
// end of wiMAC_SetAddress()

// ================================================================================================
// FUNCTION  : wiMAC_Init
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiMAC_Init(void)
{
    wiMAC_State_t *pState = wiMAC_State();

    // --------------------
    // Setup default values
    // --------------------
    pState->ProtocolVersion = 0x0;
    pState->Type            = 0x2;
    pState->Subtype         = 0x0;
    pState->ToDS            = 0;
    pState->FromDS          = 0;
    pState->MoreFrag        = 0;
    pState->Retry           = 0;
    pState->PwrMgt          = 0;
    pState->MoreData        = 0;
    pState->WEP             = 0;
    pState->Order           = 0;
    pState->DurationID      = 0;
    pState->SequenceControl = 0x1870;
    pState->QoSControl      = 0x79; 
    pState->Ns_AMPDU        = 4;
    pState->Ns_AMSDU        = 4; 
    pState->SeedPRBS        = 0x7FFFFF;
    wiMAC_SetAddress(1, "FFFFFFFFFFFF");
    wiMAC_SetAddress(2, "000E4C000001");
    wiMAC_SetAddress(3, "001020304050");
    wiMAC_SetAddress(4, "000000000000");

    XSTATUS(wiParse_AddMethod(wiMAC_ParseLine));
    return WI_SUCCESS;
}
// end of wiMAC_Init()

// ================================================================================================
// FUNCTION  : wiMAC_Close
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiMAC_Close(void)
{
    wiMAC_State_t *pState = wiMAC_State();

    WIFREE_ARRAY(pState->DataBuffer);
    XSTATUS(wiParse_RemoveMethod(wiMAC_ParseLine));
    return WI_SUCCESS;
}
// end of wiMAC_Close()

// ================================================================================================
// FUNCTION  : wiMAC_GetFrameControlWord
// ------------------------------------------------------------------------------------------------
// Purpose   : 
// Parameters: FrameControl
// ================================================================================================
wiStatus wiMAC_GetFrameControlWord(wiUWORD *FrameControl, wiBoolean IsACK)
{
    wiMAC_State_t *pState = wiMAC_State();

    if (IsACK)
    {
        FrameControl->word= pState->ProtocolVersion | (0x1<<2) | (0xD<<4);
        FrameControl->b8  = 0;
        FrameControl->b9  = 0;
        FrameControl->b10 = 0;
        FrameControl->b11 = 0;
        FrameControl->b12 = pState->PwrMgt;
        FrameControl->b13 = 0;
        FrameControl->b14 = 0;
        FrameControl->b15 = 0;
    }
    else
    {
        FrameControl->word= pState->ProtocolVersion | (pState->Type<<2) | (pState->Subtype<<4);
        FrameControl->b8  = pState->ToDS;
        FrameControl->b9  = pState->FromDS;
        FrameControl->b10 = pState->MoreFrag;
        FrameControl->b11 = pState->Retry;
        FrameControl->b12 = pState->PwrMgt;
        FrameControl->b13 = pState->MoreData;
        FrameControl->b14 = pState->WEP;
        FrameControl->b15 = pState->Order;
    }    
    return WI_SUCCESS;
}
// end of wiMAC_GetFrameControlWord()

// ================================================================================================
// FUNCTION  : wiMAC_InsertField
// ------------------------------------------------------------------------------------------------
// Purpose   : 
// Parameters: 
// ================================================================================================
wiStatus wiMAC_InsertField(wiInt nBytes, wiUWORD *Y, wiUInt X)
{
    int i;
    for (i=0; i<nBytes; i++)
        Y[i].word = (X >> (8*i)) & 0xFF;
    
    return WI_SUCCESS;
}
// end of wiMAC_InsertField()

// ================================================================================================
// FUNCTION  : wiMAC_InsertArray
// ------------------------------------------------------------------------------------------------
// Purpose   : 
// Parameters: 
// ================================================================================================
wiStatus wiMAC_InsertArray(wiInt nBytes, wiUWORD *Y, wiUInt X[])
{
    int i;
    for (i=0; i<nBytes; i++) 
        Y[i].word = X[i] & 0xFF;

    return WI_SUCCESS;
}
// end of wiMAC_InsertArray()

// ================================================================================================
// FUNCTION  : wiMAC_InsertFCS
// ------------------------------------------------------------------------------------------------
// Purpose   : Compute and insert the 802.11 FCS (algorithm from FrameGen2 program)
// Parameters: 
// ================================================================================================
wiStatus wiMAC_InsertFCS(wiInt nBytes, wiUWORD X[])
{
    int i, j;
    unsigned int p = 0xEDB88320UL;
    unsigned int c = 0xFFFFFFFFUL;
    unsigned int a, b;

    for (i=0; i<nBytes; i++)
    {
        a = (c & 0xFF) ^ X[i].w8;
        for (j=0; j<8; j++)
            a = p*(a&1) ^ (a>>1);
        b = c >> 8;
        c = a ^ b;
    }
    c = c ^ 0xFFFFFFFFUL;
    X[nBytes+0].word = (c >>  0) & 0xFF;
    X[nBytes+1].word = (c >>  8) & 0xFF;
    X[nBytes+2].word = (c >> 16) & 0xFF;
    X[nBytes+3].word = (c >> 24) & 0xFF;
    return WI_SUCCESS;
}
// end of wiMAC_InsertFCS()

// ================================================================================================
// FUNCTION  : wiMAC_ValidFCS
// ------------------------------------------------------------------------------------------------
// Purpose   : Compute and check the 802.11 Frame Check Sequence
// Parameters: nBytes -- Number of elements in X
//             X      -- Data array (bytes)
// Returns   : 0 = Invalid FCS, 1 = Valid FCS
// ================================================================================================
wiBoolean wiMAC_ValidFCS(wiInt nBytes, wiUWORD X[])
{
    int i, j;
    unsigned int p = 0xEDB88320UL;
    unsigned int c = 0xFFFFFFFFUL;
    unsigned int a, b;
    wiUWORD d = {0};

    for (i=0; i<nBytes; i++)
    {
        a = (c & 0xFF) ^ X[i].w8;
        for (j=0; j<8; j++)
            a = p*(a&1) ^ (a>>1);
        b = c >> 8;
        c = a ^ b;
    }
    for (i=0; i<32; i++) 
    {
       d.word = d.word << 1;
       d.b0 = (c&1);
       c = c>>1;
    }
    return (d.word == 0xC704DD7BUL) ? 1 : 0;
}
// end of wiMAC_ValidFCS()

// ================================================================================================
// FUNCTION  : wiMAC_Modulo256ByteSequence
// ------------------------------------------------------------------------------------------------
// Parameters: nLength -- Data array length (bytes)
//             pData   -- Data byte array
// ================================================================================================
wiStatus wiMAC_Modulo256ByteSequence(wiInt n, wiUWORD *pData)
{
    int i;
 
    for (i=0; i<n; i++)
        pData[i].word = i % 256;
 
    return WI_SUCCESS;
}
// end of wiMAC_Modulo256ByteSequence()

// ================================================================================================
// FUNCTION  : wiMAC_PseudoRandomByteSequence
// ------------------------------------------------------------------------------------------------
// Parameters: nLength -- Data array length (bytes)
//             pData   -- Data byte array
// ================================================================================================
wiStatus wiMAC_PseudoRandomByteSequence(wiInt n, wiUWORD *pData)
{
    int i,j;
    wiUWORD X;
    wiMAC_State_t *pState = wiMAC_State();
    
    if (wiProcess_ThreadIndex() ) return WI_ERROR_INVALID_FOR_THREAD;

    X.word = pState->SeedPRBS;        // retrieve the shift register state
    if (!X.word) X.word = 0xFFFFFFFF; // if seed is zero, force to all ones

    for ( i=0; i<n; i++ )
    {
        pData[i].word = X.w8;     // grab the data byte
        for (j=0; j<8; j++)        // we took a byte so advance the generator by 8 bits
            X.word = (X.w30 << 1) | (X.b30 ^ X.b27);
    }
    pState->SeedPRBS = X.word;      // save the shift register state

    return WI_SUCCESS;
}
// end of wiMAC_PseudoRandomByteSequence()

// ================================================================================================
// FUNCTION  : wiMAC_GetDataBuffer
// ------------------------------------------------------------------------------------------------
// Parameters: nLength -- Data array length (bytes)
//             pData   -- Data byte array
// ================================================================================================
wiStatus wiMAC_GetDataBuffer(wiInt n, wiUWORD *pData)
{
    wiMAC_State_t *pState = wiMAC_State();

    int i;
    int m = min(n, (int)pState->DataBuffer.Length);

    for (i=0; i<m; i++)
        pData[i].word = pState->DataBuffer.ptr[i];

    for ( ;   i<n; i++)
        pData[i].word = 0;

    return WI_SUCCESS;
}
// end of wiMAC_GetDataBuffer()

// ================================================================================================
// FUNCTION  : wiMAC_SetDataBuffer
// ------------------------------------------------------------------------------------------------
// Parameters: nLength -- Data array length (bytes)
//             pData   -- Data byte array
// ================================================================================================
wiStatus wiMAC_SetDataBuffer(wiInt n, wiUWORD *pData)
{
    int i;
    wiMAC_State_t *pState = wiMAC_State();
    wiByte *pDataBuffer;

    if (InvalidRange(n, 1, 65535)) return STATUS(WI_ERROR_PARAMETER1);
    if (!pData) return STATUS(WI_ERROR_PARAMETER2);
    if (wiProcess_ThreadIndex() ) return WI_ERROR_INVALID_FOR_THREAD;

    GET_WIARRAY( pDataBuffer, pState->DataBuffer, n, wiByte );

    for (i=0; i<n; i++)
        pDataBuffer[i] = (unsigned char)pData[i].w8;

    return WI_SUCCESS;
}
// end of wiMAC_GetDataBuffer()

// ================================================================================================
// FUNCTION  : wiMAC_GetFrameBody
// ------------------------------------------------------------------------------------------------
// Parameters: pFrameBody    -- Destination for the frame body
//             Length        -- Number of bytes in the frame body
//             FrameBodyType -- Frame data source
// ================================================================================================
wiStatus wiMAC_GetFrameBody(wiUWORD *pFrameBody, wiInt Length, wiInt FrameBodyType )
{
    switch (FrameBodyType)
    {
        case 0: XSTATUS( wiMAC_Modulo256ByteSequence   (Length, pFrameBody) ); break;
        case 1: XSTATUS( wiTest_RandomMessage          (Length, pFrameBody) ); break;
        case 2: XSTATUS( wiMAC_PseudoRandomByteSequence(Length, pFrameBody) ); break;
        case 3: XSTATUS( wiMAC_GetDataBuffer           (Length, pFrameBody) ); break;
        default: return STATUS(WI_ERROR_UNDEFINED_CASE);
    }
    return WI_SUCCESS;
}
// end of wiMAC_GetFrameBody()

// ================================================================================================
// FUNCTION  : wiMAC_DataFrame
// ------------------------------------------------------------------------------------------------
// Parameters: Length        -- PSDU length
//             PSDU          -- PSDU array
//             FrameBodyType -- Type of frame body
//             Broadcast     -- Broadcast or use Address[0] from state
// ================================================================================================
wiStatus wiMAC_DataFrame(wiInt Length, wiUWORD PSDU[], wiInt FrameBodyType, wiBoolean Broadcast)
{
    unsigned int n = Length - 24 - 4;
    wiUWORD FrameControl;

    wiMAC_State_t *pState = wiMAC_State();

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Special Case: ACK Frame is 14 Bytes
    //
    if (PSDU && Length == 14)
    {
        wiMAC_GetFrameControlWord(&FrameControl, 1);
        wiMAC_InsertField(2, PSDU + 0, FrameControl.w16);   
        wiMAC_InsertField(2, PSDU + 2, pState->DurationID);
        wiMAC_InsertArray(6, PSDU + 4, pState->Address[1]);
    }
    else // Generic Data Frame //////////////////////////////////////////////////////////
    {
        // --------------------
        // Error/Range Checking
        // --------------------
        if (!PSDU) return WI_ERROR_PARAMETER2;
        if (InvalidRange(Length, 28, 65535)) return WI_ERROR_PARAMETER1;

        // -------------------
        // Assemble the header
        // -------------------
        wiMAC_GetFrameControlWord(&FrameControl, 0);
        wiMAC_InsertField(2, PSDU +  0, FrameControl.w16);   
        wiMAC_InsertField(2, PSDU +  2, pState->DurationID);
        wiMAC_InsertArray(6, PSDU +  4, Broadcast ? BroadcastAddr : pState->Address[0] );
        wiMAC_InsertArray(6, PSDU + 10, pState->Address[1]);
        wiMAC_InsertArray(6, PSDU + 16, pState->Address[2]);
        wiMAC_InsertField(2, PSDU + 22, pState->SequenceControl);
        wiMAC_GetFrameBody(  PSDU + 24, n, FrameBodyType );
    }
    wiMAC_InsertFCS(Length - 4, PSDU); // Frame Check Sequence

    return WI_SUCCESS;
}
// end of wiMAC_DataFrame()

// ================================================================================================
// FUNCTION   : wiMAC_AMSDUDataFrame   
// ------------------------------------------------------------------------------------------------
// Purpose   : Generate PSDU for aggregated MSDU 
// Parameters: Length        -- PSDU length
//             PSDU          -- PSDU array
//             FrameBodyType -- Type of frame body
//             Broadcast     -- Broadcast or use Address[0] from state
// ================================================================================================
wiStatus wiMAC_AMSDUDataFrame(wiInt Length, wiUWORD PSDU[], wiInt FrameBodyType, wiBoolean Broadcast)
{	
    wiMAC_State_t *pState = wiMAC_State();

    wiUWORD   FrameControl, *pSubframe;
    wiInt     n, LengthMSDU, SubframeLength1, SubframeLength2;
    
    wiBoolean UseAddr4 = pState->ToDS & pState->FromDS ? 1:0;
    wiInt     FrameBodyLength = Length - 26 - 4 - (UseAddr4 ? 6:0);
    wiUInt   *pDestinationAddress = Broadcast ? BroadcastAddr : pState->Address[0];

    if (FrameBodyLength > 7935) return STATUS(WI_ERROR_PARAMETER1); // limit per 802.11n 7.1.2

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Compute the Subframe Length
    //
    //  All subframes include a 14 byte header and, except for the last subframe, must be
    //  padded to a length that is a multiple of 4.
    //
    if (pState->Ns_AMSDU > 1)
    {
        SubframeLength1 = 4 * ((FrameBodyLength - 14) / (4 * pState->Ns_AMSDU));
        SubframeLength2 = FrameBodyLength - SubframeLength1 * (pState->Ns_AMSDU - 1);
    }
    else
    {
        SubframeLength1 = FrameBodyLength;
        SubframeLength2 = 0;
    }

    if (SubframeLength1 < 14) return STATUS(WI_ERROR_PARAMETER1);

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Create the MAC Header
    //
    //  Force the MSB for the subtype to be 1 to indicate a QoS type. This is not valid
    //  for all subtypes but is required for an A-MSDU frame. Generally, WiSE is used to
    //  generate DATA frames, so this correctly indicates QoS DATA.
    //
    //  Within the QoS control field, b7 is forced active to indicate an A-MSDU frame.
    //
	wiMAC_GetFrameControlWord(&FrameControl, 0);
    FrameControl.word = FrameControl.w16 | 0x0080; // Force subtype to indicate QoS
	
    wiMAC_InsertField(2, PSDU +  0, FrameControl.w16);
	wiMAC_InsertField(2, PSDU +  2, pState->DurationID);
	wiMAC_InsertArray(6, PSDU +  4, pDestinationAddress);
	wiMAC_InsertArray(6, PSDU + 10, pState->Address[1]);
	wiMAC_InsertArray(6, PSDU + 16, pState->Address[2]);
	wiMAC_InsertField(2, PSDU + 22, pState->SequenceControl);

    if (UseAddr4)
    {
	    wiMAC_InsertArray(6, PSDU+24, pState->Address[3]);
	    wiMAC_InsertField(2, PSDU+30, pState->QoSControl | 0x0080); // Force A-MSDU Present
        pSubframe = PSDU + 32;
    }
    else
    {
	    wiMAC_InsertField(2, PSDU+24, pState->QoSControl | 0x0080); // Force A-MSDU Present
        pSubframe = PSDU + 26;
    }

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Create the A-MSDU Body
    //
    //  For standard compliance, the individual MSDU length should be limited; however,
    //  this is not enforced which allows the creation of an A-MSDU type containing a
    //  single large MSDU. This case accomodates the DwPhyTest PER test where the MAC
    //  is configured for bypass/test mode so only the PSDU FCS check is considered.
    //
    for (n=1; n<=pState->Ns_AMSDU; n++)
    {
        LengthMSDU = (n < pState->Ns_AMSDU ? SubframeLength1:SubframeLength2) - 14;

		wiMAC_InsertArray(6, pSubframe,      pDestinationAddress);
		wiMAC_InsertArray(6, pSubframe + 6,  pState->Address[1]  );
		wiMAC_InsertField(1, pSubframe + 12, LengthMSDU/256     );
		wiMAC_InsertField(1, pSubframe + 13, LengthMSDU%256     );
        wiMAC_GetFrameBody(  pSubframe + 14, LengthMSDU, FrameBodyType );
        pSubframe += (14 + LengthMSDU);
	}
	// ---------------------------------
    // Generate the frame check sequence
    // ---------------------------------
	wiMAC_InsertFCS(Length-4, PSDU);
    
    return WI_SUCCESS;
}
// end of wiMAC_AMSDUDataFrame()

// ================================================================================================
// FUNCTION   : wiMAC_AMPDUDataFrame   
// ------------------------------------------------------------------------------------------------
// Purpose    : Generate PSDU for aggregated MPDU 
// Parameters : Length   -- The total aggregated MPDU Length 
// ================================================================================================
wiStatus wiMAC_AMPDUDataFrame(wiInt Length, wiUWORD PSDU[], wiBoolean RandomMSDU, wiBoolean Broadcast)
{    	
    wiInt   i,n,k, ofs, n_hdr, length1, length2, len; 
    wiInt   Addr4, QoS;
    wiUWORD FrameControl,c;  
	wiUInt  i_c, input;

    wiMAC_State_t *pState = wiMAC_State();
    
    // Miminum MPDU spacing set to be upper limit 16*65/8
    if (InvalidRange(Length, 130*pState->Ns_AMPDU, 65535)) return STATUS(WI_ERROR_PARAMETER1);
    
    length1 = Length/pState->Ns_AMPDU;   // MPDU+ delimiter for subframes except last one;
    length2 = Length - (pState->Ns_AMPDU - 1)*4*((length1 + 3)/4); // MPDU +delimiter for last subframe

	// MPDU limit for non A-MSDU: 4 (delimiter)+2304 (MSDU)+26+4+6 (MAC header +FCS;  
	if (InvalidRange(length1, 0, 2344)) return STATUS(WI_ERROR_PARAMETER1);
	if (InvalidRange(length2, 0, 2344)) return STATUS(WI_ERROR_PARAMETER1);

    Addr4 = pState->ToDS & pState->FromDS? 1:0;
    QoS   = pState->Subtype > 7 ? 1:0;  
    n_hdr = 24 + 6*Addr4 + 2*QoS; // MPDU header length; 

    wiMAC_GetFrameControlWord(&FrameControl, 0);
    ofs = 0;

    // ---------------------
    // Create the frame body
    // ---------------------
	for (k=0; k<pState->Ns_AMPDU; k++)
	{			
		len = (k == (pState->Ns_AMPDU - 1) ? length2 : length1) - 4; // MPDU Length
        n = len - n_hdr - 4; // MSDU length

        // 4 octets delimiter
		wiMAC_InsertField(1, PSDU+ofs, len<<4 &0xF0); // LSB first: reserved bits, b0, b1, b2, b3
		wiMAC_InsertField(1, PSDU+ofs+1, len>>4); // b4, b5,..., b11
	
		c.word = 0xFF;  // Reset shift register to all ones
        for (i_c=0; i_c < 16; i_c++)
		{       
			input = ((PSDU[ofs+i_c/8].word >> (i_c % 8)) & 1) ^ c.b7;
			c.w8 = (c.w7<<1) |(input &1);
			c.b1 = c.b1^input;
			c.b2 = c.b2^input;;
		}
		
		PSDU[ofs+2].b0 = !c.b7;
		PSDU[ofs+2].b1 = !c.b6;   
		PSDU[ofs+2].b2 = !c.b5;
		PSDU[ofs+2].b3 = !c.b4;
		PSDU[ofs+2].b4 = !c.b3;
		PSDU[ofs+2].b5 = !c.b2;
		PSDU[ofs+2].b6 = !c.b1;
		PSDU[ofs+2].b7 = !c.b0;
		PSDU[ofs+2].word = PSDU[ofs+2].word & 0xFF;
		
		wiMAC_InsertField(1, PSDU + ofs + 3, 0x4E); // Delimiter Signature
		ofs += 4;

		// -------------------
		// Assemble the header
		// -------------------
		wiMAC_InsertField(2, PSDU+ofs+ 0, FrameControl.w16);   
		wiMAC_InsertField(2, PSDU+ofs+ 2, pState->DurationID);
		wiMAC_InsertArray(6, PSDU+ofs+ 4, Broadcast ? BroadcastAddr : pState->Address[0]);
		wiMAC_InsertArray(6, PSDU+ofs+10, pState->Address[1]);
		wiMAC_InsertArray(6, PSDU+ofs+16, pState->Address[2]);
		wiMAC_InsertField(2, PSDU+ofs+22, pState->SequenceControl);
        wiMAC_InsertArray(6*Addr4, PSDU+ofs+24, pState->Address[3]);
		wiMAC_InsertField(2*QoS, PSDU+ofs+24+6*Addr4, pState->QoSControl);  
	
		if (RandomMSDU)
		{
			XSTATUS( wiTest_RandomMessage(n, PSDU+ofs+n_hdr) );
		}
		else 
		{
			for (i=0; i<n; i++)
				PSDU[ofs + n_hdr + i].word = i % 256;
		}

		// ---------------------------------
		// Generate the frame check sequence
		// ---------------------------------
		wiMAC_InsertFCS(len-4, PSDU+ofs);

		// ---------------------------------
		// Subframe padding to 4 octets 
		// Except last subframe
		// ---------------------------------
		i=0;
		if ((len % 4) && (k != pState->Ns_AMPDU - 1))
			for (i = 0; i < 4 - len % 4; i++)
				PSDU[ofs + len + i].word = 0;
		ofs += len + i;
	}    
 
    return WI_SUCCESS;
}
// end of wiMAC_AMPDUDataFrame()

// ================================================================================================
// FUNCTION  : wiMAC_ParseLine
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiMAC_ParseLine(wiString text)
{
    #define PSTATUS(arg) if ((pstatus=WICHK(arg))<=0) return pstatus;

    if (strstr(text, "wiMAC"))
    {
        wiStatus status, pstatus;
        wiMAC_State_t *pState = wiMAC_State();

        if (!pState) return WI_ERROR_MODULE_NOT_INITIALIZED;

        PSTATUS( wiParse_UInt(text,"wiMAC.ProtocolVersion",&(pState->ProtocolVersion),0, 0x3) );
	    PSTATUS( wiParse_UInt(text,"wiMAC.Type",      &(pState->Type),      0, 2) ); // Type 0x3 is reserved
        PSTATUS( wiParse_UInt(text,"wiMAC.SubType",   &(pState->Subtype),   0, 15) );
        PSTATUS( wiParse_UInt(text,"wiMAC.ToDS",      &(pState->ToDS),      0, 1) );
        PSTATUS( wiParse_UInt(text,"wiMAC.FromDS",    &(pState->FromDS),    0, 1) );
        PSTATUS( wiParse_UInt(text,"wiMAC.QoSControl",&(pState->QoSControl),0, 0xFFFF) );
        PSTATUS( wiParse_UInt(text,"wiMAC.SeedPRBS",  &(pState->SeedPRBS),  0, 0x7FFFFFFF) );
        PSTATUS( wiParse_Int (text,"wiMAC.Ns_AMPDU",  &(pState->Ns_AMPDU),  1, 256) );
        PSTATUS( wiParse_Int (text,"wiMAC.Ns_AMSDU",  &(pState->Ns_AMSDU),  1, 256) );

        // ------------------------------
        // Parameter sweep test functions
        // ------------------------------
        {
            int n; char paramstr[32];

            XSTATUS(status = wiParse_Function(text,"wiMAC_SetAddress(%d, %32s)",&n, paramstr));
            if (status == WI_SUCCESS)
            {
                XSTATUS(wiMAC_SetAddress(n, paramstr));
                return WI_SUCCESS;
            }
        }
        // ----- -
        // Debug %
        // ----- -
        {
            wiUWORD X[4096];
            XSTATUS(status = wiParse_Command(text,"wiMAC_DEBUG()"));
            if (status == WI_SUCCESS)
            {
                int i, n = 2000;
                wiMAC_DataFrame(n, X, 0, 0);
                for (i=0; i<n; i++) printf("%02X\n",X[i].w8);
                return WI_SUCCESS;
            }
        }
    }
    return WI_WARN_PARSE_FAILED;
}
// end of wiMAC_ParseLine()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------
