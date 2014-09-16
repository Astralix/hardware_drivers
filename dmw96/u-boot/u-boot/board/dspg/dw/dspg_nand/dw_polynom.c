/********************************************************************************
*              N A N D   F L A S H   C O N T R O L L E R
*********************************************************************************
*
*   Title:           Polynom math operations implementation.
*
*   Filename:        $Workfile: $
*   SubSystem:
*   Authors:         Avi Miller
*   Latest update:   $Modtime: $
*   Created:         10 APR. 2005
*
*********************************************************************************
*  Description:
*
*  This package implements binary Polynom math operations.
*  A polynom is represented by a uint32 number.
*  Each bit n represents the coefficient C of the
*  C * 2^n expression, where Ci is either '0' or '1'.
*
*  The package supports 16-bits Polynoms, that is Polynoms up to Order of 15, i.e:
*
*  C15*2^15 + C14*2^14 + C13*2^13 + ... + C2*2^2 + C1*2 + C0
*
*  where Ci = 0 or 1.
*
*  In order to save precision in polynoms Multiplication, the polynoms are saved
*  internally as uint32 values.
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
#include "dw_nfls_type_defs.h"
#include "dw_polynom.h"

/*========   Local function prototypes   =================================*/

static void  F_AdjOrderShort( POL_Y_Polynom *  PolP );
static void  F_AdjOrderLong( POL_Y_Polynom *  PolP );


/**************************************************************************
*              P U B L I C    F U N C T I O N S
**************************************************************************/


/**************************************************************************
* Function Name:  POL_F_Clr
*
* Description:    Sets a Polynom to be a Zero polynom.
*
* Input:          PolP: A pointer to a Polynom structure.
*
* Output:         None.
*
* Return Value:   None.
***************************************************************************/
void  POL_F_Clr( POL_Y_Polynom *  PolP )
{
   if (PolP == NULL)
   {
      return;
   }

   PolP->PolData = 0x0000;
   PolP->PolOrder = 0;

} /* end POL_F_Clr() */


/**************************************************************************
* Function Name:  POL_F_InitFromStr
*
* Description:    Sets a Polynom from an input "binary" character string,
*                 holding up to 16 coefficients.
*
* Input:          PolP:   A pointer to a Polynom.
*                 PolStr: Character string, holding '0' and '1', defining a
*                         binary Polynom.
*
* Output:         None.
*
* Return Value:   None.
***************************************************************************/
void  POL_F_InitFromStr( POL_Y_Polynom *  PolP,  const char *  PolStr )
{
   bool          EndFound;
   const char *  tmpPolStr;
   __u32        bitMask;
   __s8          i;


   if (PolP == NULL)
   {
      return;
   }
   POL_F_Clr(PolP);

   if (PolStr == NULL)
   {
      return;
   }

   EndFound = false;
   tmpPolStr = PolStr;
   /*! FIND Polynom's order - upto 15 (POL_C_MaxPolOrder/2 - 1) = (16 - 1) */
   for (i = 0; ((i < POL_C_MaxPolOrder / 2) && (EndFound == false)); /* No increment here */)
   {
      switch (*tmpPolStr++)
      {
         case '0':
         case '1':
            i++;
            break;

         case '\0':  // end of input string
            EndFound = true;
            break;

         default:
            /*! SKIP "non-Binary" characters */
            break;

      } /* end_switch */
   } /* end_for i */

   /* NOTE: i now contains the number of coefficients found (<=16) **
   **       The polynoms Order is (number of coefficients - 1)     */

   if (EndFound != false)
   {
      /*! SET polynom's order */
      PolP->PolOrder = --i;

      /*! SET polynom's coefficients */
      tmpPolStr = PolStr;
      bitMask = 0x0001 << i;
      while (i >= 0)
      {
         switch (*tmpPolStr++)
         {
            case '1':
               PolP->PolData |= bitMask;
               /* No 'break' - Fall Through ! */
            case '0':
               i--;
               bitMask >>= 1;
               break;

            default:
               break;

         }
      } /* end_while i */

      /*! ADJUST polynom's order in case it has leading zeros coefficient(s) */
      F_AdjOrderLong(PolP);

   }  /* end_if EndFound */

} /* end POL_F_InitFromStr() */


/**************************************************************************
* Function Name:  POL_F_InitFromVal
*
* Description:    Sets a Polynom from an input "binary" character string,
*                 holding up to 16 coefficients.
*
* Input:          PolP:   A pointer to a Polynom.
*                 PolVal: A __u16 number, representing the Polynom coefficients.
*
* Output:         None.
*
* Return Value:   None.
***************************************************************************/
void  POL_F_InitFromVal( POL_Y_Polynom *  PolP,  __u16  PolVal )
{
   if (PolP == NULL)
   {
      return;
   }

   PolP->PolData = (__u32)PolVal;
   PolP->PolOrder = (POL_C_MaxPolOrder / 2) - 1; // 15

   /*! ADJUST polynom's order in case it has leading zeros coefficient(s) */
   F_AdjOrderLong(PolP);

} /* end POL_F_InitFromVal() */


/**************************************************************************
* Function Name:  POL_F_Assign
*
* Description:    Sets one Polynom value as another Polynom - Copy Polynoms
*
* Input:          PolInP: A pointer to the Reference Polynome.
*
* Output:         PolResP:  A pointer to the Result Polynom: Set to be equal to
*                           Reference Polynom.
*
* Return Value:   None.
***************************************************************************/
void  POL_F_Assign( POL_Y_Polynom *  PolInP, POL_Y_Polynom *  PolResP )
{
   if (PolResP == NULL)
   {
      return;
   }
   POL_F_Clr(PolResP);

   if (PolInP == NULL)
   {
      return;
   }

   *PolResP = *PolInP;

} /* end POL_F_Assign() */


/**************************************************************************
* Function Name:  POL_F_IsZero
*
* Description:    Compare a Polynom to Zero polynom.
*
* Input:          PolP: A pointer to a Polynom structure.
*
* Output:         None.
*
* Return Value:   true: Polynom equals the Zero polynom; false: not a Zero polynom.
***************************************************************************/
bool  POL_F_IsZero( POL_Y_Polynom *  PolP )
{
   if (PolP == NULL)
   {
      return false;
   }

   if ((PolP->PolOrder == 0) && (PolP->PolData == 0x00000000))
   {
      return (true);
   }
   return (false);

} /* end POL_F_IsZero() */


/**************************************************************************
* Function Name:  POL_F_Compare
*
* Description:    Compares two Polynoms.
*
* Input:          Pol1P:  A pointer to the first Polynom structure.
*                 Pol2P:  A pointer to the second Polynom structure.
*
* Output:         None.
*
* Return Value:   true: Polynoms are Equal; false: Not equal.
***************************************************************************/
bool  POL_F_Compare( POL_Y_Polynom *  Pol1P, POL_Y_Polynom *  Pol2P )
{
   if ((Pol1P == NULL) || (Pol2P == NULL))
   {
      return (false);
   }

   if ((Pol1P->PolOrder == Pol2P->PolOrder) && (Pol1P->PolData == Pol2P->PolData))
   {
      return (true);
   }
   return (false);

} /* end POL_F_Compare() */


/**************************************************************************
* Function Name:  POL_F_Add
*
* Description:    Performs Addition of two Polynoms.
*
* Input:          Pol1P:  A pointer to the first Polynom structure.
*                 Pol2P:  A pointer to the second Polynom structure.
*
* Output:         PolResP:  A pointer to the addition result Polynom.
*
* Return Value:   None.
***************************************************************************/
void  POL_F_Add( POL_Y_Polynom *  Pol1P, POL_Y_Polynom *  Pol2P,
                 POL_Y_Polynom *  PolResP )
{
   if (PolResP == NULL)
   {
      return;
   }
   POL_F_Clr(PolResP);

   if ((Pol1P == NULL) || (Pol2P == NULL))
   {
      return;
   }

   PolResP->PolData = Pol1P->PolData ^ Pol2P->PolData;
   PolResP->PolOrder = MAX(Pol1P->PolOrder, Pol2P->PolOrder);

   /*! ADJUST polynom's order in case it has leading zeros coefficient(s) */
   F_AdjOrderLong(PolResP);

} /* end POL_F_Add() */


/**************************************************************************
* Function Name:  POL_F_Sub
*
* Description:    Performs Subtraction of two Polynoms.
*                 NOTE: Subtraction here is the same as Addition !
*
* Input:          Pol1P:  A pointer to the first Polynom structure.
*                 Pol2P:  A pointer to the second Polynom structure.
*
* Output:         PolResP:  A pointer to the subtraction result Polynom.
*
* Return Value:   None.
***************************************************************************/
void  POL_F_Sub( POL_Y_Polynom *  Pol1P, POL_Y_Polynom *  Pol2P,
                 POL_Y_Polynom *  PolResP )
{
   POL_F_Add(Pol1P, Pol2P, PolResP);

} /* end POL_F_Sub() */


/**************************************************************************
* Function Name:  POL_F_Mult
*
* Description:    Performs Multiplication of two Polynoms.
*
*                 NOTE: The product Order may exceed [POL_C_MaxPolOrder / 2) - 1].
*                       (0 <= Product Polynom Order <= POL_C_MaxPolOrder - 2 = 30),
*                       thus the result may take up to 31 bits.
*
* Input:          Pol1P:  A pointer to the first Polynom structure.
*                 Pol2P:  A pointer to the second Polynom structure.
*
* Output:         PolResP:  A pointer to the product result Polynom.
*
* Return Value:   None.
***************************************************************************/
void  POL_F_Mult( POL_Y_Polynom *  Pol1P, POL_Y_Polynom *  Pol2P,
                  POL_Y_Polynom *  PolResP )
{
   __u8    i;
   __u8    j;
   __u32   Mij;
   __u32   i_mask;
   __u32   j_mask;
   __s8     InitProdOrder;


   if (PolResP == NULL)
   {
      return;
   }
   POL_F_Clr(PolResP);

   if ((Pol1P == NULL) || (Pol2P == NULL))
   {
      return;
   }

   if (( InitProdOrder = (Pol1P->PolOrder + Pol2P->PolOrder) ) >= POL_C_MaxPolOrder)
   {
      /*! RETURN badly - can't treat polynom of this Order */
      return;
   }

#if 0

   for (i = 0, i_mask = 0x00000001; i <= Pol1P->PolOrder; i++, i_mask <<= 1)
   {
      for (j = 0, j_mask = 0x00000001; j <= Pol2P->PolOrder; j++, j_mask <<= 1)
      {
         if ((Pol1P->PolData & i_mask) && (Pol2P->PolData & j_mask))
         {
            Mij = 0x00000001 << (i + j);
            PolResP->PolData ^= Mij;
         }
      } /* end_for i */
   } /* end_for i */

#else

   /* Faster Implementation ? */
   for (i = 0, i_mask = 0x00000001; i <= Pol1P->PolOrder; i++, i_mask <<= 1)
   {
      __u8  ShiftVal;
      for ( j = 0, j_mask = 0x00000001, ShiftVal = i;
            j <= Pol2P->PolOrder;
            j++, j_mask <<= 1, ShiftVal++)
      {
         if ((Pol1P->PolData & i_mask) && (Pol2P->PolData & j_mask))
         {
            Mij = 0x00000001 << ShiftVal;  // ShiftVal == (i+j)
            PolResP->PolData ^= Mij;
         }
      } /* end_for i */
   } /* end_for i */

#endif

   /*! ADUJT product polynom's Order */
   PolResP->PolOrder = InitProdOrder;
   F_AdjOrderLong(PolResP);

} /* end POL_F_Mult() */


/**************************************************************************
* Function Name:  POL_F_Shift
*
* Description:    'Shifts' a Polynom Left by ShiftBy (>= 0) places.
*                 This is equivalent ot multiplying the Polynom
*                 by the Polynon X^ShiftBy.
*
* Input:          PolP:     Pointer to the input Polynom, to shift-left.
*                 ShiftBy:  The number of places to shift.
*
* Output:         PolResP:  Pointer to the result Polynom.
*
* Return Value:   None.
***************************************************************************/
void  POL_F_Shift( POL_Y_Polynom *  PolP, __s8  ShiftBy,
                   POL_Y_Polynom *  PolResP )
{
   __s8  ShiftedPolOrder;

   if (PolResP == NULL)
   {
      return;
   }
   POL_F_Clr(PolResP);

   if ((PolP == NULL) || (ShiftBy < 0))
   {
      return;
   }

   if (( ShiftedPolOrder = (PolP->PolOrder + ShiftBy) ) >= POL_C_MaxPolOrder)
   {
      /*! RETURN badly - can't treat polynom of this Order */
      return;
   }

   PolResP->PolData = PolP->PolData << ShiftBy;
   PolResP->PolOrder = ShiftedPolOrder;

} /* end POL_F_Shift() */


/**************************************************************************
* Function Name:  POL_F_Div
*
* Description:    Performs Division of two Polynoms. The result is two
*                 Polynoms 'Main' and 'Residue':
*                 P1 / P2 = P_Main + P_Residue
*                (or: P1 = P_Main * P2 + P_Residue).
*
* Input:          Pol1P: Pointer to the Divided Polynom.
*                 Pol2P: Pointer to the Divider (divisor) Polynom.
*
* Output:         PolMainP:     Pointer to the Main of division Polynom.
*                 PolResdiueP:  Pointer to the Residue of division Polynom.
*
* Return Value:   None.
***************************************************************************/
void  POL_F_Div( POL_Y_Polynom *  Pol1P, POL_Y_Polynom *  Pol2P,
                 POL_Y_Polynom *  PolMainP, POL_Y_Polynom *  PolResdiueP )
{
   POL_Y_Polynom   tmpPol;
   POL_Y_Polynom   tmpRes;
   __u32          bitMask;
   __u32          i_mask;
   __s8            i;
   __u8           OrderDiff;


   /*! SUBTRACT Divider from Divided until Residue Order is less than the Divider Order */
   POL_F_Assign(Pol1P, PolResdiueP);  // Pol1 --> PolResdiue
   POL_F_Clr(PolMainP);               //    0 --> PolMain

   i = (__s8)( (__u8)Pol1P->PolOrder - (__u8)Pol2P->PolOrder );
   i_mask = 0x00000001 << i;
   bitMask = 0x00000001 << (i + Pol2P->PolOrder);
   while (i >= 0)
   {
      if (PolResdiueP->PolData & bitMask)
      {
         POL_F_Clr(&tmpPol);
         OrderDiff = (__u8)PolResdiueP->PolOrder - (__u8)Pol2P->PolOrder;
         POL_F_Shift(Pol2P, (__s8)OrderDiff, &tmpPol);
         PolMainP->PolData |= i_mask;
         POL_F_Sub(PolResdiueP, &tmpPol, &tmpRes);
         POL_F_Assign(&tmpRes, PolResdiueP);
      }
      bitMask >>= 1;
      i_mask >>= 1;
      i--;
   } /* end_while i */

   /*! ADUJT polynoms Order */
   PolMainP->PolOrder = Pol1P->PolOrder; // Main Order can't be higher than the Divided Order
   F_AdjOrderLong(PolMainP);

   PolResdiueP->PolOrder = Pol2P->PolOrder; // Residue Order can't be higher than the Divider Order
   F_AdjOrderLong(PolResdiueP);

} /* end POL_F_Div() */


/**************************************************************************
* Function Name:  POL_F_InverseWithMod
*
* Description:    Finds the Inverse Polynom, modulo another polynom.
*                 Given R(x) and M(x), find P(x) where:
*                 R(x) * P(x) = 1 mod M(x).
*
* Input:          RinP: Pointer to the Polynom to Inverse.
*                 MinP: Pointer to the Modulo Polynom.
*
* Output:         InvrsP: Pointer to the Inversed Polynom
*
* Return Value:   None.
***************************************************************************/
void  POL_F_InverseWithMod( POL_Y_Polynom *  RinP, POL_Y_Polynom *  MinP,
                            POL_Y_Polynom *  InvrsP )
{
   POL_Y_Polynom   R_min2; // R(-2)
   POL_Y_Polynom   R_min1; // R(-1)
   POL_Y_Polynom   P_min2; // P(-2)
   POL_Y_Polynom   P_min1; // P(-1)
   POL_Y_Polynom   R_;
   POL_Y_Polynom   A_;
   POL_Y_Polynom   tmpPol;
   POL_Y_Polynom   P_;
   __u8           k;


   /*! CHECK parameters */
   if (InvrsP == NULL)
   {
      return;
   }
   POL_F_Clr(InvrsP);

   if ((RinP == NULL) || (MinP == NULL))
   {
      return;
   }

   /*  Q(k) means Polynom Q at the k's iteration step,
   **  R(x) is the Polynom to Inverse (Rin).
   **  M(x) is the Creation Function Polynom (Min).
   **
   **  Initialization (step k = 0):
   **   R(-2) = M(x), R(-1) = R(x), P(-2) = 0, P(-1) = 1
   **
   **  On step k, k = 0,1,..., dividing R(k-2) by R(k-1) find A(k) and R(k)
   **     R(k-2) = A(k) * R(k-1) + R(k).
   **
   **     Using A(k) compute:
   **     P(k) = A(k) * P(k-1) + P(k-2)
   **
   **  Proceed till R(n) = 0. The solution is P(n-1).
   */

   /*! SET initial values: */
   POL_F_Assign(MinP, &R_min2);  // Min --> R_min2
   POL_F_Assign(RinP, &R_min1);  // Rin --> R_min1
   POL_F_Clr(&P_min2);           //   0 --> P_min2

   /* Options:                   //   1 -> P_min1
   **   POL_F_InitFromStr(&P_min1, "1");
   **   POL_F_InitFromVal(&P_min1, 0x00000001);
   ** Most Fast: */
   P_min1.PolOrder = 1;
   P_min1.PolData = 0x00000001;

   k = 0;
   do
   {
      /*! LOOP for iteration k */
      POL_F_Div( &R_min2, &R_min1, &A_, &R_ );  // R_min2 / R_min1 --> A_ + (R_)
      POL_F_Mult( &A_, &P_min1, &tmpPol );      // A_ * P_min1 --> tmpPol
      POL_F_Add( &tmpPol, &P_min2, &P_ );       // tmpPol + P_min2 --> P_

      /*! PREPARE for next iteration */
      POL_F_Assign( &P_min1, &P_min2 );         //  P_min1 --> P_min2
      POL_F_Assign( &P_, &P_min1 );             //  P_     --> P_min1
      POL_F_Assign( &R_min1, &R_min2 );         //  R_min1 --> R_min2
      POL_F_Assign( &R_, &R_min1 );             //  R_     --> R_min1
      k++;

   } while ( (POL_F_IsZero( &R_) == false) && (k < 32) );
   //printf("k = %i\n", k);

   /*! SET returned result */
   POL_F_Assign(&P_min2, InvrsP);               // P_min2  --> InvrsP

} /* end POL_F_InverseWithMod() */



/**************************************************************************
*              L O C A L      F U N C T I O N S
***************************************************************************/

/**************************************************************************
* Function Name:  F_AdjOrderShort
*
* Description:    Decreases Small polynom's (16bit) Order, if it has Leading-Zero
*                 coefficients.
*
* Input:          PolP:   A pointer to a Polynom structure.
*
* Output:         None.
*
* Return Value:   None.
***************************************************************************/
static void  F_AdjOrderShort( POL_Y_Polynom *  PolP )
{
   __u32  bitMask;
   __s8    i;


   /*! ADJUST polynom's order in case it has leading zeros coefficient(s) */
   if ((PolP->PolData & 0xFFFFFF00) == 0)
   {
      if ((PolP->PolData & 0xFFFFFFF0) == 0)
      {
         PolP->PolOrder = MIN(PolP->PolOrder, 3);  // bits 15 - 4 are '0'
      }
      else
      {
         PolP->PolOrder = MIN(PolP->PolOrder, 7);  // bits 15 - 8 are '0'
      }
   }
   else
   {
      if ((PolP->PolData & 0xFFFFF000) == 0)
      {
         PolP->PolOrder = MIN(PolP->PolOrder, 11); // bits 15 - 12 are '0'
      }
   }

   for ( i = PolP->PolOrder, bitMask = 0x0001 << i;
         (i > 0) && ((PolP->PolData & bitMask) == 0);
         i--, bitMask >>= 1 )
   {
      PolP->PolOrder--;
   }

} /* end F_AdjOrderShort() */


/**************************************************************************
* Function Name:  F_AdjOrderLong
*
* Description:    Decreases Large polynom's (32bit) Order, if it has
*                 Leading-Zero coefficients.
*
* Input:          PolP:   A pointer to a Polynom structure.
*
* Output:         None.
*
* Return Value:   None.
***************************************************************************/
static void  F_AdjOrderLong( POL_Y_Polynom *  PolP )
{
   __u32  bitMask;
   __s8    i;


   /*! ADJUST polynom's order in case it has leading zeros coefficient(s) */
   if ((PolP->PolData & 0xFFFF0000) == 0)
   {
      /*! ADJUST short polynom */
      F_AdjOrderShort(PolP);                       // bits 31 - 16 are '0'
      return;
   }

   // Some bits of bits 31 - 16 are Not '0'

   if ((PolP->PolData & 0xFF000000) == 0)
   {
      if ((PolP->PolData & 0xFFF00000) == 0)
      {
         PolP->PolOrder = MIN(PolP->PolOrder, 19); // bits 31 - 20 are '0'
      }
      else
      {
         PolP->PolOrder = MIN(PolP->PolOrder, 23); // bits 31 - 24 are '0'
      }
   }
   else
   {
      if ((PolP->PolOrder & 0xF0000000) == 0)
      {
         PolP->PolOrder = MIN(PolP->PolOrder, 27); // bits 31 - 28 are '0'
      }
   }

   for ( i = PolP->PolOrder, bitMask = 0x00000001 << i;
         (i > 0) && ((PolP->PolData & bitMask) == 0);
         i--, bitMask >>= 1 )
   {
      PolP->PolOrder--;
   }

} /* end F_AdjOrderLong() */


/**************************************************************************
* Function Name:
*
* Description:
*
* Input:          None.
* Output:         None.
* Return Value:   None.
***************************************************************************/


/**************************************************************************
*             D E B U G    S E C T I O N
**************************************************************************/

#if _POLYNOM_DEBUG
#include <stdio.h>

void  F_TstPol_DivResult( POL_Y_Polynom *  Pol1P, POL_Y_Polynom *  Pol2P,
                          POL_Y_Polynom *  PolMainP, POL_Y_Polynom *  PolResdiueP )
{
   POL_Y_Polynom   Pol1, Pol2, PolMain, PolResidue, PolTmp1, PolTmp2;
   bool            IsSameF;

   Pol1 = *Pol1P;
   Pol2 = *Pol2P;
   PolMain = *PolMainP;
   PolResidue = *PolResdiueP;

   POL_F_Mult( &PolMain, &Pol2, &PolTmp1 );
   POL_F_Add( &PolTmp1, &PolResidue, &PolTmp2 );

   IsSameF = POL_F_Compare( &Pol1, &PolTmp2 );
   if (IsSameF)
   {
      printf("Division OK\n");
   }
   else
   {
      printf("Division Failed!\n");
   }

} /* end F_TstPol_DivResult() */


#else  /* _POLYNOM_DEBUG */

void  F_TstPol_DivResult( POL_Y_Polynom *  Pol1P, POL_Y_Polynom *  Pol2P,
                          POL_Y_Polynom *  PolMainP, POL_Y_Polynom *  PolResdiueP )
{
}


#endif /* !_POLYNOM_DEBUG */

/*==========================   End of File   ============================*/

