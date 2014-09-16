/********************************************************************************
*              D M A  &  F I F O   C O N T R O L L E R
*********************************************************************************
*
*   Title:           DMA and FIFO controller Internal implementation.
*
*   Filename:        $Workfile: $
*   SubSystem:
*   Authors:         Avi Miller
*   Latest update:   $Modtime: $
*   Created:         21 Mars, 2005
*
*********************************************************************************
*  Description:      Low-Level Generic DMA and FIFO controller.
*                    Running on ARM 7 in the DM-56 chip.
*                    Configures the access from Flash/IDE to the AHB bus.
*
*********************************************************************************
*  $Header: $
*
*  Change history:
*  ---------------
*  $History: $
*
********************************************************************************/


/*=======   Compilation switces ==========================================*/

/*========================  Include Files  ===============================*/


#include "dw_dma_cntl.h"


/*========================  Globals  ===============================*/

Y_DMAConfig dw_DMA_config;


/**************************************************************************
*              P U B L I C    F U N C T I O N S
**************************************************************************/


/**************************************************************************
* Function Name:  DMF_F_ConfDmaFifo
*
* Description:    Configures the generic DMA controller, FIFO and GCR2
*
* Input:          Direction: Transfer direction, to/from AHB bus
*                 AhbPeriph: Peripheral: Flash-Controller or IDE or MMC card.
*                 FifoWidth: FIFO external bus width: 8 or 16 bits.
*                 DataSize:  Data size in bytes: Flash - Net Page; IDE - Block
*                 SpareSize: Flash - Spare size in bytes: IDE/MMC - Must(!) be 0.
*
* Output:         DataP:  Pointer to output Data buffer, in ARM addr space.
*                 SpareP: Pointer to output Spare buffer, in ARM addr space (NULL for IDE/MMC).
*
* Return Value:   None.
***************************************************************************/
void  DMF_F_ConfDmaFifo(  struct dw_dma_params *dw_dma_param,
			 DMF_E_DmaDir   Direction,
                         DMF_E_Periph   AhbPeriph,
                         DMF_E_Width    FifoWidth,
                         __u16         DataSize,
                         __u16         SpareSize,
                         __u32 *       DataP,
                         __u32 *       SpareP )
{
//   Y_FIFO_cntl  FIFO_cntlImg;
//   Y_DMA_Cntl   DMA_CntlImg;
/*
DW_SPOT("Direction %u, DW_FIFO_CTL_FIFO_MODE(Direction) %u, AhbPeriph %u, \nFifoWidth %u, DataSize %u, SpareSize %u, DataP %0#10x, SpareP %0#10x", 
		Direction,DW_FIFO_CTL_FIFO_MODE(Direction),AhbPeriph,FifoWidth,DataSize,SpareSize,(unsigned)DataP,(unsigned)SpareP);
*/
   /*! RESET FIFO */
   /*V_FIFO_RegsP->FIFO_Rst.FifoReset = 1;
   V_FIFO_RegsP->FIFO_Icr.OverrunIntrClr = 1;
   V_FIFO_RegsP->FIFO_Icr.UnderrunIntrClr = 1;*/
   writel(SET, (dw_dma_param->fifo_base_reg + DW_FIFO_RST));
   writel(0x0, (dw_dma_param->fifo_base_reg + DW_FIFO_IER));
   writel(DW_FIFO_ICR_OVER|DW_FIFO_ICR_UNDER, (dw_dma_param->fifo_base_reg + DW_FIFO_ICR));
   

   /*! CONFIGURE FIFO */
   /*FIFO_cntlImg.FifoEn = 0; // Disable Fifo for now
   switch (FifoWidth)
   {
      case DMF_K_Width8bit:
         FIFO_cntlImg.FifoWidth = C_FIFO_FifoWidth8bit;
         break;
      case DMF_K_Width16bit:
         FIFO_cntlImg.FifoWidth = C_FIFO_FifoWidth16bit;
         break;
      default:
         //break;
   }*/
   /*FIFO_cntlImg.FifoMode =
      (Direction == DMF_K_DmaDirReadAHB) ? C_FIFO_FifoMode_WriteFls : C_FIFO_FifoMode_ReadFls;*/
   //V_FIFO_RegsP->FIFO_cntl = FIFO_cntlImg;

   DW_FIFO_SET(dw_dma_param->fifo_base_reg, DW_FIFO_CTL,
             DW_FIFO_CTL_FIFO_WIDTH(FifoWidth), DW_FIFO_CTL_FIFO_MODE(Direction),
             0,0,0,0,0,0,0,0);

   /*V_FIFO_RegsP->FIFO_AeLvl.LowWaterMark = C_FIFO_LowWaterMark;
   V_FIFO_RegsP->FIFO_AfLvl.HighWaterMark = C_FIFO_HighWaterMark;
   V_FIFO_RegsP->FIFO_TstEn.FifoTstEn = 0;*/
   writel(C_FIFO_LowWaterMark, (dw_dma_param->fifo_base_reg + DW_FIFO_AE_LEVEL));
   writel(C_FIFO_HighWaterMark, (dw_dma_param->fifo_base_reg + DW_FIFO_AF_LEVEL));
   writel(0, (dw_dma_param->fifo_base_reg + DW_FIFO_TST_EN)); // R.B. 22/05/2008

   // Temp: Disable FIFO interrupts
   /*V_FIFO_RegsP->FIFO_Ier.OverrunIntrEn = 0;
   V_FIFO_RegsP->FIFO_Ier.UnderrunIntrEn = 0;*/
   //previously implemented


   /*! CONFIGURE DMA Controller */
   //DMA_CntlImg.DmaEn = 0; // Disable DMA for now
   /*DMA_CntlImg.DmaMode = (Direction == DMF_K_DmaDirReadAHB) ? C_DMA_DmaMode_ReadAhb : C_DMA_DmaMode_WriteAhb;
   if ((SpareSize > 0) && (AhbPeriph == DMF_K_PeriphFlsCntl))
   {*/
      /* **********************************************************************
      *  Temp, for H/W Bug Bypass Only !!! :                                  *
      *     Disable splitting between Page & Spare Data buffres !             *
      *     Also Spare buffer Must be adjacent (right after)                  *
      *     The Page Buffer !                                                 *
      *                                                                       *
      *  Should be set to C_DMA_FlsMode_Enable when H/W bug is corrected.     *
      *                                                                       *
      ************************************************************************/
      /*DMA_CntlImg.FlsMode = C_DMA_FlsMode_Disable;
      //DMA_CntlImg.FlsMode = C_DMA_FlsMode_Enable;

      // For Flash_Mode only: Physical Spare size in 32bit words
      DMA_CntlImg.EccLen = SpareSize >> 2;
   }
   else
   {
      DMA_CntlImg.FlsMode = C_DMA_FlsMode_Disable;
      DMA_CntlImg.EccLen = 0;
   }
   DMA_CntlImg.SwapMode = C_DMA_SwapMode_Disable;

   V_DMA_RegsP->DMA_Cntl = DMA_CntlImg;*/


   writel(0x0, (dw_dma_param->dma_base_reg + DW_DMA_IER));
   writel(DW_DMA_ICR_DMA_DONE|DW_DMA_ICR_DMA_ERROR, (dw_dma_param->dma_base_reg + DW_DMA_ICR));
/*
   DW_FC_SET(DW_DMA_CTL,
             DW_DMA_DMA_MODE(Direction), DW_DMA_FLASH_MODE(C_DMA_FlsMode_Enable),
             DW_DMA_FLASH_ECC_WORDS(SpareSize >> 2), DW_DMA_SWAP_MODE(C_DMA_SwapMode_Disable),
             0,0,0,0,0,0);
*/ // R.B. 22/05/2008 - disabling FlsMode due to HW bug
   DW_DMA_SET(dw_dma_param->dma_base_reg ,DW_DMA_CTL,
             DW_DMA_DMA_MODE(Direction), DW_DMA_FLASH_MODE(C_DMA_FlsMode_Disable), // R.B. 22/05/2008
             DW_DMA_FLASH_ECC_WORDS(SpareSize >> 2), DW_DMA_SWAP_MODE(C_DMA_SwapMode_Disable),
             0,0,0,0,0,0);

   //V_DMA_RegsP->DMA_Addr1.PageDataTrgt = NFC_M_ArmToFpgaAddr(DataP);
   //V_DMA_RegsP->DMA_Addr2.SpareDataTrgt = NFC_M_ArmToFpgaAddr(SpareP);
   writel((volatile unsigned int)DataP, (dw_dma_param->dma_base_reg + DW_DMA_ADDR1));
   writel((volatile unsigned int)SpareP, (dw_dma_param->dma_base_reg + DW_DMA_ADDR2));

   // Burst size in 32bit Words
   /*V_DMA_RegsP->DMA_Burst.BurstLen =
      (Direction == DMF_K_DmaDirReadAHB) ? C_FIFO_LowWaterMark : C_FIFO_HighWaterMark;*/
   writel(0x8, (dw_dma_param->dma_base_reg + DW_DMA_BURST));
   
   // Data (for Flash_Mode: Physical Page+Spare) size in 32bit words
   //V_DMA_RegsP->DMA_BlkLen.BlkLen = (DataSize + SpareSize) >> 2;
   writel((DataSize + SpareSize) >> 2, (dw_dma_param->dma_base_reg + DW_DMA_BLK_LEN));
   //V_DMA_RegsP->DMA_BlkCnt.NumBlks = 1;
   writel(0x1, (dw_dma_param->dma_base_reg + DW_DMA_BLK_CNT));
   // Temp: Disable DMA interrupts
   //V_DMA_RegsP->DMA_Ier.XferDoneIntrEn = 0;
   //V_DMA_RegsP->DMA_Ier.AhbErrIntrEn = 0;
   writel(0, (dw_dma_param->dma_base_reg + DW_DMA_IER)); // R.B. 22/05/2008
   //V_DMA_RegsP->DMA_Icr.XferDoneIntrClr = 1;
   //V_DMA_RegsP->DMA_Icr.AhbErrIntrClr = 1;
   writel(0x3, (dw_dma_param->dma_base_reg + DW_DMA_ICR)); // R.B. 22/05/2008
   

   /*! CONFIGURE GCR2 */
   /**V_GCR2RegP &= ~C_GCR2_PeriphMask;
   switch (AhbPeriph)
   {
      case DMF_K_PeriphFlsCntl:
         *V_GCR2RegP |= C_GCR2_DmaWithFifoFlsCntl;
         break;
      case DMF_K_PeriphIde:
         *V_GCR2RegP |= C_GCR2_DmaWithFifoIde;
         break;
      case DMF_K_PeriphMmc:
         *V_GCR2RegP |= C_GCR2_DmaNoFifoMmc;
         break;
      default:
         break;
   }*/

} /* end DMF_F_ConfDmaFifo() */


/**************************************************************************
* Function Name:  DMF_F_TrigDma
*
* Description:    Triggers a DMA transaction, usint the FIFO
*
* Input:          None.
* Output:         None.
* Return Value:   None.
***************************************************************************/
void  DMF_F_TrigDma(  struct dw_dma_params *dw_dma_param )
{
   //V_FIFO_RegsP->FIFO_cntl.FifoEn = 1;
   writel(readl(dw_dma_param->fifo_base_reg + DW_FIFO_CTL)|DW_FIFO_CTL_FIFO_EN, (dw_dma_param->fifo_base_reg + DW_FIFO_CTL));
   //V_DMA_RegsP->DMA_Cntl.DmaEn = 1;
   //V_DMA_RegsP->DMA_Start.Trig = 1;
   writel(readl(dw_dma_param->dma_base_reg + DW_DMA_CTL)|DW_DMA_EN, (dw_dma_param->dma_base_reg + DW_DMA_CTL));
   writel(DW_DMA_START_TRIG, (dw_dma_param->dma_base_reg + DW_DMA_START));
} /* end DMF_F_TrigDma() */


/**************************************************************************
* Function Name:  DMF_F_ConfDmaBypass
*
* Description:    Disables the DMA and FIFO modules, for Bypass read/write
*                 the Falsh device. Still configures GCR2 register, for
*                 setting the MUX'es selection.
*
* Input:          AhbPeriph: Flash, IDE or MMC
*
* Output:         None.
*
* Return Value:   None.
***************************************************************************/
void inline DMF_F_ConfDmaBypass( struct dw_dma_params *dw_dma_param, DMF_E_Periph  AhbPeriph )
{

   /*************************************************************************************
   ** NOTE !
   **
   ** In FPGA Emulation of the DM-56, version 1.6 of 30 Jan, 2005 (Eli Reuven)
   ** You Write GCR2 as the ususal bit description (e.g. the Ahb2_periph-select bits
   ** are bits 9,8), but when Reading this register, bits ARE SHIFTED as follows:
   **    Bits 4 - 1 --> Bits 3 - 0
   **    Bits 9 - 8 --> Bits 5 - 4
   **    Other bits are Not Readable!
   **
   ** For example: Writing 0x0100 will result Reading of 0x0010
   **
   *************************************************************************************/

#if 0
   /*! CONFIGURE GCR2 */
   *V_GCR2RegP &= ~C_GCR2_PeriphMask;
   switch (AhbPeriph)
   {
      case DMF_K_PeriphFlsCntl:
         *V_GCR2RegP |= C_GCR2_DmaWithFifoFlsCntl;
         break;
      case DMF_K_PeriphIde:
         *V_GCR2RegP |= C_GCR2_DmaWithFifoIde;
         break;
      case DMF_K_PeriphMmc:
         *V_GCR2RegP |= C_GCR2_DmaNoFifoMmc;
         break;
      default:
         /*! LOG Error */
         break;
   }
#endif

   /*! DISABLE DMA & FIFO */
   //V_DMA_RegsP->DMA_Cntl.DmaEn = 0;
   writel(!DW_DMA_EN, (dw_dma_param->dma_base_reg + DW_DMA_CTL));
   //V_FIFO_RegsP->FIFO_cntl.FifoEn = 0;
   writel(!DW_FIFO_CTL_FIFO_EN, (dw_dma_param->fifo_base_reg + DW_FIFO_CTL));

} /* end DMF_F_ConfDmaBypass() */


/**************************************************************************
* Function Name:  DMF_F_IsXferDone
*
* Description:    Checks whether a DMA transfer has been completed.
*
* Input:          None.
* Output:         None.
*
* Return Value:   true:  Transaction is Done;
*                 false: Transaction has not ended yet.
***************************************************************************/
bool  DMF_F_IsXferDone( struct dw_dma_params *dw_dma_param )
{
//   return ( V_DMA_RegsP->DMA_Isr.XferDoneStat == 0 ? false : true );
   return (readl(dw_dma_param->dma_base_reg + DW_DMA_ISR) & DW_DMA_ISR_DMA_DONE) == 0 ? false : true;

} /* end DMF_F_IsXferDone() */



/*==========================   End of File   ============================*/

