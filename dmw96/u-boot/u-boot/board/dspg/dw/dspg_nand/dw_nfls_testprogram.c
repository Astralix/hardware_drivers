
//#define TEST_PROGRAM

#ifdef TEST_PROGRAM
// For Testing Only !

/***********************************/
//#define C_MaxPhysPageSize   4096
//#define C_MaxPhysSpareSize   128
/**********************************/
/***********************************/
//#define C_MaxPhysPageSize   2048
//#define C_MaxPhysSpareSize    64
/**********************************/
/***********************************/
#define C_MaxPhysPageSize    512
#define C_MaxPhysSpareSize    16
/**********************************/



__u8    PageReadData[C_MaxPhysPageSize];
__u8    SpareReadData[C_MaxPhysSpareSize];

__u8    PageWriteData[C_MaxPhysPageSize];
__u8    SpareWriteData[C_MaxPhysSpareSize];

__u8    PageModData[C_MaxPhysPageSize];
__u8    SpareModData[C_MaxPhysSpareSize];

__u32   V_PageDataErrors;
__u32   V_SpareDataErrors;
__u32   V_HammErrors;
__u32   V_UnfixedDataErrors;
__u32   V_UnfixedSpareErrors;

/*static*/ 
bool  F_CompareData( volatile __u8 *  SrcP,
                            volatile __u8 *  DstP,
                            __u16            Size )
{
   __u32    i;
   volatile __u32 *  p1;
   volatile __u32 *  p2;
   __u32   srcVal, dstVal;

   p1 = (volatile __u32 *)SrcP;
   p2 = (volatile __u32 *)DstP;

   for (i = 0; i < ((__u32)Size >> 2); i++)
   {
      srcVal = *p1;
      dstVal = *p2;

      if (srcVal != dstVal)
      {
         return (false);
      }
      p1++;
      p2++;
   }

   return (true);

} /* end F_CompareData() */

__u16  V_BadBlksArr[NFC_K_FlsDevMAX][200];


int  main( void )
{
#define ERASE_BLOCKS_First
//#define BYPASS_DMA_FlashREAD
//#define BYPASS_DMA_FlashWRITE
#define DATA_BUFS_ON_FPGA
#define ACTIVATE_ECC
#define ECC_HAMM_TEST_ModifySinglebit
#define DEASSERT_Bits_In_PAGE_AREA
//#define DEASSERT_Bits_In_N1_And_Hamm_AREA


   NFC_E_Err  FlashReadStat, FlashWriteStat, FlashEraseStat;
   __u16     i, j, CurrBlk;
   __u8      CurrPage, SpareN1Size;
   __u16     NumBadBlocks, NumPagesInBlock;
   __u16     NetPageSize, NetSpareSize;
   __u32     NumBlocks;
   bool       DevStat;
   volatile   __u32 *   TmpU32P;
   NFC_E_FlsDev  CurrFlsDev;


#ifdef PC_SIMULATION
   volatile __u8 *  PageReadDataP = &PageReadData[0];
   volatile __u8 *  SpareReadDataP = &SpareReadData[0];

   volatile __u8 *  PageWriteDataP = &PageWriteData[0];
   volatile __u8 *  SpareWriteDataP = &SpareWriteData[0];

   volatile __u8 *  PageModDataP = &PageModData[0];
   volatile __u8 *  SpareModDataP = &SpareModData[0];

#else

   #ifdef DATA_BUFS_ON_FPGA

   /* NOTE: When data buffers are located within the emulation FPGA's RAM,
   **       the aceess Must(!) be in __u32 Reads/Writes only !!!
   */

      #define C_FpgaRamWriteBase (C_IntrnlSRamAddr)
      #define C_FpgaRamReadBase  (C_FpgaRamWriteBase + 6 * 1024)
      #define C_FpgaRamModBase   (C_FpgaRamReadBase  + 6 * 1024)
      #define C_offset           0 /*16*/ /*32*/

      volatile __u8 *  PageWriteDataP = (__u8 *)C_FpgaRamWriteBase;
      volatile __u8 *  SpareWriteDataP = (__u8 *)(C_FpgaRamWriteBase + C_MaxPhysPageSize + C_offset);

      volatile __u8 *  PageReadDataP = (__u8 *)C_FpgaRamReadBase;
      volatile __u8 *  SpareReadDataP = (__u8 *)(C_FpgaRamReadBase + C_MaxPhysPageSize + C_offset);

      volatile __u8 *  PageModDataP = (__u8 *)C_FpgaRamModBase;
      volatile __u8 *  SpareModDataP = (__u8 *)(C_FpgaRamModBase + C_MaxPhysPageSize + C_offset);

   #else

      volatile __u8 *  PageReadDataP = &PageReadData[0];
      volatile __u8 *  SpareReadDataP = &SpareReadData[0];

      volatile __u8 *  PageWriteDataP = &PageWriteData[0];
      volatile __u8 *  SpareWriteDataP = &SpareWriteData[0];

      volatile __u8 *  PageModDataP = &PageModData[0];
      volatile __u8 *  SpareModDataP = &SpareModData[0];

   #endif // ! DATA_BUFS_ON_FPGA

#endif // !PC_SIMULATION

   V_PageDataErrors = 0;
   V_SpareDataErrors = 0;
   V_HammErrors = 0;
   V_UnfixedDataErrors = 0;
   V_UnfixedSpareErrors = 0;
   j = 0;


#ifdef ERASE_BLOCKS_First
   /*! FORCED ERASE Most blocks, to remove previously set bad-block-mark */
   for (CurrBlk = 0; CurrBlk < 1024 /*2048*/ /*1024*/; CurrBlk++)
   {
      F_CmdEraseBlkBypass(NFC_K_Chip_SamK9F5608, NFC_K_FlsDev1, CurrBlk);
      // F_CmdEraseBlkBypass(NFC_K_Chip_SamK9F2G08, NFC_K_FlsDev0, CurrBlk);
      F_CmdEraseBlkBypass(NFC_K_Chip_SamK9F5616, NFC_K_FlsDev0, CurrBlk);
      // F_CmdEraseBlkBypass(NFC_K_Chip_SamK9F1G16, NFC_K_FlsDev0, CurrBlk);
      // F_CmdEraseBlkBypass(NFC_K_Chip_TosTH58NVG3, NFC_K_FlsDev0, CurrBlk);
   }
#endif // ERASE_BLOCKS_First

   /*! INIT Flash-Controller Driver */
   //NFC_F_Init();


   for (CurrFlsDev = NFC_K_FlsDev0; CurrFlsDev < NFC_K_FlsDevMAX; CurrFlsDev++)
   {
      DevStat = NFC_F_GetFlsStatus(CurrFlsDev);
      if (DevStat == false)
      {
         continue;
      }

      NFC_F_GetBadBlocks(CurrFlsDev, &NumBadBlocks, &V_BadBlksArr[CurrFlsDev][0]);
      NumBlocks = NFC_F_NumBlocks(CurrFlsDev);
      NumPagesInBlock = NFC_F_BlockSize(CurrFlsDev);
      SpareN1Size = (__u8)NFC_F_N1Size(CurrFlsDev);

      /*! GO over all Blocks */
      //for (CurrBlk = 0; CurrBlk < NumBlocks; CurrBlk++)
      for (CurrBlk = (__u16)(NumBlocks - 1); CurrBlk > 0; CurrBlk--)
      {
         /*! ERASE Block (Skipping Bad Blocks) */
         FlashEraseStat = NFC_F_EraseBlk(CurrFlsDev, CurrBlk);
         if (FlashEraseStat != NFC_K_Err_OK)
         {
            if (NFC_K_Err_DfctBlk == NFC_K_Err_DfctBlk)
            {
               /*! MOVE to next Block */
               continue;
            }
            else
            {
               /*! STUCK here */
               while (1);
            }
         }

         NetPageSize = NFC_F_PageNetSize(CurrFlsDev);  // 512 , 1024, 2048
         NetSpareSize = NFC_F_SpareSize(CurrFlsDev);   //  16 , 64

         /*! READ First Page to see that Block is Erased: */

         /*! CLEAR Page-Read buffer */
         for (i = 0, TmpU32P = (__u32 *)PageReadDataP; i < (NetPageSize / 4); i++)
         {
            *TmpU32P++ = (__u32)0;
         }
         /*! CLEASR Spare-Read buffer */
         for (i = 0, TmpU32P = (__u32 *)SpareReadDataP; i < (NetSpareSize / 4); i++)
         {
            *TmpU32P++ = (__u32)0;
         }

         /*! READ first Page */
         NFC_F_ReadPage( CurrFlsDev,
                         CurrBlk,
                         0,       // Page in Block
                         0,       // N1Len,
                         false,   // IsEccF
                         true,    // DMA ByPassed
                         (void *)PageReadDataP,
                         (void *)SpareReadDataP );

         /*! VERIFY FF's in Page Area */
         for (i = 0, TmpU32P = (__u32 *)PageReadDataP; i < (NetPageSize / 4); i++)
         {
            if (*TmpU32P++ != 0xFFFFFFFF)
            {
               /*! STUCK here */
               while (1);
            }
         }
#ifndef ACTIVATE_ECC
         /*! VERIFY FF's in Spare Area */
         for (i = 0, TmpU32P = (__u32 *)SpareReadDataP; i < (NetSpareSize / 4); i++)
         {
            if (*TmpU32P++ != 0xFFFFFFFF)
            {
               /*! STUCK here */
               while (1);
            }
         }
#endif // !ACTIVATE_ECC

         /*! GO Over all block's Pages - SKIP first two pages (where bad-block-Mark is) */
         for (CurrPage = /*2*/ 0; CurrPage < NumPagesInBlock; CurrPage++)
         {
            /*! FILL Page-Write Data buffer */
            for (i = 0, TmpU32P = (__u32 *)PageWriteDataP; i < (NetPageSize / 4); i++)
            {
               *TmpU32P++ = (__u32)(i + j + CurrPage + CurrBlk);
               //*TmpU32P++ = 0xFFFFFFFF;
               // *TmpU32P++ = 0xA5F7FDEF;
            }

            /*! FILL Spare-Write Data buffer */
            for (i = 0, TmpU32P = (__u32 *)SpareWriteDataP; i < (NetSpareSize / 4); i++)
            {
               *TmpU32P++ = (__u32)(i + j + CurrPage + CurrBlk + 7777);
            }

            /*! WRITE Page+Spare with data */

            FlashWriteStat = NFC_F_ProgPage( CurrFlsDev,
                                             CurrBlk,
                                             CurrPage,
#ifdef ACTIVATE_ECC
                                             SpareN1Size, // N1Len
                                             true,        // IsEccF
#else
                                             0,           // N1Len
                                             false,       // IsEccF
#endif

#ifdef BYPASS_DMA_FlashWRITE
                                             true,     // DMA ByPassed
#else
                                             false,    // DMA Used
#endif
                                             (void *)PageWriteDataP,
                                             (void *)SpareWriteDataP );

            if (FlashWriteStat != NFC_K_Err_OK)
            {
               if ( (FlashWriteStat == NFC_K_Err_ProgFail) || (FlashWriteStat == NFC_K_Err_DfctBlk) )
               {
                  /*! MOVE to next Page */
                  continue;
               }
               else
               {
                  /*! STUCK here */
                  while (1);
               }
            }

            /*! CLEAR Page-Read buffer */
            for (i = 0, TmpU32P = (__u32 *)PageReadDataP; i < (NetPageSize / 4); i++)
            {
               *TmpU32P++ = (__u32)0xAAAAAAAA;
            }

            /*! CLEASR Spare-Read buffer */
            for (i = 0, TmpU32P = (__u32 *)SpareReadDataP; i < (NetSpareSize / 4); i++)
            {
               *TmpU32P++ = (__u32)0x55555555;
            }


            /*! READ Page+Spare data */

            FlashReadStat = NFC_F_ReadPage( CurrFlsDev,
                                            CurrBlk,
                                            CurrPage,
#ifdef ACTIVATE_ECC
                                            SpareN1Size, // N1Len
                                            true,        // IsEccF
#else
                                            0,           // N1Len,
                                            false,       // IsEccF
#endif

#ifdef BYPASS_DMA_FlashREAD
                                            true,    // DMA ByPassed
#else
                                            false,   // DMA Used
#endif
                                            (void *)PageReadDataP,
                                            (void *)SpareReadDataP );

            if (FlashReadStat != NFC_K_Err_OK)
            {
               /*! STUCK here */
               while (1);
            }

            /*! COMPARE Page Data */
            if (F_CompareData(PageWriteDataP, PageReadDataP, NetPageSize) == false)
            {
               V_PageDataErrors++;
               F_AddBadBlk(CurrFlsDev, CurrBlk);
            }

#ifndef ACTIVATE_ECC
            /*! COMPARE Spare Data */
            if (F_CompareData(SpareWriteDataP, SpareReadDataP, NetSpareSize) == false)
            {
               V_SpareDataErrors++;
               F_AddBadBlk(CurrFlsDev, CurrBlk);
            }
#endif /* !ACTIVATE_ECC */


#ifdef ACTIVATE_ECC
            /*! CHECK for Hamming errors */
            if (V_FC_RegsP->FC_Hamming.HammCodeOut != 0)
            {
               V_HammErrors++;
            }
#endif


            /*** TEMP !!!!!!!!!!!!!!!! ****/
            // continue;




#ifdef ACTIVATE_ECC
#ifdef ECC_HAMM_TEST_ModifySinglebit
            {
               __u32    tempU32;
               __u32 *  srcP;
               __u32 *  dstP;
               __u32 *  modP;

               /*! COPY WRITE Page Aera into Modify buffer, for Writing Over */
               for ( i = 0, srcP = (__u32 *)PageWriteDataP, dstP = (__u32 *)PageModDataP;
                     i < (NetPageSize / 4);
                     i++ )
               {
                  *dstP++ = *srcP++;
               }

               /*! COPY READ Spare Area into Modify buffer, for Writing Over */
               for ( i = 0, srcP = (__u32 *)SpareReadDataP, dstP = (__u32 *)SpareModDataP;
                     i < (NetSpareSize / 4);
                     i++ )
               {
                  *dstP++ = *srcP++;
               }

#ifdef DEASSERT_Bits_In_PAGE_AREA
               /*! MODIFY ('1' --> '0') a single bit in Page Area of Modify buffer */
               modP = (__u32 *)PageModDataP + CurrPage + 0 /*256*/ /*32*/ /*64*/ ; // increments in __u32 units !
               tempU32 = *modP;
              // tempU32 &= ~(0x0001 << CurrPage);
              // tempU32 &= ~(0x0011 << CurrPage);
              // tempU32 &= ~(0x0111 << CurrPage);
              // tempU32 &= ~(0x10101001 << CurrPage);
               tempU32 &= ~(0x101001 << CurrPage);
               *modP = tempU32;

               modP += 3;
               tempU32 = *modP;
               tempU32 &= ~(0x0001 << (CurrPage + 2));
               *modP = tempU32;
#endif /* DEASSERT_Bits_In_PAGE_AREA */

#ifdef DEASSERT_Bits_In_N1_And_Hamm_AREA
               {
                  __u8  Offset;
                  __u8  Limit;
                  switch (SpareN1Size)
                  {
                     case 0: Limit = 0; break;
                     case 2: Limit = 1; break;
                     case 4: Limit = 1; break;
                     case 6: Limit = 2; break;
                     default: Limit = 0; break;
                  }
                  if (Offset > Limit)
                  {
                     Offset = 0;
                  }
                  else
                  {
                     Offset++;
                  }
                  /*! MODIFY ('1' --> '0') a single bit in N1+Hamming Area of Modify buffer */
                  modP = (__u32 *)SpareModDataP + Offset; // increments in __u32 units !
                  tempU32 = *modP;
                  //tempU32 &= ~(0x1 << CurrPage);    /* Hamming */
                  tempU32 &= ~(0x1101 << CurrPage); /* BCH */
                  *modP = tempU32;
               }
#endif /* DEASSERT_Bits_In_N1_And_Hamm_AREA */


               /*! PROGRAM Modified buffer while !! ECC is Disabled !!, on top of current Page data,
               **  (Page is NOT erased), !! Keeping the Spare area !! (Including ECC generated on Write)
               **  as read before */
               NFC_F_ProgPage( CurrFlsDev, CurrBlk, CurrPage,
                               SpareN1Size,  // N1Len
                               false,        // IsEccF
                               false,        // DMA Used
                               (void *)PageModDataP,
                               (void *)SpareModDataP );


               /*! CLEAR Page-Read buffer */
               for (i = 0, TmpU32P = (__u32 *)PageReadDataP; i < (NetPageSize / 4); i++)
               {
                  *TmpU32P++ = (__u32)0xDDDDDDDD;
               }

               /*! CLEAR Spare-Read buffer */
               for (i = 0, TmpU32P = (__u32 *)SpareReadDataP; i < (NetSpareSize / 4); i++)
               {
                  *TmpU32P++ = (__u32)0xEEEEEEEE;
               }


               /*! READ flash while ECC is Enabled - Hamming should correct a single error */
               NFC_F_ReadPage( CurrFlsDev, CurrBlk, CurrPage,
                               SpareN1Size, // N1Len
                               true,        // IsEccF
                               false,       // DMA Used
                               (void *)PageReadDataP,
                               (void *)SpareReadDataP );

               /*! COMPARE Page Data with Original (unmodified) Data in Write buffer */
               if (F_CompareData(PageWriteDataP, PageReadDataP, NetPageSize) == false)
               {
                  V_UnfixedDataErrors++;
               }

               /*! COMPARE Spare Data with Original (unmodified) Data in Modify buffer */
               if (F_CompareData(SpareModDataP, SpareReadDataP, NetSpareSize) == false)
               {
                  V_UnfixedSpareErrors++;
               }

            }

#endif /* ECC_HAMM_TEST_ModifySinglebit */
#endif /* ACTIVATE_ECC */


            F_DebugDelay(50);

         } /* end_for CurrPage */


      } /* end_for CurrBlk */


      /*! Increment data values */
      j++;

      F_DebugDelay(5000000);



   } /* end_for CurrFlsDev */

   return (0);

} /* end main() */

#endif /* TEST_PROGRAM */


/**************************************************************************
* Function Name:  NFC_F_EraseBlk
*
* Description:    Erases a NAND Flash Block.
*                 If the Block is Bad (contains faulty Pages), the actual
*                 erasure is skipped.
*                 If erasure fails, the Block is added to the chip's bad-blocks
*                 list.
*
* Input:          FlsDev:     Flash Device number.
*                 Block:      Block number.
*
* Output:         None.
*
* Return Value:   NFC_K_Err_OK:            Erase OK.
*                 NFC_K_Err_InvalidChip:   Invalid chip.
*                 NFC_K_Err_EraseFail:     Erase failed.
*                 NFC_K_Err_DfctBlk:       Defected Block.
*                 NFC_K_Err_InvalidParam:  Invalid Parameter.
*                 NFC_K_Err_SwErr:         Software error.
***************************************************************************/
#ifdef TEST_PROGRAM
NFC_E_Err NFC_F_EraseBlk(NFC_E_FlsDev FlsDev, __u16 Block)
{
   NFC_E_Err    Stat;
   NFC_E_Chip   ChipType;

   /*! CHECK parameters */
   if (FlsDev >= NFC_K_FlsDevMAX)
   {
      return (NFC_K_Err_InvalidParam);
   }
   if (V_FlsDevTbl[FlsDev].ChipValid == false)
   {
      return (NFC_K_Err_InvalidChip);
   }

   ChipType = V_FlsDevTbl[FlsDev].ChipType;
   if ((ChipType <= NFC_K_Chip_None) || (ChipType >= NFC_K_Chip_MAX))
   {
      return (NFC_K_Err_SwErr);
   }

   if (Block >= V_NFlsParms[ChipType].NumBlocks)
   {
      return (NFC_K_Err_InvalidParam);
   }

   /*! CHECK Block validity */
   if (F_IsBlkOk(FlsDev, Block) == false)
   {
      return (NFC_K_Err_DfctBlk);
   }

   Stat = F_CmdEraseBlkBypass(ChipType, FlsDev, Block);
   if (Stat != NFC_K_Err_OK)
   {
      /*! ADD block to bad-block list */
      F_AddBadBlk(FlsDev, Block);
      return (NFC_K_Err_EraseFail);
   }

   /*! RETURN Ok */
   return (NFC_K_Err_OK);

} /* end NFC_F_EraseBlk() */
#endif // TEST_PROGRAM
/**************************************************************************
* Function Name:  NFC_F_ProgPage
*
* Description:    Programs a whole Physical(!) Page, including its Spare area.
*                 If the Page belongs to a Bad Block, the actual programming is skipped.
*                 If programming fails, the Block is added to the chip's bad-blocks
*                 list.
*
* Input:          FlsDev:     Flash Device number.
*                 Block:      Block number.
*                 PageInBlk:  Page offset within the block.
*                 N1Len:      Ecc N1 field length.
*                 IsEccF:     Whether to activate ECC machine.
*                 BypassDmaF: Whether to Bypass DMA&FIFO.
*                 DataP:      User's buffer holding the net-Page Data (w/o the spare data).
*                 SpareP:     User's buffer holding the Spare Data.
*
* Output:         None.
*
* Return Value:   NFC_K_Err_OK:            Erase OK.
*                 NFC_K_Err_InvalidChip:   Invalid chip.
*                 NFC_K_Err_ProgFail:      Programming failed.
*                 NFC_K_Err_DfctBlk:       Defected Block.
*                 NFC_K_Err_InvalidParam:  Invalid Parameter.
*                 NFC_K_Err_SwErr:         Software error.
*
* NOTE:
*  1. Under FPGA emulation of the DM56 And DMA is Used (BypassDmaF is false),
*     Page (DataP) and Spare (SpareP) buffers MUST be allocated within the FPGA RAM,
*     due to the DMA transfer limitation (not transferring over the AHB bus, just inside
*     the FPGA).
***************************************************************************/
NFC_E_Err  NFC_F_ProgPage( NFC_E_FlsDev  FlsDev,
                           __u16        Block,
                           __u8         PageInBlk,
                           __u8         N1Len,
                           bool          IsEccF,
                           bool          BypassDmaF,
                           void *        DataP,
                           void *        SpareP )
{
   NFC_E_Err    Stat=0;
   NFC_E_Chip   ChipType;


   /*! CHECK parameters */
   if (FlsDev >= NFC_K_FlsDevMAX)
   {
      return (NFC_K_Err_InvalidParam);
   }
   if (V_FlsDevTbl[FlsDev].ChipValid == false)
   {
      return (NFC_K_Err_InvalidChip);
   }

   ChipType = V_FlsDevTbl[FlsDev].ChipType;
   if ((ChipType <= NFC_K_Chip_None) || (ChipType >= NFC_K_Chip_MAX))
   {
      return (NFC_K_Err_SwErr);
   }

   if (Block >= V_NFlsParms[ChipType].NumBlocks)
   {
      return (NFC_K_Err_InvalidParam);
   }

   /*! CHECK Block validity */
   if (F_IsBlkOk(FlsDev, Block) == false)
   {
      return (NFC_K_Err_DfctBlk);
   }

   if (BypassDmaF == false)
   {
      //Stat = F_CmdWritePageAndSpare( ChipType, FlsDev, Block, PageInBlk, N1Len, IsEccF, DataP, SpareP );
   }
   else
   {
      //Stat = F_CmdWritePageAndSpareBypass( ChipType, FlsDev, Block, PageInBlk, N1Len, IsEccF, DataP, SpareP );
   }

   if (Stat != NFC_K_Err_OK)
   {
      /*! ADD block to bad-block list */
      F_AddBadBlk(FlsDev, Block);
      return (NFC_K_Err_ProgFail);
   }

   /*! RETURN Ok */
   return (NFC_K_Err_OK);

} /* end NFC_F_ProgPage() */


/**************************************************************************
* Function Name:  NFC_F_ReadPage
*
* Description:    Reads a whole Physical(!) Page, including its Spare area.
*                 Reading is allowed even when the Page belongs to a bad Block.
*
* Input:          FlsDev:     Flash Device number.
*                 Block:      Block number.
*                 PageInBlk:  Page offset within the block.
*                 N1Len:      Ecc N1 field length.
*                 IsEccF:     Whether to activate ECC machine.
*                 BypassDmaF: Whether to Bypass DMA&FIFO.
*
* Output:         DataP:      User's buffer to hold the returned net-Page Data (w/o the spare data).
*                 SpareP:     User's buffer to hold the returned page's Spare Data.
*
* Return Value:   NFC_K_Err_OK:            Erase OK.
*                 NFC_K_Err_InvalidChip:   Invalid chip.
*                 NFC_K_Err_TimeOut:       Read Timeout.
*                 NFC_K_Err_InvalidParam:  Invalid Parameter.
*                 NFC_K_Err_SwErr:         Software error.
*
* NOTE:
*  1. Under FPGA emulation of the DM56 And DMA is Used (BypassDmaF is false),
*     Page (DataP) and Spare (SpareP) buffers MUST be allocated within the FPGA RAM,
*     due to the DMA transfer limitation (not transferring over the AHB bus, just inside
*     the FPGA).
***************************************************************************/
NFC_E_Err  NFC_F_ReadPage( NFC_E_FlsDev     FlsDev,
                           __u16           Block,
                           __u8            PageInBlk,
                           __u8            N1Len,
                           bool             IsEccF,
                           bool             BypassDmaF,
                           void *           DataP,
                           void *           SpareP )
{
   NFC_E_Err    Stat=0;
   NFC_E_Chip   ChipType;


   /*! CHECK parameters */
   if (FlsDev >= NFC_K_FlsDevMAX)
   {
      return (NFC_K_Err_InvalidParam);
   }
   if (V_FlsDevTbl[FlsDev].ChipValid == false)
   {
      return (NFC_K_Err_InvalidChip);
   }

   ChipType = V_FlsDevTbl[FlsDev].ChipType;
   if ((ChipType <= NFC_K_Chip_None) || (ChipType >= NFC_K_Chip_MAX))
   {
      return (NFC_K_Err_SwErr);
   }

   if (Block >= V_NFlsParms[ChipType].NumBlocks)
   {
      return (NFC_K_Err_InvalidParam);
   }

   if (BypassDmaF == false)
   {
	printk("NFC_F_ReadPage: skipping F_CmdReadPageAndSpare\n");
      //Stat = F_CmdReadPageAndSpare( ChipType, FlsDev, Block, PageInBlk, N1Len, IsEccF, DataP, SpareP );
   }
   else
   {
      printk("NFC_F_ReadPage: skipping F_CmdReadPageAndSpareBypass\n");
      //Stat = F_CmdReadPageAndSpareBypass( ChipType, FlsDev, Block, PageInBlk, N1Len,IsEccF, DataP, SpareP );
   }

   if (Stat != NFC_K_Err_OK)
   {
      /*! ADD block to bad-block list */
      F_AddBadBlk(FlsDev, Block);
      return (NFC_K_Err_TimeOut);
   }

   /*! RETURN Ok */
   return (NFC_K_Err_OK);

} /* end NFC_F_ReadPage() */

/**************************************************************************
* Function Name:  NFC_F_ChipType
*
* Description:    Returns the actual NAND Flash Chip "behind" the Flash Device.
*
* Input:          FlsDev:  Flash Device number.
*
* Output:         None.
*
* Return Value:   The actual chip, by NFC_E_Chip enumaration.
*                 Of failure, NFC_K_Chip_None is returned.
***************************************************************************/
NFC_E_Chip  NFC_F_ChipType( NFC_E_FlsDev  FlsDev )
{
   NFC_E_Chip   ChipType;


   /*! CHECK parameters */
   if (FlsDev >= NFC_K_FlsDevMAX)
   {
      return (NFC_K_Chip_None);
   }
   if (V_FlsDevTbl[FlsDev].ChipValid == false)
   {
      return (NFC_K_Chip_None);
   }

   ChipType = V_FlsDevTbl[FlsDev].ChipType;
   if ((ChipType <= NFC_K_Chip_None) || (ChipType >= NFC_K_Chip_MAX))
   {
      return (NFC_K_Chip_None);
   }

   return (ChipType);

} /* end NFC_F_ChipType() */


/**************************************************************************
* Function Name:  NFC_F_BadBlkMarkPos
*
* Description:    Returns the Position of the Bad Block mark in the Spare
*                 Area.
*
*                 The Bad-Block Mark is a Mark that specifies that the Block
*                 is deteriorated and should Not(!) be further Programmed or Erased.
*                 Pages in such a Block should only be Read in order to retrieve there
*                 (probably faulty) data.
*
*                 This Mark(s) appear in the First or Second (or both) Pages of a Block.
*                 For a 8-bit chip, the Mark is one Byte (8bits) long. Any value Other Than
*                 0xFF determined that the Block is Faulty.
*                 For a 16-bit chip, the Mark is one Word (16bits) long. Any value Other Than
*                 0xFFFF determined that the Block is Faulty.
*
*                 The Position of the Mark depends on the chip. It is usually on the 1st or 6th
*                 Byte/Word of the Spare aera.
*                 For Example:
*                     If the Mark is on the 6th Byte on 8bit chip,
*                     then: if (Spare[5] != 0xFF) { Block is Bad }.
*
*                     If the Mark is on the 1st Word on 16bit chip,
*                     then: if (Spare[0] == 0xFFFF) { Block is OK }.
*
*
* Input:          FlsDev:  Flash Device number.
*
* Output:         None.
*
* Return Value:   Position of the Bad-Block Mark Byte/Word in the Spare area.
*                 In case of failure, 0 is returned.
*                 (0 is not a valid Position value; should be 1 or greater).
*
***************************************************************************/
__u16  NFC_F_BadBlkMarkPos( NFC_E_FlsDev  FlsDev )
{
   NFC_E_Chip   ChipType;


   /*! CHECK parameters */
   if (FlsDev >= NFC_K_FlsDevMAX)
   {
      return (0);
   }
   if (V_FlsDevTbl[FlsDev].ChipValid == false)
   {
      return (0);
   }

   ChipType = V_FlsDevTbl[FlsDev].ChipType;
   if ((ChipType <= NFC_K_Chip_None) || (ChipType >= NFC_K_Chip_MAX))
   {
      return (0);
   }

   /*! RETURN Bad-Block-Mark position */
   return (V_NFlsParms[ChipType].BadBlkMarkPos);

} /* end NFC_F_BadBlkMarkPos() */



/**************************************************************************
* Function Name:  NFC_F_NumTotalPages
*
* Description:    Returns the total(!) number of Pages in the chip
*                 (sum of all pages in all blocks).
*
* Input:          FlsDev:  Flash Device number.
*
* Output:         None.
*
* Return Value:   Total number of Pages in the chip..
*                 In case of failure, 0 is returned.
***************************************************************************/
__u32  NFC_F_NumTotalPages( NFC_E_FlsDev  FlsDev )
{
   NFC_E_Chip   ChipType;


   /*! CHECK parameters */
   if (FlsDev >= NFC_K_FlsDevMAX)
   {
      return (0);
   }
   if (V_FlsDevTbl[FlsDev].ChipValid == false)
   {
      return (0);
   }

   ChipType = V_FlsDevTbl[FlsDev].ChipType;
   if ((ChipType <= NFC_K_Chip_None) || (ChipType >= NFC_K_Chip_MAX))
   {
      return (0);
   }

   /*! RETURN total number of Pages */
   return (V_NFlsParms[ChipType].NumTotPages);

} /* end NFC_F_NumTotalPages() */
/**************************************************************************
* Function Name:  NFC_F_GetFlsStatus
*
* Description:    Gets the current Flash chip status.
*
* Input:          FlsDev:  Flash Device number.
*
* Output:         None.
*
* Return Value:   true:  Device is Ready.
*                 false: Device is Busy or Error.
***************************************************************************/
#ifdef UNUSED_CODE
bool  NFC_F_GetFlsStatus( NFC_E_FlsDev  FlsDev )
{
   __u8  ProgEraseOk;
   __u8  ReadyBusy;
   bool   ReadyStat;


   /*! CHECK parameters */
   if (FlsDev >= NFC_K_FlsDevMAX)
   {
      return (false);
   }
   if (V_FlsDevTbl[FlsDev].ChipValid == false)
   {
      return (false);
   }

   F_CmdReadDeviceStatBypass( FlsDev, &ProgEraseOk, &ReadyBusy );
   ReadyStat = (ReadyBusy == 0x00) ? false : true;

   return (ReadyStat);

} /* end NFC_F_GetFlsStatus() */
#endif


/**************************************************************************
* Function Name:  NFC_F_GetBadBlocks
*
* Description:    Returns a list of Bad Blocks in the Flash device.
*
* Input:          FlsDev:  Flash Device number.
*
* Output:         NumBadBlocksP: Returns the number of Bad Blocks.
*                 BadBlockListP: Returns a list of Bad Blocks.
*
* Return Value:   NFC_K_Err_OK:            OK - List of Bad blocks was built.
*                 NFC_K_Err_InvalidParam:  Invalid parameter.
*                 NFC_K_Err_InvalidChip:   Invalid chip.
*
* NOTE: User supplied BadBlockListP must point to a big enough allocated
*       memory area - The maximum number of allowed Bad Blocks per chip type
*       (at least 64 Bytes !
*
***************************************************************************/
NFC_E_Err  NFC_F_GetBadBlocks( NFC_E_FlsDev  FlsDev,
                               __u16 *      NumBadBlocksP,
                               __u16 *      BadBlockListP )
{
   __u16 *  ListP;
   __u16 *  BadBlkArrP;
   __u16    i;


   ListP = BadBlockListP;
   *NumBadBlocksP = 0;

   if (FlsDev >= NFC_K_FlsDevMAX)
   {
      /*! RETURN badly */
      return (NFC_K_Err_InvalidParam);
   }

   if (V_FlsDevTbl[FlsDev].ChipValid == false)
   {
      return (NFC_K_Err_InvalidChip);
   }

   *NumBadBlocksP = V_FlsDevTbl[FlsDev].BadBlkTbl.NumBadBlks;
   for ( i = 0, BadBlkArrP = V_FlsDevTbl[FlsDev].BadBlkTbl.BadBlkArrP;
         i < *NumBadBlocksP;
         i++ )
   {
      *ListP++ = *BadBlkArrP++;
   }

   return (NFC_K_Err_OK);

} /* end NFC_F_GetBadBlocks() */


/**************************************************************************
*              L O C A L      F U N C T I O N S
***************************************************************************/

/**************************************************************************
* Function Name:  F_Init
*
* Description:    Initializes Flash controller registers, which have to
*                 set once only.
*                 Identifies the mounted NAND Flash chips and determines
*                 their bad blocks.
*
* Input:          None.
* Output:         None.
* Return Value:   None.
***************************************************************************/
#ifdef TEST_PROGRAM
void F_Init(Y_FlsDevId *FlsId)
{
   NFC_E_Chip   ChipType;
   NFC_E_FlsDev FlsDev;
//   __u16        MaxNumBadBlks;
//   __u16        ActualNumBadBlks;


   /*! CLEAR controller Status bits */
   writel(SET_32BIT, (DW_NAND_LCD_BASE + DW_FC_STS_CLR));

   /*! CONFIGURE one-time-set controller registers: */
   F_SetFixedCntrlRegs();

   /*! CREATE Flash devices table - LIMIT for one NAND flash device*/
   for (FlsDev = NFC_K_FlsDev0; FlsDev < NFC_K_FlsDevMAX - 1; FlsDev++)
   {
      /*! GET Flash-chip Manufacture & Device Id */
      F_CmdReadDeviceIdBypass(FlsDev, FlsId);
   
      /*! TRANSLATE to chip type */
      ChipType = F_MatchChipByDevId(FlsId->MnfctrCde, FlsId->DvcId);
      V_FlsDevTbl[FlsDev].ChipType = ChipType;
      V_FlsDevTbl[FlsDev].ChipValid = true; // R.B.: bypass own bbt (see original logic below).
DW_SPOT("ChipType = %d", ChipType);
#if 0
      /*! CREATE Bad Blocks table */
      if (ChipType != NFC_K_Chip_None)
      {
         MaxNumBadBlks = V_NFlsParms[ChipType].MaxBadBlks;
         V_FlsDevTbl[FlsDev].BadBlkTbl.BadBlkArrP = 
             kmalloc(sizeof(__u16) * MaxNumBadBlks, GFP_KERNEL);
/*
#ifdef PC_SIMULATION
               (__u16 *)malloc(MaxNumBadBlks * sizeof(__u16));
#else
               &BadBlkArr[FlsDev][0];
#endif // ! PC_SIMULATION
*/

         /*! GET chip's bad-blocks list */
         F_ListBadBlocks( ChipType, FlsDev, V_FlsDevTbl[FlsDev].BadBlkTbl.BadBlkArrP,
                          &ActualNumBadBlks );
         V_FlsDevTbl[FlsDev].ChipValid = (ActualNumBadBlks <= MaxNumBadBlks) ? true : false;
      }
      else
      {  // Missing or unidentified chip */
         V_FlsDevTbl[FlsDev].ChipValid = false;
      }
#endif //0
   } /* end_for FlsDev */
}
#endif // TEST_PROGRAM


/**************************************************************************
* Function Name:  F_ClrCntrlStatReg
*
* Description:    Clears Status bits (of former transaction) of the flash
*                 Controller
*
* Input:          None.
* Output:         None.
* Return Value:   None.
***************************************************************************/
/*static*/ void  F_ClrCntrlStatReg( void )
{
   Y_FC_StatClr StatClrImg;

   StatClrImg.ClrRdy2RiseStat = C_BitClear;
   StatClrImg.ClrRdy1RiseStat = C_BitClear;
   StatClrImg.ClrRdyTimeOut = C_BitClear;
   StatClrImg.ClrChainSrcErrFnd = C_BitClear;
   StatClrImg.ClrEccDone = C_BitClear;
   StatClrImg.ClrTransDone = C_BitClear;

   /*DW_FC_SET(DW_NAND_LCD_BASE, DW_FC_ADDR_COL,DW_FC_STATUS_ECC_DONE,
             DW_FC_STATUS_ERR_FOUND,DW_FC_STATUS_RDY_TIMEOUT,
             DW_FC_STATUS_RDY1_RISE, DW_FC_STATUS_RDY2_RISE,0,0,0,0,0);*/

} /* end F_ClrCntrlStatReg() */

#ifdef UNUSED_CODE
/**************************************************************************
* Function Name:  F_SetChipSelectAndReadyLine
*
* Description:    Chooses the right Chip-Select and Ready line.
*                 There are 4 chip-selects lines for 4 possible NAND Flash
*                 chips. There are 2 Ready lines:
*                        Ready1: Wire-Or of first & second chip.
*                        Ready2: Wire-Or of third & fourth chip.
*
* Input:          FlsDev:  Flash Device number.
*
* Output:         FC_CmdSeqImgP:  Pointer to returned Y_FC_CmdSeq structure,
*                                 where RdySlct and ChipSlct fields are
*                                 correctly set.
*
* Return Value:   None.
*
* NOTE: In FPGA emulation board, only 2 CS and 1 Ready line are implemented.
***************************************************************************/
/*static*/ void inline F_SetChipSelectAndReadyLine( NFC_E_FlsDev  FlsDev, Y_FC_CmdSeq *  FC_CmdSeqImgP )
{
   switch (FlsDev)
   {
      // Choose Ready line & Chip-Select
      case NFC_K_FlsDev0:
         FC_CmdSeqImgP->RdySlct = C_FC_CmdSeq_RdySlct_Rdy1;
         FC_CmdSeqImgP->ChipSlct = C_FC_CmdSeq_ChipSlct_Dev0;
         break;
      case NFC_K_FlsDev1:
         FC_CmdSeqImgP->RdySlct = C_FC_CmdSeq_RdySlct_Rdy1;
         FC_CmdSeqImgP->ChipSlct = C_FC_CmdSeq_ChipSlct_Dev1;
         break;

#ifndef DM56_SIM_ON_FPGA
      case NFC_K_FlsDev2:
         FC_CmdSeqImgP->RdySlct = C_FC_CmdSeq_RdySlct_Rdy2;
         FC_CmdSeqImgP->ChipSlct = C_FC_CmdSeq_ChipSlct_Dev2;
         break;
      case NFC_K_FlsDev3:
         FC_CmdSeqImgP->RdySlct = C_FC_CmdSeq_RdySlct_Rdy2;
         FC_CmdSeqImgP->ChipSlct = C_FC_CmdSeq_ChipSlct_Dev3;
         break;
#endif // ! DM56_SIM_ON_FPGA
   }

} /* end F_SetChipSelectAndReadyLine() */
#endif

#if 0
/**************************************************************************
* Function Name:  F_ListBadBlocks
*
* Description:    Finds and makes a list of all Bad Blocks in a chip,
*                 as marked by the manufacturer.
*
* Input:          ChipType:  Flash chip identity.
*                 FlsDev:    Flash Device number.
*
* Output:         BadBlocksArr:   Array of bad-blocks numbers, in user supplied array.
*                 NumBadBlocksP:  Number of bad blocks found.
*
* Return Value:   None.
***************************************************************************/
/*static*/ void  F_ListBadBlocks( NFC_E_Chip    ChipType,
                              NFC_E_FlsDev  FlsDev,
                              __u16 *      BadBlocksArr,
                              __u16 *      NumBadBlocksP )
{
   __u16        CurrBlk;
   __u16        NumBlocks;
   __u16        MaxAllowedBadBlocks;
   E_FlsAccess   FlsAccess;
   __u16 *      BadBlockP;

   __u16        NumActualBadBlcks;
   __u16        BadMarkVal;
   __u8         Page;
   bool          BlockOk;

   const Y_NFlsParms *  NFlsParmsP;


   NFlsParmsP = &V_NFlsParms[ChipType];
   NumBlocks = NFlsParmsP->NumBlocks;
   MaxAllowedBadBlocks = NFlsParmsP->MaxBadBlks;
   FlsAccess = NFlsParmsP->AccMode;
   BadBlockP = BadBlocksArr;

   /*! GO over all chip's Blocks */
   NumActualBadBlcks = 0;
   for (CurrBlk = 0; CurrBlk < NumBlocks; CurrBlk++)
   {
      /*! READ the BadBlockMark area of First & Second Page in Block */
      for (Page = 0; Page < 2; Page++)
      {
         F_CmdReadBadBlockMarkBypass( ChipType, FlsDev, CurrBlk, Page, &BadMarkVal );
         /*! Determine Block validity */
         if (FlsAccess == K_FlsAccess_8bit)
         {  /* 8 bit flash */
            BlockOk = ((__u8)BadMarkVal == (__u8)0xFF) ? true : false;
         }
         else
         {  /* 16 bit flash */
            BlockOk = ((__u16)BadMarkVal == (__u16)0xFFFF) ? true : false;
         }

         /* NOTE: If bad-mark on First page, Skip testing Second page !
         **       (Don't add to the list the same block twice) */
         if (BlockOk == false)
         {
            break;
         }
      }  /* end_for Page */

      if (BlockOk == false)
      {
         *BadBlockP++ = CurrBlk;
         NumActualBadBlcks++;
         if (NumActualBadBlcks >= MaxAllowedBadBlocks)
         {
            /* Too many bad blocks --> Faulty chip */
            //break;
             printk("\nTOO MANY BAD BLOCKS!!!!\n");
         }
      }
   } /* end_for CurrBlk */

   *NumBadBlocksP = NumActualBadBlcks;

} /* end F_ListBadBlocks() */
#endif

/**************************************************************************
* Function Name:  F_IsBlkOk
*
* Description:    Searches the bad blocks list to check whether the specified
*                 block is Fualty or Ok.
*
* Input:          FlsDev: The Flash device number.
*                 Blk:    The block to test its validity.
*
* Output:         None.
*
* Return Value:   true: Block is Valid; false: Block is Bad.
***************************************************************************/
/*static*/ bool  F_IsBlkOk( NFC_E_FlsDev  FlsDev, __u16  Blk)
{
   __u16         NumBadBlks;
   __u16         CurrBlk;
   __u16 *       CurrBlkNumP;
   Y_FlsDevTbl *  FlsDevTblP;


   FlsDevTblP = &V_FlsDevTbl[FlsDev];
   NumBadBlks = FlsDevTblP->BadBlkTbl.NumBadBlks;

   if (FlsDevTblP->ChipValid == false)
   {
       printk("\n%s %d dw_nand: CHIP NOT VALID!!!!!\n", __FILE__, __LINE__);
       return (false);

   }

   for ( CurrBlk = 0, CurrBlkNumP = FlsDevTblP->BadBlkTbl.BadBlkArrP;
         CurrBlk < NumBadBlks;
         CurrBlk++, CurrBlkNumP++)
   {
      if (Blk == *CurrBlkNumP)
      {
          printk("%s %d dw_nand: FOUND BAD BLOCK!!!!!!!!!!\n", __FILE__, __LINE__);
         /*! Block found in bad-blocks list - RETURN Faulty */
          return (false);
      }
   }

   /*! RETURN Ok */
   return (true);

} /* end F_IsBlkOk() */


/**************************************************************************
* Function Name:  F_AddBadBlk
*
* Description:    Adds a block to the device's list of Bad Blocks.
*
* Input:          FlsDev: The Flash device number.
*                 Blk:    The block to test its validity.
*
* Output:         None.
* Return Value:   None.
***************************************************************************/
/*static*/ void  F_AddBadBlk( NFC_E_FlsDev  FlsDev, __u16  Blk)
{
   __u16         MaxBadBlks;
   __u16         NumBadBlks;
   Y_FlsDevTbl *  FlsDevTblP;


   if (F_IsBlkOk(FlsDev, Blk) != false)
   {
      /*! Block is already marked as Bad - RETURN */
      return;
   }

   FlsDevTblP = &V_FlsDevTbl[FlsDev];
   NumBadBlks = FlsDevTblP->BadBlkTbl.NumBadBlks;
   MaxBadBlks = V_NFlsParms[ FlsDevTblP->ChipType ].MaxBadBlks;

   if (NumBadBlks < MaxBadBlks)
   {
      *(FlsDevTblP->BadBlkTbl.BadBlkArrP + NumBadBlks) = Blk;
      FlsDevTblP->BadBlkTbl.NumBadBlks++;
   }

} /* end F_AddBadBlk() */


/**************************************************************************
* Function Name:  F_CmdReadBadBlockMarkBypass
*
* Description:    Reads the Bytes/Words in the Spare area, where the manufacturer
*                 has written the Bad Block marking, bypassing the DAM and FIFO.
*
* Input:          Chip:       Flash chip identity.
*                 FlsDev:     Flash Device number.
*                 Block:      Block number.
*                 PageInBlk:  Number of Page inside the Block.
*
* Output:         MarkAreaP:  The value of the Bad Block Marking area.
*
* Return Value:   None.
*
* NOTE: Value returned at *MarkAreaP:
*        should be (__u8)0xFF for 8bit chip, (__u16)0xFFFF for 16bit chip.
*        Oterwise, the block is Bad.
***************************************************************************/
#if 0
/*static*/ void  F_CmdReadBadBlockMarkBypass( NFC_E_Chip    Chip,
                                          NFC_E_FlsDev  FlsDev,
                                          __u16        Block,
                                          __u8         PageInBlk,
                                          __u16 *      MarkAreaP )

{
//   Y_FC_Ctrl  FC_CtrlImg;
//   Y_FC_CmdSeq   FC_CmdSeqImg;

   __u16     FlashData;

   down(&unified_dw_nflc_mutex);
   F_SetFixedCntrlRegs();

//   *((__u32 *)&FC_CtrlImg) = 0;
//   *((__u32 *)&FC_CmdSeqImg) = 0;
   V_FlsCycle1 = 0;
   V_FlsCycle2 = 0;
   V_FlsCycle3 = 0;
   V_FlsCycle4 = 0;
   V_FlsCycle5 = 0;

   /*! DISABLE DMA & FIFO */
   DMF_F_ConfDmaBypass(&nand_info->dma_regs, DMF_K_PeriphFlsCntl);

   /*! PREPARE Manufacture & Device Id read, using 16bit width */
   // Read DeviceId command
   switch (Chip)
   {
      // 8-bit devices
      case NFC_K_Chip_SamK9F5608:
         V_FlsCmnd1 = C_SamK9F56_FlsCmdReadSpare;
         // 1st Cycle: Column Addr: Byte-offset in Page's Spare area.
         V_FlsCycle1 = 5; /* byte #517 - the 6th byte of Spare area */
                                                           // b7-b0: A7 - A0; A7-A4: don't care. A8 discarded!
         // 2nd Cycle: 1st Row Addr: Page-In-Block, Block's 3 LSBits
         V_FlsCycle2 = PageInBlk & 0x1F;                   // b4-b0: A13 - A9
         V_FlsCycle2 |= ((__u8)Block & 0x07) << 5;        // b7-b5: A16 - A14
         // 3rd Cycle: 2nd Row Addr: Block's 8 MSBits
         V_FlsCycle3 = ((__u8)Block & 0xF8) >> 3;         // b4-b0: A21 - A17
         V_FlsCycle3 |= ((__u8)(Block >> 8) & 0x07) << 5; // b7-b5: A24 - A22
         break;

      case NFC_K_Chip_TosTH58DVG02: // R.B.: TODO: may be unified with NFC_K_Chip_SamK9F5616 etc.
         V_FlsCmnd1 = C_SamK9F56_FlsCmdReadSpare;
         // 1st Cycle: Column Addr: Word-offset in Page's Spare area.
         V_FlsCycle1 = 5; /* word #517 - the 6th word of Spare area */
                                                           // b7-b0: A7 - A0; A7-A3: '0'. A8 discarded!
         // 2nd Cycle: 1st Row Addr: Page-In-Block, Block's 3 LSBits
         V_FlsCycle2 = PageInBlk & 0x1F;                   // b4-b0: A13 - A9
         V_FlsCycle2 |= ((__u8)Block & 0x07) << 5;        // b7-b5: A16 - A14
         // 3rd Cycle: 2nd Row Addr: Block's 8 MSBits
         V_FlsCycle3 = ((__u8)Block & 0xF8) >> 3;         // b4-b0: A21 - A17
         V_FlsCycle3 |= ((__u8)(Block >> 8) & 0x07) << 5; // b7-b5: A24 - A22
         // 4th Cycle: 2nd Row Addr: Block's next 8 Bits
         V_FlsCycle4 = ((__u8)(Block >> 16) & 0x03);      // b7-b5: A26 - A25
         break;

      case NFC_K_Chip_SamK9F1G08:
         V_FlsCmnd1 = C_SamK9F2G_FlsCmdRead1Cyc;
         V_FlsCmnd2 = C_SamK9F2G_FlsCmdRead2Cyc;
         // 1st Cycle: First Column Addr:  Byte-offset in Page, 8 LSBits
         V_FlsCycle1 = (__u8)2048; /* byte #2048 - the 1th byte of Spare area */
         // 2st Cycle: Second Column Addr: Byte-offset in Page, 4 MSBits
         V_FlsCycle2 = (__u8)(2048 >> 8);                 // b3-b0: A11 - A8; = 0x08 for 2048
         V_FlsCycle2 &= 0x0F;
         // 3rd Cycle: 1st Row Addr: Page-In-Block, Block's 2 LSBits
         V_FlsCycle3 = PageInBlk & 0x3F;                   // b5-b0: A17 - A12
         V_FlsCycle3 |= ((__u8)Block & 0x03) << 6;        // b7-b6: A19 - A18
         // 4th Cycle: 2nd Row Addr: Block's next 8 Bits
         V_FlsCycle4 = (__u8)(Block >> 2);                // b7-b0: A27 - A20
         // 5th Cycle: 3rd Row Addr: Block's 1 MSBits - 5-th cycle is discarded
         V_FlsCycle5 = (__u8)(Block >> 10);               //    b1: A28
         V_FlsCycle5 &= 0x01;
         break;

      case NFC_K_Chip_SamK9F2G08:
         V_FlsCmnd1 = C_SamK9F2G_FlsCmdRead1Cyc;
         V_FlsCmnd2 = C_SamK9F2G_FlsCmdRead2Cyc;
         // 1st Cycle: First Column Addr:  Byte-offset in Page, 8 LSBits
         V_FlsCycle1 = (__u8)2048; /* byte #2048 - the 1th byte of Spare area */
         // 2st Cycle: Second Column Addr: Byte-offset in Page, 4 MSBits
         V_FlsCycle2 = (__u8)(2048 >> 8);                 // b3-b0: A11 - A8; = 0x08 for 2048
         V_FlsCycle2 &= 0x0F;
         // 3rd Cycle: 1st Row Addr: Page-In-Block, Block's 2 LSBits
         V_FlsCycle3 = PageInBlk & 0x3F;                   // b5-b0: A17 - A12
         V_FlsCycle3 |= ((__u8)Block & 0x03) << 6;        // b7-b6: A19 - A18
         // 4th Cycle: 2nd Row Addr: Block's next 8 Bits
         V_FlsCycle4 = (__u8)(Block >> 2);                // b7-b0: A27 - A20
         // 5th Cycle: 3rd Row Addr: Block's 1 MSBits
         V_FlsCycle5 = (__u8)(Block >> 10);               //    b1: A28
         V_FlsCycle5 &= 0x01;
         break;

      case NFC_K_Chip_TosTH58NVG3:
         V_FlsCmnd1 = C_SamK9F2G_FlsCmdRead1Cyc;
         V_FlsCmnd2 = C_SamK9F2G_FlsCmdRead2Cyc;
         // 1st Cycle: First Column Addr:  Byte-offset in Page, 8 LSBits
         V_FlsCycle1 = (__u8)2048; /* byte #2048 - the 1th byte of Spare area */
         // 2st Cycle: Second Column Addr: Byte-offset in Page, 4 MSBits
         V_FlsCycle2 = (__u8)(2048 >> 8);                 // b3-b0: CA11 - CA8; = 0x08 for 2048
         V_FlsCycle2 &= 0x0F;
         // 3rd Cycle: 1st Row Addr: Page-In-Block, Block's 1 LSBit
         V_FlsCycle3 = PageInBlk & 0x7F;                   // b6-b0: PA6 - PA0
         V_FlsCycle3 |= ((__u8)Block & 0x01) << 7;        //    b7: PA7
         // 4th Cycle: 2nd Row Addr: Block's next 8 Bits
         V_FlsCycle4 = (__u8)(Block >> 1);                // b7-b0: PA15 - PA8
         // 5th Cycle: 3rd Row Addr: Block's 3 MSBits
         V_FlsCycle5 = (__u8)(Block >> 9);                //    b1: PA18
         V_FlsCycle5 &= 0x07;
         break;

      // 16-bit devices
      case NFC_K_Chip_SamK9F5616:
         V_FlsCmnd1 = C_SamK9F56_FlsCmdReadSpare;
         // 1st Cycle: Column Addr: Word-offset in Page's Spare area.
         V_FlsCycle1 = 0; /* word #256 - the 1st word of Spare area */
                                                           // b7-b0: A7 - A0; A7-A3: '0'. A8 discarded!
         // 2nd Cycle: 1st Row Addr: Page-In-Block, Block's 3 LSBits
         V_FlsCycle2 = PageInBlk & 0x1F;                   // b4-b0: A13 - A9
         V_FlsCycle2 |= ((__u8)Block & 0x07) << 5;        // b7-b5: A16 - A14
         // 3rd Cycle: 2nd Row Addr: Block's 8 MSBits
         V_FlsCycle3 = ((__u8)Block & 0xF8) >> 3;         // b4-b0: A21 - A17
         V_FlsCycle3 |= ((__u8)(Block >> 8) & 0x07) << 5; // b7-b5: A24 - A22
         break;

      case NFC_K_Chip_SamK9F1G16:
         V_FlsCmnd1 = C_SamK9F2G_FlsCmdRead1Cyc;
         V_FlsCmnd2 = C_SamK9F2G_FlsCmdRead2Cyc;
         // 1st Cycle: First Column Addr:  Word-offset in Page, 8 LSBits
         V_FlsCycle1 = (__u8)1024; /* word #1024 - the 1th word of Spare area */
         // 2st Cycle: Second Column Addr: Word-offset in Page, 3 MSBits
         V_FlsCycle2 = (__u8)(1024 >> 8);                 // b2-b0: A10 - A8; = 0x04 for 1024
         V_FlsCycle2 &= 0x07;
         // 3rd Cycle: 1st Row Addr: Page-In-Block, Block's 2 LSBits
         V_FlsCycle3 = PageInBlk & 0x3F;                   // b5-b0: A16 - A11;
         V_FlsCycle3 |= ((__u8)Block & 0x03) << 6;        // b7-b6: A18 - A17
         // 4th Cycle: 2nd Row Addr: Block's next 8 Bits
         V_FlsCycle4 = (__u8)(Block >> 2);                // b7-b0: A26 - A19
         break;

      default:
	 DW_SPOT("Unknown chip: %0#10x, driver is stuck to prevent system crash.", Chip);
         while (1);
         break;
   } /* end_switch Chip */

   /*! CONFIGURE Flash-Controller registers: */
   /* NOTE: Image of FC_Ctrl register is needed, because a write to
   **       this register triggers a Flash-Controller transaction. */
   /*FC_CtrlImg.ChainCntStart = 0;
   FC_CtrlImg.PageSelect = 0;
   FC_CtrlImg.EccOpMode = C_FC_Ctrl_EccOpMode_MemTrans; // Memory transaction*/

   // Clear relevant bits in Status register
   writel(SET_32BIT, (DW_NAND_LCD_BASE + DW_FC_STS_CLR));

   // Disable all FC interrupts - To be changed
   /* NOTE: This register is configured by F_Init() */

   // Configutre Sequence register
   //F_SetChipSelectAndReadyLine(FlsDev, &FC_CmdSeqImg);

   /*FC_CmdSeqImg.RdyInEn = C_BitEnable;      // Monitor Ready line
   FC_CmdSeqImg.KeepChpSlct = C_BitDisable; // Don't keep CS for next transaction
   FC_CmdSeqImg.DataEccEn = C_BitDisable;   // Disable ECC
   FC_CmdSeqImg.DataRWDir = 0;              // Read from Flash
#ifndef USE_ReadOnce
   FC_CmdSeqImg.DataRWEn = C_BitEnable;     // Enable Read/Write transfer
#else
   FC_CmdSeqImg.DataRWEn = C_BitDisable;    // Disable Read/Write transfer
#endif

   FC_CmdSeqImg.Mode16 = K_FlsAccess_16bit;
#ifndef USE_ReadOnce
   FC_CmdSeqImg.ReadOnce = C_BitDisable;    // Disable ReadOnce
#else
   FC_CmdSeqImg.ReadOnce = C_BitEnable;     // Enable ReadOnce
#endif
   FC_CmdSeqImg.Wait4En = C_BitDisable;
   FC_CmdSeqImg.Cmd4En = C_BitDisable;
   FC_CmdSeqImg.Wait3En = C_BitDisable;
   FC_CmdSeqImg.Cmd3En = C_BitDisable;
   FC_CmdSeqImg.Cmd1En = C_BitEnable;
   FC_CmdSeqImg.Addr3En = C_BitEnable;
   FC_CmdSeqImg.Addr2En = C_BitEnable;
   FC_CmdSeqImg.Addr1En = C_BitEnable;*/
   DW_FC_SET(DW_NAND_LCD_BASE, DW_FC_SEQUENCE,
             DW_FC_SEQ_CMD1_EN,
             DW_FC_SEQ_ADD1_EN, DW_FC_SEQ_ADD2_EN, DW_FC_SEQ_ADD3_EN,
             DW_FC_SEQ_RW_EN, DW_FC_SEQ_DATA_READ_DIR/*read*/, DW_FC_SEQ_MODE16,
             DW_FC_SEQ_CHIP_SEL(FlsDev), DW_FC_SEQ_RDY_EN,
             DW_FC_SEQ_RDY_SEL(FlsDev >> 1));

   switch (Chip)
   {
      case NFC_K_Chip_SamK9F5608:
         /*FC_CmdSeqImg.Wait2En = C_BitDisable;
         FC_CmdSeqImg.Cmd2En = C_BitDisable;
         FC_CmdSeqImg.Wait1En = C_BitEnable;  // for tWB, tRR
         FC_CmdSeqImg.Addr5En = C_BitDisable;
         FC_CmdSeqImg.Addr4En = C_BitDisable;*/
         DW_FC_ADD(DW_NAND_LCD_BASE, DW_FC_SEQUENCE,
                   DW_FC_SEQ_WAIT1_EN,
                   0,0,0,0,0,0,0,0,0);
         break;

      case NFC_K_Chip_SamK9F1G16:
         /*FC_CmdSeqImg.Wait2En = C_BitEnable;   // for tWB, tRR
         FC_CmdSeqImg.Cmd2En = C_BitEnable;
         FC_CmdSeqImg.Wait1En = C_BitDisable;
         FC_CmdSeqImg.Addr5En = C_BitDisable;
         FC_CmdSeqImg.Addr4En = C_BitEnable;*/
         DW_FC_ADD(DW_NAND_LCD_BASE, DW_FC_SEQUENCE,
                   DW_FC_SEQ_ADD4_EN,
                   DW_FC_SEQ_CMD2_EN, DW_FC_SEQ_WAIT2_EN,
                   0,0,0,0,0,0,0);

         break;

    case NFC_K_Chip_TosTH58DVG02:
         /*FC_CmdSeqImg.Wait2En = C_BitEnable;   // for tWB, tRR
         FC_CmdSeqImg.Cmd2En  = C_BitDisable;
         FC_CmdSeqImg.Wait1En = C_BitDisable;
         FC_CmdSeqImg.Addr5En = C_BitDisable;
         FC_CmdSeqImg.Addr4En = C_BitEnable;*/
         DW_FC_ADD(DW_NAND_LCD_BASE, DW_FC_SEQUENCE,
                   DW_FC_SEQ_ADD4_EN,
                   DW_FC_SEQ_WAIT2_EN, DW_FC_SEQ_MODE8/*8 bit*/,
                   0,0,0,0,0,0,0);
         break;

      case NFC_K_Chip_SamK9F1G08:
      case NFC_K_Chip_SamK9F2G08:
      case NFC_K_Chip_TosTH58NVG3:
         /*FC_CmdSeqImg.Wait2En = C_BitEnable;   // for tWB, tRR
         FC_CmdSeqImg.Cmd2En = C_BitEnable;
         FC_CmdSeqImg.Wait1En = C_BitDisable;
         FC_CmdSeqImg.Addr5En = C_BitEnable;
         FC_CmdSeqImg.Addr4En = C_BitEnable;*/
         DW_FC_ADD(DW_NAND_LCD_BASE, DW_FC_SEQUENCE,
                   DW_FC_SEQ_ADD4_EN, DW_FC_SEQ_ADD5_EN,
                   DW_FC_SEQ_CMD2_EN, DW_FC_SEQ_WAIT2_EN,
                   0,0,0,0,0,0);
         break;

       case NFC_K_Chip_SamK9F5616:
          /*FC_CmdSeqImg.Wait2En = C_BitDisable;
          FC_CmdSeqImg.Cmd2En = C_BitDisable;
          FC_CmdSeqImg.Wait1En = C_BitEnable;  // for tWB, tRR
          FC_CmdSeqImg.Addr5En = C_BitDisable;
          FC_CmdSeqImg.Addr4En = C_BitDisable;*/
          DW_FC_ADD(DW_NAND_LCD_BASE, DW_FC_SEQUENCE,
                    DW_FC_SEQ_WAIT1_EN,
                    0,0,0,0,0,0,0,0,0);
            break;

      default:
         DW_SPOT("Unknown chip: %0#10x, driver is stuck to prevent system crash.", Chip);
         while (1);
         break;
   }

   //V_FC_RegsP->FC_CmdSeq = FC_CmdSeqImg;

   // Configure Address registers
   /*V_FC_RegsP->FC_AddrCol.Addr1 = V_FlsCycle1;
   V_FC_RegsP->FC_AddrCol.Addr2 = V_FlsCycle2;
   V_FC_RegsP->FC_AddrRow.Addr3 = V_FlsCycle3;
   V_FC_RegsP->FC_AddrRow.Addr4 = V_FlsCycle4;
   V_FC_RegsP->FC_AddrRow.Addr5 = V_FlsCycle5;*/
   DW_FC_SET(DW_NAND_LCD_BASE, DW_FC_ADDR_COL,
             DW_FC_ADD1(V_FlsCycle1), DW_FC_ADD2(V_FlsCycle2),
             0,0,0,0,0,0,0,0);
   DW_FC_SET(DW_NAND_LCD_BASE, DW_FC_ADDR_ROW,
             DW_FC_ADD3(V_FlsCycle3), DW_FC_ADD4(V_FlsCycle4), DW_FC_ADD5(V_FlsCycle5),
             0,0,0,0,0,0,0);

   // Configure Command Code registers
   /*V_FC_RegsP->FC_Cmd.Cmd1 = V_FlsCmnd1;
   V_FC_RegsP->FC_Cmd.Cmd2 = V_FlsCmnd2;*/
   DW_FC_SET(DW_NAND_LCD_BASE, DW_FC_CMD,
             DW_FC_CMD1(V_FlsCmnd1), DW_FC_CMD2(V_FlsCmnd2),
             0,0,0,0,0,0,0,0);

   // Configure DataCount register
   /*V_FC_RegsP->FC_Dcount.VirtPageCnt = 0; // ECC not activated
   V_FC_RegsP->FC_Dcount.SpareN1Cnt = 0;
   V_FC_RegsP->FC_Dcount.SpareN2Cnt = 0;
#ifndef USE_ReadOnce
   V_FC_RegsP->FC_Dcount.VirtMainCnt = 2; // 2 bytes on 16bit access --> Output 1 Read signals: MarkArea.
#else
   V_FC_RegsP->FC_Dcount.VirtMainCnt = 0;
#endif*/
   writel(DW_FC_DCOUNT_MAIN_CNT(2), (DW_NAND_LCD_BASE + DW_FC_DCOUNT));

   // Configure Ready-Timeout register
   /*V_FC_RegsP->FC_TimeOut.RdyTimeOutEn = C_BitEnable;
   V_FC_RegsP->FC_TimeOut.RdyTimeOutCnt = C_FlsRdyTimeout;*/
   DW_FC_SET(DW_NAND_LCD_BASE, DW_FC_TIMEOUT,
             DW_FC_TIMEOUT_RDY_EN, DW_FC_TIMEOUT_RDY_CNT(C_FlsRdyTimeout),
             0,0,0,0,0,0,0,0);

   /*! SET Bypass enabled */
   //V_FC_RegsP->FC_FBypCtrl.FBypEn = C_BitEnable;
   writel(DW_FC_FBYP_CTL_BP_EN, (DW_NAND_LCD_BASE + DW_FC_FBYP_CTL));

   writel(SET_32BIT, (DW_NAND_LCD_BASE + DW_FC_STS_CLR));
   /*! TRIGGER Flash controller */
NFLC_CAPTURE();
   //V_FC_RegsP->FC_Ctrl = FC_CtrlImg;
   writel(DW_FC_CTL_ECC_OP_MODE(DW_FC_MEM_TRANS), (DW_NAND_LCD_BASE + DW_FC_CTL));

   /*! READ Flash Data via Bypass */
//#ifndef USE_ReadOnce
   //V_FC_RegsP->FC_FBypCtrl.FBypRdFls = C_BitEnable;
        writel(DW_FC_FBYP_CTL_BP_READ_FLASH|DW_FC_FBYP_CTL_BP_EN,
            (DW_NAND_LCD_BASE + DW_FC_FBYP_CTL));

   //while (V_FC_RegsP->FC_FBypCtrl.FBypBusy != 0);
   while((readl(DW_NAND_LCD_BASE + DW_FC_FBYP_CTL) & DW_FC_FBYP_CTL_BUSY) != 0);
//#else
   /*! POLL for transaction End */
   //while (V_FC_RegsP->FC_Stat.TransDone == 0);
   /*while((readl(DW_NAND_LCD_BASE + DW_FC_STATUS) 
          & DW_FC_STATUS_TRANS_DONE) == 0);*/
//#endif
   //FlashData = (__u16)(V_FC_RegsP->FC_FBypData.FlsData);
   FlashData = (__u16)((readl(DW_NAND_LCD_BASE  + DW_FC_FBYP_DATA) 
                       & DW_FC_FBYP_DATA_FLASH_DATA) >> 16);
   *MarkAreaP = FlashData;

//#ifndef USE_ReadOnce
   /*! DISABLE ByPass mode */
   //V_FC_RegsP->FC_FBypCtrl.FBypEn = C_BitDisable;
   writel(!DW_FC_FBYP_CTL_BP_EN, (DW_NAND_LCD_BASE + DW_FC_FBYP_CTL));
//#endif
//out:
    up(&unified_dw_nflc_mutex);

} /* end F_CmdReadBadBlockMarkBypass() */
#endif


/**************************************************************************
* Function Name:  F_CmdWritePageAndSpare
*
* Description:    Prepares a Command Sequence for Programing a whole
*                 physical Page + its Spare area.
*                 Configures DMA & FIFO, then activates the transaction.
*                 Possible combination:
*                   1. Net Page size:  512 Bytes; Spare area size:  16 Bytes.
*                   2. Net Page size: 2048 Bytes; Spare area size:  64 Bytes.
*                   3. Net Page size: 4096 Bytes; Spare area size: 128 Bytes.
*
*                 In FPGA emulation of the DM56, Page (DataP) and Spare (SpareP) buffers
*                 !!! MUST !!! be allocated within the FPGA RAM, due to the DMA transfer !
*
* Input:          Chip:       Flash chip identity.
*                 FlsDev:     Flash Device number.
*                 Block:      Block number.
*                 PageInBlk:  Number of Page inside the Block.
*                 N1Len:      Ecc N1 field length.
*                 IsEccF:     Whether to activate ECC machine.
*                 DataP:  User supplied Page Data input buffer.
*                 SpareP: User supplied Page's Spare-Area input buffer.
*
*  Output:        None
*
* Return Value:   NFC_K_Err_OK:        Program OK.
*                 NFC_K_Err_ProgFail:  Program Error.
*                 NFC_K_Err_TimeOut:   Program timeout on chip.
*
* NOTE: P/O the spare-area data contains the ECC (Error correction Code).
***************************************************************************/
#ifdef TEST_PROGRAM
/*static*/ NFC_E_Err  F_CmdWritePageAndSpare( NFC_E_Chip    Chip,
                                          NFC_E_FlsDev  FlsDev,
                                          __u16        Block,
                                          __u16        DataLen,
                                          __u16        SpareLen,
                                          __u8         PageInBlk,
                                          __u8         N1Len,
                                          bool          IsEccF,
                                          void *        DataP,
                                          void *        SpareP )
{
   const Y_NFlsParms *  FlsParmsP;
   DMF_E_Width          FifoBusWidth;
//   Y_FC_Ctrl            FC_CtrlImg;
//   Y_FC_CmdSeq          FC_CmdSeqImg;
   __u16               FlashData;
   int ret_value = NFC_K_Err_OK;
   unsigned Cmd4En = 0, Wait4En = 0, ReadOnce = 0;
   unsigned int PageSize = NFC_F_PageNetSize(FlsDev);
   
   unsigned int Column = 0; // We start the write operation from the beginning of the physical page.

   down(&unified_dw_nflc_mutex);
//NFLC_CAPTURE();
//   writel(0, (DW_NAND_LCD_BASE + DW_FC_CTL));
   F_SetFixedCntrlRegs();

	if (PageSize <= 512) {
		Column = F_PrepareSmallPageWriteTransaction(Chip, FlsDev, Column);
	}
 
//   *((__u32 *)&FC_CtrlImg) = 0;
//DW_SPOT();
//   *((__u32 *)&FC_CmdSeqImg) = 0;

//  printk("WRIT: Chip %u, FlsDev %u, Block %4u, DataLen %u, SpareLen %u, PageInBlk %2u, N1Len %u, IsEccF %u, DataP %0#10x, SpareP %0#10x\n",
//	Chip, FlsDev, Block, DataLen, SpareLen, PageInBlk, N1Len, IsEccF, (unsigned)DataP, (unsigned)SpareP);
DW_SPOT_FUN("WRIT: Chip %u, Dev %u, Blk %4u, Pg %2u, DLen %u, SLen %u, IsEccF %u, DataP %0#10x, SpareP %0#10x",
	               Chip, FlsDev, Block, PageInBlk, DataLen, SpareLen, IsEccF, (unsigned)DataP, (unsigned)SpareP);

   /*! SET pointer to Flash chip parameters */
   FlsParmsP = &V_NFlsParms[Chip];
   FifoBusWidth = (FlsParmsP->AccMode == K_FlsAccess_8bit) ? DMF_K_Width8bit : DMF_K_Width16bit;

   F_SetAddrCycles( Chip, Block, PageInBlk, false );

   /*! CONFIGURE DMA & FIFO & GCR2 registers */
/*   DMF_F_ConfDmaFifo( DMF_K_DmaDirReadAHB,
                      DMF_K_PeriphFlsCntl,
                      FifoBusWidth,
                      FlsParmsP->PageSize,
                      FlsParmsP->SpareSize,
                      (__u32 *)DataP,
                      (__u32 *)SpareP );*/
   SET_DMACONFIG(dw_DMA_config.TransConfig, 
		      DMF_K_DmaDirReadAHB, 
                      DMF_K_PeriphFlsCntl,
                      FifoBusWidth,
                      DataLen,
                      SpareLen,
                      (__u32 *)DataP,
                      (__u32 *)SpareP);
   DMF_F_ConfDmaFifo( DMF_K_DmaDirReadAHB,
                      DMF_K_PeriphFlsCntl,
                      FifoBusWidth,
                      DataLen,
                      SpareLen,
                      (__u32 *)DataP,
                      (__u32 *)SpareP );

   /*! CONFIGURE Flash-Controller registers: */

   // Clear relevant bits in Status register
   writel(SET_32BIT, (DW_NAND_LCD_BASE + DW_FC_STS_CLR));

   // Disable all FC interrupts - To be changed
   /* NOTE: This register is configured by F_Init() */

   // Configutre Sequence register
   //F_SetChipSelectAndReadyLine(FlsDev, &FC_CmdSeqImg);

   /*FC_CmdSeqImg.RdyInEn = C_BitEnable;      // Monitor Ready line
   FC_CmdSeqImg.KeepChpSlct = C_BitDisable; // Don't keep CS for next transaction
   FC_CmdSeqImg.DataEccEn = IsEccF ? C_BitEnable: C_BitDisable; // Enable / Disable ECC
   FC_CmdSeqImg.DataRWDir = 1;              // Wrire to Flash
   FC_CmdSeqImg.DataRWEn = C_BitEnable;     // Enable Read/Write transfer

   FC_CmdSeqImg.ReadOnce = C_BitEnable;  // Enable Status read
   FC_CmdSeqImg.Wait4En  = C_BitEnable;
   FC_CmdSeqImg.Cmd4En   = C_BitEnable;  // Status Read command
   FC_CmdSeqImg.Wait3En  = C_BitEnable;  // tWB
   FC_CmdSeqImg.Cmd3En   = C_BitEnable;  // Prog 2nd command
   FC_CmdSeqImg.Wait2En  = C_BitDisable;
   FC_CmdSeqImg.Cmd2En   = C_BitDisable; // CMD2 unused here
   FC_CmdSeqImg.Wait1En  = C_BitDisable;
   FC_CmdSeqImg.Cmd1En   = C_BitEnable;  // Prog 1st command
   FC_CmdSeqImg.Addr3En  = C_BitEnable;
   FC_CmdSeqImg.Addr2En  = C_BitEnable;
   FC_CmdSeqImg.Addr1En  = C_BitEnable;
   */
/*
   DW_FC_SET(DW_NAND_LCD_BASE, DW_FC_SEQUENCE,
             DW_FC_SEQ_CMD1_EN, DW_FC_SEQ_CMD3_EN, DW_FC_SEQ_CMD4_EN,
             DW_FC_SEQ_ADD1_EN, DW_FC_SEQ_ADD2_EN, DW_FC_SEQ_ADD3_EN,
             DW_FC_SEQ_WAIT3_EN, DW_FC_SEQ_WAIT4_EN,
             0,0);
   DW_FC_ADD(DW_NAND_LCD_BASE, DW_FC_SEQUENCE,
             DW_FC_SEQ_RW_EN, DW_FC_SEQ_DATA_WRITE_DIR //write,
             DW_FC_SEQ_DATA_ECC(IsEccF), DW_FC_SEQ_READ_ONCE,
             DW_FC_SEQ_CHIP_SEL(FlsDev), DW_FC_SEQ_RDY_EN, DW_FC_SEQ_RDY_SEL(FlsDev >> 1),
             0,0,0);

*/ // R.B. - trial of read once disabling
   M_SetUpReadStatus(Cmd4En, V_FlsCmnd4, Wait4En, ReadOnce);
   DW_FC_SET(DW_NAND_LCD_BASE, DW_FC_SEQUENCE,
             DW_FC_SEQ_CMD1_EN, DW_FC_SEQ_CMD3_EN, Cmd4En,
             DW_FC_SEQ_ADD1_EN, DW_FC_SEQ_ADD2_EN, DW_FC_SEQ_ADD3_EN,
             DW_FC_SEQ_WAIT3_EN, Wait4En,
             0,0);
   DW_FC_ADD(DW_NAND_LCD_BASE, DW_FC_SEQUENCE,
             DW_FC_SEQ_RW_EN, DW_FC_SEQ_DATA_WRITE_DIR/*write*/,
             DW_FC_SEQ_DATA_ECC(IsEccF), ReadOnce,
             DW_FC_SEQ_CHIP_SEL(FlsDev), DW_FC_SEQ_RDY_EN, DW_FC_SEQ_RDY_SEL(FlsDev >> 1),
             0,0,0);


   switch (Chip)
   {
      // 8 bit devices
      case NFC_K_Chip_SamK9F5608:
         /*FC_CmdSeqImg.Mode16 = K_FlsAccess_8bit;
         FC_CmdSeqImg.Addr5En  = C_BitDisable;
         FC_CmdSeqImg.Addr4En  = C_BitDisable;*/
         DW_FC_ADD(DW_NAND_LCD_BASE, DW_FC_SEQUENCE,
                   DW_FC_SEQ_MODE8/*8 bit*/,
                   0,0,0,0,0,0,0,0,0);
         break;
      case NFC_K_Chip_TosTH58DVG02:
         /*FC_CmdSeqImg.Mode16 = K_FlsAccess_8bit;
         FC_CmdSeqImg.Addr5En  = C_BitDisable;  // 
         FC_CmdSeqImg.Addr4En  = C_BitEnable;  // 
         */
         DW_FC_ADD(DW_NAND_LCD_BASE, DW_FC_SEQUENCE,
                   DW_FC_SEQ_ADD4_EN, 0,
                   DW_FC_SEQ_MODE8/*8 bit*/,
                   0,0,0,0,0,0,0);
         break;


      case NFC_K_Chip_SamK9F1G08:
      case NFC_K_Chip_SamK9F2G08:
      case NFC_K_Chip_TosTH58NVG3:
      case NFC_K_Chip_TosTH58NVG1S3A:
         /*FC_CmdSeqImg.Mode16 = K_FlsAccess_8bit;
         FC_CmdSeqImg.Addr5En  = C_BitEnable;  // for 3rd Row addr
         FC_CmdSeqImg.Addr4En  = C_BitEnable;  // for 2nd Row addr*/
         DW_FC_ADD(DW_NAND_LCD_BASE, DW_FC_SEQUENCE,
                   DW_FC_SEQ_ADD4_EN, DW_FC_SEQ_ADD5_EN,
                   DW_FC_SEQ_MODE8/*8 bit*/,
                   0,0,0,0,0,0,0);
         break;

      // 16 bit devices
     case NFC_K_Chip_SamK9F5616:
         /*FC_CmdSeqImg.Mode16 = K_FlsAccess_16bit;
         FC_CmdSeqImg.Addr5En  = C_BitDisable;
         FC_CmdSeqImg.Addr4En  = C_BitDisable;*/
         DW_FC_ADD(DW_NAND_LCD_BASE, DW_FC_SEQUENCE,
                   DW_FC_SEQ_MODE16/*16 bit*/,
                   0,0,0,0,0,0,0,0,0);
         break;

      case NFC_K_Chip_SamK9F1G16:
         /*FC_CmdSeqImg.Mode16 = K_FlsAccess_16bit;
         FC_CmdSeqImg.Addr5En = C_BitDisable;
         FC_CmdSeqImg.Addr4En = C_BitEnable;   // for 2nd Row addr*/
         DW_FC_ADD(DW_NAND_LCD_BASE, DW_FC_SEQUENCE,
                   DW_FC_SEQ_ADD4_EN, DW_FC_SEQ_MODE16/*16 bit*/,
                   0,0,0,0,0,0,0,0);
         break;

      default:
         DW_SPOT("Unknown chip: %0#10x, driver is stuck to prevent system crash.", Chip);
         while (1);
         break;
   } /* end_switch chip */

// Commented out by Roman on 18/05/2008 - done by DW_FC_ADD(DW_NAND_LCD_BASE, DW_FC_SEQUENCE...
//   V_FC_RegsP->FC_CmdSeq = FC_CmdSeqImg;

   // Configure Address registers
   /*V_FC_RegsP->FC_AddrCol.Addr1 = V_FlsCycle1;
   V_FC_RegsP->FC_AddrCol.Addr2 = V_FlsCycle2;
   V_FC_RegsP->FC_AddrRow.Addr3 = V_FlsCycle3;
   V_FC_RegsP->FC_AddrRow.Addr4 = V_FlsCycle4;
   V_FC_RegsP->FC_AddrRow.Addr5 = V_FlsCycle5;*/
   DW_FC_SET(DW_NAND_LCD_BASE, DW_FC_ADDR_COL,
             DW_FC_ADD1(V_FlsCycle1), DW_FC_ADD2(V_FlsCycle2),
             0,0,0,0,0,0,0,0);
   DW_FC_SET(DW_NAND_LCD_BASE, DW_FC_ADDR_ROW,
             DW_FC_ADD3(V_FlsCycle3), DW_FC_ADD4(V_FlsCycle4), DW_FC_ADD5(V_FlsCycle5),
             0,0,0,0,0,0,0);

   // Configure Command Code register
   /*V_FC_RegsP->FC_Cmd.Cmd1 = V_FlsCmnd1;
   V_FC_RegsP->FC_Cmd.Cmd2 = V_FlsCmnd2; // Not used in this operation.
   V_FC_RegsP->FC_Cmd.Cmd3 = V_FlsCmnd3;
   V_FC_RegsP->FC_Cmd.Cmd4 = V_FlsCmnd4;*/

   DW_FC_SET(DW_NAND_LCD_BASE, DW_FC_CMD,
             DW_FC_CMD1(V_FlsCmnd1), DW_FC_CMD2(V_FlsCmnd2),
             DW_FC_CMD3(V_FlsCmnd3), DW_FC_CMD4(V_FlsCmnd4),
             0,0,0,0,0,0);

   // Configure WaitTime register
   /* NOTE: This register is configured by F_Init() */

   // Configure PulseTime register
   /* NOTE: This register is configured by F_Init() */

  // Configure DataCount register
   F_SetDataCount(nand_info->nand, DataLen , SpareLen, IsEccF, N1Len);

   // Configure Ready-Timeout register
   /*V_FC_RegsP->FC_TimeOut.RdyTimeOutEn = C_BitEnable;
   V_FC_RegsP->FC_TimeOut.RdyTimeOutCnt = C_FlsRdyTimeout;*/
   DW_FC_SET(DW_NAND_LCD_BASE, DW_FC_TIMEOUT,
             DW_FC_TIMEOUT_RDY_EN, DW_FC_TIMEOUT_RDY_CNT(C_FlsRdyTimeout),
             0,0,0,0,0,0,0,0);

   // Configure Flash-Controller to Disable DMA/FIFO Bypass
   //F_DisableBypassMode();
   writel(!DW_FC_FBYP_CTL_BP_EN, (DW_NAND_LCD_BASE + DW_FC_FBYP_CTL));

   /*! TRIGGER DMA controller */
   DMF_F_TrigDma();

   writel(SET_32BIT, (DW_NAND_LCD_BASE + DW_FC_STS_CLR));
NFLC_CAPTURE();
   /*! TRIGGER Flash controller */
   writel(DW_FC_CTL_ECC_OP_MODE(DW_FC_MEM_TRANS), (DW_NAND_LCD_BASE + DW_FC_CTL));

   /* NOTE: The Following section will be replaced by interrupt handling
   **       ISR, which will send an event to a waiting task.
   **       This task will fix ECC errors, then call a call-back function
   **       of the registered user - This is for ASync I/O
   */

   /*! POLL for transaction end */
   //while ((V_FC_RegsP->FC_Stat.TransDone == 0) || (DMF_F_IsXferDone() == false));
   while(((readl(DW_NAND_LCD_BASE + DW_FC_STATUS) & DW_FC_STATUS_TRANS_DONE) == 0)
 //      || (readl(__io_address(DW_DMA_BASE) + DW_DMA_ISR_DMA_DONE))); // R.B. 19/05/2008
	 || (DMF_F_IsXferDone() == false))
		;
   /*! CHECK if Timeout was marked (chip blocked) */
   //FC_StatImg = V_FC_RegsP->FC_Stat;
   //if (FC_StatImg.RdyTimeOut != 0)


// FIXME: REMOVE THE BELOW LINES, DEBUG PURPOSE ONLY!!!!!
//{
//	unsigned long tmp;
//   if ((tmp = readl(DW_NAND_LCD_BASE + 0x80)) != 0x00005300 ) {
//		DW_SPOT("GOT STATUS READY AND SMALL COUNTER: %0#10lx", tmp);
//	}
//}
   if(readl(DW_NAND_LCD_BASE + DW_FC_STATUS) & DW_FC_STATUS_RDY_TIMEOUT)
   {
//      return (NFC_K_Err_TimeOut);
	ret_value = NFC_K_Err_TimeOut;
	goto out;
   }


	if ((readl(DW_NAND_LCD_BASE + DW_FC_STATUS) & DW_FC_STATUS_RDY1_STAT) == 0) {
		DW_SPOT("We have a problem");
	}

	// Due to HW bug the DW_FC_STATUS_TRANS_DONE bit reports that the transaction is done while
	//   the transaction is still on going. Using debug register to overcome the obstacle.
 //  while ((readl(DW_NAND_LCD_BASE + 0x80) >> 11) != DataLen)
		;
//	udelay(10);

   /*! CHECK Programming result reported by the flash Chip */
   //FlashData = (__u16)(V_FC_RegsP->FC_FBypData.FlsData);
/*
   FlashData = (__u16)((readl(DW_NAND_LCD_BASE  + DW_FC_FBYP_DATA) 
                       & DW_FC_FBYP_DATA_FLASH_DATA) >> 16);
*/
   FlashData = M_ReadStatus(FlsDev);

   if ((FlashData & 0x001) != 0)
   {
//      return (NFC_K_Err_ProgFail);
		ret_value = NFC_K_Err_ProgFail;
		goto out;
   }

out:
   up(&unified_dw_nflc_mutex);
//   return (NFC_K_Err_OK);
	return ret_value;

} /* end F_CmdWritePageAndSpare() */


#ifdef UNUSED_CODE
/**************************************************************************
* Function Name:  F_CmdReadOnce
*
* Description:    Performs a single Read of the Flash device, as a
*                 "Read Once" only sequence.
*                 This function if for Testing Only !!!
*
* Input:          None.
* Output:         None.
* Return Value:   None.
*
* NOTE: User must Keep the ChipSelect active in the transaction preceding
*       this one !
*
***************************************************************************/
/*static*/ void  F_CmdReadOnce( NFC_E_FlsDev  FlsDev, E_FlsAccess  FlsAccess )
{
   Y_FC_Ctrl            FC_CtrlImg;
   Y_FC_CmdSeq          FC_CmdSeqImg;


   *((__u32 *)&FC_CtrlImg) = 0;
   *((__u32 *)&FC_CmdSeqImg) = 0;

   /*! DISABLE DMA & FIFO */
   //DMF_F_ConfDmaBypass(nand_info->dma_regs, DMF_K_PeriphFlsCntl);

   /*! CONFIGURE Flash-Controller registers: */
   /* NOTE: Image of FC_Ctrl register is needed, because a write to
   **       this register triggers a Flash-Controller transaction. */
   FC_CtrlImg.ChainCntStart = 0;
   FC_CtrlImg.PageSelect = 0;
   FC_CtrlImg.EccOpMode = C_FC_Ctrl_EccOpMode_MemTrans; // Memory transaction

   // Clear relevant bits in Status register
   writel(SET_32BIT, (DW_NAND_LCD_BASE + DW_FC_STS_CLR));

   // Disable all FC interrupts - To be changed
   /* NOTE: This register is configured by F_Init() */

   // Configutre Sequence register
   F_SetChipSelectAndReadyLine(FlsDev, &FC_CmdSeqImg); // Configures READY_SEL & CHIP_SEL
   FC_CmdSeqImg.RdyInEn = C_BitDisable;      // Don't Monitor Ready line
   FC_CmdSeqImg.Mode16 = FlsAccess;          // 8 or 16 bit access
   FC_CmdSeqImg.KeepChpSlct = C_BitDisable;  // Don't keep CS for next transaction
   FC_CmdSeqImg.ReadOnce = C_BitEnable;      // Enable ReadOnce (only!)
   FC_CmdSeqImg.Wait4En = C_BitDisable;
   FC_CmdSeqImg.Cmd4En = C_BitDisable;
   FC_CmdSeqImg.Wait3En = C_BitDisable;
   FC_CmdSeqImg.Cmd3En = C_BitDisable;
   FC_CmdSeqImg.DataEccEn = C_BitDisable;    // Disable ECC
   FC_CmdSeqImg.DataRWDir = 0;               // Read from Flash
   FC_CmdSeqImg.DataRWEn = C_BitDisable; //C_BitEnable;      // Enable Read/Write transfer
   FC_CmdSeqImg.Wait2En = C_BitDisable;
   FC_CmdSeqImg.Cmd2En = C_BitDisable;
   FC_CmdSeqImg.Wait1En = C_BitDisable;
   FC_CmdSeqImg.Addr5En = C_BitDisable;
   FC_CmdSeqImg.Addr4En = C_BitDisable;
   FC_CmdSeqImg.Addr3En = C_BitDisable;
   FC_CmdSeqImg.Addr2En = C_BitDisable;
   FC_CmdSeqImg.Addr1En = C_BitDisable;
   FC_CmdSeqImg.Cmd1En = C_BitDisable;
   V_FC_RegsP->FC_CmdSeq = FC_CmdSeqImg;

   // Configure PulseTime register
   /* NOTE: This register is configured by F_Init() */

   // Configure DataCount register
   *((__u32 *)&V_FC_RegsP->FC_Dcount) = 0;

   // Configure Ready-Timeout register
   V_FC_RegsP->FC_TimeOut.RdyTimeOutEn = C_BitDisable;
   V_FC_RegsP->FC_TimeOut.RdyTimeOutCnt = 0;

   /*! TRIGGER Flash controller */
   V_FC_RegsP->FC_Ctrl = FC_CtrlImg;

   /*! POLL for transaction end */
   //while (V_FC_RegsP->FC_Stat.TransDone == 0);
   while((readl(DW_NAND_LCD_BASE + DW_FC_STATUS) 
          & DW_FC_STATUS_TRANS_DONE) == 0);

} /* end F_CmdReadOnce() */
#endif /* UNUSED_CODE */
#endif


/**************************************************************************
* Function Name:  F_CmdReadPageAndSpareBypass
*
* Description:    Prepares a Command Sequence for Reading a whole
*                 physical Page, including its Spare area.
*                 Activates the transaction and Bypasses the DMA&FIFO module,
*                 Reads the flash via the Controller's ByPass registers.
*                 The function disables the ECC validation.
*                 Possible combination:
*                   1. Net Page size:  512 Bytes; Spare area size:  16 Bytes.
*                   2. Net Page size: 2048 Bytes; Spare area size:  64 Bytes.
*                   3. Net Page size: 4096 Bytes; Spare area size: 128 Bytes.
*
* Input:          Chip:       Flash chip identity.
*                 FlsDev:     Flash Device number.
*                 Block:      Block number.
*                 PageInBlk:  Number of Page inside the Block.
*                 N1Len:      Ecc N1 field length.
*                 IsEccF:     Whether to activate ECC machine.
*
* Output:         DataP:  User supplied Page Data output buffer.
*                 SpareP: User supplied Page's Spare-Area output buffer.
*
* Return Value:   NFC_K_Err_OK:       Read OK.
*                 NFC_K_Err_TimeOut:  Read Timeout.
*
* NOTE: This function is used for testing only !
*       For normal operation use F_CmdReadPageAndSpare().
*
***************************************************************************/
/*static*/ NFC_E_Err  F_CmdReadPageAndSpareBypass( struct dw_nand_info *nand_info,
                                               __u16        Block,
                                               __u16        DataLen,
                                               __u16        SpareLen,
                                               __u16        Column,
                                               __u8         PageInBlk,
                                               __u8         N1Len,
                                               bool          IsEccF,
                                               void *        DataP,
                                               void *        SpareP )
{
   const Y_NFlsParms *  FlsParmsP;
//   Y_FC_Ctrl            FC_CtrlImg;
//   Y_FC_CmdSeq          FC_CmdSeqImg;
   __u16               FlashData;
   __u16               i, j;
   __u16               NumBypassReads;
   __u16 *             TmpU16P;
   __u32               TmpU32;
   __u32 *             DataU32P;

   int ret_value = NFC_K_Err_OK;

	if (Column != 0) {
		BUG(); // This means we need to change the FIXME's below.
	}

DW_SPOT_FUN("Chip=%d, FlsDev=%d, Block=%u, DataLen=%u, SpareLen=%u, Column=%u, PageInBlk=%u, N1Len=%u, IsEccF=%u,  DataP=%0#10x, SpareP=%0#10x", 
	 (int)Chip, (int)FlsDev, (unsigned)Block,  (unsigned)DataLen,  (unsigned)SpareLen,  (unsigned)Column, (unsigned)PageInBlk, (unsigned)N1Len, (unsigned)IsEccF,  (unsigned)DataP, (unsigned)SpareP);

   down(&unified_dw_nflc_mutex);
   F_SetFixedCntrlRegs(nand_info->nand) ;

//   *((__u32 *)&FC_CtrlImg) = 0;
//   *((__u32 *)&FC_CmdSeqImg) = 0;

   /*! DISABLE DMA & FIFO */
   DMF_F_ConfDmaBypass(&nand_info->dma_regs, DMF_K_PeriphFlsCntl);

   /*! SET pointer to Flash chip parameters */
   FlsParmsP = &V_NFlsParms[nand_info->nand_chip_id];

   F_SetAddrCycles( nand_info->nand_chip_id, Block, PageInBlk, true );

   /*! CONFIGURE Flash-Controller registers: */
   /* NOTE: Image of FC_Ctrl register is needed, because a write to
   **       this register triggers a Flash-Controller transaction. */
   /*FC_CtrlImg.ChainCntStart = 0;
   FC_CtrlImg.PageSelect = 0;
   FC_CtrlImg.EccOpMode = C_FC_Ctrl_EccOpMode_MemTrans; // Memory transaction*/

   // Clear relevant bits in Status register
   writel(SET_32BIT, (nand_info->nand + DW_FC_STS_CLR));

   // Disable all FC interrupts - To be changed
   /* NOTE: This register is configured by F_Init() */

   // Configutre Sequence register
   //F_SetChipSelectAndReadyLine(FlsDev, &FC_CmdSeqImg);

   /*FC_CmdSeqImg.RdyInEn = C_BitEnable;      // Monitor Ready line
   FC_CmdSeqImg.KeepChpSlct = C_BitDisable; // Don't keep CS for next transaction
   FC_CmdSeqImg.DataEccEn = IsEccF ? C_BitEnable: C_BitDisable; // Enable / Disable ECC
   FC_CmdSeqImg.DataRWDir = 0;              // Read from Flash
   FC_CmdSeqImg.DataRWEn = C_BitEnable;     // Enable Read/Write transfer

   FC_CmdSeqImg.ReadOnce = C_BitDisable;
   FC_CmdSeqImg.Wait4En = C_BitDisable;
   FC_CmdSeqImg.Cmd4En = C_BitDisable;
   FC_CmdSeqImg.Wait3En = C_BitDisable;
   FC_CmdSeqImg.Cmd3En = C_BitDisable;
   FC_CmdSeqImg.Cmd1En = C_BitEnable;  // for Read command
   FC_CmdSeqImg.Addr3En = C_BitEnable;
   FC_CmdSeqImg.Addr2En = C_BitEnable;
   FC_CmdSeqImg.Addr1En = C_BitEnable;*/

   DW_FC_SET(nand_info->nand, DW_FC_SEQUENCE,
             DW_FC_SEQ_CMD1_EN,
             DW_FC_SEQ_ADD1_EN, DW_FC_SEQ_ADD2_EN, DW_FC_SEQ_ADD3_EN,
             DW_FC_SEQ_RW_EN, DW_FC_SEQ_DATA_READ_DIR/*read*/,
             DW_FC_SEQ_DATA_ECC(IsEccF), DW_FC_SEQ_CHIP_SEL(nand_info->FlsDev), DW_FC_SEQ_RDY_EN,
             DW_FC_SEQ_RDY_SEL(nand_info->FlsDev >> 1));

   switch (nand_info->nand_chip_id)
   {
      // 8 bit devices
      case NFC_K_Chip_SamK9F5608:
         /*FC_CmdSeqImg.Mode16 = K_FlsAccess_8bit;
         FC_CmdSeqImg.Wait2En = C_BitDisable;
         FC_CmdSeqImg.Cmd2En = C_BitDisable;
         FC_CmdSeqImg.Wait1En = C_BitEnable; // for tWB & tAR
         FC_CmdSeqImg.Addr5En = C_BitDisable;
         FC_CmdSeqImg.Addr4En = C_BitDisable;*/
         DW_FC_ADD(nand_info->nand, DW_FC_SEQUENCE,
                   DW_FC_SEQ_WAIT1_EN, DW_FC_SEQ_MODE8/*8 bit*/,
                   0,0,0,0,0,0,0,0);
         break;

       case NFC_K_Chip_TosTH58DVG02:
         /*FC_CmdSeqImg.Mode16 = K_FlsAccess_8bit;
         FC_CmdSeqImg.Wait2En = C_BitDisable;
         FC_CmdSeqImg.Cmd2En = C_BitDisable;
         FC_CmdSeqImg.Wait1En = C_BitEnable; // for tWB & tAR
         FC_CmdSeqImg.Addr5En = C_BitDisable;
         FC_CmdSeqImg.Addr4En = C_BitEnable;*/
         DW_FC_ADD(nand_info->nand, DW_FC_SEQUENCE, DW_FC_SEQ_ADD4_EN, 
                   DW_FC_SEQ_WAIT1_EN, DW_FC_SEQ_MODE8/*8 bit*/,
                   0,0,0,0,0,0,0);
         break;
     case NFC_K_Chip_SamK9F1G08:
      case NFC_K_Chip_SamK9F2G08:
      case NFC_K_Chip_TosTH58NVG3:
      case NFC_K_Chip_TosTH58NVG1S3A:
         /*FC_CmdSeqImg.Mode16 = K_FlsAccess_8bit;
         FC_CmdSeqImg.Wait2En = C_BitEnable; // for tWB & tAR
         FC_CmdSeqImg.Cmd2En = C_BitEnable;
         FC_CmdSeqImg.Wait1En = C_BitDisable;
         FC_CmdSeqImg.Addr5En = C_BitEnable; // for 3rd Row addr
         FC_CmdSeqImg.Addr4En = C_BitEnable; // for 2nd Row addr*/
         DW_FC_ADD(nand_info->nand, DW_FC_SEQUENCE,
                   DW_FC_SEQ_ADD4_EN, DW_FC_SEQ_ADD5_EN,
                   DW_FC_SEQ_CMD2_EN, DW_FC_SEQ_WAIT2_EN, DW_FC_SEQ_MODE8/*8 bit*/,
                   0,0,0,0,0);
         break;

      // 16 bit devices
      case NFC_K_Chip_SamK9F5616:
         /*FC_CmdSeqImg.Mode16 = K_FlsAccess_16bit;
         FC_CmdSeqImg.Wait2En = C_BitDisable;
         FC_CmdSeqImg.Cmd2En = C_BitDisable;
         FC_CmdSeqImg.Wait1En = C_BitEnable; // for tWB & tAR
         FC_CmdSeqImg.Addr5En = C_BitDisable;
         FC_CmdSeqImg.Addr4En = C_BitDisable;*/
         DW_FC_ADD(nand_info->nand, DW_FC_SEQUENCE,
                   DW_FC_SEQ_WAIT1_EN, DW_FC_SEQ_MODE16/*16 bit*/,
                   0,0,0,0,0,0,0,0);
         break;

      case NFC_K_Chip_SamK9F1G16:
         /*FC_CmdSeqImg.Mode16 = K_FlsAccess_16bit;
         FC_CmdSeqImg.Wait2En = C_BitEnable; // for tWB & tAR
         FC_CmdSeqImg.Cmd2En = C_BitEnable;
         FC_CmdSeqImg.Wait1En = C_BitDisable;
         FC_CmdSeqImg.Addr5En = C_BitDisable;
         FC_CmdSeqImg.Addr4En = C_BitEnable; // for 2nd Row addr*/
         DW_FC_ADD(nand_info->nand, DW_FC_SEQUENCE,
                   DW_FC_SEQ_ADD4_EN,
                   DW_FC_SEQ_CMD2_EN, 
                   DW_FC_SEQ_WAIT2_EN, DW_FC_SEQ_MODE16/*16 bit*/,
                   0,0,0,0,0,0);
         break;

      default:
         DW_SPOT("Unknown chip: %0#10x, driver is stuck to prevent system crash.", Chip);
         while (1);
         break;
   } /* end_switch chip */

   //V_FC_RegsP->FC_CmdSeq = FC_CmdSeqImg;

   // Configure Address registers
   /*V_FC_RegsP->FC_AddrCol.Addr1 = V_FlsCycle1;
   V_FC_RegsP->FC_AddrCol.Addr2 = V_FlsCycle2;
   V_FC_RegsP->FC_AddrRow.Addr3 = V_FlsCycle3;
   V_FC_RegsP->FC_AddrRow.Addr4 = V_FlsCycle4;
   V_FC_RegsP->FC_AddrRow.Addr5 = V_FlsCycle5;*/
   DW_FC_SET(nand_info->nand, DW_FC_ADDR_COL,
             DW_FC_ADD1(V_FlsCycle1), DW_FC_ADD2(V_FlsCycle2), // FIXME: Add the Column to V_FlsCycle1
             0,0,0,0,0,0,0,0);
   DW_FC_SET(nand_info->nand, DW_FC_ADDR_ROW,
             DW_FC_ADD3(V_FlsCycle3), DW_FC_ADD4(V_FlsCycle4), DW_FC_ADD5(V_FlsCycle5),
             0,0,0,0,0,0,0);


   // Configure Command Code register
   /*V_FC_RegsP->FC_Cmd.Cmd1 = V_FlsCmnd1;
   V_FC_RegsP->FC_Cmd.Cmd2 = V_FlsCmnd2;*/
   DW_FC_SET(nand_info->nand, DW_FC_CMD,
             DW_FC_CMD1(V_FlsCmnd1), DW_FC_CMD2(V_FlsCmnd2),
             0,0,0,0,0,0,0,0);

   // Configure WaitTime register
   /* NOTE: This register is configured by F_Init() */

   // Configure PulseTime register
   /* NOTE: This register is configured by F_Init() */

   /*****************************************************/
#if 0
   // Patch fot testing only ! - To be removed !!!
   ((Y_NFlsParms * )FlsParmsP)->PageSize = 4;
   ((Y_NFlsParms * )FlsParmsP)->SpareSize = 4;
#endif
   /*****************************************************/

   // Configure DataCount register
   F_SetDataCount(nand_info->nand, DataLen +Column, SpareLen, IsEccF, N1Len); // FIXME: only DataLen and not DataLen+Column.

   // Configure Ready-Timeout register
   /*V_FC_RegsP->FC_TimeOut.RdyTimeOutEn = C_BitEnable;
   V_FC_RegsP->FC_TimeOut.RdyTimeOutCnt = C_FlsRdyTimeout;*/
   DW_FC_SET(nand_info->nand, DW_FC_TIMEOUT,
             DW_FC_TIMEOUT_RDY_EN, DW_FC_TIMEOUT_RDY_CNT(C_FlsRdyTimeout),
             0,0,0,0,0,0,0,0);

   /*! SET Bypass enabled */
   //V_FC_RegsP->FC_FBypCtrl.FBypEn = C_BitEnable;
   writel(DW_FC_FBYP_CTL_BP_EN, (nand_info->nand + DW_FC_FBYP_CTL));

   writel(SET_32BIT, (nand_info->nand + DW_FC_STS_CLR));
   /*! TRIGGER Flash controller */
 NFLC_CAPTURE();
  //V_FC_RegsP->FC_Ctrl = FC_CtrlImg;
   writel(DW_FC_CTL_ECC_OP_MODE(DW_FC_MEM_TRANS), (nand_info->nand + DW_FC_CTL));

   /*! READ Flash Data via Bypass */
   
   /* Note: When DataP & SpareP point to buffers in FPGA memory, access is restricted to 32 bit only */
   if (FlsParmsP->AccMode == K_FlsAccess_8bit)
   {
      /* 8 bit access: */
      /*! SET returned Net Page data */
      for(i=0; i<DataLen+Column; i++) // FIXME: only DataLen and not DataLen+Column.
      {
        //V_FC_RegsP->FC_FBypCtrl.FBypRdFls = C_BitEnable;
        writel(DW_FC_FBYP_CTL_BP_READ_FLASH|DW_FC_FBYP_CTL_BP_EN,
               (nand_info->nand + DW_FC_FBYP_CTL));
        //while (V_FC_RegsP->FC_FBypCtrl.FBypBusy != 0);
        while((readl(nand_info->nand + DW_FC_FBYP_CTL) & DW_FC_FBYP_CTL_BUSY) != 0);
        //FlashData = (__u16)(V_FC_RegsP->FC_FBypData.FlsData);
        FlashData = (__u16)((readl(nand_info->nand  + DW_FC_FBYP_DATA) & DW_FC_FBYP_DATA_FLASH_DATA)
                            >> 16);
        if(i >= Column) // FIXME: No if!
        {
            //((__u8 *)DataP)[i-Column] = (__u8)FlashData;
            ((__u8 *)DataP)[i-Column] = (__u8)FlashData; // FIXME: should be like "((__u8 *)DataP)[i] = (__u8)FlashData"
        }
      }

      /*! SET returned Spare data */
      for(i=0; i<SpareLen; i++) // FIXME: The same as in MAIN apply to spare.
      {
        //V_FC_RegsP->FC_FBypCtrl.FBypRdFls = C_BitEnable;
        writel(DW_FC_FBYP_CTL_BP_READ_FLASH|DW_FC_FBYP_CTL_BP_EN,
               (nand_info->nand + DW_FC_FBYP_CTL));
        //while (V_FC_RegsP->FC_FBypCtrl.FBypBusy != 0);
        while((readl(nand_info->nand + DW_FC_FBYP_CTL) & DW_FC_FBYP_CTL_BUSY) != 0);
        //FlashData = (__u16)(V_FC_RegsP->FC_FBypData.FlsData);
        FlashData = (__u16)((readl(nand_info->nand  + DW_FC_FBYP_DATA) & DW_FC_FBYP_DATA_FLASH_DATA)
                            >> 16);
        ((__u8 *)SpareP)[i] = (__u8)FlashData;
      }
   } /* end_if 8 bit access */

   else
   {
      /* 16 bit access: */
      /*! SET returned Net Page data */
      NumBypassReads = FlsParmsP->PageSize;
      for (i = 0, DataU32P = (__u32 *)DataP; i < NumBypassReads / 4; i++)
      {
         for (j = 0, TmpU16P = (__u16 *)&TmpU32; j < 2; j++)
         {
            //V_FC_RegsP->FC_FBypCtrl.FBypRdFls = C_BitEnable;
            writel(DW_FC_FBYP_CTL_BP_READ_FLASH|DW_FC_FBYP_CTL_BP_EN,
                   (nand_info->nand + DW_FC_FBYP_CTL));
            //while (V_FC_RegsP->FC_FBypCtrl.FBypBusy != 0);
            while((readl(nand_info->nand + DW_FC_FBYP_CTL) & DW_FC_FBYP_CTL_BUSY) != 0);
            //FlashData = (__u16)(V_FC_RegsP->FC_FBypData.FlsData);
            FlashData = (__u16)((readl(nand_info->nand  + DW_FC_FBYP_DATA) 
                                & DW_FC_FBYP_DATA_FLASH_DATA) >> 16);
            *TmpU16P++ = FlashData;
         }
         *DataU32P++ = TmpU32;
      }

      /*! SET returned Spare data */
      NumBypassReads = FlsParmsP->SpareSize;
      for (i = 0, DataU32P = (__u32 *)SpareP; i < NumBypassReads / 4; i++)
      {
         for (j = 0, TmpU16P = (__u16 *)&TmpU32; j < 2; j++)
         {
            //V_FC_RegsP->FC_FBypCtrl.FBypRdFls = C_BitEnable;
            writel(DW_FC_FBYP_CTL_BP_READ_FLASH|DW_FC_FBYP_CTL_BP_EN,
                  (nand_info->nand + DW_FC_FBYP_CTL));

            //while (V_FC_RegsP->FC_FBypCtrl.FBypBusy != 0);
            while((readl(nand_info->nand + DW_FC_FBYP_CTL) & DW_FC_FBYP_CTL_BUSY) != 0);
            //FlashData = (__u16)(V_FC_RegsP->FC_FBypData.FlsData);
            FlashData = (__u16)((readl(nand_info->nand  + DW_FC_FBYP_DATA) 
                                & DW_FC_FBYP_DATA_FLASH_DATA) >> 16);
            *TmpU16P++ = FlashData;
         }
         *DataU32P++ = TmpU32;
      }
   } /* end_if 16 bit access */

   /*! POLL for transaction end */
   /* NOTE: This is a test to verify that you have performed all the declared Reads! */
   //while (V_FC_RegsP->FC_Stat.TransDone == 0);
   while((readl(nand_info->nand + DW_FC_STATUS) 
          & DW_FC_STATUS_TRANS_DONE) == 0);
   /*! DISABLE ByPass mode */
   //V_FC_RegsP->FC_FBypCtrl.FBypEn = C_BitDisable;
   writel(!DW_FC_FBYP_CTL_BP_EN, (nand_info->nand + DW_FC_FBYP_CTL));
   /*! CHECK if Timeout was marked (chip blocked) */
   //FC_StatImg = V_FC_RegsP->FC_Stat;
   //if (FC_StatImg.RdyTimeOut != 0)
    if(readl(nand_info->nand + DW_FC_STATUS) & DW_FC_STATUS_RDY_TIMEOUT)
   {
//      return (NFC_K_Err_TimeOut);
	ret_value = NFC_K_Err_TimeOut;
	goto out;
   }

   if (IsEccF != false)
   {
      F_FixEccErrors(nand_info, DataP, SpareP);
   }
out:
   /*! RETURN OK */
//   return (NFC_K_Err_OK);
   up(&unified_dw_nflc_mutex);
   return ret_value;

} /* end F_CmdReadPageAndSpareBypass() */


/**************************************************************************
* Function Name:  F_CmdWritePageAndSpareBypass
*
* Description:    Prepares a Command Sequence for Programing a whole
*                 physical Page, including Spare area.
*                 Activates the transaction and Bypasses the DMA&FIFO module,
*                 Writes the flash via the Controller's ByPass registers.
*                   1. Net Page size:  512 Bytes; Spare area size:  16 Bytes.
*                   2. Net Page size: 2048 Bytes; Spare area size:  64 Bytes.
*                   3. Net Page size: 4096 Bytes; Spare area size: 128 Bytes.
*
*                 In FPGA emulation of the DM56, Page (DataP) and Spare (SpareP) buffers
*                 !!! MUST !!! be allocated within the FPGA RAM, due to the DMA transfer !
*
* Input:          Chip:       Flash chip identity.
*                 FlsDev:     Flash Device number.
*                 Block:      Block number.
*                 PageInBlk:  Number of Page inside the Block.
*                 N1Len:      Ecc N1 field length.
*                 IsEccF:     Whether to activate ECC machine.
*                 DataP:  User supplied Page Data input buffer.
*                 SpareP: User supplied Page's Spare-Area input buffer.
*
*  Output:        None
*
* Return Value:   NFC_K_Err_OK:        Program OK.
*                 NFC_K_Err_ProgFail:  Program Error.
*
* NOTE: P/O the spare-area data contains the ECC (Error correction Code).
***************************************************************************/
/*static*/ NFC_E_Err  F_CmdWritePageAndSpareBypass( struct dw_nand_info *nand_info,
                                                __u16        Block,
                                                __u16        DataLen,
                                                __u16        SpareLen,
                                                __u8         PageInBlk,
                                                __u8         N1Len,
                                                bool          IsEccF,
                                                void *        DataP,
                                                void *        SpareP )
{
   const Y_NFlsParms *  FlsParmsP;
   Y_FC_Ctrl            FC_CtrlImg;
   Y_FC_CmdSeq          FC_CmdSeqImg;
   __u16               FlashData;
   __u16               i, j;
   __u16               NumBypassWrites;
   __u16 *             TmpU16P;
   __u32               TmpU32;
   __u32 *             DataU32P;
	unsigned int PageSize = NFC_F_PageNetSize(nand_info->FlsDev);
	unsigned int Column = 0; // We start the write operation from the beginning of the physical page.

   int ret_value = NFC_K_Err_OK;
   unsigned Cmd4En = 0, Wait4En = 0, ReadOnce = 0;

   down(&unified_dw_nflc_mutex);
   F_SetFixedCntrlRegs(nand_info->nand) ;

   *((__u32 *)&FC_CtrlImg) = 0;
   *((__u32 *)&FC_CmdSeqImg) = 0;

/*! DISABLE DMA & FIFO */
   DMF_F_ConfDmaBypass(&nand_info->dma_regs, DMF_K_PeriphFlsCntl);

   // Clear relevant bits in Status register
   writel(SET_32BIT, (nand_info->nand + DW_FC_STS_CLR));

	if (PageSize <= 512) {
		Column = F_PrepareSmallPageWriteTransaction(nand_info, Column);
	}

   /*! SET pointer to Flash chip parameters */
   FlsParmsP = &V_NFlsParms[nand_info->nand_chip_id];

   F_SetAddrCycles( nand_info->nand_chip_id, Block, PageInBlk, false );

   /*! CONFIGURE Flash-Controller registers: */
   /* NOTE: Image of FC_Ctrl register is needed, because a write to
   **       this register triggers a Flash-Controller transaction. */
   /*FC_CtrlImg.ChainCntStart = 0; // To be changed
   FC_CtrlImg.PageSelect = 0;
   FC_CtrlImg.EccOpMode = C_FC_Ctrl_EccOpMode_MemTrans; // Memory transaction*/

   // Disable all FC interrupts - To be changed
   /* NOTE: This register is configured by F_Init() */

   // Configutre Sequence register
   //F_SetChipSelectAndReadyLine(FlsDev, &FC_CmdSeqImg);

   /*FC_CmdSeqImg.RdyInEn = C_BitEnable;      // Monitor Ready line
   FC_CmdSeqImg.DataEccEn = IsEccF ? C_BitEnable: C_BitDisable; // Enable / Disable ECC
   FC_CmdSeqImg.DataRWDir = 1;              // Wrire to Flash
   FC_CmdSeqImg.DataRWEn = C_BitEnable;     // Enable Read/Write transfer
   FC_CmdSeqImg.ReadOnce = C_BitEnable;     // Enable Status read
   FC_CmdSeqImg.KeepChpSlct = C_BitDisable; // Don't keep CS for next transaction
   FC_CmdSeqImg.Wait4En  = C_BitEnable;
   FC_CmdSeqImg.Cmd4En   = C_BitEnable;  // Status Read command
   FC_CmdSeqImg.Wait3En  = C_BitEnable;  // tWB
   FC_CmdSeqImg.Cmd3En   = C_BitEnable;  // Prog 2nd command
   FC_CmdSeqImg.Wait2En  = C_BitDisable;
   FC_CmdSeqImg.Cmd2En   = C_BitDisable; // CMD2 unused here
   FC_CmdSeqImg.Wait1En  = C_BitDisable;
   FC_CmdSeqImg.Cmd1En   = C_BitEnable;  // Prog 1st command
   FC_CmdSeqImg.Addr3En  = C_BitEnable;
   FC_CmdSeqImg.Addr2En  = C_BitEnable;
   FC_CmdSeqImg.Addr1En  = C_BitEnable;
   */
   M_SetUpReadStatus(Cmd4En, V_FlsCmnd4, Wait4En, ReadOnce);
   DW_FC_SET(nand_info->nand, DW_FC_SEQUENCE,
             DW_FC_SEQ_CMD1_EN, DW_FC_SEQ_CMD3_EN, Cmd4En,
             DW_FC_SEQ_ADD1_EN, DW_FC_SEQ_ADD2_EN, DW_FC_SEQ_ADD3_EN,
             DW_FC_SEQ_WAIT3_EN, Wait4En,
             0,0);
   DW_FC_ADD(nand_info->nand, DW_FC_SEQUENCE,
             DW_FC_SEQ_RW_EN, DW_FC_SEQ_DATA_WRITE_DIR,
             DW_FC_SEQ_DATA_ECC(IsEccF), ReadOnce,
             DW_FC_SEQ_CHIP_SEL(nand_info->FlsDev), DW_FC_SEQ_RDY_EN, DW_FC_SEQ_RDY_SEL(nand_info->FlsDev >>1),
             0,0,0);

   switch (nand_info->nand_chip_id)
   {
      // 8 bit devices
      case NFC_K_Chip_SamK9F5608:
         /*FC_CmdSeqImg.Mode16 = K_FlsAccess_8bit;
         FC_CmdSeqImg.Addr5En  = C_BitDisable;
         FC_CmdSeqImg.Addr4En  = C_BitDisable;
         */
         DW_FC_ADD(nand_info->nand, DW_FC_SEQUENCE,
                   DW_FC_SEQ_MODE8/*8 bit*/,
                   0,0,0,0,0,0,0,0,0);
         break;
      case NFC_K_Chip_TosTH58DVG02:
         /*FC_CmdSeqImg.Mode16 = K_FlsAccess_8bit;
         FC_CmdSeqImg.Addr5En  = C_BitDisable;  // 
         FC_CmdSeqImg.Addr4En  = C_BitEnable;  // 
         */
         DW_FC_ADD(nand_info->nand, DW_FC_SEQUENCE,
                   DW_FC_SEQ_ADD4_EN, 0,
                   DW_FC_SEQ_MODE8/*8 bit*/,
                   0,0,0,0,0,0,0);
         break;

      case NFC_K_Chip_SamK9F1G08:
      case NFC_K_Chip_SamK9F2G08:
      case NFC_K_Chip_TosTH58NVG3:
      case NFC_K_Chip_TosTH58NVG1S3A:
         /*FC_CmdSeqImg.Mode16 = K_FlsAccess_8bit;
         FC_CmdSeqImg.Addr5En  = C_BitEnable;  // for 3rd Row addr
         FC_CmdSeqImg.Addr4En  = C_BitEnable;  // for 2nd Row addr
         */
         DW_FC_ADD(nand_info->nand, DW_FC_SEQUENCE,
                   DW_FC_SEQ_ADD4_EN, DW_FC_SEQ_ADD5_EN,
                   DW_FC_SEQ_MODE8/*8 bit*/,
                   0,0,0,0,0,0,0);
         break;

      //16 bit devices
      case NFC_K_Chip_SamK9F5616:
         /*FC_CmdSeqImg.Mode16 = K_FlsAccess_16bit;
         FC_CmdSeqImg.Addr5En  = C_BitDisable;
         FC_CmdSeqImg.Addr4En  = C_BitDisable;
         */
         DW_FC_ADD(nand_info->nand, DW_FC_SEQUENCE,
                   DW_FC_SEQ_MODE16/*16 bit*/,
                   0,0,0,0,0,0,0,0,0);
         break;

      case NFC_K_Chip_SamK9F1G16:
         /*FC_CmdSeqImg.Mode16 = K_FlsAccess_16bit;
         FC_CmdSeqImg.Addr5En = C_BitDisable;
         FC_CmdSeqImg.Addr4En = C_BitEnable;   // for 2nd Row addr
         */
         DW_FC_ADD(nand_info->nand, DW_FC_SEQUENCE,
                   DW_FC_SEQ_ADD4_EN, DW_FC_SEQ_MODE16/*16 bit*/,
                   0,0,0,0,0,0,0,0);
         break;

      default:
         DW_SPOT("Unknown chip: %0#10x, driver is stuck to prevent system crash.", Chip);
         while (1);
         break;
   } /* end_switch chip */

   //V_FC_RegsP->FC_CmdSeq = FC_CmdSeqImg;

   // Configure Address registers
   /*V_FC_RegsP->FC_AddrCol.Addr1 = V_FlsCycle1;
   V_FC_RegsP->FC_AddrCol.Addr2 = V_FlsCycle2;
   V_FC_RegsP->FC_AddrRow.Addr3 = V_FlsCycle3;
   V_FC_RegsP->FC_AddrRow.Addr4 = V_FlsCycle4;
   V_FC_RegsP->FC_AddrRow.Addr5 = V_FlsCycle5;*/
   DW_FC_SET(nand_info->nand, DW_FC_ADDR_COL,
             DW_FC_ADD1(V_FlsCycle1), DW_FC_ADD2(V_FlsCycle2),
             0,0,0,0,0,0,0,0);
   DW_FC_SET(nand_info->nand, DW_FC_ADDR_ROW,
             DW_FC_ADD3(V_FlsCycle3), DW_FC_ADD4(V_FlsCycle4), DW_FC_ADD5(V_FlsCycle5),
             0,0,0,0,0,0,0);

   // Configure Command Code register
   /*V_FC_RegsP->FC_Cmd.Cmd1 = V_FlsCmnd1;
   V_FC_RegsP->FC_Cmd.Cmd2 = V_FlsCmnd2; // Not used in this operation.
   V_FC_RegsP->FC_Cmd.Cmd3 = V_FlsCmnd3;
   V_FC_RegsP->FC_Cmd.Cmd4 = V_FlsCmnd4;*/
   DW_FC_SET(nand_info->nand, DW_FC_CMD,
             DW_FC_CMD1(V_FlsCmnd1), DW_FC_CMD2(V_FlsCmnd2),
             DW_FC_CMD3(V_FlsCmnd3), DW_FC_CMD4(V_FlsCmnd4),
             0,0,0,0,0,0);

   // Configure WaitTime register
   /* NOTE: This register is configured by F_Init() */

   // Configure PulseTime register
   /* NOTE: This register is configured by F_Init() */

   /*****************************************************/
#if 0
   // Patch fot testing only ! - To be removed !!!
   ((Y_NFlsParms * )FlsParmsP)->PageSize = 4;
   ((Y_NFlsParms * )FlsParmsP)->SpareSize = 4;
#endif
   /*****************************************************/

   // Configure DataCount register
   F_SetDataCount(nand_info->nand, DataLen , SpareLen, IsEccF, N1Len);

   // Configure Ready-Timeout register
   /*V_FC_RegsP->FC_TimeOut.RdyTimeOutEn = C_BitEnable;
   V_FC_RegsP->FC_TimeOut.RdyTimeOutCnt = C_FlsRdyTimeout;*/
   DW_FC_SET(nand_info->nand, DW_FC_TIMEOUT,
             DW_FC_TIMEOUT_RDY_EN, DW_FC_TIMEOUT_RDY_CNT(C_FlsRdyTimeout),
             0,0,0,0,0,0,0,0);

   /*! SET Bypass enabled */
   //V_FC_ResP->FC_FBypCtrl.FBypEn = C_BitEnable;
   writel(DW_FC_FBYP_CTL_BP_EN, (nand_info->nand + DW_FC_FBYP_CTL));

   writel(SET_32BIT, (nand_info->nand + DW_FC_STS_CLR));
   /*! TRIGGER Flash controller */
NFLC_CAPTURE();
   //V_FC_RegsP->FC_Ctrl = FC_CtrlImg;
   writel(DW_FC_CTL_ECC_OP_MODE(DW_FC_MEM_TRANS), (nand_info->nand + DW_FC_CTL));

   /*! WRITE Flash Data via Bypass */

   /* Note: When DataP & SpareP point to buffers in FPGA memory, access is restricted to 32 bit only */
   if (FlsParmsP->AccMode == K_FlsAccess_8bit)
   {
      /* 8 bit access: */
      /*! PROGRAM Net Page data */
    for(i=0; i<DataLen; i++)
    {
        //while (V_FC_RegsP->FC_FBypCtrl.FBypBusy != 0);
        while((readl(nand_info->nand + DW_FC_FBYP_CTL) & DW_FC_FBYP_CTL_BUSY) != 0);
        //V_FC_RegsP->FC_FBypData.FifoData = *TmpU8P++;
        writel(((__u8 *)DataP)[i], (nand_info->nand + DW_FC_FBYP_DATA));
        //V_FC_RegsP->FC_FBypCtrl.FBypWr = C_BitEnable;
        writel(DW_FC_FBYP_CTL_BP_EN|DW_FC_FBYP_CTL_BP_WRITE,
               (nand_info->nand + DW_FC_FBYP_CTL));
    }

      /*! Program Spare data */
      for(i=0; i<SpareLen; i++)
      {
          //while (V_FC_RegsP->FC_FBypCtrl.FBypBusy != 0);
          while((readl(nand_info->nand + DW_FC_FBYP_CTL) 
                 & DW_FC_FBYP_CTL_BUSY) != 0);
          //V_FC_RegsP->FC_FBypData.FifoData = *TmpU8P++;
          writel(((__u8 *)SpareP)[i], (nand_info->nand + DW_FC_FBYP_DATA));
          //V_FC_RegsP->FC_FBypCtrl.FBypWr = C_BitEnable;
          writel(DW_FC_FBYP_CTL_BP_EN|DW_FC_FBYP_CTL_BP_WRITE,
                 (nand_info->nand + DW_FC_FBYP_CTL));
      }
   } /* end_if 8 bit access */

   else
   {
      /* 16 bit access: */

      /*! PROGRAM Net Page data */
      NumBypassWrites = FlsParmsP->PageSize;
      for (i = 0, DataU32P = (__u32 *)DataP; i < NumBypassWrites / 4; i++)
      {
         TmpU32 = *DataU32P++;
         for (j = 0, TmpU16P = (__u16 *)&TmpU32; j < 2; j++)
         {
            //while (V_FC_RegsP->FC_FBypCtrl.FBypBusy != 0);
            while((readl(nand_info->nand + DW_FC_FBYP_CTL) & DW_FC_FBYP_CTL_BUSY) != 0);
            //V_FC_RegsP->FC_FBypData.FifoData = *TmpU16P++;
//            writel(*TmpU8P++, (DW_NAND_LCD_BASE + DW_FC_FBYP_DATA)); // R.B.
            writel(*TmpU16P++, (nand_info->nand + DW_FC_FBYP_DATA));
            //V_FC_RegsP->FC_FBypCtrl.FBypWr = C_BitEnable;
            writel(DW_FC_FBYP_CTL_BP_EN|DW_FC_FBYP_CTL_BP_WRITE,
                   (nand_info->nand + DW_FC_FBYP_CTL));
         }
      }

      /*! Program Spare data */
      NumBypassWrites = FlsParmsP->SpareSize;
      for (i = 0, DataU32P = (__u32 *)SpareP; i < NumBypassWrites / 4; i++)
      {
         TmpU32 = *DataU32P++;
         for (j = 0, TmpU16P = (__u16 *)&TmpU32; j < 2; j++)
         {
            //while (V_FC_RegsP->FC_FBypCtrl.FBypBusy != 0);
            while((readl(nand_info->nand + DW_FC_FBYP_CTL) & DW_FC_FBYP_CTL_BUSY) != 0);
            //V_FC_RegsP->FC_FBypData.FifoData = *TmpU16P++;
//            writel(*TmpU8P++, (DW_NAND_LCD_BASE + DW_FC_FBYP_DATA)); // R.B.
            writel(*TmpU16P++, (nand_info->nand + DW_FC_FBYP_DATA)); // R.B.
            //V_FC_RegsP->FC_FBypCtrl.FBypWr = C_BitEnable;
            writel(DW_FC_FBYP_CTL_BP_EN|DW_FC_FBYP_CTL_BP_WRITE,
                   (nand_info->nand + DW_FC_FBYP_CTL));
         }
      }
   } /* end_if 16 bit access */


   /*! POLL for transaction end */
   /* NOTE: This is a test to verify that you have performed all the declared Writes! */
   //while (V_FC_RegsP->FC_Stat.TransDone == 0);
   while((readl(nand_info->nand + DW_FC_STATUS) & DW_FC_STATUS_TRANS_DONE) == 0);

   /* DISABLE ByPass Mode */
   //F_DisableBypassMode();
   writel(!DW_FC_FBYP_CTL_BP_EN, (nand_info->nand + DW_FC_FBYP_CTL));

   /*! CHECK if Timeout was marked (chip blocked) */
   //FC_StatImg = V_FC_RegsP->FC_Stat;
   //if (FC_StatImg.RdyTimeOut != 0)
   if(readl(nand_info->nand + DW_FC_STATUS) & DW_FC_STATUS_RDY_TIMEOUT)
   {
//      return (NFC_K_Err_TimeOut);
	ret_value = NFC_K_Err_TimeOut;
	goto out;
   }
#if 0
   /*! CHECK Programming result reported by the flash Chip (read by "ReadOnce") */
   //FlashData = (__u16)(V_FC_RegsP->FC_FBypData.FlsData);
   FlashData = (__u16)((readl(DW_NAND_LCD_BASE  + DW_FC_FBYP_DATA) 
                       & DW_FC_FBYP_DATA_FLASH_DATA) >> 16);

   if(FlashData == ((__u16)((readl(DW_NAND_LCD_BASE  + DW_FC_FBYP_DATA) 
                       & DW_FC_FBYP_DATA_FIFO_DATA))))
   {
      //printk("\nWORKAROUND_WRITE_PAGE OK!!!\n");
//      return (NFC_K_Err_OK);
	ret_value = NFC_K_Err_OK;
	goto out;
   }
#endif
   FlashData = M_ReadStatus(nand_info);

   if ((FlashData & 0x001) != 0)
   {
//      return (NFC_K_Err_ProgFail);
		ret_value = NFC_K_Err_ProgFail;
		goto out;
   }

out:
   up(&unified_dw_nflc_mutex);
//   return (NFC_K_Err_OK);
	return ret_value;

} /* end F_CmdWritePageAndSpareBypass() */



/**************************************************************************
* Function Name:  F_DisableBypassMode
*
* Description:    Configures the FIFO ByPass Control Register (FC_FBYP_CTL)
*                 to Disable bypass mode.
*
* Input:          None.
* Output:         None.
* Return Value:   None.
***************************************************************************/
#ifdef UNUSED_CODE
/*static*/ void inline F_DisableBypassMode( void )
{
   Y_FC_FBypCtrl  FC_FBypCtrlImg;

   *((__u32 *)&FC_FBypCtrlImg) = 0;
   V_FC_RegsP->FC_FBypCtrl = FC_FBypCtrlImg;

} /* end F_DisableBypassMode() */
#endif




