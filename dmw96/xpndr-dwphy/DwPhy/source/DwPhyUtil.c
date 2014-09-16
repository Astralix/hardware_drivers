//--< DwPhyUtil.c >--------------------------------------------------------------------------------
//=================================================================================================
//
//  DW MAC/PHY Interface Function Not Included in the Driver
//  Copyright 2007 DSP Group, Inc. All rights reserved.
//
//=================================================================================================
//-------------------------------------------------------------------------------------------------

#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#include "vwal.h"

#include "hw-phy.h"
#include "DwPhyUtil.h"

// ================================================================================================
// -- GLOBAL PARAMETERS
// ================================================================================================
char     DevName[16] = {"wifi0"};   // target device

// ================================================================================================
// FUNCTION   : ReadMib()
// ------------------------------------------------------------------------------------------------
// Purpose    : Reads the MIB using VWAL
// Parameters : MibObjectID -- defined in WifiApi.h
//              pMibValue   -- pointer to buffer for returned value (type depends on mibObjectID)
// ================================================================================================
int32_t ReadMib( uint16_t MibObjectID, void *pMibValue )
{
    mibObjectContainer_t MibObjectContainer[] = {
        {MibObjectID, WIFI_MIB_API_OPERATION_READ, 0, pMibValue},
    };
    int MibApiStatus = WifiMibAPI(DevName, MibObjectContainer, 1);

    if( MibApiStatus != WIFI_SUCCESS )
    {
        printf(">>> ReadMib( %d, * )\n",MibObjectID);
        printf(">>> WifiMibAPI Status %d in %s, line %d\n",MibApiStatus,__FILE__,__LINE__);
        return -1;
    }
    return 0;
}
// end of ReadMib()

// ================================================================================================
// FUNCTION   : WriteMib()
// ------------------------------------------------------------------------------------------------
// Purpose    : Writes the MIB using VWAL
// Parameters : MibObjectID -- defined in WifiApi.h
//              pMibValue   -- pointer to buffer for the write data (type depends on mibObjectID)
// ================================================================================================
int32_t WriteMib( uint16_t MibObjectID, void *pMibValue )
{
    mibObjectContainer_t MibObjectContainer[] = {
        {MibObjectID, WIFI_MIB_API_OPERATION_WRITE, 0, pMibValue},
    };
    int MibApiStatus = WifiMibAPI(DevName, MibObjectContainer, 1);
        
    if( MibApiStatus != WIFI_SUCCESS )
    {
        printf(">>> WriteMib( %d, * )\n",MibObjectID);
        printf(">>> WifiMibAPI Status %d in %s, line %d\n",MibApiStatus,__FILE__,__LINE__);
        return -1;
    }
    return 0;
}
// end of WriteMib()

// ================================================================================================
// FUNCTION   : OS_RegRead()
// ------------------------------------------------------------------------------------------------
// Purpose    : Read the specified MAC register
// Parameters : Addr -- Address (only 16 bits used)
// ================================================================================================
uint32_t OS_RegRead(uint32_t Addr)
{
    uint32_t Data;

    if( WriteMib(MIBOID_MAC_REG_ACCESS_ADDR,  &Addr) == 0)
        if( ReadMib (MIBOID_MAC_REG_ACCESS_DWORD, &Data) == 0)
            return Data;

    printf(">>> Error in %s, %d\n",__FILE__,__LINE__);
    return 0xDEAD1234;
} 
// end of OS_RegRead()

// ================================================================================================
// FUNCTION   : OS_RegWrite()
// ------------------------------------------------------------------------------------------------
// Purpose    : Write the specified MAC register
// Parameters : Addr  -- Address (only 16 bits used)
//              Data  -- Value to be written (32 bits)
// ================================================================================================
void OS_RegWrite(uint32_t Addr, uint32_t Data)
{
    if( WriteMib(MIBOID_MAC_REG_ACCESS_ADDR,  &Addr) == WIFI_SUCCESS)
        if( WriteMib(MIBOID_MAC_REG_ACCESS_DWORD, &Data) == WIFI_SUCCESS)
            return ;

    printf(">>> Error in %s,%d (Addr = 0x%04X, Data = 0x%02X\n",__FILE__,__LINE__,Addr,Data);
}
// ed of OS_RegWrite()

// ================================================================================================
// FUNCTION   : OS_RegReadBB()
// ------------------------------------------------------------------------------------------------
// Purpose    : Read the specified baseband register
// Parameters : Addr  -- Address (only 10 bits valid)
// ================================================================================================
uint8_t OS_RegReadBB(uint32_t Addr)
{
    uint32_t Data = OS_RegRead( Addr );
    return (uint8_t)Data;
} 
// end of OS_RegReadBB()

// ================================================================================================
// FUNCTION   : OS_RegWriteBB()
// ------------------------------------------------------------------------------------------------
// Purpose    : Write the specified baseband register
// Parameters : Addr  -- Address (only 10 bits valid)
//              Value -- Value to be written (8 bits)
// ================================================================================================
void OS_RegWriteBB(uint32_t Addr, uint8_t Value)
{
    OS_RegWrite( Addr, Value );
}
// end of OS_RegWriteBB()

// ================================================================================================
// FUNCTION   : OS_Delay()
// ------------------------------------------------------------------------------------------------
// Purpose    : Implement a delay
// Parameters : microsecond -- Request delay period (microseconds)
// ================================================================================================
void OS_Delay(uint32_t microsecond)
{
    double tEnd;

    // --------------------------------------------------
    // Contingency: use 'C' process time (low resolution)
    // --------------------------------------------------
    {
        tEnd = (double)clock() + (double)microsecond/1e6 * CLOCKS_PER_SEC;
        while( (double)clock() < tEnd )
            ;
    }
}
// end of OS_Delay()

// ================================================================================================
// FUNCTION   : DwPhyEnableFn()
// ------------------------------------------------------------------------------------------------
// Purpose    : Function to set the state of DW_PHYEnB
// Parameters : pDevInfo -- Not used but include for consistency with the driver
//              Enable -- PHY state (0=sleep, 1=wake)
// ================================================================================================
uint32_t DwPhyEnableFn( void *pDevInfo, uint32_t Enable )
{
    OS_RegWrite( 0xFFFFFE3D, Enable );
    return 0;
}
// end of DwPhyEnableFn()

// ================================================================================================
// FUNCTION  : DwPhyUtil_5GHzRadioName()
// ------------------------------------------------------------------------------------------------
// Purpose   : Returns a string with the Bermai/DSPG WiFi radio name
// Parameters: PartID - radio part ID (from ID register)
// ================================================================================================
char *DwPhyUtil_RadioName(unsigned int PartID)
{
    static char s[32];

    switch(PartID)
    {
        case  0: sprintf(s,"-- No Radio --"); break;
        case  1: sprintf(s,"Armstrong 1/B1"); break;
        case  2: sprintf(s,"Armstrong 2/B2"); break;
        case  3: sprintf(s,"Armstrong 3/B3"); break;
        case  4: sprintf(s,"Armstrong 4/B4"); break;
        case  6: sprintf(s,"Armstrong 6/B6"); break;
        case  7: sprintf(s,"Armstrong C1"); break;
        case  8: sprintf(s,"Armstrong C2"); break;
        case  9: sprintf(s,"Armstrong C3"); break;
        case 10: sprintf(s,"Armstrong C4"); break;
        case 12: sprintf(s,"Armstrong C6"); break;
        case 13: sprintf(s,"Armstrong D1"); break;
        case 14: sprintf(s,"Armstrong D2"); break;
        case 15: sprintf(s,"Armstrong D3"); break;
        case 16: sprintf(s,"Armstrong D4"); break;
        case 18: sprintf(s,"Armstrong D6"); break;
        case 19: sprintf(s,"Basie"   ); break;
        case 20: sprintf(s,"Basie2"  ); break;
        case 21: sprintf(s,"Basie B1"); break;
        case 22: sprintf(s,"Basie B2"); break;
        case 23: sprintf(s,"Basie B3"); break;
        case 24: sprintf(s,"Basie C1"); break;
        case 25: sprintf(s,"Basie C2"); break;
        case 26: sprintf(s,"Basie C3"); break;
        case 27: sprintf(s,"Basie D1"); break;
        case 28: sprintf(s,"Basie D2"); break;
        case 29: sprintf(s,"Basie D3"); break;
        case 30: sprintf(s,"RF52SHC" ); break;
        case 31: sprintf(s,"RF52A01" ); break;
        case 32: sprintf(s,"RF52A02" ); break;
        case 33: sprintf(s,"RF52A03" ); break;
        case 34: sprintf(s,"RF52A04" ); break;
        case 35: sprintf(s,"RF52A110"); break;
        case 36: sprintf(s,"RF52A111"); break;
        case 37: sprintf(s,"RF52A112"); break;
        case 38: sprintf(s,"RF52A113"); break;
        case 39: sprintf(s,"RF52A120"); break;
        case 40: sprintf(s,"RF52A130"); break;
        case 41: sprintf(s,"RF52A131"); break;
        case 42: sprintf(s,"RF52A140"); break;
        case 43: sprintf(s,"RF52A141"); break;
        case 44: sprintf(s,"RF52A210"); break;
        case 45: sprintf(s,"RF52A211"); break;
        case 46: sprintf(s,"RF52A212"); break;
        case 47: sprintf(s,"RF52A220"); break;
        case 48: sprintf(s,"RF52A230"); break;
        case 49: sprintf(s,"RF52A240"); break;
        case 50: sprintf(s,"RF52A320"); break;
        case 51: sprintf(s,"RF52A321"); break;
        case 52: sprintf(s,"RF52A421"); break;
        case 53: sprintf(s,"RF52A521"); break;
        case 54: sprintf(s,"RF52B11");  break;
        case 55: sprintf(s,"RF52B12");  break;
        case 56: sprintf(s,"RF52B13");  break;
        case 57: sprintf(s,"RF52B21");  break;
        case 58: sprintf(s,"RF52B22");  break;
        case 59: sprintf(s,"RF52B23");  break;
        case 60: sprintf(s,"RF52B31");  break;
        case 61: sprintf(s,"RF52B32");  break;
        case 65: sprintf(s,"RF22A01");  break;
        case 66: sprintf(s,"RF22A02");  break;
        case 67: sprintf(s,"RF22A11");  break;
        case 68: sprintf(s,"RF22A12");  break;                        
        default: sprintf(s,"UNKNOWN" ); break;
    }
    return s;
}
// end of DwPhyUtil_RadioName()

// ================================================================================================
// FUNCTION  : DwPhyUtil_BasebandName()
// ------------------------------------------------------------------------------------------------
// Purpose   : Returns a string with the baseband name
// Parameters: PartID - baseband part ID (from ID register)
// ================================================================================================
char *DwPhyUtil_BasebandName(unsigned int PartID)
{
    static char s[32];

    switch(PartID)
    {
        case  1: sprintf(s,"Dakota1");  break;
        case  2: sprintf(s,"Dakota2");  break;
        case  3: sprintf(s,"Dakota2g"); break;
        case  4: sprintf(s,"Dakota4");  break;
        case  5: sprintf(s,"Mojave");   break;
        default: sprintf(s,"Mojave1b"); break;
    }
    return s;
}
// end of DwPhyUtil_BasebandName()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------
