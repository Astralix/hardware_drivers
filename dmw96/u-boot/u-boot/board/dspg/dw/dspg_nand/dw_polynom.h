/********************************************************************************
*              N A N D   F L A S H   C O N T R O L L E R
*********************************************************************************
*
*   Title:           Polynom math operations interface.
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
*  This package provides binary Polynom math operations.
*  A polynom is represented by a uint32 number.
*  Each bit n represents the coefficient C of the
*  C * 2^n expression, wher C is either '0' or '1'.
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
#ifndef _POLYNOM__H_
#define _POLYNOM__H_


/*========   Compilation Swithes  ========================================*/
#define _POLYNOM_DEBUG  0

/*========   Macro definitions   =========================================*/


/*========   Const, types and variables definitions    ===================*/

#define POL_C_MaxPolOrder  32

typedef struct
{
   __u8    PolOrder; // Actual order of the polynom
   __u32  PolData;  // Coefficients of the polynom
} POL_Y_Polynom;


/**************************************************************************
*             P U B L I C    F U N C T I O N S
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
extern void  POL_F_Clr( POL_Y_Polynom *  PolP );


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
extern void  POL_F_InitFromStr( POL_Y_Polynom *  PolP,  const char *  PolStr );


/**************************************************************************
* Function Name:  POL_F_InitFromVal
*
* Description:    Sets a Polynom from an input "binary" character string,
*                 holding up to 16 coefficients.
*
* Input:          PolP:   A pointer to a Polynom.
*                 PolVal: A uint16 number, representing the Polynom coefficients.
*
* Output:         None.
*
* Return Value:   None.
***************************************************************************/
extern void  POL_F_InitFromVal( POL_Y_Polynom *  PolP,  __u16  PolVal );


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
extern void  POL_F_Assign( POL_Y_Polynom *  PolInP, POL_Y_Polynom *  PolResP );


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
extern bool  POL_F_IsZero( POL_Y_Polynom *  PolP );


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
extern bool  POL_F_Compare( POL_Y_Polynom *  Pol1P, POL_Y_Polynom *  Pol2P );


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
extern void  POL_F_Add( POL_Y_Polynom *  Pol1P, POL_Y_Polynom *  Pol2P,
                        POL_Y_Polynom *  PolResP );


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
extern void  POL_F_Sub( POL_Y_Polynom *  Pol1P, POL_Y_Polynom *  Pol2P,
                        POL_Y_Polynom *  PolResP );


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
extern void  POL_F_Mult( POL_Y_Polynom *  Pol1P, POL_Y_Polynom *  Pol2P,
                         POL_Y_Polynom *  PolResP );


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
extern void  POL_F_Shift( POL_Y_Polynom *  PolP, __s8  ShiftBy,
                          POL_Y_Polynom *  PolResP );


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
extern void  POL_F_Div( POL_Y_Polynom *  Pol1P, POL_Y_Polynom *  Pol2P,
                        POL_Y_Polynom *  PolMainP, POL_Y_Polynom *  PolResdiueP );


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
extern void  POL_F_InverseWithMod( POL_Y_Polynom *  RinP, POL_Y_Polynom *  MinP,
                                   POL_Y_Polynom *  InvrsP );


/*=======   Macro definitions   ==========================================*/


/*=======   Exported variables ===========================================*/


/*=======   Local const, types and variables =============================*/




/**************************************************************************
*             D E B U G    S E C T I O N
**************************************************************************/

extern void  F_TstPol_DivResult( POL_Y_Polynom *  Pol1P, POL_Y_Polynom *  Pol2P,
                                 POL_Y_Polynom *  PolMainP, POL_Y_Polynom *  PolResdiueP );


#endif /* _POLYNOM__H_ */

/*============================   End of File   ==========================*/
