// BCH.cpp : Defines the entry point for the console application.
//

//#include "stdafx.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <time.h>

#define RANDOM() rand()
#define SEED(s) srand(s)

/*
* File:	bch3.c
* Title:	Encoder/decoder	for	binary BCH codes in	C (Version 3.1)
* Author:	Robert Morelos-Zaragoza
* Date:	August 1994
* Revised:	June 13, 1997
*
* ===============	Encoder/Decoder	for	binary BCH codes in	C =================
*
* Version 1:	Original program. The user provides	the	generator polynomial
*				of the code	(cumbersome!).
* Version 2:	Computes the generator polynomial of the code.
* Version 3:	No need	to input the coefficients of a primitive polynomial	of
*				degree m, used to construct	the	Galois Field GF(2**m). The
*				program	now	works for any binary BCH code of length	such that:
*				2**(m-1) - 1 < length <= 2**m -	1
*
* Note:		You	may	have to	change the size	of the arrays to make it work.
*
* The encoding	and	decoding methods used in this program are based	on the
* book	"Error Control Coding: Fundamentals	and	Applications", by Lin and
* Costello, Prentice Hall,	1983.
*
* Thanks to Patrick Boyle (pboyle@era.com)	for	his	observation	that 'bch2.c'
* did not work	for	lengths	other than 2**m-1 which	led	to this	new	version.
* Portions	of this	program	are	from 'rs.c', a Reed-Solomon	encoder/decoder
* in C, written by	Simon Rockliff (simon@augean.ua.oz.au) on 21/9/89. The
* previous	version	of the BCH encoder/decoder in C, 'bch2.c', was written by
* Robert Morelos-Zaragoza (robert@spectra.eng.hawaii.edu) on 5/19/92.
*
* NOTE:	
*			The	author is not responsible for any malfunctioning of
*			this program, nor for any damage caused	by it. Please include the
*			original program along with	these comments in any redistribution.
*
*	For	more information, suggestions, or other	ideas on implementing error
*	correcting codes, please contact me	at:
*
*							 Robert	Morelos-Zaragoza
*							 5120 Woodway, Suite 7036
*							 Houston, Texas	77056
*
*					  email: r.morelos-zaragoza@ieee.org
*
* COPYRIGHT NOTICE: This computer program is free for non-commercial purposes.
* You may implement this program for any non-commercial application. You may 
* also	implement this program for commercial purposes,	provided that you
* obtain my written permission. Any modification of this program is covered
* by this copyright.
*
* == Copyright	(c)	1994-7,	 Robert	Morelos-Zaragoza. All rights reserved.	==
*
* m = order of	the	Galois field GF(2**m) 
* n = 2**m	- 1	= size of the multiplicative group of GF(2**m)
* length =	length of the BCH code
* t = error correcting	capability (max. no. of	errors the code	corrects)
* Please note that the theoretical maximal number of correctable errors is t_max=floor((d-1)/2)
* d = 2*t + 1 = designed min. distance	= no. of consecutive roots of g(x) + 1
* k = n - deg(g(x)) = dimension (no. of information bits/codeword)	of the code
* p[] = coefficients of a primitive polynomial	used to	generate GF(2**m)
* g[] = coefficients of the generator polynomial, g(x)
* alpha_to	[] = log table of GF(2**m) 
* index_of[] =	antilog	table of GF(2**m)
* data[] =	information	bits = coefficients	of data	polynomial,	i(x)
* bb[]	= coefficients of redundancy polynomial	x^(length-k) i(x) modulo g(x)
* numerr =	number of errors 
* errpos[]	= error	positions 
* recd[] =	coefficients of	the	received polynomial	
* decerror	= number of	decoding errors	(in	_message_ positions) 
*
*/


// constants definition for compile-time
#define SYN_CALC 0
#define ERR_LOC_CALC 1
#define ENCODE_MODE 0
#define DECODE_MODE 1
// Space configuration for HW configuration
#define FIELD_SIZE 13
#define RAND_ERRORS_COUNT 8
#define SPARE_DATA 0
#define DATA_SIZE 4096 + SPARE_DATA
#define REDUNDANCY_SIZE RAND_ERRORS_COUNT*FIELD_SIZE
#define BLOCK_SIZE DATA_SIZE + REDUNDANCY_SIZE
#define HAMMING_REDUNDANCY_SIZE 14

int				m, n, length, k, t,	d;
int				p[21];
int				alpha_to[1048576], index_of[1048576], g[548576];
int				checksum[400]; // This array holds the checksum of the received data. On trsnsmit it is in bb[] array
int				recd[1048576], data[1048576], bb[548576];
int				n2_data[512];
int				seed;
int				numerr,	errpos[1024], decerror = 0;

int				use_random_data = 1; // Set this flag to generate the random data and check the result for a given configuration
int				use_determinitstic_seed = 0; // When this parameter is set to 1, the seed value is set from the command line
int             rand_t = RAND_ERRORS_COUNT; //24; //4; //8 ;//12;                   // The number of possible errors to correct in random runs
int             rand_block_size = BLOCK_SIZE; //4408; //4148; //4252;        // The actual block_size = Data_size + ECC_size
int             rand_m = FIELD_SIZE; // 13;                   // The block rank is set to 8192
int             print_data_in_random_run = 0;  // When 1: enables logging of data, when 0 data logging is disabled
int             random_iterations = 1000000; //400000; //100000;         // Number of random iterations
int				Pause_Between_Iterations = 0; // Set this variable to 1 if need to pause between iterations
int				limit_errors_count = 0; // Set this variable to 1 if the number if errors should be fixed
int             errors_count_value = 2; // Set this value to force the number of errors to this value (when limit_errors_count == 1)
int             debug_gal_mult = 0; // Set this variable to 1 if the galua multiplier should be fully verified
int             dsp_decode_mode = 1; // Set this variable to 1 if the decoding algorithem uses DSPG implementation
int             regular_BM_Enable = 0; // Set this variable to 1 if the BM algorithem should be used
int             dsp_SiBM_Enable = 0; // Set this variable to 1 if the error locating polynomial is created using SiBM algorithm
int             dsp_RiBM_Enable = 1; // Set this variable to 1 if the error locating polynomial is created using RiBM algorithm
int             dsp_DcRiBM_Enable = 0; // Set this variable to 1 if the error locating polynomial is created using DCRiBM algorithm
int				pause_on_Errors = 1; // Set this variable to 1 if need to pause on errors
int				log_for_debug = 0; // Set this variable to 1 if states variables log is needed
int             only_create_mult_equations = 0; // Set this variable to 1 if need to create logic equations for GF multiplier
int				calculate_encoding_polynomial = 0; // Set this variable to 1 if need to calculate the encoding polynomial equations
int				calculate_syndrome_polynomials = 0; // Set this variable to 1 if need to generate the syndrome calculation polynomials
int				emulate_HW_only = 1; // Set this variable to 1 if the code should only perform the HW processing
int				debug_syndrome_chien_search = 0; // Set this variable to 1 if the syndrome engine should be debugged for Chien search
int				create_alpha_file = 0; // Set this variable to 1 if need to create csv file for alpha powers
int				n1; // Number of extra data in the spare area
int				n1_mask; //number of bytes of n1 to be ignored (always treated as zero regardless the value
int				n2;  // Number of bytes of n2 (including the checksum bytes. n2>=ECC_CHECKSUM_SIZE
int				ecc_checksum_size; // Number of bytes used for the checksum section
int				param_err_count; // Command line parameter for the number of errors
int				param_ecc; // Command line parameter for the number of correctable_bits
int				generate_test_files = 0;
char			prefix_string[50]; // A temporal string used to process command line parameters
char			test_name[100]; // This string holds the common section of the test name
FILE			*CHECKSUM_FILE; // This is the file pointer used when we need to generate checksum calculation logfile for BCH
int				print_checksum_for_debug=0; // This parameter selects if we want to have checksum log file or not

char			input_data_file_name[255];
FILE			*input_file;

typedef struct _struct_syndromes{
	int S[13];
} struct_syndromes;

struct_syndromes Syndromes[50];

void read_p(void)
/*
*	Read m,	the	degree of a	primitive polynomial p(x) used to compute the
*	Galois field GF(2**m). Get precomputed coefficients	p[]	of p(x). Read
*	the	code length.
*/
{
	int			i, ninf;

	if (use_random_data == 0)
	{
		printf("bch3: An encoder/decoder for binary	BCH	codes\n");
		printf("Copyright (c) 1994-7. Robert Morelos-Zaragoza.\n");
		printf("This program is	free, please read first	the	copyright notice.\n");
		printf("\nFirst, enter a value of m	such that the code length is\n");
		printf("2**(m-1) - 1 < length <= 2**m -	1\n\n");
	}
	do {
		printf("The block size is 2^m\n");
		if (use_random_data == 0)
		{
			printf("Enter m (between	2 and 20): ");
		}
		else
		{
			printf("m = ");
		}
		if (use_random_data == 1)
		{
			m = rand_m;
			printf("%d\n",m);
		}
		else
		{
			scanf("%d", &m);
		}
	} while	( !(m>1) ||	!(m<21)	);
	for	(i=1; i<m; i++)
		p[i] = 0;
	p[0] = p[m]	= 1;
	if (m == 2)			p[1] = 1;
	else if	(m == 3)	p[1] = 1;
	else if	(m == 4)	p[1] = 1;
	else if	(m == 5)	p[2] = 1;
	else if	(m == 6)	p[1] = 1;
	else if	(m == 7)	p[1] = 1;
	else if	(m == 8)	p[4] = p[5]	= p[6] = 1;
	else if	(m == 9)	p[4] = 1;
	else if	(m == 10)	p[3] = 1;
	else if	(m == 11)	p[2] = 1;
	else if	(m == 12)	p[3] = p[4]	= p[7] = 1;
	else if	(m == 13)	p[1] = p[3]	= p[4] = 1;
	else if	(m == 14)	p[1] = p[11] = p[12] = 1;
	else if	(m == 15)	p[1] = 1;
	else if	(m == 16)	p[2] = p[3]	= p[5] = 1;
	else if	(m == 17)	p[3] = 1;
	else if	(m == 18)	p[7] = 1;
	else if	(m == 19)	p[1] = p[5]	= p[6] = 1;
	else if	(m == 20)	p[3] = 1;
	printf("p(x) = ");
	n =	1;
	for	(i = 0;	i <= m;	i++) {
		n *= 2;
		printf("%1d", p[i]);
	}
	printf("\n");
	n =	n /	2 -	1;
	ninf = (n +	1) / 2 - 1;
	do	{
		if (use_random_data == 1)
		{
			length = rand_block_size;
			printf("Data block length is %d\n",length);
		}
		else
		{
			printf("The code length sets the correctable errors to length/2 \n");
			printf("Enter code length (%d <	length <= %d): ", ninf,	n);
			scanf("%d",	&length);
		}
	} while	( !((length	<= n)&&(length>ninf)) );
}


void generate_gf(void)
/*
* Generate	field GF(2**m) from	the	irreducible	polynomial p(X)	with
* coefficients	in p[0]..p[m].
*
* Lookup tables:
*	 index->polynomial form: alpha_to[]	contains j=alpha^i;
*	 polynomial	form ->	index form:	index_of[j=alpha^i]	= i
*
* alpha=2 is the primitive	element	of GF(2**m)	
*/
{
	register int	i, mask;

	mask = 1;
	alpha_to[m]	= 0;
	for	(i = 0;	i <	m; i++)	{
		alpha_to[i]	= mask;
		index_of[alpha_to[i]] =	i;
		if (p[i] !=	0)
			alpha_to[m]	^= mask;
		mask <<= 1;
	}
	index_of[alpha_to[m]] =	m;
	mask >>= 1;
	for	(i = m + 1;	i <	n; i++)	{
		if (alpha_to[i - 1]	>= mask)
			alpha_to[i] =	alpha_to[m]	^ ((alpha_to[i - 1]	^ mask)	<< 1);
		else
			alpha_to[i] =	alpha_to[i - 1]	<< 1;
		index_of[alpha_to[i]] =	i;
	}
	index_of[0]	= -1;
}






/* This function is used to emulate GF(field_index) multiplier)
* It accepts two numbers and calculates the multiplication result of these fields *
* using the field creating polynomial and an LFSR implementation of the multiplier 
* Field index is used to create a mask for the relevant bits in the registers and the *
* field generating polynomial is used to create a mask of the multiplier data_a register */

int galua_mult(int a, int b)
{
	int i;
	int j;
	int data_a; 
	int tmp_data_a;
	int data_b; 
	int tmp_data_b;
	int mult = 0; 
	int tmp_mult;
	int field_mask = 0;
	int pol_mask = 0; // This field holds "1" in the p(x) polynomial powers
	int msb_field = 0;
	int current_bit_mask;
	// at first create a mask of exact number of bits that matches the field size
	for (i=0;i<m;i++)
	{
		field_mask |= (0x1 << i);
		if (p[i] == 1)
		{
			pol_mask |= (0x1 << i);
		}
	}
	msb_field = 0x1 << (m - 1); // This should create a patetrn with only the MSB set to "1"
	data_a = a;
	data_b = b;
	for (i=0;i<m;i++)
	{
		tmp_data_a = data_a;
		tmp_data_b = data_b;
		tmp_mult = mult;

		/* data_b is shifted left (to the MSB) one bit at a time, while the p(x) elements are xored with the current MSB */
		if ((tmp_data_b & msb_field) == msb_field)
		{
			current_bit_mask=msb_field >> 1;
			data_b = 0;
			for (j=m-1; j>0; j--)
			{
				if (p[j] == 1)
				{
					data_b |= (current_bit_mask ^ (current_bit_mask & tmp_data_b)) << 1;
				}
				else
				{
					data_b |= (current_bit_mask & tmp_data_b) << 1;
				}
				current_bit_mask = current_bit_mask >> 1;
			}
			data_b |= ((tmp_data_b & msb_field) >> (m-1)) & 0x1;
		}
		else
		{
			data_b = (tmp_data_b << 1) & field_mask ;
		}
		data_a = tmp_data_a >> 1;
		if ((tmp_data_a & 0x1) == 1)
		{
			mult = tmp_mult ^ tmp_data_b;
		}
		else
		{
			mult = tmp_mult;
		}
	}
	return(mult);
}

/* This function is used to emulate GF(field_index) multiplier) logic equations
* It assumes that the input values are given as a and b each having "m" bits
* The function calculates the multiplication logic by calculating the logic function value of every cell
*/

int galua_mult_calc()
{
	char string_a[30][600]; // Holds the strings of variable a logic
	char string_b[30][600]; // Holds the strings of variable b logic
	char string_c[30][600]; // holds the strings of variable c logic
	char tmp_string_a[30][600];
	char tmp_string_b[30][600];
	char tmp_string_c[30][600];
	int i;
	int j;
	//	int data_a; 
	//	int tmp_data_a;
	//	int data_b; 
	//	int tmp_data_b;
	int mult = 0; 
	//	int tmp_mult;
	int field_mask = 0;
	int pol_mask = 0; // This field holds "1" in the p(x) polynomial powers
	int msb_field = 0;
	int msb_index = 0;
	//	int current_bit_mask;
	// at first create a mask of exact number of bits that matches the field size
	for (i=0;i<m;i++)
	{
		field_mask |= (0x1 << i);
		if (p[i] == 1)
		{
			pol_mask |= (0x1 << i);
		}
	}
	msb_field = 0x1 << (m - 1); // This should create a patetrn with only the MSB set to "1"
	msb_index = m-1; // This is the index of the Most Significant Bit in the field

	// Initialize the arrays
	for (i=0;i<m;i++)
	{
		sprintf(string_a[i],"A%02d",i);
		sprintf(string_b[i],"B%02d",i);
		sprintf(string_c[i],"",i);
	}
	// Print the initialized values
	printf("A = ");
	for (i=0;i<m;i++)
	{
		printf("%s ",string_a[i]);
		strcpy(tmp_string_a[i], string_a[i]);
	}
	printf("\n");

	printf("B = ");
	for (i=0;i<m;i++)
	{
		printf("%s ",string_b[i]);
		strcpy(tmp_string_b[i], string_b[i]);
	}
	printf("\n");

	//data_a = a;
	//data_b = b;

	// Loop the correct number of iterations
	for (i=0;i<m;i++)
	{
		// Copy the current values into a temporary buffer to be used in the new calculation
		for (j=0;j<m;j++)
		{
			strcpy(tmp_string_a[j],string_a[j]);
			strcpy(tmp_string_b[j],string_b[j]);
			strcpy(tmp_string_c[j],string_c[j]);
		}


		/* data_b is shifted left (to the MSB) one bit at a time, while the p(x) elements are xored with the current MSB */
		// Unlike the regular multiplication, when calculating the logic, we ALWAYS perform the XOR operation if needed
		// The most significant bit is bit index msb_index

		//if ((tmp_data_b & msb_field) == msb_field)
		//{
		//	current_bit_mask=msb_field >> 1;
		//	data_b = 0;
		for (j=m-1; j>0; j--)
		{
			if (p[j] == 1) // If on this bit we need to XOR with the feedback then
			{
				//data_b |= (current_bit_mask ^ (current_bit_mask & tmp_data_b)) << 1;
				sprintf(string_b[j],"%s ^ %s",tmp_string_b[j-1], tmp_string_b[msb_index]);
			}
			else
			{
				// data_b |= (current_bit_mask & tmp_data_b) << 1;
				strcpy(string_b[j],tmp_string_b[j-1]);
			}
			//current_bit_mask = current_bit_mask >> 1;
		}
		strcpy(string_b[0],tmp_string_b[msb_index]);
		// data_b |= ((tmp_data_b & msb_field) >> (m-1)) & 0x1;
		//}
		//else
		//{
		//	data_b = (tmp_data_b << 1) & field_mask ;
		//}
		printf("B = ");
		for (j=0;j<m;j++)
		{
			printf("%s, ",string_b[j]);
		}
		printf("\n");

		printf("A = ");
		for (j=0;j<m;j++)
		{
			if(j<msb_index)
			{
				strcpy(string_a[j],tmp_string_a[j+1]);
			}
			else
			{
				strcpy(string_a[j],"");
			}
			printf("%s, ",string_a[j]);
			//data_a = tmp_data_a >> 1;
		}
		printf("\n");


		for(j=0;j<m;j++)
		{
			if (strcmp(string_c[j],"") == 0)
			{
				sprintf(string_c[j],"%s & %s",tmp_string_b[j],tmp_string_a[0]);
			}
			else
			{
				sprintf(string_c[j],"%s\n^((%s) & %s)",tmp_string_c[j],tmp_string_b[j],tmp_string_a[0]);
			}
		}

		for (j=0;j<m;j++)
		{
			printf("C");
			printf("[%2d] = %s\n ",j,string_c[j]);
		}
		printf("\n");

		/*	
		if ((tmp_data_a & 0x1) == 1)
		{
		mult = tmp_mult ^ tmp_data_b;
		}
		else
		{
		mult = tmp_mult;
		}
		*/
	}
	return(mult);
}


/* This function performs galua multiplication in GF13 in a single cycle
The equations were calculated by execution of the code with galua_mult_calc() activated
then loading the log file into this program */

int galua_mult_single_cycle(int a, int b)
{
	int i;
	//	int j;
	int data_a;
	int data_b;
	int data_c;
	int A[20];
	int B[20];
	int C[20];
	//	int mult;

	data_a = a;
	data_b = b;
	for (i=0;i<m;i++)
	{
		A[i]= data_a & 0x01;
		B[i]= data_b & 0x1;
		C[i]= 0;
		data_a = data_a >> 1;
		data_b = data_b >> 1;
	}

	C[ 0] = (B[ 0] & A[ 0])
		^((B[12]) & A[ 1])
		^((B[11]) & A[ 2])
		^((B[10]) & A[ 3])
		^((B[ 9]) & A[ 4])
		^((B[ 8]) & A[ 5])
		^((B[ 7]) & A[ 6])
		^((B[ 6]) & A[ 7])
		^((B[ 5]) & A[ 8])
		^((B[ 4]) & A[ 9])
		^((B[ 3] ^ B[12]) & A[10])
		^((B[ 2] ^ B[12] ^ B[11]) & A[11])
		^((B[ 1] ^ B[11] ^ B[10]) & A[12]);
	C[ 1] = (B[ 1] & A[ 0])
		^((B[ 0] ^ B[12]) & A[ 1])
		^((B[12] ^ B[11]) & A[ 2])
		^((B[11] ^ B[10]) & A[ 3])
		^((B[10] ^ B[ 9]) & A[ 4])
		^((B[ 9] ^ B[ 8]) & A[ 5])
		^((B[ 8] ^ B[ 7]) & A[ 6])
		^((B[ 7] ^ B[ 6]) & A[ 7])
		^((B[ 6] ^ B[ 5]) & A[ 8])
		^((B[ 5] ^ B[ 4]) & A[ 9])
		^((B[ 4] ^ B[ 3] ^ B[12]) & A[10])
		^((B[ 3] ^ B[12] ^ B[ 2] ^ B[12] ^ B[11]) & A[11])
		^((B[ 2] ^ B[12] ^ B[11] ^ B[ 1] ^ B[11] ^ B[10]) & A[12]);
	C[ 2] = (B[ 2] & A[ 0])
		^((B[ 1]) & A[ 1])
		^((B[ 0] ^ B[12]) & A[ 2])
		^((B[12] ^ B[11]) & A[ 3])
		^((B[11] ^ B[10]) & A[ 4])
		^((B[10] ^ B[ 9]) & A[ 5])
		^((B[ 9] ^ B[ 8]) & A[ 6])
		^((B[ 8] ^ B[ 7]) & A[ 7])
		^((B[ 7] ^ B[ 6]) & A[ 8])
		^((B[ 6] ^ B[ 5]) & A[ 9])
		^((B[ 5] ^ B[ 4]) & A[10])
		^((B[ 4] ^ B[ 3] ^ B[12]) & A[11])
		^((B[ 3] ^ B[12] ^ B[ 2] ^ B[12] ^ B[11]) & A[12]);
	C[ 3] = (B[ 3] & A[ 0])
		^((B[ 2] ^ B[12]) & A[ 1])
		^((B[ 1] ^ B[11]) & A[ 2])
		^((B[ 0] ^ B[12] ^ B[10]) & A[ 3])
		^((B[12] ^ B[11] ^ B[ 9]) & A[ 4])
		^((B[11] ^ B[10] ^ B[ 8]) & A[ 5])
		^((B[10] ^ B[ 9] ^ B[ 7]) & A[ 6])
		^((B[ 9] ^ B[ 8] ^ B[ 6]) & A[ 7])
		^((B[ 8] ^ B[ 7] ^ B[ 5]) & A[ 8])
		^((B[ 7] ^ B[ 6] ^ B[ 4]) & A[ 9])
		^((B[ 6] ^ B[ 5] ^ B[ 3] ^ B[12]) & A[10])
		^((B[ 5] ^ B[ 4] ^ B[ 2] ^ B[12] ^ B[11]) & A[11])
		^((B[ 4] ^ B[ 3] ^ B[12] ^ B[ 1] ^ B[11] ^ B[10]) & A[12]);
	C[ 4] = (B[ 4] & A[ 0])
		^((B[ 3] ^ B[12]) & A[ 1])
		^((B[ 2] ^ B[12] ^ B[11]) & A[ 2])
		^((B[ 1] ^ B[11] ^ B[10]) & A[ 3])
		^((B[ 0] ^ B[12] ^ B[10] ^ B[ 9]) & A[ 4])
		^((B[12] ^ B[11] ^ B[ 9] ^ B[ 8]) & A[ 5])
		^((B[11] ^ B[10] ^ B[ 8] ^ B[ 7]) & A[ 6])
		^((B[10] ^ B[ 9] ^ B[ 7] ^ B[ 6]) & A[ 7])
		^((B[ 9] ^ B[ 8] ^ B[ 6] ^ B[ 5]) & A[ 8])
		^((B[ 8] ^ B[ 7] ^ B[ 5] ^ B[ 4]) & A[ 9])
		^((B[ 7] ^ B[ 6] ^ B[ 4] ^ B[ 3] ^ B[12]) & A[10])
		^((B[ 6] ^ B[ 5] ^ B[ 3] ^ B[12] ^ B[ 2] ^ B[12] ^ B[11]) & A[11])
		^((B[ 5] ^ B[ 4] ^ B[ 2] ^ B[12] ^ B[11] ^ B[ 1] ^ B[11] ^ B[10]) & A[12]);
	C[ 5] = (B[ 5] & A[ 0])
		^((B[ 4]) & A[ 1])
		^((B[ 3] ^ B[12]) & A[ 2])
		^((B[ 2] ^ B[12] ^ B[11]) & A[ 3])
		^((B[ 1] ^ B[11] ^ B[10]) & A[ 4])
		^((B[ 0] ^ B[12] ^ B[10] ^ B[ 9]) & A[ 5])
		^((B[12] ^ B[11] ^ B[ 9] ^ B[ 8]) & A[ 6])
		^((B[11] ^ B[10] ^ B[ 8] ^ B[ 7]) & A[ 7])
		^((B[10] ^ B[ 9] ^ B[ 7] ^ B[ 6]) & A[ 8])
		^((B[ 9] ^ B[ 8] ^ B[ 6] ^ B[ 5]) & A[ 9])
		^((B[ 8] ^ B[ 7] ^ B[ 5] ^ B[ 4]) & A[10])
		^((B[ 7] ^ B[ 6] ^ B[ 4] ^ B[ 3] ^ B[12]) & A[11])
		^((B[ 6] ^ B[ 5] ^ B[ 3] ^ B[12] ^ B[ 2] ^ B[12] ^ B[11]) & A[12]);
	C[ 6] = (B[ 6] & A[ 0])
		^((B[ 5]) & A[ 1])
		^((B[ 4]) & A[ 2])
		^((B[ 3] ^ B[12]) & A[ 3])
		^((B[ 2] ^ B[12] ^ B[11]) & A[ 4])
		^((B[ 1] ^ B[11] ^ B[10]) & A[ 5])
		^((B[ 0] ^ B[12] ^ B[10] ^ B[ 9]) & A[ 6])
		^((B[12] ^ B[11] ^ B[ 9] ^ B[ 8]) & A[ 7])
		^((B[11] ^ B[10] ^ B[ 8] ^ B[ 7]) & A[ 8])
		^((B[10] ^ B[ 9] ^ B[ 7] ^ B[ 6]) & A[ 9])
		^((B[ 9] ^ B[ 8] ^ B[ 6] ^ B[ 5]) & A[10])
		^((B[ 8] ^ B[ 7] ^ B[ 5] ^ B[ 4]) & A[11])
		^((B[ 7] ^ B[ 6] ^ B[ 4] ^ B[ 3] ^ B[12]) & A[12]);
	C[ 7] = (B[ 7] & A[ 0])
		^((B[ 6]) & A[ 1])
		^((B[ 5]) & A[ 2])
		^((B[ 4]) & A[ 3])
		^((B[ 3] ^ B[12]) & A[ 4])
		^((B[ 2] ^ B[12] ^ B[11]) & A[ 5])
		^((B[ 1] ^ B[11] ^ B[10]) & A[ 6])
		^((B[ 0] ^ B[12] ^ B[10] ^ B[ 9]) & A[ 7])
		^((B[12] ^ B[11] ^ B[ 9] ^ B[ 8]) & A[ 8])
		^((B[11] ^ B[10] ^ B[ 8] ^ B[ 7]) & A[ 9])
		^((B[10] ^ B[ 9] ^ B[ 7] ^ B[ 6]) & A[10])
		^((B[ 9] ^ B[ 8] ^ B[ 6] ^ B[ 5]) & A[11])
		^((B[ 8] ^ B[ 7] ^ B[ 5] ^ B[ 4]) & A[12]);
	C[ 8] = (B[ 8] & A[ 0])
		^((B[ 7]) & A[ 1])
		^((B[ 6]) & A[ 2])
		^((B[ 5]) & A[ 3])
		^((B[ 4]) & A[ 4])
		^((B[ 3] ^ B[12]) & A[ 5])
		^((B[ 2] ^ B[12] ^ B[11]) & A[ 6])
		^((B[ 1] ^ B[11] ^ B[10]) & A[ 7])
		^((B[ 0] ^ B[12] ^ B[10] ^ B[ 9]) & A[ 8])
		^((B[12] ^ B[11] ^ B[ 9] ^ B[ 8]) & A[ 9])
		^((B[11] ^ B[10] ^ B[ 8] ^ B[ 7]) & A[10])
		^((B[10] ^ B[ 9] ^ B[ 7] ^ B[ 6]) & A[11])
		^((B[ 9] ^ B[ 8] ^ B[ 6] ^ B[ 5]) & A[12]);
	C[ 9] = (B[ 9] & A[ 0])
		^((B[ 8]) & A[ 1])
		^((B[ 7]) & A[ 2])
		^((B[ 6]) & A[ 3])
		^((B[ 5]) & A[ 4])
		^((B[ 4]) & A[ 5])
		^((B[ 3] ^ B[12]) & A[ 6])
		^((B[ 2] ^ B[12] ^ B[11]) & A[ 7])
		^((B[ 1] ^ B[11] ^ B[10]) & A[ 8])
		^((B[ 0] ^ B[12] ^ B[10] ^ B[ 9]) & A[ 9])
		^((B[12] ^ B[11] ^ B[ 9] ^ B[ 8]) & A[10])
		^((B[11] ^ B[10] ^ B[ 8] ^ B[ 7]) & A[11])
		^((B[10] ^ B[ 9] ^ B[ 7] ^ B[ 6]) & A[12]);
	C[10] = (B[10] & A[ 0])
		^((B[ 9]) & A[ 1])
		^((B[ 8]) & A[ 2])
		^((B[ 7]) & A[ 3])
		^((B[ 6]) & A[ 4])
		^((B[ 5]) & A[ 5])
		^((B[ 4]) & A[ 6])
		^((B[ 3] ^ B[12]) & A[ 7])
		^((B[ 2] ^ B[12] ^ B[11]) & A[ 8])
		^((B[ 1] ^ B[11] ^ B[10]) & A[ 9])
		^((B[ 0] ^ B[12] ^ B[10] ^ B[ 9]) & A[10])
		^((B[12] ^ B[11] ^ B[ 9] ^ B[ 8]) & A[11])
		^((B[11] ^ B[10] ^ B[ 8] ^ B[ 7]) & A[12]);
	C[11] = (B[11] & A[ 0])
		^((B[10]) & A[ 1])
		^((B[ 9]) & A[ 2])
		^((B[ 8]) & A[ 3])
		^((B[ 7]) & A[ 4])
		^((B[ 6]) & A[ 5])
		^((B[ 5]) & A[ 6])
		^((B[ 4]) & A[ 7])
		^((B[ 3] ^ B[12]) & A[ 8])
		^((B[ 2] ^ B[12] ^ B[11]) & A[ 9])
		^((B[ 1] ^ B[11] ^ B[10]) & A[10])
		^((B[ 0] ^ B[12] ^ B[10] ^ B[ 9]) & A[11])
		^((B[12] ^ B[11] ^ B[ 9] ^ B[ 8]) & A[12]);
	C[12] = (B[12] & A[ 0])
		^((B[11]) & A[ 1])
		^((B[10]) & A[ 2])
		^((B[ 9]) & A[ 3])
		^((B[ 8]) & A[ 4])
		^((B[ 7]) & A[ 5])
		^((B[ 6]) & A[ 6])
		^((B[ 5]) & A[ 7])
		^((B[ 4]) & A[ 8])
		^((B[ 3] ^ B[12]) & A[ 9])
		^((B[ 2] ^ B[12] ^ B[11]) & A[10])
		^((B[ 1] ^ B[11] ^ B[10]) & A[11])
		^((B[ 0] ^ B[12] ^ B[10] ^ B[ 9]) & A[12]);
	data_c = 0; // Initialize the output register value	
	for (i=m-1;i>=0;i--)
	{	
		data_c = data_c << 1;
		data_c += C[i];
	}
	return(data_c);
}


// this function calculates the checksum on the received data. The calculation is based on the calculated polynomial
// in a similar way that was used on the encoding phase.
void calculate_checksum(int D[8], int Mode)
{
	int bit_index;
	int Denc[8];
	int Ddec[8];
	int tmp_checksum[400];

	for(bit_index=0;bit_index<8;bit_index++)
	{
		if(Mode==ENCODE_MODE)
		{
			Denc[bit_index]=D[bit_index];
			Ddec[bit_index]=0;
		}
		else
		{
			Denc[bit_index]=0;
			Ddec[bit_index]=D[bit_index];
		}
	}
	switch (t)
	{
	case (4):
		{
			tmp_checksum[  0]=Ddec[7] ^ Denc[ 7] ^ Denc[ 5] ^ Denc[ 3] ^ checksum[ 48] ^ checksum[ 46] ^ checksum[ 44];
			tmp_checksum[  1]=Ddec[6] ^ Denc[ 7] ^ Denc[ 5] ^ Denc[ 3] ^ checksum[ 48] ^ checksum[ 46] ^ checksum[ 44] ^ Denc[ 6] ^ Denc[ 4] ^ Denc[ 2] ^ checksum[ 49] ^ checksum[ 47] ^ checksum[ 45];
			tmp_checksum[  2]=Ddec[5] ^ Denc[ 6] ^ Denc[ 4] ^ Denc[ 2] ^ checksum[ 49] ^ checksum[ 47] ^ checksum[ 45] ^ Denc[ 5] ^ Denc[ 3] ^ Denc[ 1] ^ checksum[ 50] ^ checksum[ 48] ^ checksum[ 46];
			tmp_checksum[  3]=Ddec[4] ^ Denc[ 7] ^ checksum[ 44] ^ Denc[ 1] ^ checksum[ 50] ^ Denc[ 4] ^ Denc[ 2] ^ Denc[ 0] ^ checksum[ 51] ^ checksum[ 49] ^ checksum[ 47];
			tmp_checksum[  4]=Ddec[3] ^ Denc[ 6] ^ checksum[ 45] ^ Denc[ 0] ^ checksum[ 51] ^ Denc[ 3] ^ Denc[ 1] ^ checksum[ 50] ^ checksum[ 48];
			tmp_checksum[  5]=Ddec[2] ^ Denc[ 7] ^ Denc[ 3] ^ checksum[ 48] ^ checksum[ 44] ^ Denc[ 2] ^ Denc[ 0] ^ checksum[ 51] ^ checksum[ 49];
			tmp_checksum[  6]=Ddec[1] ^ Denc[ 6] ^ Denc[ 2] ^ checksum[ 49] ^ checksum[ 45] ^ Denc[ 1] ^ checksum[ 50];
			tmp_checksum[  7]=Ddec[0] ^ Denc[ 7] ^ Denc[ 3] ^ checksum[ 48] ^ checksum[ 44] ^ Denc[ 1] ^ checksum[ 50] ^ Denc[ 0] ^ checksum[ 51];
			tmp_checksum[  8]=Denc[ 6] ^ Denc[ 2] ^ checksum[ 49] ^ checksum[ 45] ^ Denc[ 0] ^ checksum[ 51] ^ checksum[  0];
			tmp_checksum[  9]=Denc[ 7] ^ Denc[ 3] ^ checksum[ 48] ^ checksum[ 44] ^ Denc[ 1] ^ checksum[ 50] ^ checksum[  1];
			tmp_checksum[ 10]=Denc[ 7] ^ Denc[ 5] ^ Denc[ 3] ^ checksum[ 48] ^ checksum[ 46] ^ checksum[ 44] ^ Denc[ 6] ^ Denc[ 2] ^ checksum[ 49] ^ checksum[ 45] ^ Denc[ 0] ^ checksum[ 51] ^ checksum[  2];
			tmp_checksum[ 11]=Denc[ 6] ^ Denc[ 4] ^ Denc[ 2] ^ checksum[ 49] ^ checksum[ 47] ^ checksum[ 45] ^ Denc[ 5] ^ Denc[ 1] ^ checksum[ 50] ^ checksum[ 46] ^ checksum[  3];
			tmp_checksum[ 12]=Denc[ 5] ^ Denc[ 3] ^ Denc[ 1] ^ checksum[ 50] ^ checksum[ 48] ^ checksum[ 46] ^ Denc[ 4] ^ Denc[ 0] ^ checksum[ 51] ^ checksum[ 47] ^ checksum[  4];
			tmp_checksum[ 13]=Denc[ 4] ^ Denc[ 2] ^ Denc[ 0] ^ checksum[ 51] ^ checksum[ 49] ^ checksum[ 47] ^ Denc[ 3] ^ checksum[ 48] ^ checksum[  5];
			tmp_checksum[ 14]=Denc[ 3] ^ Denc[ 1] ^ checksum[ 50] ^ checksum[ 48] ^ Denc[ 2] ^ checksum[ 49] ^ checksum[  6];
			tmp_checksum[ 15]=Denc[ 7] ^ Denc[ 5] ^ Denc[ 3] ^ checksum[ 48] ^ checksum[ 46] ^ checksum[ 44] ^ Denc[ 2] ^ Denc[ 0] ^ checksum[ 51] ^ checksum[ 49] ^ Denc[ 1] ^ checksum[ 50] ^ checksum[  7];
			tmp_checksum[ 16]=Denc[ 7] ^ Denc[ 5] ^ Denc[ 3] ^ checksum[ 48] ^ checksum[ 46] ^ checksum[ 44] ^ Denc[ 6] ^ Denc[ 4] ^ Denc[ 2] ^ checksum[ 49] ^ checksum[ 47] ^ checksum[ 45] ^ Denc[ 1] ^ checksum[ 50] ^ Denc[ 0] ^ checksum[ 51] ^ checksum[  8];
			tmp_checksum[ 17]=Denc[ 7] ^ checksum[ 44] ^ Denc[ 6] ^ Denc[ 4] ^ Denc[ 2] ^ checksum[ 49] ^ checksum[ 47] ^ checksum[ 45] ^ Denc[ 1] ^ checksum[ 50] ^ Denc[ 0] ^ checksum[ 51] ^ checksum[  9];
			tmp_checksum[ 18]=Denc[ 6] ^ checksum[ 45] ^ Denc[ 5] ^ Denc[ 3] ^ Denc[ 1] ^ checksum[ 50] ^ checksum[ 48] ^ checksum[ 46] ^ Denc[ 0] ^ checksum[ 51] ^ checksum[ 10];
			tmp_checksum[ 19]=Denc[ 7] ^ Denc[ 3] ^ checksum[ 48] ^ checksum[ 44] ^ Denc[ 4] ^ Denc[ 2] ^ Denc[ 0] ^ checksum[ 51] ^ checksum[ 49] ^ checksum[ 47] ^ checksum[ 11];
			tmp_checksum[ 20]=Denc[ 6] ^ Denc[ 2] ^ checksum[ 49] ^ checksum[ 45] ^ Denc[ 3] ^ Denc[ 1] ^ checksum[ 50] ^ checksum[ 48] ^ checksum[ 12];
			tmp_checksum[ 21]=Denc[ 7] ^ Denc[ 3] ^ checksum[ 48] ^ checksum[ 44] ^ Denc[ 1] ^ checksum[ 50] ^ Denc[ 2] ^ Denc[ 0] ^ checksum[ 51] ^ checksum[ 49] ^ checksum[ 13];
			tmp_checksum[ 22]=Denc[ 6] ^ Denc[ 2] ^ checksum[ 49] ^ checksum[ 45] ^ Denc[ 0] ^ checksum[ 51] ^ Denc[ 1] ^ checksum[ 50] ^ checksum[ 14];
			tmp_checksum[ 23]=Denc[ 7] ^ Denc[ 3] ^ checksum[ 48] ^ checksum[ 44] ^ Denc[ 1] ^ checksum[ 50] ^ Denc[ 0] ^ checksum[ 51] ^ checksum[ 15];
			tmp_checksum[ 24]=Denc[ 7] ^ Denc[ 5] ^ Denc[ 3] ^ checksum[ 48] ^ checksum[ 46] ^ checksum[ 44] ^ Denc[ 6] ^ Denc[ 2] ^ checksum[ 49] ^ checksum[ 45] ^ Denc[ 0] ^ checksum[ 51] ^ checksum[ 16];
			tmp_checksum[ 25]=Denc[ 7] ^ Denc[ 3] ^ checksum[ 48] ^ checksum[ 44] ^ Denc[ 6] ^ Denc[ 4] ^ Denc[ 2] ^ checksum[ 49] ^ checksum[ 47] ^ checksum[ 45] ^ Denc[ 1] ^ checksum[ 50] ^ checksum[ 17];
			tmp_checksum[ 26]=Denc[ 6] ^ Denc[ 2] ^ checksum[ 49] ^ checksum[ 45] ^ Denc[ 5] ^ Denc[ 3] ^ Denc[ 1] ^ checksum[ 50] ^ checksum[ 48] ^ checksum[ 46] ^ Denc[ 0] ^ checksum[ 51] ^ checksum[ 18];
			tmp_checksum[ 27]=Denc[ 5] ^ Denc[ 1] ^ checksum[ 50] ^ checksum[ 46] ^ Denc[ 4] ^ Denc[ 2] ^ Denc[ 0] ^ checksum[ 51] ^ checksum[ 49] ^ checksum[ 47] ^ checksum[ 19];
			tmp_checksum[ 28]=Denc[ 4] ^ Denc[ 0] ^ checksum[ 51] ^ checksum[ 47] ^ Denc[ 3] ^ Denc[ 1] ^ checksum[ 50] ^ checksum[ 48] ^ checksum[ 20];
			tmp_checksum[ 29]=Denc[ 3] ^ checksum[ 48] ^ Denc[ 2] ^ Denc[ 0] ^ checksum[ 51] ^ checksum[ 49] ^ checksum[ 21];
			tmp_checksum[ 30]=Denc[ 7] ^ Denc[ 5] ^ Denc[ 3] ^ checksum[ 48] ^ checksum[ 46] ^ checksum[ 44] ^ Denc[ 2] ^ checksum[ 49] ^ Denc[ 1] ^ checksum[ 50] ^ checksum[ 22];
			tmp_checksum[ 31]=Denc[ 6] ^ Denc[ 4] ^ Denc[ 2] ^ checksum[ 49] ^ checksum[ 47] ^ checksum[ 45] ^ Denc[ 1] ^ checksum[ 50] ^ Denc[ 0] ^ checksum[ 51] ^ checksum[ 23];
			tmp_checksum[ 32]=Denc[ 5] ^ Denc[ 3] ^ Denc[ 1] ^ checksum[ 50] ^ checksum[ 48] ^ checksum[ 46] ^ Denc[ 0] ^ checksum[ 51] ^ checksum[ 24];
			tmp_checksum[ 33]=Denc[ 4] ^ Denc[ 2] ^ Denc[ 0] ^ checksum[ 51] ^ checksum[ 49] ^ checksum[ 47] ^ checksum[ 25];
			tmp_checksum[ 34]=Denc[ 3] ^ Denc[ 1] ^ checksum[ 50] ^ checksum[ 48] ^ checksum[ 26];
			tmp_checksum[ 35]=Denc[ 2] ^ Denc[ 0] ^ checksum[ 51] ^ checksum[ 49] ^ checksum[ 27];
			tmp_checksum[ 36]=Denc[ 7] ^ Denc[ 5] ^ Denc[ 3] ^ checksum[ 48] ^ checksum[ 46] ^ checksum[ 44] ^ Denc[ 1] ^ checksum[ 50] ^ checksum[ 28];
			tmp_checksum[ 37]=Denc[ 7] ^ Denc[ 5] ^ Denc[ 3] ^ checksum[ 48] ^ checksum[ 46] ^ checksum[ 44] ^ Denc[ 6] ^ Denc[ 4] ^ Denc[ 2] ^ checksum[ 49] ^ checksum[ 47] ^ checksum[ 45] ^ Denc[ 0] ^ checksum[ 51] ^ checksum[ 29];
			tmp_checksum[ 38]=Denc[ 6] ^ Denc[ 4] ^ Denc[ 2] ^ checksum[ 49] ^ checksum[ 47] ^ checksum[ 45] ^ Denc[ 5] ^ Denc[ 3] ^ Denc[ 1] ^ checksum[ 50] ^ checksum[ 48] ^ checksum[ 46] ^ checksum[ 30];
			tmp_checksum[ 39]=Denc[ 5] ^ Denc[ 3] ^ Denc[ 1] ^ checksum[ 50] ^ checksum[ 48] ^ checksum[ 46] ^ Denc[ 4] ^ Denc[ 2] ^ Denc[ 0] ^ checksum[ 51] ^ checksum[ 49] ^ checksum[ 47] ^ checksum[ 31];
			tmp_checksum[ 40]=Denc[ 4] ^ Denc[ 2] ^ Denc[ 0] ^ checksum[ 51] ^ checksum[ 49] ^ checksum[ 47] ^ Denc[ 3] ^ Denc[ 1] ^ checksum[ 50] ^ checksum[ 48] ^ checksum[ 32];
			tmp_checksum[ 41]=Denc[ 7] ^ Denc[ 5] ^ checksum[ 46] ^ checksum[ 44] ^ Denc[ 1] ^ checksum[ 50] ^ Denc[ 2] ^ Denc[ 0] ^ checksum[ 51] ^ checksum[ 49] ^ checksum[ 33];
			tmp_checksum[ 42]=Denc[ 6] ^ Denc[ 4] ^ checksum[ 47] ^ checksum[ 45] ^ Denc[ 0] ^ checksum[ 51] ^ Denc[ 1] ^ checksum[ 50] ^ checksum[ 34];
			tmp_checksum[ 43]=Denc[ 5] ^ Denc[ 3] ^ checksum[ 48] ^ checksum[ 46] ^ Denc[ 0] ^ checksum[ 51] ^ checksum[ 35];
			tmp_checksum[ 44]=Denc[ 7] ^ Denc[ 5] ^ Denc[ 3] ^ checksum[ 48] ^ checksum[ 46] ^ checksum[ 44] ^ Denc[ 4] ^ Denc[ 2] ^ checksum[ 49] ^ checksum[ 47] ^ checksum[ 36];
			tmp_checksum[ 45]=Denc[ 6] ^ Denc[ 4] ^ Denc[ 2] ^ checksum[ 49] ^ checksum[ 47] ^ checksum[ 45] ^ Denc[ 3] ^ Denc[ 1] ^ checksum[ 50] ^ checksum[ 48] ^ checksum[ 37];
			tmp_checksum[ 46]=Denc[ 7] ^ checksum[ 44] ^ Denc[ 1] ^ checksum[ 50] ^ Denc[ 2] ^ Denc[ 0] ^ checksum[ 51] ^ checksum[ 49] ^ checksum[ 38];
			tmp_checksum[ 47]=Denc[ 6] ^ checksum[ 45] ^ Denc[ 0] ^ checksum[ 51] ^ Denc[ 1] ^ checksum[ 50] ^ checksum[ 39];
			tmp_checksum[ 48]=Denc[ 5] ^ checksum[ 46] ^ Denc[ 0] ^ checksum[ 51] ^ checksum[ 40];
			tmp_checksum[ 49]=Denc[ 4] ^ checksum[ 47] ^ checksum[ 41];
			tmp_checksum[ 50]=Denc[ 7] ^ Denc[ 5] ^ checksum[ 46] ^ checksum[ 44] ^ checksum[ 42];
			tmp_checksum[ 51]=Denc[ 6] ^ Denc[ 4] ^ checksum[ 47] ^ checksum[ 45] ^ checksum[ 43];
			break;
		} // End case(4)
	case (8):
		{
			tmp_checksum[  0]=Ddec[7] ^ Denc[ 7] ^ checksum[ 96] ^ Denc[ 1] ^ checksum[102] ^ Denc[ 3] ^ checksum[100];
			tmp_checksum[  1]=Ddec[6] ^ Denc[ 6] ^ checksum[ 97] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 2] ^ checksum[101] ^ Denc[ 7] ^ checksum[ 96] ^ Denc[ 1] ^ checksum[102] ^ Denc[ 3] ^ checksum[100];
			tmp_checksum[  2]=Ddec[5] ^ Denc[ 5] ^ checksum[ 98] ^ Denc[ 1] ^ checksum[102] ^ Denc[ 6] ^ checksum[ 97] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 2] ^ checksum[101];
			tmp_checksum[  3]=Ddec[4] ^ Denc[ 4] ^ checksum[ 99] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 5] ^ checksum[ 98] ^ Denc[ 1] ^ checksum[102];
			tmp_checksum[  4]=Ddec[3] ^ Denc[ 3] ^ checksum[100] ^ Denc[ 4] ^ checksum[ 99] ^ Denc[ 0] ^ checksum[103];
			tmp_checksum[  5]=Ddec[2] ^ Denc[ 2] ^ checksum[101] ^ Denc[ 7] ^ checksum[ 96] ^ Denc[ 1] ^ checksum[102];
			tmp_checksum[  6]=Ddec[1] ^ Denc[ 1] ^ checksum[102] ^ Denc[ 6] ^ checksum[ 97] ^ Denc[ 0] ^ checksum[103];
			tmp_checksum[  7]=Ddec[0] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 5] ^ checksum[ 98];
			tmp_checksum[  8]=checksum[  0] ^ Denc[ 4] ^ checksum[ 99] ^ Denc[ 7] ^ checksum[ 96] ^ Denc[ 1] ^ checksum[102] ^ Denc[ 3] ^ checksum[100];
			tmp_checksum[  9]=checksum[  1] ^ Denc[ 6] ^ checksum[ 97] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 2] ^ checksum[101] ^ Denc[ 7] ^ checksum[ 96] ^ Denc[ 1] ^ checksum[102];
			tmp_checksum[ 10]=checksum[  2] ^ Denc[ 5] ^ checksum[ 98] ^ Denc[ 1] ^ checksum[102] ^ Denc[ 6] ^ checksum[ 97] ^ Denc[ 0] ^ checksum[103];
			tmp_checksum[ 11]=checksum[  3] ^ Denc[ 4] ^ checksum[ 99] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 5] ^ checksum[ 98] ^ Denc[ 7] ^ checksum[ 96] ^ Denc[ 1] ^ checksum[102] ^ Denc[ 3] ^ checksum[100];
			tmp_checksum[ 12]=checksum[  4] ^ Denc[ 4] ^ checksum[ 99] ^ Denc[ 6] ^ checksum[ 97] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 2] ^ checksum[101] ^ Denc[ 7] ^ checksum[ 96] ^ Denc[ 1] ^ checksum[102];
			tmp_checksum[ 13]=checksum[  5] ^ Denc[ 5] ^ checksum[ 98] ^ Denc[ 6] ^ checksum[ 97] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 7] ^ checksum[ 96];
			tmp_checksum[ 14]=checksum[  6] ^ Denc[ 4] ^ checksum[ 99] ^ Denc[ 5] ^ checksum[ 98] ^ Denc[ 6] ^ checksum[ 97] ^ Denc[ 7] ^ checksum[ 96] ^ Denc[ 1] ^ checksum[102] ^ Denc[ 3] ^ checksum[100];
			tmp_checksum[ 15]=checksum[  7] ^ Denc[ 4] ^ checksum[ 99] ^ Denc[ 5] ^ checksum[ 98] ^ Denc[ 6] ^ checksum[ 97] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 2] ^ checksum[101] ^ Denc[ 7] ^ checksum[ 96] ^ Denc[ 1] ^ checksum[102];
			tmp_checksum[ 16]=checksum[  8] ^ Denc[ 3] ^ checksum[100] ^ Denc[ 4] ^ checksum[ 99] ^ Denc[ 5] ^ checksum[ 98] ^ Denc[ 1] ^ checksum[102] ^ Denc[ 6] ^ checksum[ 97] ^ Denc[ 0] ^ checksum[103];
			tmp_checksum[ 17]=checksum[  9] ^ Denc[ 2] ^ checksum[101] ^ Denc[ 3] ^ checksum[100] ^ Denc[ 4] ^ checksum[ 99] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 5] ^ checksum[ 98];
			tmp_checksum[ 18]=checksum[ 10] ^ Denc[ 2] ^ checksum[101] ^ Denc[ 4] ^ checksum[ 99] ^ Denc[ 7] ^ checksum[ 96];
			tmp_checksum[ 19]=checksum[ 11] ^ Denc[ 1] ^ checksum[102] ^ Denc[ 3] ^ checksum[100] ^ Denc[ 6] ^ checksum[ 97];
			tmp_checksum[ 20]=checksum[ 12] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 2] ^ checksum[101] ^ Denc[ 5] ^ checksum[ 98];
			tmp_checksum[ 21]=checksum[ 13] ^ Denc[ 1] ^ checksum[102] ^ Denc[ 4] ^ checksum[ 99];
			tmp_checksum[ 22]=checksum[ 14] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 7] ^ checksum[ 96] ^ Denc[ 1] ^ checksum[102];
			tmp_checksum[ 23]=checksum[ 15] ^ Denc[ 6] ^ checksum[ 97] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 7] ^ checksum[ 96] ^ Denc[ 1] ^ checksum[102] ^ Denc[ 3] ^ checksum[100];
			tmp_checksum[ 24]=checksum[ 16] ^ Denc[ 5] ^ checksum[ 98] ^ Denc[ 6] ^ checksum[ 97] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 2] ^ checksum[101] ^ Denc[ 7] ^ checksum[ 96] ^ Denc[ 1] ^ checksum[102] ^ Denc[ 3] ^ checksum[100];
			tmp_checksum[ 25]=checksum[ 17] ^ Denc[ 4] ^ checksum[ 99] ^ Denc[ 5] ^ checksum[ 98] ^ Denc[ 1] ^ checksum[102] ^ Denc[ 6] ^ checksum[ 97] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 2] ^ checksum[101];
			tmp_checksum[ 26]=checksum[ 18] ^ Denc[ 4] ^ checksum[ 99] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 5] ^ checksum[ 98] ^ Denc[ 7] ^ checksum[ 96];
			tmp_checksum[ 27]=checksum[ 19] ^ Denc[ 3] ^ checksum[100] ^ Denc[ 4] ^ checksum[ 99] ^ Denc[ 6] ^ checksum[ 97];
			tmp_checksum[ 28]=checksum[ 20] ^ Denc[ 2] ^ checksum[101] ^ Denc[ 3] ^ checksum[100] ^ Denc[ 5] ^ checksum[ 98];
			tmp_checksum[ 29]=checksum[ 21] ^ Denc[ 1] ^ checksum[102] ^ Denc[ 2] ^ checksum[101] ^ Denc[ 4] ^ checksum[ 99];
			tmp_checksum[ 30]=checksum[ 22] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 7] ^ checksum[ 96];
			tmp_checksum[ 31]=checksum[ 23] ^ Denc[ 6] ^ checksum[ 97] ^ Denc[ 7] ^ checksum[ 96] ^ Denc[ 1] ^ checksum[102] ^ Denc[ 3] ^ checksum[100];
			tmp_checksum[ 32]=checksum[ 24] ^ Denc[ 5] ^ checksum[ 98] ^ Denc[ 6] ^ checksum[ 97] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 2] ^ checksum[101] ^ Denc[ 7] ^ checksum[ 96] ^ Denc[ 1] ^ checksum[102] ^ Denc[ 3] ^ checksum[100];
			tmp_checksum[ 33]=checksum[ 25] ^ Denc[ 4] ^ checksum[ 99] ^ Denc[ 5] ^ checksum[ 98] ^ Denc[ 1] ^ checksum[102] ^ Denc[ 6] ^ checksum[ 97] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 2] ^ checksum[101];
			tmp_checksum[ 34]=checksum[ 26] ^ Denc[ 3] ^ checksum[100] ^ Denc[ 4] ^ checksum[ 99] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 5] ^ checksum[ 98] ^ Denc[ 1] ^ checksum[102];
			tmp_checksum[ 35]=checksum[ 27] ^ Denc[ 2] ^ checksum[101] ^ Denc[ 3] ^ checksum[100] ^ Denc[ 4] ^ checksum[ 99] ^ Denc[ 0] ^ checksum[103];
			tmp_checksum[ 36]=checksum[ 28] ^ Denc[ 1] ^ checksum[102] ^ Denc[ 2] ^ checksum[101] ^ Denc[ 3] ^ checksum[100];
			tmp_checksum[ 37]=checksum[ 29] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 1] ^ checksum[102] ^ Denc[ 2] ^ checksum[101];
			tmp_checksum[ 38]=checksum[ 30] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 7] ^ checksum[ 96] ^ Denc[ 3] ^ checksum[100];
			tmp_checksum[ 39]=checksum[ 31] ^ Denc[ 6] ^ checksum[ 97] ^ Denc[ 2] ^ checksum[101];
			tmp_checksum[ 40]=checksum[ 32] ^ Denc[ 5] ^ checksum[ 98] ^ Denc[ 7] ^ checksum[ 96] ^ Denc[ 3] ^ checksum[100];
			tmp_checksum[ 41]=checksum[ 33] ^ Denc[ 4] ^ checksum[ 99] ^ Denc[ 6] ^ checksum[ 97] ^ Denc[ 2] ^ checksum[101] ^ Denc[ 7] ^ checksum[ 96] ^ Denc[ 1] ^ checksum[102] ^ Denc[ 3] ^ checksum[100];
			tmp_checksum[ 42]=checksum[ 34] ^ Denc[ 5] ^ checksum[ 98] ^ Denc[ 6] ^ checksum[ 97] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 2] ^ checksum[101] ^ Denc[ 7] ^ checksum[ 96];
			tmp_checksum[ 43]=checksum[ 35] ^ Denc[ 4] ^ checksum[ 99] ^ Denc[ 5] ^ checksum[ 98] ^ Denc[ 1] ^ checksum[102] ^ Denc[ 6] ^ checksum[ 97];
			tmp_checksum[ 44]=checksum[ 36] ^ Denc[ 3] ^ checksum[100] ^ Denc[ 4] ^ checksum[ 99] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 5] ^ checksum[ 98];
			tmp_checksum[ 45]=checksum[ 37] ^ Denc[ 2] ^ checksum[101] ^ Denc[ 3] ^ checksum[100] ^ Denc[ 4] ^ checksum[ 99];
			tmp_checksum[ 46]=checksum[ 38] ^ Denc[ 1] ^ checksum[102] ^ Denc[ 2] ^ checksum[101] ^ Denc[ 3] ^ checksum[100];
			tmp_checksum[ 47]=checksum[ 39] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 2] ^ checksum[101] ^ Denc[ 7] ^ checksum[ 96] ^ Denc[ 3] ^ checksum[100];
			tmp_checksum[ 48]=checksum[ 40] ^ Denc[ 6] ^ checksum[ 97] ^ Denc[ 2] ^ checksum[101] ^ Denc[ 7] ^ checksum[ 96] ^ Denc[ 3] ^ checksum[100];
			tmp_checksum[ 49]=checksum[ 41] ^ Denc[ 5] ^ checksum[ 98] ^ Denc[ 6] ^ checksum[ 97] ^ Denc[ 2] ^ checksum[101] ^ Denc[ 7] ^ checksum[ 96] ^ Denc[ 3] ^ checksum[100];
			tmp_checksum[ 50]=checksum[ 42] ^ Denc[ 4] ^ checksum[ 99] ^ Denc[ 5] ^ checksum[ 98] ^ Denc[ 1] ^ checksum[102] ^ Denc[ 6] ^ checksum[ 97] ^ Denc[ 2] ^ checksum[101];
			tmp_checksum[ 51]=checksum[ 43] ^ Denc[ 3] ^ checksum[100] ^ Denc[ 4] ^ checksum[ 99] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 5] ^ checksum[ 98] ^ Denc[ 1] ^ checksum[102];
			tmp_checksum[ 52]=checksum[ 44] ^ Denc[ 2] ^ checksum[101] ^ Denc[ 4] ^ checksum[ 99] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 7] ^ checksum[ 96] ^ Denc[ 1] ^ checksum[102];
			tmp_checksum[ 53]=checksum[ 45] ^ Denc[ 1] ^ checksum[102] ^ Denc[ 3] ^ checksum[100] ^ Denc[ 6] ^ checksum[ 97] ^ Denc[ 0] ^ checksum[103];
			tmp_checksum[ 54]=checksum[ 46] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 2] ^ checksum[101] ^ Denc[ 5] ^ checksum[ 98];
			tmp_checksum[ 55]=checksum[ 47] ^ Denc[ 1] ^ checksum[102] ^ Denc[ 4] ^ checksum[ 99];
			tmp_checksum[ 56]=checksum[ 48] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 3] ^ checksum[100];
			tmp_checksum[ 57]=checksum[ 49] ^ Denc[ 2] ^ checksum[101];
			tmp_checksum[ 58]=checksum[ 50] ^ Denc[ 7] ^ checksum[ 96] ^ Denc[ 3] ^ checksum[100];
			tmp_checksum[ 59]=checksum[ 51] ^ Denc[ 6] ^ checksum[ 97] ^ Denc[ 2] ^ checksum[101] ^ Denc[ 7] ^ checksum[ 96] ^ Denc[ 1] ^ checksum[102] ^ Denc[ 3] ^ checksum[100];
			tmp_checksum[ 60]=checksum[ 52] ^ Denc[ 5] ^ checksum[ 98] ^ Denc[ 1] ^ checksum[102] ^ Denc[ 6] ^ checksum[ 97] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 2] ^ checksum[101];
			tmp_checksum[ 61]=checksum[ 53] ^ Denc[ 4] ^ checksum[ 99] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 5] ^ checksum[ 98] ^ Denc[ 1] ^ checksum[102];
			tmp_checksum[ 62]=checksum[ 54] ^ Denc[ 3] ^ checksum[100] ^ Denc[ 4] ^ checksum[ 99] ^ Denc[ 0] ^ checksum[103];
			tmp_checksum[ 63]=checksum[ 55] ^ Denc[ 2] ^ checksum[101] ^ Denc[ 3] ^ checksum[100];
			tmp_checksum[ 64]=checksum[ 56] ^ Denc[ 2] ^ checksum[101] ^ Denc[ 7] ^ checksum[ 96] ^ Denc[ 3] ^ checksum[100];
			tmp_checksum[ 65]=checksum[ 57] ^ Denc[ 6] ^ checksum[ 97] ^ Denc[ 2] ^ checksum[101] ^ Denc[ 7] ^ checksum[ 96] ^ Denc[ 3] ^ checksum[100];
			tmp_checksum[ 66]=checksum[ 58] ^ Denc[ 5] ^ checksum[ 98] ^ Denc[ 1] ^ checksum[102] ^ Denc[ 6] ^ checksum[ 97] ^ Denc[ 2] ^ checksum[101];
			tmp_checksum[ 67]=checksum[ 59] ^ Denc[ 4] ^ checksum[ 99] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 5] ^ checksum[ 98] ^ Denc[ 7] ^ checksum[ 96] ^ Denc[ 3] ^ checksum[100];
			tmp_checksum[ 68]=checksum[ 60] ^ Denc[ 4] ^ checksum[ 99] ^ Denc[ 6] ^ checksum[ 97] ^ Denc[ 2] ^ checksum[101] ^ Denc[ 7] ^ checksum[ 96] ^ Denc[ 1] ^ checksum[102];
			tmp_checksum[ 69]=checksum[ 61] ^ Denc[ 5] ^ checksum[ 98] ^ Denc[ 6] ^ checksum[ 97] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 7] ^ checksum[ 96];
			tmp_checksum[ 70]=checksum[ 62] ^ Denc[ 4] ^ checksum[ 99] ^ Denc[ 5] ^ checksum[ 98] ^ Denc[ 6] ^ checksum[ 97] ^ Denc[ 7] ^ checksum[ 96] ^ Denc[ 1] ^ checksum[102] ^ Denc[ 3] ^ checksum[100];
			tmp_checksum[ 71]=checksum[ 63] ^ Denc[ 3] ^ checksum[100] ^ Denc[ 4] ^ checksum[ 99] ^ Denc[ 5] ^ checksum[ 98] ^ Denc[ 6] ^ checksum[ 97] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 2] ^ checksum[101];
			tmp_checksum[ 72]=checksum[ 64] ^ Denc[ 2] ^ checksum[101] ^ Denc[ 3] ^ checksum[100] ^ Denc[ 4] ^ checksum[ 99] ^ Denc[ 5] ^ checksum[ 98] ^ Denc[ 1] ^ checksum[102];
			tmp_checksum[ 73]=checksum[ 65] ^ Denc[ 1] ^ checksum[102] ^ Denc[ 2] ^ checksum[101] ^ Denc[ 3] ^ checksum[100] ^ Denc[ 4] ^ checksum[ 99] ^ Denc[ 0] ^ checksum[103];
			tmp_checksum[ 74]=checksum[ 66] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 1] ^ checksum[102] ^ Denc[ 2] ^ checksum[101] ^ Denc[ 3] ^ checksum[100];
			tmp_checksum[ 75]=checksum[ 67] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 1] ^ checksum[102] ^ Denc[ 2] ^ checksum[101];
			tmp_checksum[ 76]=checksum[ 68] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 1] ^ checksum[102];
			tmp_checksum[ 77]=checksum[ 69] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 7] ^ checksum[ 96] ^ Denc[ 1] ^ checksum[102] ^ Denc[ 3] ^ checksum[100];
			tmp_checksum[ 78]=checksum[ 70] ^ Denc[ 6] ^ checksum[ 97] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 2] ^ checksum[101] ^ Denc[ 7] ^ checksum[ 96] ^ Denc[ 1] ^ checksum[102] ^ Denc[ 3] ^ checksum[100];
			tmp_checksum[ 79]=checksum[ 71] ^ Denc[ 5] ^ checksum[ 98] ^ Denc[ 6] ^ checksum[ 97] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 2] ^ checksum[101] ^ Denc[ 7] ^ checksum[ 96] ^ Denc[ 3] ^ checksum[100];
			tmp_checksum[ 80]=checksum[ 72] ^ Denc[ 4] ^ checksum[ 99] ^ Denc[ 5] ^ checksum[ 98] ^ Denc[ 1] ^ checksum[102] ^ Denc[ 6] ^ checksum[ 97] ^ Denc[ 2] ^ checksum[101];
			tmp_checksum[ 81]=checksum[ 73] ^ Denc[ 3] ^ checksum[100] ^ Denc[ 4] ^ checksum[ 99] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 5] ^ checksum[ 98] ^ Denc[ 1] ^ checksum[102];
			tmp_checksum[ 82]=checksum[ 74] ^ Denc[ 2] ^ checksum[101] ^ Denc[ 4] ^ checksum[ 99] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 7] ^ checksum[ 96] ^ Denc[ 1] ^ checksum[102];
			tmp_checksum[ 83]=checksum[ 75] ^ Denc[ 1] ^ checksum[102] ^ Denc[ 3] ^ checksum[100] ^ Denc[ 6] ^ checksum[ 97] ^ Denc[ 0] ^ checksum[103];
			tmp_checksum[ 84]=checksum[ 76] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 2] ^ checksum[101] ^ Denc[ 5] ^ checksum[ 98] ^ Denc[ 7] ^ checksum[ 96] ^ Denc[ 1] ^ checksum[102] ^ Denc[ 3] ^ checksum[100];
			tmp_checksum[ 85]=checksum[ 77] ^ Denc[ 1] ^ checksum[102] ^ Denc[ 4] ^ checksum[ 99] ^ Denc[ 6] ^ checksum[ 97] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 2] ^ checksum[101];
			tmp_checksum[ 86]=checksum[ 78] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 3] ^ checksum[100] ^ Denc[ 5] ^ checksum[ 98] ^ Denc[ 1] ^ checksum[102];
			tmp_checksum[ 87]=checksum[ 79] ^ Denc[ 2] ^ checksum[101] ^ Denc[ 4] ^ checksum[ 99] ^ Denc[ 0] ^ checksum[103];
			tmp_checksum[ 88]=checksum[ 80] ^ Denc[ 7] ^ checksum[ 96];
			tmp_checksum[ 89]=checksum[ 81] ^ Denc[ 6] ^ checksum[ 97];
			tmp_checksum[ 90]=checksum[ 82] ^ Denc[ 5] ^ checksum[ 98];
			tmp_checksum[ 91]=checksum[ 83] ^ Denc[ 4] ^ checksum[ 99] ^ Denc[ 7] ^ checksum[ 96] ^ Denc[ 1] ^ checksum[102] ^ Denc[ 3] ^ checksum[100];
			tmp_checksum[ 92]=checksum[ 84] ^ Denc[ 6] ^ checksum[ 97] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 2] ^ checksum[101] ^ Denc[ 7] ^ checksum[ 96] ^ Denc[ 1] ^ checksum[102];
			tmp_checksum[ 93]=checksum[ 85] ^ Denc[ 5] ^ checksum[ 98] ^ Denc[ 6] ^ checksum[ 97] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 7] ^ checksum[ 96] ^ Denc[ 3] ^ checksum[100];
			tmp_checksum[ 94]=checksum[ 86] ^ Denc[ 4] ^ checksum[ 99] ^ Denc[ 5] ^ checksum[ 98] ^ Denc[ 6] ^ checksum[ 97] ^ Denc[ 2] ^ checksum[101] ^ Denc[ 7] ^ checksum[ 96] ^ Denc[ 1] ^ checksum[102] ^ Denc[ 3] ^ checksum[100];
			tmp_checksum[ 95]=checksum[ 87] ^ Denc[ 4] ^ checksum[ 99] ^ Denc[ 5] ^ checksum[ 98] ^ Denc[ 6] ^ checksum[ 97] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 2] ^ checksum[101] ^ Denc[ 7] ^ checksum[ 96];
			tmp_checksum[ 96]=checksum[ 88] ^ Denc[ 4] ^ checksum[ 99] ^ Denc[ 5] ^ checksum[ 98] ^ Denc[ 6] ^ checksum[ 97] ^ Denc[ 7] ^ checksum[ 96];
			tmp_checksum[ 97]=checksum[ 89] ^ Denc[ 3] ^ checksum[100] ^ Denc[ 4] ^ checksum[ 99] ^ Denc[ 5] ^ checksum[ 98] ^ Denc[ 6] ^ checksum[ 97];
			tmp_checksum[ 98]=checksum[ 90] ^ Denc[ 2] ^ checksum[101] ^ Denc[ 4] ^ checksum[ 99] ^ Denc[ 5] ^ checksum[ 98] ^ Denc[ 7] ^ checksum[ 96] ^ Denc[ 1] ^ checksum[102];
			tmp_checksum[ 99]=checksum[ 91] ^ Denc[ 1] ^ checksum[102] ^ Denc[ 3] ^ checksum[100] ^ Denc[ 4] ^ checksum[ 99] ^ Denc[ 6] ^ checksum[ 97] ^ Denc[ 0] ^ checksum[103];
			tmp_checksum[100]=checksum[ 92] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 2] ^ checksum[101] ^ Denc[ 5] ^ checksum[ 98] ^ Denc[ 7] ^ checksum[ 96] ^ Denc[ 1] ^ checksum[102];
			tmp_checksum[101]=checksum[ 93] ^ Denc[ 1] ^ checksum[102] ^ Denc[ 4] ^ checksum[ 99] ^ Denc[ 6] ^ checksum[ 97] ^ Denc[ 0] ^ checksum[103];
			tmp_checksum[102]=checksum[ 94] ^ Denc[ 0] ^ checksum[103] ^ Denc[ 3] ^ checksum[100] ^ Denc[ 5] ^ checksum[ 98];
			tmp_checksum[103]=checksum[ 95] ^ Denc[ 2] ^ checksum[101] ^ Denc[ 4] ^ checksum[ 99];
			break;
		} // End case(8)
	case (12):
		{
			tmp_checksum[  0]=Ddec[7] ^ Denc[ 7] ^ Denc[ 6] ^ Denc[ 1] ^ checksum[154] ^ Denc[ 2] ^ checksum[153] ^ checksum[149] ^ Denc[ 3] ^ checksum[152] ^ checksum[148];
			tmp_checksum[  1]=Ddec[6] ^ Denc[ 6] ^ Denc[ 5] ^ Denc[ 0] ^ checksum[155] ^ Denc[ 1] ^ checksum[154] ^ checksum[150] ^ Denc[ 2] ^ checksum[153] ^ checksum[149];
			tmp_checksum[  2]=Ddec[5] ^ Denc[ 5] ^ Denc[ 4] ^ Denc[ 0] ^ checksum[155] ^ checksum[151] ^ Denc[ 1] ^ checksum[154] ^ checksum[150];
			tmp_checksum[  3]=Ddec[4] ^ Denc[ 4] ^ Denc[ 3] ^ checksum[152] ^ Denc[ 0] ^ checksum[155] ^ checksum[151];
			tmp_checksum[  4]=Ddec[3] ^ Denc[ 7] ^ Denc[ 6] ^ Denc[ 1] ^ checksum[154] ^ checksum[149] ^ checksum[152] ^ checksum[148] ^ checksum[152];
			tmp_checksum[  5]=Ddec[2] ^ Denc[ 6] ^ Denc[ 5] ^ Denc[ 0] ^ checksum[155] ^ checksum[150] ^ checksum[153] ^ checksum[149] ^ checksum[153];
			tmp_checksum[  6]=Ddec[1] ^ Denc[ 7] ^ Denc[ 6] ^ Denc[ 1] ^ Denc[ 2] ^ checksum[153] ^ checksum[149] ^ Denc[ 3] ^ checksum[152] ^ checksum[148] ^ Denc[ 5] ^ Denc[ 4] ^ checksum[151] ^ checksum[150] ^ checksum[154];
			tmp_checksum[  7]=Ddec[0] ^ Denc[ 6] ^ Denc[ 5] ^ Denc[ 0] ^ Denc[ 1] ^ checksum[154] ^ checksum[150] ^ Denc[ 2] ^ checksum[153] ^ checksum[149] ^ Denc[ 4] ^ Denc[ 3] ^ checksum[152] ^ checksum[151] ^ checksum[155];
			tmp_checksum[  8]=Denc[ 5] ^ Denc[ 4] ^ Denc[ 0] ^ checksum[155] ^ checksum[151] ^ Denc[ 1] ^ checksum[154] ^ checksum[150] ^ Denc[ 3] ^ Denc[ 2] ^ checksum[153] ^ checksum[152] ^ checksum[  0];
			tmp_checksum[  9]=Denc[ 4] ^ Denc[ 3] ^ checksum[152] ^ Denc[ 0] ^ checksum[155] ^ checksum[151] ^ Denc[ 2] ^ Denc[ 1] ^ checksum[154] ^ checksum[153] ^ checksum[  1];
			tmp_checksum[ 10]=Denc[ 3] ^ Denc[ 2] ^ checksum[153] ^ checksum[152] ^ Denc[ 1] ^ Denc[ 0] ^ checksum[155] ^ checksum[154] ^ checksum[  2];
			tmp_checksum[ 11]=Denc[ 2] ^ Denc[ 1] ^ checksum[154] ^ checksum[153] ^ Denc[ 0] ^ checksum[155] ^ checksum[  3];
			tmp_checksum[ 12]=Denc[ 1] ^ Denc[ 0] ^ checksum[155] ^ checksum[154] ^ checksum[  4];
			tmp_checksum[ 13]=Denc[ 7] ^ Denc[ 6] ^ Denc[ 1] ^ checksum[154] ^ Denc[ 2] ^ checksum[153] ^ checksum[149] ^ Denc[ 3] ^ checksum[152] ^ checksum[148] ^ Denc[ 0] ^ checksum[155] ^ checksum[  5];
			tmp_checksum[ 14]=Denc[ 7] ^ Denc[ 3] ^ checksum[152] ^ checksum[148] ^ Denc[ 5] ^ Denc[ 0] ^ checksum[155] ^ checksum[150] ^ checksum[  6];
			tmp_checksum[ 15]=Denc[ 7] ^ Denc[ 1] ^ checksum[154] ^ Denc[ 3] ^ checksum[152] ^ checksum[148] ^ Denc[ 4] ^ checksum[151] ^ checksum[  7];
			tmp_checksum[ 16]=Denc[ 7] ^ Denc[ 1] ^ checksum[154] ^ checksum[148] ^ Denc[ 0] ^ checksum[155] ^ checksum[  8];
			tmp_checksum[ 17]=Denc[ 6] ^ Denc[ 0] ^ checksum[155] ^ checksum[149] ^ checksum[  9];
			tmp_checksum[ 18]=Denc[ 7] ^ Denc[ 6] ^ Denc[ 1] ^ checksum[154] ^ Denc[ 2] ^ checksum[153] ^ checksum[149] ^ Denc[ 3] ^ checksum[152] ^ checksum[148] ^ Denc[ 5] ^ checksum[150] ^ checksum[ 10];
			tmp_checksum[ 19]=Denc[ 6] ^ Denc[ 5] ^ Denc[ 0] ^ checksum[155] ^ Denc[ 1] ^ checksum[154] ^ checksum[150] ^ Denc[ 2] ^ checksum[153] ^ checksum[149] ^ Denc[ 4] ^ checksum[151] ^ checksum[ 11];
			tmp_checksum[ 20]=Denc[ 5] ^ Denc[ 4] ^ Denc[ 0] ^ checksum[155] ^ checksum[151] ^ Denc[ 1] ^ checksum[154] ^ checksum[150] ^ Denc[ 3] ^ checksum[152] ^ checksum[ 12];
			tmp_checksum[ 21]=Denc[ 4] ^ Denc[ 3] ^ checksum[152] ^ Denc[ 0] ^ checksum[155] ^ checksum[151] ^ Denc[ 2] ^ checksum[153] ^ checksum[ 13];
			tmp_checksum[ 22]=Denc[ 3] ^ Denc[ 2] ^ checksum[153] ^ checksum[152] ^ Denc[ 1] ^ checksum[154] ^ checksum[ 14];
			tmp_checksum[ 23]=Denc[ 2] ^ Denc[ 1] ^ checksum[154] ^ checksum[153] ^ Denc[ 0] ^ checksum[155] ^ checksum[ 15];
			tmp_checksum[ 24]=Denc[ 1] ^ Denc[ 0] ^ checksum[155] ^ checksum[154] ^ checksum[ 16];
			tmp_checksum[ 25]=Denc[ 7] ^ Denc[ 6] ^ Denc[ 1] ^ checksum[154] ^ Denc[ 2] ^ checksum[153] ^ checksum[149] ^ Denc[ 3] ^ checksum[152] ^ checksum[148] ^ Denc[ 0] ^ checksum[155] ^ checksum[ 17];
			tmp_checksum[ 26]=Denc[ 6] ^ Denc[ 5] ^ Denc[ 0] ^ checksum[155] ^ Denc[ 1] ^ checksum[154] ^ checksum[150] ^ Denc[ 2] ^ checksum[153] ^ checksum[149] ^ checksum[ 18];
			tmp_checksum[ 27]=Denc[ 5] ^ Denc[ 4] ^ Denc[ 0] ^ checksum[155] ^ checksum[151] ^ Denc[ 1] ^ checksum[154] ^ checksum[150] ^ checksum[ 19];
			tmp_checksum[ 28]=Denc[ 7] ^ Denc[ 6] ^ Denc[ 1] ^ checksum[154] ^ Denc[ 2] ^ checksum[153] ^ checksum[149] ^ checksum[148] ^ Denc[ 4] ^ Denc[ 0] ^ checksum[155] ^ checksum[151] ^ checksum[ 20];
			tmp_checksum[ 29]=Denc[ 6] ^ Denc[ 5] ^ Denc[ 0] ^ checksum[155] ^ Denc[ 1] ^ checksum[154] ^ checksum[150] ^ checksum[149] ^ Denc[ 3] ^ checksum[152] ^ checksum[ 21];
			tmp_checksum[ 30]=Denc[ 5] ^ Denc[ 4] ^ Denc[ 0] ^ checksum[155] ^ checksum[151] ^ checksum[150] ^ Denc[ 2] ^ checksum[153] ^ checksum[ 22];
			tmp_checksum[ 31]=Denc[ 4] ^ Denc[ 3] ^ checksum[152] ^ checksum[151] ^ Denc[ 1] ^ checksum[154] ^ checksum[ 23];
			tmp_checksum[ 32]=Denc[ 3] ^ Denc[ 2] ^ checksum[153] ^ checksum[152] ^ Denc[ 0] ^ checksum[155] ^ checksum[ 24];
			tmp_checksum[ 33]=Denc[ 7] ^ Denc[ 6] ^ checksum[149] ^ Denc[ 3] ^ checksum[152] ^ checksum[148] ^ checksum[ 25];
			tmp_checksum[ 34]=Denc[ 7] ^ Denc[ 1] ^ checksum[154] ^ Denc[ 3] ^ checksum[152] ^ checksum[148] ^ Denc[ 5] ^ checksum[150] ^ checksum[ 26];
			tmp_checksum[ 35]=Denc[ 7] ^ Denc[ 1] ^ checksum[154] ^ Denc[ 3] ^ checksum[152] ^ checksum[148] ^ Denc[ 0] ^ checksum[155] ^ Denc[ 4] ^ checksum[151] ^ checksum[ 27];
			tmp_checksum[ 36]=Denc[ 7] ^ Denc[ 1] ^ checksum[154] ^ checksum[148] ^ Denc[ 0] ^ checksum[155] ^ checksum[ 28];
			tmp_checksum[ 37]=Denc[ 7] ^ Denc[ 1] ^ checksum[154] ^ Denc[ 2] ^ checksum[153] ^ Denc[ 3] ^ checksum[152] ^ checksum[148] ^ Denc[ 0] ^ checksum[155] ^ checksum[ 29];
			tmp_checksum[ 38]=Denc[ 7] ^ Denc[ 3] ^ checksum[152] ^ checksum[148] ^ Denc[ 0] ^ checksum[155] ^ checksum[ 30];
			tmp_checksum[ 39]=Denc[ 6] ^ Denc[ 2] ^ checksum[153] ^ checksum[149] ^ checksum[ 31];
			tmp_checksum[ 40]=Denc[ 7] ^ Denc[ 6] ^ Denc[ 2] ^ checksum[153] ^ checksum[149] ^ Denc[ 3] ^ checksum[152] ^ checksum[148] ^ Denc[ 5] ^ checksum[150] ^ checksum[ 32];
			tmp_checksum[ 41]=Denc[ 6] ^ Denc[ 5] ^ Denc[ 1] ^ checksum[154] ^ checksum[150] ^ Denc[ 2] ^ checksum[153] ^ checksum[149] ^ Denc[ 4] ^ checksum[151] ^ checksum[ 33];
			tmp_checksum[ 42]=Denc[ 7] ^ Denc[ 6] ^ Denc[ 2] ^ checksum[153] ^ checksum[149] ^ checksum[148] ^ Denc[ 5] ^ Denc[ 4] ^ Denc[ 0] ^ checksum[155] ^ checksum[151] ^ checksum[150] ^ checksum[ 34];
			tmp_checksum[ 43]=Denc[ 7] ^ Denc[ 2] ^ checksum[153] ^ checksum[148] ^ Denc[ 5] ^ checksum[150] ^ Denc[ 4] ^ checksum[151] ^ checksum[ 35];
			tmp_checksum[ 44]=Denc[ 6] ^ Denc[ 1] ^ checksum[154] ^ checksum[149] ^ Denc[ 4] ^ checksum[151] ^ Denc[ 3] ^ checksum[152] ^ checksum[ 36];
			tmp_checksum[ 45]=Denc[ 7] ^ Denc[ 6] ^ Denc[ 1] ^ checksum[154] ^ checksum[149] ^ checksum[148] ^ Denc[ 5] ^ Denc[ 0] ^ checksum[155] ^ checksum[150] ^ checksum[ 37];
			tmp_checksum[ 46]=Denc[ 7] ^ Denc[ 1] ^ checksum[154] ^ Denc[ 2] ^ checksum[153] ^ Denc[ 3] ^ checksum[152] ^ checksum[148] ^ Denc[ 5] ^ Denc[ 0] ^ checksum[155] ^ checksum[150] ^ Denc[ 4] ^ checksum[151] ^ checksum[ 38];
			tmp_checksum[ 47]=Denc[ 6] ^ Denc[ 0] ^ checksum[155] ^ Denc[ 1] ^ checksum[154] ^ Denc[ 2] ^ checksum[153] ^ checksum[149] ^ Denc[ 4] ^ checksum[151] ^ Denc[ 3] ^ checksum[152] ^ checksum[ 39];
			tmp_checksum[ 48]=Denc[ 5] ^ Denc[ 0] ^ checksum[155] ^ Denc[ 1] ^ checksum[154] ^ checksum[150] ^ Denc[ 3] ^ checksum[152] ^ Denc[ 2] ^ checksum[153] ^ checksum[ 40];
			tmp_checksum[ 49]=Denc[ 7] ^ Denc[ 6] ^ checksum[149] ^ Denc[ 3] ^ checksum[152] ^ checksum[148] ^ Denc[ 4] ^ Denc[ 0] ^ checksum[155] ^ checksum[151] ^ checksum[ 41];
			tmp_checksum[ 50]=Denc[ 7] ^ Denc[ 1] ^ checksum[154] ^ checksum[148] ^ Denc[ 5] ^ checksum[150] ^ checksum[ 42];
			tmp_checksum[ 51]=Denc[ 7] ^ Denc[ 1] ^ checksum[154] ^ Denc[ 2] ^ checksum[153] ^ Denc[ 3] ^ checksum[152] ^ checksum[148] ^ Denc[ 0] ^ checksum[155] ^ Denc[ 4] ^ checksum[151] ^ checksum[ 43];
			tmp_checksum[ 52]=Denc[ 6] ^ Denc[ 0] ^ checksum[155] ^ Denc[ 1] ^ checksum[154] ^ Denc[ 2] ^ checksum[153] ^ checksum[149] ^ Denc[ 3] ^ checksum[152] ^ checksum[ 44];
			tmp_checksum[ 53]=Denc[ 7] ^ Denc[ 6] ^ checksum[149] ^ Denc[ 3] ^ checksum[152] ^ checksum[148] ^ Denc[ 5] ^ Denc[ 0] ^ checksum[155] ^ checksum[150] ^ checksum[ 45];
			tmp_checksum[ 54]=Denc[ 7] ^ Denc[ 1] ^ checksum[154] ^ Denc[ 3] ^ checksum[152] ^ checksum[148] ^ Denc[ 5] ^ checksum[150] ^ Denc[ 4] ^ checksum[151] ^ checksum[ 46];
			tmp_checksum[ 55]=Denc[ 6] ^ Denc[ 0] ^ checksum[155] ^ Denc[ 2] ^ checksum[153] ^ checksum[149] ^ Denc[ 4] ^ checksum[151] ^ Denc[ 3] ^ checksum[152] ^ checksum[ 47];
			tmp_checksum[ 56]=Denc[ 5] ^ Denc[ 1] ^ checksum[154] ^ checksum[150] ^ Denc[ 3] ^ checksum[152] ^ Denc[ 2] ^ checksum[153] ^ checksum[ 48];
			tmp_checksum[ 57]=Denc[ 4] ^ Denc[ 0] ^ checksum[155] ^ checksum[151] ^ Denc[ 2] ^ checksum[153] ^ Denc[ 1] ^ checksum[154] ^ checksum[ 49];
			tmp_checksum[ 58]=Denc[ 7] ^ Denc[ 6] ^ Denc[ 2] ^ checksum[153] ^ checksum[149] ^ checksum[148] ^ Denc[ 0] ^ checksum[155] ^ checksum[ 50];
			tmp_checksum[ 59]=Denc[ 7] ^ Denc[ 2] ^ checksum[153] ^ Denc[ 3] ^ checksum[152] ^ checksum[148] ^ Denc[ 5] ^ checksum[150] ^ checksum[ 51];
			tmp_checksum[ 60]=Denc[ 6] ^ Denc[ 1] ^ checksum[154] ^ Denc[ 2] ^ checksum[153] ^ checksum[149] ^ Denc[ 4] ^ checksum[151] ^ checksum[ 52];
			tmp_checksum[ 61]=Denc[ 5] ^ Denc[ 0] ^ checksum[155] ^ Denc[ 1] ^ checksum[154] ^ checksum[150] ^ Denc[ 3] ^ checksum[152] ^ checksum[ 53];
			tmp_checksum[ 62]=Denc[ 7] ^ Denc[ 6] ^ Denc[ 1] ^ checksum[154] ^ checksum[149] ^ Denc[ 3] ^ checksum[152] ^ checksum[148] ^ Denc[ 4] ^ Denc[ 0] ^ checksum[155] ^ checksum[151] ^ checksum[ 54];
			tmp_checksum[ 63]=Denc[ 6] ^ Denc[ 5] ^ Denc[ 0] ^ checksum[155] ^ checksum[150] ^ Denc[ 2] ^ checksum[153] ^ checksum[149] ^ Denc[ 3] ^ checksum[152] ^ checksum[ 55];
			tmp_checksum[ 64]=Denc[ 5] ^ Denc[ 4] ^ checksum[151] ^ Denc[ 1] ^ checksum[154] ^ checksum[150] ^ Denc[ 2] ^ checksum[153] ^ checksum[ 56];
			tmp_checksum[ 65]=Denc[ 7] ^ Denc[ 6] ^ Denc[ 2] ^ checksum[153] ^ checksum[149] ^ checksum[148] ^ Denc[ 4] ^ Denc[ 0] ^ checksum[155] ^ checksum[151] ^ checksum[ 57];
			tmp_checksum[ 66]=Denc[ 6] ^ Denc[ 5] ^ Denc[ 1] ^ checksum[154] ^ checksum[150] ^ checksum[149] ^ Denc[ 3] ^ checksum[152] ^ checksum[ 58];
			tmp_checksum[ 67]=Denc[ 7] ^ Denc[ 6] ^ Denc[ 1] ^ checksum[154] ^ checksum[149] ^ Denc[ 3] ^ checksum[152] ^ checksum[148] ^ Denc[ 5] ^ Denc[ 4] ^ Denc[ 0] ^ checksum[155] ^ checksum[151] ^ checksum[150] ^ checksum[ 59];
			tmp_checksum[ 68]=Denc[ 6] ^ Denc[ 5] ^ Denc[ 0] ^ checksum[155] ^ checksum[150] ^ Denc[ 2] ^ checksum[153] ^ checksum[149] ^ Denc[ 4] ^ Denc[ 3] ^ checksum[152] ^ checksum[151] ^ checksum[ 60];
			tmp_checksum[ 69]=Denc[ 5] ^ Denc[ 4] ^ checksum[151] ^ Denc[ 1] ^ checksum[154] ^ checksum[150] ^ Denc[ 3] ^ Denc[ 2] ^ checksum[153] ^ checksum[152] ^ checksum[ 61];
			tmp_checksum[ 70]=Denc[ 4] ^ Denc[ 3] ^ checksum[152] ^ Denc[ 0] ^ checksum[155] ^ checksum[151] ^ Denc[ 2] ^ Denc[ 1] ^ checksum[154] ^ checksum[153] ^ checksum[ 62];
			tmp_checksum[ 71]=Denc[ 3] ^ Denc[ 2] ^ checksum[153] ^ checksum[152] ^ Denc[ 1] ^ Denc[ 0] ^ checksum[155] ^ checksum[154] ^ checksum[ 63];
			tmp_checksum[ 72]=Denc[ 2] ^ Denc[ 1] ^ checksum[154] ^ checksum[153] ^ Denc[ 0] ^ checksum[155] ^ checksum[ 64];
			tmp_checksum[ 73]=Denc[ 1] ^ Denc[ 0] ^ checksum[155] ^ checksum[154] ^ checksum[ 65];
			tmp_checksum[ 74]=Denc[ 7] ^ Denc[ 6] ^ Denc[ 1] ^ checksum[154] ^ Denc[ 2] ^ checksum[153] ^ checksum[149] ^ Denc[ 3] ^ checksum[152] ^ checksum[148] ^ Denc[ 0] ^ checksum[155] ^ checksum[ 66];
			tmp_checksum[ 75]=Denc[ 6] ^ Denc[ 5] ^ Denc[ 0] ^ checksum[155] ^ Denc[ 1] ^ checksum[154] ^ checksum[150] ^ Denc[ 2] ^ checksum[153] ^ checksum[149] ^ checksum[ 67];
			tmp_checksum[ 76]=Denc[ 7] ^ Denc[ 6] ^ Denc[ 2] ^ checksum[153] ^ checksum[149] ^ Denc[ 3] ^ checksum[152] ^ checksum[148] ^ Denc[ 5] ^ Denc[ 4] ^ Denc[ 0] ^ checksum[155] ^ checksum[151] ^ checksum[150] ^ checksum[ 68];
			tmp_checksum[ 77]=Denc[ 6] ^ Denc[ 5] ^ Denc[ 1] ^ checksum[154] ^ checksum[150] ^ Denc[ 2] ^ checksum[153] ^ checksum[149] ^ Denc[ 4] ^ Denc[ 3] ^ checksum[152] ^ checksum[151] ^ checksum[ 69];
			tmp_checksum[ 78]=Denc[ 5] ^ Denc[ 4] ^ Denc[ 0] ^ checksum[155] ^ checksum[151] ^ Denc[ 1] ^ checksum[154] ^ checksum[150] ^ Denc[ 3] ^ Denc[ 2] ^ checksum[153] ^ checksum[152] ^ checksum[ 70];
			tmp_checksum[ 79]=Denc[ 7] ^ Denc[ 6] ^ checksum[149] ^ checksum[148] ^ Denc[ 4] ^ Denc[ 0] ^ checksum[155] ^ checksum[151] ^ checksum[ 71];
			tmp_checksum[ 80]=Denc[ 6] ^ Denc[ 5] ^ checksum[150] ^ checksum[149] ^ Denc[ 3] ^ checksum[152] ^ checksum[ 72];
			tmp_checksum[ 81]=Denc[ 7] ^ Denc[ 6] ^ Denc[ 1] ^ checksum[154] ^ checksum[149] ^ Denc[ 3] ^ checksum[152] ^ checksum[148] ^ Denc[ 5] ^ Denc[ 4] ^ checksum[151] ^ checksum[150] ^ checksum[ 73];
			tmp_checksum[ 82]=Denc[ 7] ^ Denc[ 1] ^ checksum[154] ^ checksum[148] ^ Denc[ 5] ^ Denc[ 0] ^ checksum[155] ^ checksum[150] ^ Denc[ 4] ^ checksum[151] ^ checksum[ 74];
			tmp_checksum[ 83]=Denc[ 6] ^ Denc[ 0] ^ checksum[155] ^ checksum[149] ^ Denc[ 4] ^ checksum[151] ^ Denc[ 3] ^ checksum[152] ^ checksum[ 75];
			tmp_checksum[ 84]=Denc[ 5] ^ checksum[150] ^ Denc[ 3] ^ checksum[152] ^ Denc[ 2] ^ checksum[153] ^ checksum[ 76];
			tmp_checksum[ 85]=Denc[ 7] ^ Denc[ 6] ^ checksum[149] ^ Denc[ 3] ^ checksum[152] ^ checksum[148] ^ Denc[ 4] ^ checksum[151] ^ checksum[ 77];
			tmp_checksum[ 86]=Denc[ 6] ^ Denc[ 5] ^ checksum[150] ^ Denc[ 2] ^ checksum[153] ^ checksum[149] ^ Denc[ 3] ^ checksum[152] ^ checksum[ 78];
			tmp_checksum[ 87]=Denc[ 7] ^ Denc[ 6] ^ checksum[149] ^ Denc[ 3] ^ checksum[152] ^ checksum[148] ^ Denc[ 5] ^ Denc[ 4] ^ checksum[151] ^ checksum[150] ^ checksum[ 79];
			tmp_checksum[ 88]=Denc[ 6] ^ Denc[ 5] ^ checksum[150] ^ Denc[ 2] ^ checksum[153] ^ checksum[149] ^ Denc[ 4] ^ Denc[ 3] ^ checksum[152] ^ checksum[151] ^ checksum[ 80];
			tmp_checksum[ 89]=Denc[ 5] ^ Denc[ 4] ^ checksum[151] ^ Denc[ 1] ^ checksum[154] ^ checksum[150] ^ Denc[ 3] ^ Denc[ 2] ^ checksum[153] ^ checksum[152] ^ checksum[ 81];
			tmp_checksum[ 90]=Denc[ 7] ^ Denc[ 6] ^ checksum[149] ^ checksum[148] ^ Denc[ 4] ^ Denc[ 0] ^ checksum[155] ^ checksum[151] ^ checksum[ 82];
			tmp_checksum[ 91]=Denc[ 6] ^ Denc[ 5] ^ checksum[150] ^ checksum[149] ^ Denc[ 3] ^ checksum[152] ^ checksum[ 83];
			tmp_checksum[ 92]=Denc[ 5] ^ Denc[ 4] ^ checksum[151] ^ checksum[150] ^ Denc[ 2] ^ checksum[153] ^ checksum[ 84];
			tmp_checksum[ 93]=Denc[ 4] ^ Denc[ 3] ^ checksum[152] ^ checksum[151] ^ Denc[ 1] ^ checksum[154] ^ checksum[ 85];
			tmp_checksum[ 94]=Denc[ 3] ^ Denc[ 2] ^ checksum[153] ^ checksum[152] ^ Denc[ 0] ^ checksum[155] ^ checksum[ 86];
			tmp_checksum[ 95]=Denc[ 7] ^ Denc[ 6] ^ checksum[149] ^ Denc[ 3] ^ checksum[152] ^ checksum[148] ^ checksum[ 87];
			tmp_checksum[ 96]=Denc[ 7] ^ Denc[ 1] ^ checksum[154] ^ Denc[ 3] ^ checksum[152] ^ checksum[148] ^ Denc[ 5] ^ checksum[150] ^ checksum[ 88];
			tmp_checksum[ 97]=Denc[ 7] ^ Denc[ 1] ^ checksum[154] ^ Denc[ 3] ^ checksum[152] ^ checksum[148] ^ Denc[ 0] ^ checksum[155] ^ Denc[ 4] ^ checksum[151] ^ checksum[ 89];
			tmp_checksum[ 98]=Denc[ 7] ^ Denc[ 1] ^ checksum[154] ^ checksum[148] ^ Denc[ 0] ^ checksum[155] ^ checksum[ 90];
			tmp_checksum[ 99]=Denc[ 6] ^ Denc[ 0] ^ checksum[155] ^ checksum[149] ^ checksum[ 91];
			tmp_checksum[100]=Denc[ 5] ^ checksum[150] ^ checksum[ 92];
			tmp_checksum[101]=Denc[ 7] ^ Denc[ 6] ^ Denc[ 1] ^ checksum[154] ^ Denc[ 2] ^ checksum[153] ^ checksum[149] ^ Denc[ 3] ^ checksum[152] ^ checksum[148] ^ Denc[ 4] ^ checksum[151] ^ checksum[ 93];
			tmp_checksum[102]=Denc[ 7] ^ checksum[148] ^ Denc[ 5] ^ Denc[ 0] ^ checksum[155] ^ checksum[150] ^ checksum[ 94];
			tmp_checksum[103]=Denc[ 6] ^ checksum[149] ^ Denc[ 4] ^ checksum[151] ^ checksum[ 95];
			tmp_checksum[104]=Denc[ 7] ^ Denc[ 6] ^ Denc[ 1] ^ checksum[154] ^ Denc[ 2] ^ checksum[153] ^ checksum[149] ^ checksum[148] ^ Denc[ 5] ^ checksum[150] ^ checksum[ 96];
			tmp_checksum[105]=Denc[ 6] ^ Denc[ 5] ^ Denc[ 0] ^ checksum[155] ^ Denc[ 1] ^ checksum[154] ^ checksum[150] ^ checksum[149] ^ Denc[ 4] ^ checksum[151] ^ checksum[ 97];
			tmp_checksum[106]=Denc[ 7] ^ Denc[ 6] ^ Denc[ 1] ^ checksum[154] ^ Denc[ 2] ^ checksum[153] ^ checksum[149] ^ checksum[148] ^ Denc[ 5] ^ Denc[ 4] ^ Denc[ 0] ^ checksum[155] ^ checksum[151] ^ checksum[150] ^ checksum[ 98];
			tmp_checksum[107]=Denc[ 6] ^ Denc[ 5] ^ Denc[ 0] ^ checksum[155] ^ Denc[ 1] ^ checksum[154] ^ checksum[150] ^ checksum[149] ^ Denc[ 4] ^ Denc[ 3] ^ checksum[152] ^ checksum[151] ^ checksum[ 99];
			tmp_checksum[108]=Denc[ 5] ^ Denc[ 4] ^ Denc[ 0] ^ checksum[155] ^ checksum[151] ^ checksum[150] ^ Denc[ 3] ^ Denc[ 2] ^ checksum[153] ^ checksum[152] ^ checksum[100];
			tmp_checksum[109]=Denc[ 7] ^ Denc[ 6] ^ checksum[149] ^ checksum[148] ^ Denc[ 4] ^ checksum[151] ^ checksum[101];
			tmp_checksum[110]=Denc[ 6] ^ Denc[ 5] ^ checksum[150] ^ checksum[149] ^ Denc[ 3] ^ checksum[152] ^ checksum[102];
			tmp_checksum[111]=Denc[ 7] ^ Denc[ 6] ^ Denc[ 1] ^ checksum[154] ^ checksum[149] ^ Denc[ 3] ^ checksum[152] ^ checksum[148] ^ Denc[ 5] ^ Denc[ 4] ^ checksum[151] ^ checksum[150] ^ checksum[103];
			tmp_checksum[112]=Denc[ 7] ^ Denc[ 1] ^ checksum[154] ^ checksum[148] ^ Denc[ 5] ^ Denc[ 0] ^ checksum[155] ^ checksum[150] ^ Denc[ 4] ^ checksum[151] ^ checksum[104];
			tmp_checksum[113]=Denc[ 6] ^ Denc[ 0] ^ checksum[155] ^ checksum[149] ^ Denc[ 4] ^ checksum[151] ^ Denc[ 3] ^ checksum[152] ^ checksum[105];
			tmp_checksum[114]=Denc[ 7] ^ Denc[ 6] ^ Denc[ 1] ^ checksum[154] ^ checksum[149] ^ checksum[148] ^ Denc[ 5] ^ checksum[150] ^ checksum[106];
			tmp_checksum[115]=Denc[ 6] ^ Denc[ 5] ^ Denc[ 0] ^ checksum[155] ^ checksum[150] ^ checksum[149] ^ Denc[ 4] ^ checksum[151] ^ checksum[107];
			tmp_checksum[116]=Denc[ 7] ^ Denc[ 6] ^ Denc[ 1] ^ checksum[154] ^ Denc[ 2] ^ checksum[153] ^ checksum[149] ^ checksum[148] ^ Denc[ 5] ^ Denc[ 4] ^ checksum[151] ^ checksum[150] ^ checksum[108];
			tmp_checksum[117]=Denc[ 6] ^ Denc[ 5] ^ Denc[ 0] ^ checksum[155] ^ Denc[ 1] ^ checksum[154] ^ checksum[150] ^ checksum[149] ^ Denc[ 4] ^ Denc[ 3] ^ checksum[152] ^ checksum[151] ^ checksum[109];
			tmp_checksum[118]=Denc[ 5] ^ Denc[ 4] ^ Denc[ 0] ^ checksum[155] ^ checksum[151] ^ checksum[150] ^ Denc[ 3] ^ Denc[ 2] ^ checksum[153] ^ checksum[152] ^ checksum[110];
			tmp_checksum[119]=Denc[ 4] ^ Denc[ 3] ^ checksum[152] ^ checksum[151] ^ Denc[ 2] ^ Denc[ 1] ^ checksum[154] ^ checksum[153] ^ checksum[111];
			tmp_checksum[120]=Denc[ 7] ^ Denc[ 6] ^ checksum[149] ^ checksum[148] ^ Denc[ 0] ^ checksum[155] ^ checksum[112];
			tmp_checksum[121]=Denc[ 6] ^ Denc[ 5] ^ checksum[150] ^ checksum[149] ^ checksum[113];
			tmp_checksum[122]=Denc[ 5] ^ Denc[ 4] ^ checksum[151] ^ checksum[150] ^ checksum[114];
			tmp_checksum[123]=Denc[ 4] ^ Denc[ 3] ^ checksum[152] ^ checksum[151] ^ checksum[115];
			tmp_checksum[124]=Denc[ 3] ^ Denc[ 2] ^ checksum[153] ^ checksum[152] ^ checksum[116];
			tmp_checksum[125]=Denc[ 7] ^ Denc[ 6] ^ checksum[149] ^ Denc[ 3] ^ checksum[152] ^ checksum[148] ^ checksum[117];
			tmp_checksum[126]=Denc[ 7] ^ Denc[ 1] ^ checksum[154] ^ Denc[ 3] ^ checksum[152] ^ checksum[148] ^ Denc[ 5] ^ checksum[150] ^ checksum[118];
			tmp_checksum[127]=Denc[ 6] ^ Denc[ 0] ^ checksum[155] ^ Denc[ 2] ^ checksum[153] ^ checksum[149] ^ Denc[ 4] ^ checksum[151] ^ checksum[119];
			tmp_checksum[128]=Denc[ 7] ^ Denc[ 6] ^ Denc[ 2] ^ checksum[153] ^ checksum[149] ^ checksum[148] ^ Denc[ 5] ^ checksum[150] ^ checksum[120];
			tmp_checksum[129]=Denc[ 6] ^ Denc[ 5] ^ Denc[ 1] ^ checksum[154] ^ checksum[150] ^ checksum[149] ^ Denc[ 4] ^ checksum[151] ^ checksum[121];
			tmp_checksum[130]=Denc[ 7] ^ Denc[ 6] ^ Denc[ 1] ^ checksum[154] ^ Denc[ 2] ^ checksum[153] ^ checksum[149] ^ checksum[148] ^ Denc[ 5] ^ Denc[ 4] ^ Denc[ 0] ^ checksum[155] ^ checksum[151] ^ checksum[150] ^ checksum[122];
			tmp_checksum[131]=Denc[ 6] ^ Denc[ 5] ^ Denc[ 0] ^ checksum[155] ^ Denc[ 1] ^ checksum[154] ^ checksum[150] ^ checksum[149] ^ Denc[ 4] ^ Denc[ 3] ^ checksum[152] ^ checksum[151] ^ checksum[123];
			tmp_checksum[132]=Denc[ 5] ^ Denc[ 4] ^ Denc[ 0] ^ checksum[155] ^ checksum[151] ^ checksum[150] ^ Denc[ 3] ^ Denc[ 2] ^ checksum[153] ^ checksum[152] ^ checksum[124];
			tmp_checksum[133]=Denc[ 7] ^ Denc[ 6] ^ checksum[149] ^ checksum[148] ^ Denc[ 4] ^ checksum[151] ^ checksum[125];
			tmp_checksum[134]=Denc[ 6] ^ Denc[ 5] ^ checksum[150] ^ checksum[149] ^ Denc[ 3] ^ checksum[152] ^ checksum[126];
			tmp_checksum[135]=Denc[ 5] ^ Denc[ 4] ^ checksum[151] ^ checksum[150] ^ Denc[ 2] ^ checksum[153] ^ checksum[127];
			tmp_checksum[136]=Denc[ 7] ^ Denc[ 6] ^ Denc[ 2] ^ checksum[153] ^ checksum[149] ^ checksum[148] ^ Denc[ 4] ^ checksum[151] ^ checksum[128];
			tmp_checksum[137]=Denc[ 7] ^ Denc[ 2] ^ checksum[153] ^ checksum[148] ^ Denc[ 5] ^ checksum[150] ^ checksum[129];
			tmp_checksum[138]=Denc[ 6] ^ Denc[ 1] ^ checksum[154] ^ checksum[149] ^ Denc[ 4] ^ checksum[151] ^ checksum[130];
			tmp_checksum[139]=Denc[ 5] ^ Denc[ 0] ^ checksum[155] ^ checksum[150] ^ Denc[ 3] ^ checksum[152] ^ checksum[131];
			tmp_checksum[140]=Denc[ 7] ^ Denc[ 6] ^ Denc[ 1] ^ checksum[154] ^ checksum[149] ^ Denc[ 3] ^ checksum[152] ^ checksum[148] ^ Denc[ 4] ^ checksum[151] ^ checksum[132];
			tmp_checksum[141]=Denc[ 7] ^ Denc[ 1] ^ checksum[154] ^ checksum[148] ^ Denc[ 5] ^ Denc[ 0] ^ checksum[155] ^ checksum[150] ^ checksum[133];
			tmp_checksum[142]=Denc[ 7] ^ Denc[ 1] ^ checksum[154] ^ Denc[ 2] ^ checksum[153] ^ Denc[ 3] ^ checksum[152] ^ checksum[148] ^ Denc[ 0] ^ checksum[155] ^ Denc[ 4] ^ checksum[151] ^ checksum[134];
			tmp_checksum[143]=Denc[ 6] ^ Denc[ 0] ^ checksum[155] ^ Denc[ 1] ^ checksum[154] ^ Denc[ 2] ^ checksum[153] ^ checksum[149] ^ Denc[ 3] ^ checksum[152] ^ checksum[135];
			tmp_checksum[144]=Denc[ 5] ^ Denc[ 0] ^ checksum[155] ^ Denc[ 1] ^ checksum[154] ^ checksum[150] ^ Denc[ 2] ^ checksum[153] ^ checksum[136];
			tmp_checksum[145]=Denc[ 4] ^ Denc[ 0] ^ checksum[155] ^ checksum[151] ^ Denc[ 1] ^ checksum[154] ^ checksum[137];
			tmp_checksum[146]=Denc[ 3] ^ checksum[152] ^ Denc[ 0] ^ checksum[155] ^ checksum[138];
			tmp_checksum[147]=Denc[ 7] ^ Denc[ 6] ^ Denc[ 1] ^ checksum[154] ^ checksum[149] ^ Denc[ 3] ^ checksum[152] ^ checksum[148] ^ checksum[139];
			tmp_checksum[148]=Denc[ 6] ^ Denc[ 5] ^ Denc[ 0] ^ checksum[155] ^ checksum[150] ^ Denc[ 2] ^ checksum[153] ^ checksum[149] ^ checksum[140];
			tmp_checksum[149]=Denc[ 5] ^ Denc[ 4] ^ checksum[151] ^ Denc[ 1] ^ checksum[154] ^ checksum[150] ^ checksum[141];
			tmp_checksum[150]=Denc[ 7] ^ Denc[ 6] ^ Denc[ 1] ^ checksum[154] ^ Denc[ 2] ^ checksum[153] ^ checksum[149] ^ checksum[148] ^ Denc[ 4] ^ Denc[ 0] ^ checksum[155] ^ checksum[151] ^ checksum[142];
			tmp_checksum[151]=Denc[ 6] ^ Denc[ 5] ^ Denc[ 0] ^ checksum[155] ^ Denc[ 1] ^ checksum[154] ^ checksum[150] ^ checksum[149] ^ Denc[ 3] ^ checksum[152] ^ checksum[143];
			tmp_checksum[152]=Denc[ 5] ^ Denc[ 4] ^ Denc[ 0] ^ checksum[155] ^ checksum[151] ^ checksum[150] ^ Denc[ 2] ^ checksum[153] ^ checksum[144];
			tmp_checksum[153]=Denc[ 7] ^ Denc[ 6] ^ Denc[ 2] ^ checksum[153] ^ checksum[149] ^ checksum[148] ^ Denc[ 4] ^ checksum[151] ^ checksum[145];
			tmp_checksum[154]=Denc[ 7] ^ Denc[ 2] ^ checksum[153] ^ checksum[148] ^ Denc[ 5] ^ checksum[150] ^ checksum[146];
			tmp_checksum[155]=Denc[ 7] ^ Denc[ 2] ^ checksum[153] ^ Denc[ 3] ^ checksum[152] ^ checksum[148] ^ Denc[ 4] ^ checksum[151] ^ checksum[147];
			break;
		} // End case(12)
	case (24):
		{
			tmp_checksum[  0]=Ddec[7] ^ Denc[ 7] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[304];
			tmp_checksum[  1]=Ddec[6] ^ Denc[ 7] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 0] ^ checksum[311] ^ Denc[ 2] ^ checksum[309] ^ checksum[305];
			tmp_checksum[  2]=Ddec[5] ^ Denc[ 6] ^ Denc[ 3] ^ Denc[ 0] ^ checksum[311] ^ checksum[308] ^ checksum[305] ^ Denc[ 5] ^ Denc[ 1] ^ checksum[310] ^ checksum[306];
			tmp_checksum[  3]=Ddec[4] ^ Denc[ 7] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ checksum[306] ^ Denc[ 0] ^ checksum[311] ^ checksum[307];
			tmp_checksum[  4]=Ddec[3] ^ Denc[ 6] ^ Denc[ 0] ^ checksum[311] ^ checksum[308] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ checksum[308];
			tmp_checksum[  5]=Ddec[2] ^ Denc[ 5] ^ checksum[309] ^ Denc[ 1] ^ checksum[310] ^ checksum[306] ^ Denc[ 3] ^ Denc[ 0] ^ checksum[311] ^ checksum[308] ^ checksum[309];
			tmp_checksum[  6]=Ddec[1] ^ Denc[ 4] ^ checksum[310] ^ Denc[ 0] ^ checksum[311] ^ checksum[307] ^ Denc[ 2] ^ checksum[309] ^ checksum[310];
			tmp_checksum[  7]=Ddec[0] ^ Denc[ 3] ^ checksum[311] ^ checksum[308] ^ Denc[ 1] ^ checksum[310] ^ checksum[311];
			tmp_checksum[  8]=Denc[ 2] ^ checksum[309] ^ Denc[ 0] ^ checksum[311] ^ checksum[  0];
			tmp_checksum[  9]=Denc[ 1] ^ checksum[310] ^ checksum[  1];
			tmp_checksum[ 10]=Denc[ 7] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 0] ^ checksum[311] ^ checksum[  2];
			tmp_checksum[ 11]=Denc[ 6] ^ Denc[ 3] ^ Denc[ 0] ^ checksum[311] ^ checksum[308] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ checksum[  3];
			tmp_checksum[ 12]=Denc[ 7] ^ Denc[ 4] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ checksum[306] ^ checksum[  4];
			tmp_checksum[ 13]=Denc[ 6] ^ Denc[ 3] ^ checksum[308] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ checksum[  5];
			tmp_checksum[ 14]=Denc[ 7] ^ Denc[ 4] ^ checksum[307] ^ checksum[304] ^ Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ checksum[306] ^ Denc[ 0] ^ checksum[311] ^ checksum[  6];
			tmp_checksum[ 15]=Denc[ 7] ^ checksum[304] ^ Denc[ 6] ^ checksum[305] ^ checksum[  7];
			tmp_checksum[ 16]=Denc[ 6] ^ checksum[305] ^ Denc[ 5] ^ checksum[306] ^ checksum[  8];
			tmp_checksum[ 17]=Denc[ 5] ^ checksum[306] ^ Denc[ 4] ^ checksum[307] ^ checksum[  9];
			tmp_checksum[ 18]=Denc[ 7] ^ Denc[ 1] ^ checksum[310] ^ checksum[304] ^ checksum[ 10];
			tmp_checksum[ 19]=Denc[ 6] ^ Denc[ 0] ^ checksum[311] ^ checksum[305] ^ checksum[ 11];
			tmp_checksum[ 20]=Denc[ 5] ^ checksum[306] ^ checksum[ 12];
			tmp_checksum[ 21]=Denc[ 7] ^ Denc[ 1] ^ checksum[310] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ checksum[ 13];
			tmp_checksum[ 22]=Denc[ 7] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 0] ^ checksum[311] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ checksum[ 14];
			tmp_checksum[ 23]=Denc[ 6] ^ Denc[ 3] ^ Denc[ 0] ^ checksum[311] ^ checksum[308] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 5] ^ Denc[ 1] ^ checksum[310] ^ checksum[306] ^ checksum[ 15];
			tmp_checksum[ 24]=Denc[ 7] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ checksum[306] ^ Denc[ 0] ^ checksum[311] ^ checksum[ 16];
			tmp_checksum[ 25]=Denc[ 7] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ checksum[ 17];
			tmp_checksum[ 26]=Denc[ 7] ^ Denc[ 4] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 5] ^ checksum[306] ^ checksum[ 18];
			tmp_checksum[ 27]=Denc[ 6] ^ Denc[ 3] ^ checksum[308] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 5] ^ Denc[ 1] ^ checksum[310] ^ checksum[306] ^ Denc[ 4] ^ checksum[307] ^ checksum[ 19];
			tmp_checksum[ 28]=Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ Denc[ 1] ^ checksum[310] ^ checksum[306] ^ Denc[ 4] ^ Denc[ 0] ^ checksum[311] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[ 20];
			tmp_checksum[ 29]=Denc[ 7] ^ checksum[304] ^ Denc[ 0] ^ checksum[311] ^ Denc[ 2] ^ checksum[309] ^ checksum[ 21];
			tmp_checksum[ 30]=Denc[ 7] ^ Denc[ 4] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 6] ^ checksum[305] ^ checksum[ 22];
			tmp_checksum[ 31]=Denc[ 6] ^ Denc[ 3] ^ checksum[308] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 5] ^ checksum[306] ^ checksum[ 23];
			tmp_checksum[ 32]=Denc[ 7] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ checksum[306] ^ checksum[ 24];
			tmp_checksum[ 33]=Denc[ 7] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ checksum[ 25];
			tmp_checksum[ 34]=Denc[ 7] ^ Denc[ 4] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 5] ^ checksum[306] ^ checksum[ 26];
			tmp_checksum[ 35]=Denc[ 6] ^ Denc[ 3] ^ checksum[308] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 5] ^ Denc[ 1] ^ checksum[310] ^ checksum[306] ^ Denc[ 4] ^ checksum[307] ^ checksum[ 27];
			tmp_checksum[ 36]=Denc[ 7] ^ checksum[304] ^ Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ checksum[306] ^ Denc[ 0] ^ checksum[311] ^ checksum[ 28];
			tmp_checksum[ 37]=Denc[ 7] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 6] ^ checksum[305] ^ checksum[ 29];
			tmp_checksum[ 38]=Denc[ 7] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 5] ^ checksum[306] ^ checksum[ 30];
			tmp_checksum[ 39]=Denc[ 6] ^ Denc[ 3] ^ Denc[ 0] ^ checksum[311] ^ checksum[308] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 5] ^ Denc[ 1] ^ checksum[310] ^ checksum[306] ^ Denc[ 4] ^ checksum[307] ^ checksum[ 31];
			tmp_checksum[ 40]=Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ Denc[ 1] ^ checksum[310] ^ checksum[306] ^ Denc[ 4] ^ Denc[ 0] ^ checksum[311] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[ 32];
			tmp_checksum[ 41]=Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ Denc[ 0] ^ checksum[311] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ Denc[ 2] ^ checksum[309] ^ checksum[ 33];
			tmp_checksum[ 42]=Denc[ 7] ^ Denc[ 4] ^ checksum[307] ^ checksum[304] ^ Denc[ 0] ^ checksum[311] ^ Denc[ 2] ^ checksum[309] ^ checksum[ 34];
			tmp_checksum[ 43]=Denc[ 6] ^ Denc[ 3] ^ checksum[308] ^ checksum[305] ^ Denc[ 1] ^ checksum[310] ^ checksum[ 35];
			tmp_checksum[ 44]=Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ checksum[306] ^ Denc[ 0] ^ checksum[311] ^ checksum[ 36];
			tmp_checksum[ 45]=Denc[ 7] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ checksum[ 37];
			tmp_checksum[ 46]=Denc[ 6] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ checksum[ 38];
			tmp_checksum[ 47]=Denc[ 5] ^ Denc[ 1] ^ checksum[310] ^ checksum[306] ^ checksum[ 39];
			tmp_checksum[ 48]=Denc[ 7] ^ Denc[ 1] ^ checksum[310] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 0] ^ checksum[311] ^ checksum[ 40];
			tmp_checksum[ 49]=Denc[ 7] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 0] ^ checksum[311] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ checksum[ 41];
			tmp_checksum[ 50]=Denc[ 7] ^ Denc[ 4] ^ checksum[307] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 0] ^ checksum[311] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 5] ^ checksum[306] ^ checksum[ 42];
			tmp_checksum[ 51]=Denc[ 7] ^ checksum[304] ^ Denc[ 6] ^ checksum[305] ^ Denc[ 5] ^ checksum[306] ^ checksum[ 43];
			tmp_checksum[ 52]=Denc[ 6] ^ checksum[305] ^ Denc[ 5] ^ checksum[306] ^ Denc[ 4] ^ checksum[307] ^ checksum[ 44];
			tmp_checksum[ 53]=Denc[ 7] ^ Denc[ 1] ^ checksum[310] ^ checksum[304] ^ Denc[ 5] ^ checksum[306] ^ checksum[ 45];
			tmp_checksum[ 54]=Denc[ 6] ^ Denc[ 0] ^ checksum[311] ^ checksum[305] ^ Denc[ 4] ^ checksum[307] ^ checksum[ 46];
			tmp_checksum[ 55]=Denc[ 5] ^ checksum[306] ^ Denc[ 3] ^ checksum[308] ^ checksum[ 47];
			tmp_checksum[ 56]=Denc[ 4] ^ checksum[307] ^ Denc[ 2] ^ checksum[309] ^ checksum[ 48];
			tmp_checksum[ 57]=Denc[ 7] ^ Denc[ 4] ^ checksum[307] ^ checksum[304] ^ checksum[ 49];
			tmp_checksum[ 58]=Denc[ 7] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ checksum[304] ^ Denc[ 6] ^ checksum[305] ^ checksum[ 50];
			tmp_checksum[ 59]=Denc[ 7] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 0] ^ checksum[311] ^ checksum[305] ^ Denc[ 5] ^ checksum[306] ^ checksum[ 51];
			tmp_checksum[ 60]=Denc[ 6] ^ Denc[ 3] ^ Denc[ 0] ^ checksum[311] ^ checksum[308] ^ checksum[305] ^ Denc[ 5] ^ checksum[306] ^ Denc[ 4] ^ checksum[307] ^ checksum[ 52];
			tmp_checksum[ 61]=Denc[ 7] ^ Denc[ 1] ^ checksum[310] ^ checksum[304] ^ Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ checksum[306] ^ checksum[ 53];
			tmp_checksum[ 62]=Denc[ 6] ^ Denc[ 0] ^ checksum[311] ^ checksum[305] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ checksum[ 54];
			tmp_checksum[ 63]=Denc[ 5] ^ checksum[306] ^ Denc[ 3] ^ Denc[ 0] ^ checksum[311] ^ checksum[308] ^ checksum[ 55];
			tmp_checksum[ 64]=Denc[ 4] ^ checksum[307] ^ Denc[ 2] ^ checksum[309] ^ checksum[ 56];
			tmp_checksum[ 65]=Denc[ 3] ^ checksum[308] ^ Denc[ 1] ^ checksum[310] ^ checksum[ 57];
			tmp_checksum[ 66]=Denc[ 7] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 2] ^ checksum[309] ^ Denc[ 0] ^ checksum[311] ^ checksum[ 58];
			tmp_checksum[ 67]=Denc[ 7] ^ Denc[ 4] ^ checksum[307] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 0] ^ checksum[311] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ checksum[ 59];
			tmp_checksum[ 68]=Denc[ 7] ^ Denc[ 4] ^ checksum[307] ^ checksum[304] ^ Denc[ 6] ^ checksum[305] ^ Denc[ 5] ^ checksum[306] ^ checksum[ 60];
			tmp_checksum[ 69]=Denc[ 6] ^ Denc[ 3] ^ checksum[308] ^ checksum[305] ^ Denc[ 5] ^ checksum[306] ^ Denc[ 4] ^ checksum[307] ^ checksum[ 61];
			tmp_checksum[ 70]=Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ checksum[306] ^ Denc[ 4] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[ 62];
			tmp_checksum[ 71]=Denc[ 7] ^ checksum[304] ^ Denc[ 2] ^ checksum[309] ^ checksum[ 63];
			tmp_checksum[ 72]=Denc[ 6] ^ checksum[305] ^ Denc[ 1] ^ checksum[310] ^ checksum[ 64];
			tmp_checksum[ 73]=Denc[ 7] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 5] ^ checksum[306] ^ Denc[ 0] ^ checksum[311] ^ checksum[ 65];
			tmp_checksum[ 74]=Denc[ 7] ^ Denc[ 1] ^ checksum[310] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 0] ^ checksum[311] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ checksum[ 66];
			tmp_checksum[ 75]=Denc[ 6] ^ Denc[ 0] ^ checksum[311] ^ checksum[305] ^ Denc[ 5] ^ Denc[ 1] ^ checksum[310] ^ checksum[306] ^ checksum[ 67];
			tmp_checksum[ 76]=Denc[ 5] ^ checksum[306] ^ Denc[ 4] ^ Denc[ 0] ^ checksum[311] ^ checksum[307] ^ checksum[ 68];
			tmp_checksum[ 77]=Denc[ 4] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[ 69];
			tmp_checksum[ 78]=Denc[ 3] ^ checksum[308] ^ Denc[ 2] ^ checksum[309] ^ checksum[ 70];
			tmp_checksum[ 79]=Denc[ 2] ^ checksum[309] ^ Denc[ 1] ^ checksum[310] ^ checksum[ 71];
			tmp_checksum[ 80]=Denc[ 1] ^ checksum[310] ^ Denc[ 0] ^ checksum[311] ^ checksum[ 72];
			tmp_checksum[ 81]=Denc[ 7] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 0] ^ checksum[311] ^ checksum[ 73];
			tmp_checksum[ 82]=Denc[ 7] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 0] ^ checksum[311] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ checksum[ 74];
			tmp_checksum[ 83]=Denc[ 6] ^ Denc[ 3] ^ Denc[ 0] ^ checksum[311] ^ checksum[308] ^ checksum[305] ^ Denc[ 5] ^ Denc[ 1] ^ checksum[310] ^ checksum[306] ^ checksum[ 75];
			tmp_checksum[ 84]=Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ checksum[306] ^ Denc[ 4] ^ Denc[ 0] ^ checksum[311] ^ checksum[307] ^ checksum[ 76];
			tmp_checksum[ 85]=Denc[ 7] ^ checksum[304] ^ checksum[ 77];
			tmp_checksum[ 86]=Denc[ 7] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 6] ^ checksum[305] ^ checksum[ 78];
			tmp_checksum[ 87]=Denc[ 7] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 0] ^ checksum[311] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 5] ^ checksum[306] ^ checksum[ 79];
			tmp_checksum[ 88]=Denc[ 6] ^ Denc[ 3] ^ Denc[ 0] ^ checksum[311] ^ checksum[308] ^ checksum[305] ^ Denc[ 5] ^ Denc[ 1] ^ checksum[310] ^ checksum[306] ^ Denc[ 4] ^ checksum[307] ^ checksum[ 80];
			tmp_checksum[ 89]=Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ checksum[306] ^ Denc[ 4] ^ Denc[ 0] ^ checksum[311] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[ 81];
			tmp_checksum[ 90]=Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ Denc[ 2] ^ checksum[309] ^ checksum[ 82];
			tmp_checksum[ 91]=Denc[ 3] ^ Denc[ 0] ^ checksum[311] ^ checksum[308] ^ Denc[ 2] ^ checksum[309] ^ Denc[ 1] ^ checksum[310] ^ checksum[ 83];
			tmp_checksum[ 92]=Denc[ 2] ^ checksum[309] ^ Denc[ 1] ^ checksum[310] ^ Denc[ 0] ^ checksum[311] ^ checksum[ 84];
			tmp_checksum[ 93]=Denc[ 7] ^ Denc[ 4] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 0] ^ checksum[311] ^ checksum[ 85];
			tmp_checksum[ 94]=Denc[ 6] ^ Denc[ 3] ^ checksum[308] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ checksum[ 86];
			tmp_checksum[ 95]=Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ Denc[ 1] ^ checksum[310] ^ checksum[306] ^ checksum[ 87];
			tmp_checksum[ 96]=Denc[ 7] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 0] ^ checksum[311] ^ checksum[ 88];
			tmp_checksum[ 97]=Denc[ 7] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ checksum[ 89];
			tmp_checksum[ 98]=Denc[ 7] ^ Denc[ 4] ^ checksum[307] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 0] ^ checksum[311] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 5] ^ checksum[306] ^ checksum[ 90];
			tmp_checksum[ 99]=Denc[ 6] ^ Denc[ 3] ^ checksum[308] ^ checksum[305] ^ Denc[ 5] ^ Denc[ 1] ^ checksum[310] ^ checksum[306] ^ Denc[ 4] ^ checksum[307] ^ checksum[ 91];
			tmp_checksum[100]=Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ checksum[306] ^ Denc[ 4] ^ Denc[ 0] ^ checksum[311] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[ 92];
			tmp_checksum[101]=Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ Denc[ 2] ^ checksum[309] ^ checksum[ 93];
			tmp_checksum[102]=Denc[ 7] ^ Denc[ 4] ^ checksum[307] ^ checksum[304] ^ Denc[ 0] ^ checksum[311] ^ Denc[ 2] ^ checksum[309] ^ checksum[ 94];
			tmp_checksum[103]=Denc[ 6] ^ Denc[ 3] ^ checksum[308] ^ checksum[305] ^ Denc[ 1] ^ checksum[310] ^ checksum[ 95];
			tmp_checksum[104]=Denc[ 7] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ checksum[306] ^ Denc[ 0] ^ checksum[311] ^ checksum[ 96];
			tmp_checksum[105]=Denc[ 6] ^ Denc[ 3] ^ Denc[ 0] ^ checksum[311] ^ checksum[308] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ checksum[ 97];
			tmp_checksum[106]=Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ Denc[ 1] ^ checksum[310] ^ checksum[306] ^ Denc[ 3] ^ Denc[ 0] ^ checksum[311] ^ checksum[308] ^ checksum[ 98];
			tmp_checksum[107]=Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ Denc[ 0] ^ checksum[311] ^ checksum[307] ^ Denc[ 2] ^ checksum[309] ^ checksum[ 99];
			tmp_checksum[108]=Denc[ 7] ^ Denc[ 4] ^ checksum[307] ^ checksum[304] ^ Denc[ 0] ^ checksum[311] ^ checksum[100];
			tmp_checksum[109]=Denc[ 7] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ checksum[304] ^ Denc[ 6] ^ checksum[305] ^ checksum[101];
			tmp_checksum[110]=Denc[ 6] ^ Denc[ 3] ^ Denc[ 0] ^ checksum[311] ^ checksum[308] ^ checksum[305] ^ Denc[ 5] ^ checksum[306] ^ checksum[102];
			tmp_checksum[111]=Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ checksum[306] ^ Denc[ 4] ^ checksum[307] ^ checksum[103];
			tmp_checksum[112]=Denc[ 7] ^ checksum[304] ^ checksum[104];
			tmp_checksum[113]=Denc[ 6] ^ checksum[305] ^ checksum[105];
			tmp_checksum[114]=Denc[ 7] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 5] ^ checksum[306] ^ checksum[106];
			tmp_checksum[115]=Denc[ 6] ^ Denc[ 3] ^ Denc[ 0] ^ checksum[311] ^ checksum[308] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 4] ^ checksum[307] ^ checksum[107];
			tmp_checksum[116]=Denc[ 7] ^ Denc[ 4] ^ checksum[307] ^ checksum[304] ^ Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ checksum[306] ^ checksum[108];
			tmp_checksum[117]=Denc[ 7] ^ checksum[304] ^ Denc[ 6] ^ checksum[305] ^ checksum[109];
			tmp_checksum[118]=Denc[ 6] ^ checksum[305] ^ Denc[ 5] ^ checksum[306] ^ checksum[110];
			tmp_checksum[119]=Denc[ 5] ^ checksum[306] ^ Denc[ 4] ^ checksum[307] ^ checksum[111];
			tmp_checksum[120]=Denc[ 7] ^ Denc[ 1] ^ checksum[310] ^ checksum[304] ^ checksum[112];
			tmp_checksum[121]=Denc[ 6] ^ Denc[ 0] ^ checksum[311] ^ checksum[305] ^ checksum[113];
			tmp_checksum[122]=Denc[ 7] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 5] ^ checksum[306] ^ checksum[114];
			tmp_checksum[123]=Denc[ 6] ^ Denc[ 3] ^ Denc[ 0] ^ checksum[311] ^ checksum[308] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 4] ^ checksum[307] ^ checksum[115];
			tmp_checksum[124]=Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ Denc[ 1] ^ checksum[310] ^ checksum[306] ^ Denc[ 3] ^ checksum[308] ^ checksum[116];
			tmp_checksum[125]=Denc[ 7] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 0] ^ checksum[311] ^ Denc[ 2] ^ checksum[309] ^ checksum[117];
			tmp_checksum[126]=Denc[ 7] ^ Denc[ 4] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ checksum[118];
			tmp_checksum[127]=Denc[ 6] ^ Denc[ 3] ^ checksum[308] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 5] ^ Denc[ 1] ^ checksum[310] ^ checksum[306] ^ checksum[119];
			tmp_checksum[128]=Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ Denc[ 1] ^ checksum[310] ^ checksum[306] ^ Denc[ 4] ^ Denc[ 0] ^ checksum[311] ^ checksum[307] ^ checksum[120];
			tmp_checksum[129]=Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ Denc[ 0] ^ checksum[311] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[121];
			tmp_checksum[130]=Denc[ 7] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ checksum[304] ^ Denc[ 0] ^ checksum[311] ^ Denc[ 2] ^ checksum[309] ^ checksum[122];
			tmp_checksum[131]=Denc[ 6] ^ Denc[ 3] ^ Denc[ 0] ^ checksum[311] ^ checksum[308] ^ checksum[305] ^ Denc[ 1] ^ checksum[310] ^ checksum[123];
			tmp_checksum[132]=Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ checksum[306] ^ Denc[ 0] ^ checksum[311] ^ checksum[124];
			tmp_checksum[133]=Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ checksum[125];
			tmp_checksum[134]=Denc[ 7] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ checksum[304] ^ Denc[ 0] ^ checksum[311] ^ checksum[126];
			tmp_checksum[135]=Denc[ 6] ^ Denc[ 3] ^ Denc[ 0] ^ checksum[311] ^ checksum[308] ^ checksum[305] ^ checksum[127];
			tmp_checksum[136]=Denc[ 7] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ checksum[306] ^ checksum[128];
			tmp_checksum[137]=Denc[ 7] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 0] ^ checksum[311] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ checksum[129];
			tmp_checksum[138]=Denc[ 6] ^ checksum[305] ^ Denc[ 5] ^ Denc[ 1] ^ checksum[310] ^ checksum[306] ^ checksum[130];
			tmp_checksum[139]=Denc[ 7] ^ Denc[ 1] ^ checksum[310] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 5] ^ checksum[306] ^ Denc[ 0] ^ checksum[311] ^ checksum[131];
			tmp_checksum[140]=Denc[ 7] ^ Denc[ 1] ^ checksum[310] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 0] ^ checksum[311] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ checksum[132];
			tmp_checksum[141]=Denc[ 6] ^ Denc[ 0] ^ checksum[311] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 5] ^ Denc[ 1] ^ checksum[310] ^ checksum[306] ^ checksum[133];
			tmp_checksum[142]=Denc[ 7] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 5] ^ checksum[306] ^ Denc[ 0] ^ checksum[311] ^ checksum[134];
			tmp_checksum[143]=Denc[ 6] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 4] ^ checksum[307] ^ checksum[135];
			tmp_checksum[144]=Denc[ 7] ^ Denc[ 4] ^ checksum[307] ^ checksum[304] ^ Denc[ 5] ^ checksum[306] ^ checksum[136];
			tmp_checksum[145]=Denc[ 7] ^ Denc[ 1] ^ checksum[310] ^ checksum[304] ^ Denc[ 6] ^ checksum[305] ^ checksum[137];
			tmp_checksum[146]=Denc[ 7] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 0] ^ checksum[311] ^ checksum[305] ^ Denc[ 5] ^ checksum[306] ^ checksum[138];
			tmp_checksum[147]=Denc[ 6] ^ Denc[ 3] ^ Denc[ 0] ^ checksum[311] ^ checksum[308] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 5] ^ checksum[306] ^ Denc[ 4] ^ checksum[307] ^ checksum[139];
			tmp_checksum[148]=Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ Denc[ 1] ^ checksum[310] ^ checksum[306] ^ Denc[ 4] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[140];
			tmp_checksum[149]=Denc[ 7] ^ checksum[304] ^ Denc[ 0] ^ checksum[311] ^ Denc[ 2] ^ checksum[309] ^ checksum[141];
			tmp_checksum[150]=Denc[ 6] ^ checksum[305] ^ Denc[ 1] ^ checksum[310] ^ checksum[142];
			tmp_checksum[151]=Denc[ 5] ^ checksum[306] ^ Denc[ 0] ^ checksum[311] ^ checksum[143];
			tmp_checksum[152]=Denc[ 7] ^ Denc[ 1] ^ checksum[310] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ checksum[144];
			tmp_checksum[153]=Denc[ 7] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 0] ^ checksum[311] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ checksum[145];
			tmp_checksum[154]=Denc[ 6] ^ Denc[ 3] ^ Denc[ 0] ^ checksum[311] ^ checksum[308] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 5] ^ Denc[ 1] ^ checksum[310] ^ checksum[306] ^ checksum[146];
			tmp_checksum[155]=Denc[ 7] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ checksum[306] ^ Denc[ 0] ^ checksum[311] ^ checksum[147];
			tmp_checksum[156]=Denc[ 6] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ checksum[148];
			tmp_checksum[157]=Denc[ 7] ^ Denc[ 4] ^ checksum[307] ^ checksum[304] ^ Denc[ 5] ^ checksum[306] ^ Denc[ 0] ^ checksum[311] ^ checksum[149];
			tmp_checksum[158]=Denc[ 7] ^ Denc[ 1] ^ checksum[310] ^ checksum[304] ^ Denc[ 6] ^ checksum[305] ^ checksum[150];
			tmp_checksum[159]=Denc[ 7] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 0] ^ checksum[311] ^ checksum[305] ^ Denc[ 5] ^ checksum[306] ^ checksum[151];
			tmp_checksum[160]=Denc[ 6] ^ Denc[ 3] ^ Denc[ 0] ^ checksum[311] ^ checksum[308] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 5] ^ checksum[306] ^ Denc[ 4] ^ checksum[307] ^ checksum[152];
			tmp_checksum[161]=Denc[ 7] ^ checksum[304] ^ Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ checksum[306] ^ checksum[153];
			tmp_checksum[162]=Denc[ 6] ^ checksum[305] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ checksum[154];
			tmp_checksum[163]=Denc[ 5] ^ checksum[306] ^ Denc[ 3] ^ Denc[ 0] ^ checksum[311] ^ checksum[308] ^ checksum[155];
			tmp_checksum[164]=Denc[ 7] ^ Denc[ 1] ^ checksum[310] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 2] ^ checksum[309] ^ checksum[156];
			tmp_checksum[165]=Denc[ 6] ^ Denc[ 0] ^ checksum[311] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 1] ^ checksum[310] ^ checksum[157];
			tmp_checksum[166]=Denc[ 5] ^ Denc[ 1] ^ checksum[310] ^ checksum[306] ^ Denc[ 0] ^ checksum[311] ^ checksum[158];
			tmp_checksum[167]=Denc[ 7] ^ Denc[ 1] ^ checksum[310] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 0] ^ checksum[311] ^ checksum[159];
			tmp_checksum[168]=Denc[ 7] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 0] ^ checksum[311] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ checksum[160];
			tmp_checksum[169]=Denc[ 6] ^ Denc[ 3] ^ Denc[ 0] ^ checksum[311] ^ checksum[308] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 5] ^ Denc[ 1] ^ checksum[310] ^ checksum[306] ^ checksum[161];
			tmp_checksum[170]=Denc[ 7] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ checksum[306] ^ Denc[ 0] ^ checksum[311] ^ checksum[162];
			tmp_checksum[171]=Denc[ 7] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ checksum[163];
			tmp_checksum[172]=Denc[ 7] ^ Denc[ 4] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 5] ^ checksum[306] ^ checksum[164];
			tmp_checksum[173]=Denc[ 7] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 5] ^ checksum[306] ^ checksum[165];
			tmp_checksum[174]=Denc[ 7] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 6] ^ checksum[305] ^ Denc[ 5] ^ checksum[306] ^ checksum[166];
			tmp_checksum[175]=Denc[ 7] ^ Denc[ 1] ^ checksum[310] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 5] ^ checksum[306] ^ checksum[167];
			tmp_checksum[176]=Denc[ 6] ^ Denc[ 0] ^ checksum[311] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 5] ^ Denc[ 1] ^ checksum[310] ^ checksum[306] ^ Denc[ 4] ^ checksum[307] ^ checksum[168];
			tmp_checksum[177]=Denc[ 5] ^ Denc[ 1] ^ checksum[310] ^ checksum[306] ^ Denc[ 4] ^ Denc[ 0] ^ checksum[311] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[169];
			tmp_checksum[178]=Denc[ 4] ^ Denc[ 0] ^ checksum[311] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ Denc[ 2] ^ checksum[309] ^ checksum[170];
			tmp_checksum[179]=Denc[ 3] ^ checksum[308] ^ Denc[ 2] ^ checksum[309] ^ Denc[ 1] ^ checksum[310] ^ checksum[171];
			tmp_checksum[180]=Denc[ 7] ^ Denc[ 4] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 2] ^ checksum[309] ^ Denc[ 0] ^ checksum[311] ^ checksum[172];
			tmp_checksum[181]=Denc[ 6] ^ Denc[ 3] ^ checksum[308] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 1] ^ checksum[310] ^ checksum[173];
			tmp_checksum[182]=Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ Denc[ 1] ^ checksum[310] ^ checksum[306] ^ Denc[ 0] ^ checksum[311] ^ checksum[174];
			tmp_checksum[183]=Denc[ 7] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 0] ^ checksum[311] ^ checksum[175];
			tmp_checksum[184]=Denc[ 6] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ checksum[176];
			tmp_checksum[185]=Denc[ 5] ^ Denc[ 1] ^ checksum[310] ^ checksum[306] ^ checksum[177];
			tmp_checksum[186]=Denc[ 4] ^ Denc[ 0] ^ checksum[311] ^ checksum[307] ^ checksum[178];
			tmp_checksum[187]=Denc[ 7] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ checksum[304] ^ checksum[179];
			tmp_checksum[188]=Denc[ 6] ^ Denc[ 3] ^ Denc[ 0] ^ checksum[311] ^ checksum[308] ^ checksum[305] ^ checksum[180];
			tmp_checksum[189]=Denc[ 7] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ checksum[306] ^ checksum[181];
			tmp_checksum[190]=Denc[ 6] ^ Denc[ 3] ^ Denc[ 0] ^ checksum[311] ^ checksum[308] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ checksum[182];
			tmp_checksum[191]=Denc[ 7] ^ Denc[ 4] ^ checksum[307] ^ checksum[304] ^ Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ checksum[306] ^ Denc[ 0] ^ checksum[311] ^ checksum[183];
			tmp_checksum[192]=Denc[ 6] ^ Denc[ 3] ^ checksum[308] ^ checksum[305] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ checksum[184];
			tmp_checksum[193]=Denc[ 7] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ checksum[304] ^ Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ checksum[306] ^ Denc[ 0] ^ checksum[311] ^ checksum[185];
			tmp_checksum[194]=Denc[ 6] ^ Denc[ 3] ^ Denc[ 0] ^ checksum[311] ^ checksum[308] ^ checksum[305] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ checksum[186];
			tmp_checksum[195]=Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ checksum[306] ^ Denc[ 3] ^ Denc[ 0] ^ checksum[311] ^ checksum[308] ^ checksum[187];
			tmp_checksum[196]=Denc[ 7] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 2] ^ checksum[309] ^ checksum[188];
			tmp_checksum[197]=Denc[ 7] ^ Denc[ 4] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ checksum[189];
			tmp_checksum[198]=Denc[ 7] ^ Denc[ 4] ^ checksum[307] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 5] ^ checksum[306] ^ checksum[190];
			tmp_checksum[199]=Denc[ 6] ^ Denc[ 3] ^ checksum[308] ^ checksum[305] ^ Denc[ 5] ^ Denc[ 1] ^ checksum[310] ^ checksum[306] ^ Denc[ 4] ^ checksum[307] ^ checksum[191];
			tmp_checksum[200]=Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ checksum[306] ^ Denc[ 4] ^ Denc[ 0] ^ checksum[311] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[192];
			tmp_checksum[201]=Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ Denc[ 2] ^ checksum[309] ^ checksum[193];
			tmp_checksum[202]=Denc[ 7] ^ Denc[ 4] ^ checksum[307] ^ checksum[304] ^ Denc[ 0] ^ checksum[311] ^ Denc[ 2] ^ checksum[309] ^ checksum[194];
			tmp_checksum[203]=Denc[ 6] ^ Denc[ 3] ^ checksum[308] ^ checksum[305] ^ Denc[ 1] ^ checksum[310] ^ checksum[195];
			tmp_checksum[204]=Denc[ 7] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ checksum[306] ^ Denc[ 0] ^ checksum[311] ^ checksum[196];
			tmp_checksum[205]=Denc[ 6] ^ Denc[ 3] ^ Denc[ 0] ^ checksum[311] ^ checksum[308] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ checksum[197];
			tmp_checksum[206]=Denc[ 7] ^ Denc[ 4] ^ checksum[307] ^ checksum[304] ^ Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ checksum[306] ^ Denc[ 0] ^ checksum[311] ^ checksum[198];
			tmp_checksum[207]=Denc[ 6] ^ Denc[ 3] ^ checksum[308] ^ checksum[305] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ checksum[199];
			tmp_checksum[208]=Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ checksum[306] ^ Denc[ 3] ^ Denc[ 0] ^ checksum[311] ^ checksum[308] ^ checksum[200];
			tmp_checksum[209]=Denc[ 7] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 2] ^ checksum[309] ^ checksum[201];
			tmp_checksum[210]=Denc[ 6] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 1] ^ checksum[310] ^ checksum[202];
			tmp_checksum[211]=Denc[ 7] ^ Denc[ 4] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 5] ^ checksum[306] ^ Denc[ 0] ^ checksum[311] ^ checksum[203];
			tmp_checksum[212]=Denc[ 6] ^ Denc[ 3] ^ checksum[308] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 4] ^ checksum[307] ^ checksum[204];
			tmp_checksum[213]=Denc[ 7] ^ Denc[ 4] ^ checksum[307] ^ checksum[304] ^ Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ checksum[306] ^ checksum[205];
			tmp_checksum[214]=Denc[ 6] ^ Denc[ 3] ^ checksum[308] ^ checksum[305] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ checksum[206];
			tmp_checksum[215]=Denc[ 7] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ checksum[304] ^ Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ checksum[306] ^ Denc[ 0] ^ checksum[311] ^ checksum[207];
			tmp_checksum[216]=Denc[ 7] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 0] ^ checksum[311] ^ checksum[305] ^ checksum[208];
			tmp_checksum[217]=Denc[ 7] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 6] ^ checksum[305] ^ Denc[ 5] ^ checksum[306] ^ checksum[209];
			tmp_checksum[218]=Denc[ 6] ^ Denc[ 3] ^ Denc[ 0] ^ checksum[311] ^ checksum[308] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 5] ^ checksum[306] ^ Denc[ 4] ^ checksum[307] ^ checksum[210];
			tmp_checksum[219]=Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ Denc[ 1] ^ checksum[310] ^ checksum[306] ^ Denc[ 4] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[211];
			tmp_checksum[220]=Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ Denc[ 0] ^ checksum[311] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ Denc[ 2] ^ checksum[309] ^ checksum[212];
			tmp_checksum[221]=Denc[ 7] ^ Denc[ 4] ^ checksum[307] ^ checksum[304] ^ Denc[ 0] ^ checksum[311] ^ Denc[ 2] ^ checksum[309] ^ checksum[213];
			tmp_checksum[222]=Denc[ 6] ^ Denc[ 3] ^ checksum[308] ^ checksum[305] ^ Denc[ 1] ^ checksum[310] ^ checksum[214];
			tmp_checksum[223]=Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ checksum[306] ^ Denc[ 0] ^ checksum[311] ^ checksum[215];
			tmp_checksum[224]=Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ checksum[216];
			tmp_checksum[225]=Denc[ 3] ^ Denc[ 0] ^ checksum[311] ^ checksum[308] ^ checksum[217];
			tmp_checksum[226]=Denc[ 7] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 2] ^ checksum[309] ^ checksum[218];
			tmp_checksum[227]=Denc[ 6] ^ Denc[ 3] ^ Denc[ 0] ^ checksum[311] ^ checksum[308] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 1] ^ checksum[310] ^ checksum[219];
			tmp_checksum[228]=Denc[ 7] ^ Denc[ 4] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ checksum[306] ^ Denc[ 0] ^ checksum[311] ^ checksum[220];
			tmp_checksum[229]=Denc[ 6] ^ Denc[ 3] ^ checksum[308] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ checksum[221];
			tmp_checksum[230]=Denc[ 7] ^ Denc[ 4] ^ checksum[307] ^ checksum[304] ^ Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ checksum[306] ^ Denc[ 0] ^ checksum[311] ^ checksum[222];
			tmp_checksum[231]=Denc[ 6] ^ Denc[ 3] ^ checksum[308] ^ checksum[305] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ checksum[223];
			tmp_checksum[232]=Denc[ 7] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ checksum[304] ^ Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ checksum[306] ^ Denc[ 0] ^ checksum[311] ^ checksum[224];
			tmp_checksum[233]=Denc[ 6] ^ Denc[ 3] ^ Denc[ 0] ^ checksum[311] ^ checksum[308] ^ checksum[305] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ checksum[225];
			tmp_checksum[234]=Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ checksum[306] ^ Denc[ 3] ^ Denc[ 0] ^ checksum[311] ^ checksum[308] ^ checksum[226];
			tmp_checksum[235]=Denc[ 7] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 2] ^ checksum[309] ^ checksum[227];
			tmp_checksum[236]=Denc[ 6] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 1] ^ checksum[310] ^ checksum[228];
			tmp_checksum[237]=Denc[ 7] ^ Denc[ 4] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 5] ^ checksum[306] ^ Denc[ 0] ^ checksum[311] ^ checksum[229];
			tmp_checksum[238]=Denc[ 7] ^ Denc[ 1] ^ checksum[310] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ checksum[230];
			tmp_checksum[239]=Denc[ 6] ^ Denc[ 0] ^ checksum[311] ^ checksum[305] ^ Denc[ 5] ^ Denc[ 1] ^ checksum[310] ^ checksum[306] ^ checksum[231];
			tmp_checksum[240]=Denc[ 5] ^ checksum[306] ^ Denc[ 4] ^ Denc[ 0] ^ checksum[311] ^ checksum[307] ^ checksum[232];
			tmp_checksum[241]=Denc[ 4] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[233];
			tmp_checksum[242]=Denc[ 3] ^ checksum[308] ^ Denc[ 2] ^ checksum[309] ^ checksum[234];
			tmp_checksum[243]=Denc[ 7] ^ Denc[ 4] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 2] ^ checksum[309] ^ checksum[235];
			tmp_checksum[244]=Denc[ 7] ^ Denc[ 4] ^ checksum[307] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ checksum[236];
			tmp_checksum[245]=Denc[ 6] ^ Denc[ 3] ^ checksum[308] ^ checksum[305] ^ Denc[ 5] ^ Denc[ 1] ^ checksum[310] ^ checksum[306] ^ checksum[237];
			tmp_checksum[246]=Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ checksum[306] ^ Denc[ 4] ^ Denc[ 0] ^ checksum[311] ^ checksum[307] ^ checksum[238];
			tmp_checksum[247]=Denc[ 7] ^ checksum[304] ^ checksum[239];
			tmp_checksum[248]=Denc[ 7] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 6] ^ checksum[305] ^ checksum[240];
			tmp_checksum[249]=Denc[ 6] ^ Denc[ 3] ^ Denc[ 0] ^ checksum[311] ^ checksum[308] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 5] ^ checksum[306] ^ checksum[241];
			tmp_checksum[250]=Denc[ 7] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ checksum[306] ^ checksum[242];
			tmp_checksum[251]=Denc[ 7] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ checksum[243];
			tmp_checksum[252]=Denc[ 6] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 5] ^ Denc[ 1] ^ checksum[310] ^ checksum[306] ^ checksum[244];
			tmp_checksum[253]=Denc[ 5] ^ Denc[ 1] ^ checksum[310] ^ checksum[306] ^ Denc[ 4] ^ Denc[ 0] ^ checksum[311] ^ checksum[307] ^ checksum[245];
			tmp_checksum[254]=Denc[ 7] ^ Denc[ 1] ^ checksum[310] ^ checksum[304] ^ Denc[ 0] ^ checksum[311] ^ checksum[246];
			tmp_checksum[255]=Denc[ 6] ^ Denc[ 0] ^ checksum[311] ^ checksum[305] ^ checksum[247];
			tmp_checksum[256]=Denc[ 7] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 5] ^ checksum[306] ^ checksum[248];
			tmp_checksum[257]=Denc[ 7] ^ Denc[ 1] ^ checksum[310] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 0] ^ checksum[311] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ checksum[249];
			tmp_checksum[258]=Denc[ 7] ^ Denc[ 4] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 0] ^ checksum[311] ^ checksum[305] ^ Denc[ 5] ^ checksum[306] ^ checksum[250];
			tmp_checksum[259]=Denc[ 7] ^ Denc[ 1] ^ checksum[310] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 5] ^ checksum[306] ^ checksum[251];
			tmp_checksum[260]=Denc[ 6] ^ Denc[ 0] ^ checksum[311] ^ checksum[305] ^ Denc[ 5] ^ Denc[ 1] ^ checksum[310] ^ checksum[306] ^ Denc[ 4] ^ checksum[307] ^ checksum[252];
			tmp_checksum[261]=Denc[ 5] ^ checksum[306] ^ Denc[ 4] ^ Denc[ 0] ^ checksum[311] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[253];
			tmp_checksum[262]=Denc[ 7] ^ Denc[ 1] ^ checksum[310] ^ checksum[304] ^ Denc[ 2] ^ checksum[309] ^ checksum[254];
			tmp_checksum[263]=Denc[ 6] ^ Denc[ 0] ^ checksum[311] ^ checksum[305] ^ Denc[ 1] ^ checksum[310] ^ checksum[255];
			tmp_checksum[264]=Denc[ 7] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 5] ^ checksum[306] ^ Denc[ 0] ^ checksum[311] ^ checksum[256];
			tmp_checksum[265]=Denc[ 7] ^ Denc[ 1] ^ checksum[310] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 0] ^ checksum[311] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ checksum[257];
			tmp_checksum[266]=Denc[ 7] ^ Denc[ 4] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 0] ^ checksum[311] ^ checksum[305] ^ Denc[ 5] ^ checksum[306] ^ checksum[258];
			tmp_checksum[267]=Denc[ 7] ^ Denc[ 1] ^ checksum[310] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 5] ^ checksum[306] ^ checksum[259];
			tmp_checksum[268]=Denc[ 7] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 0] ^ checksum[311] ^ checksum[305] ^ Denc[ 5] ^ checksum[306] ^ checksum[260];
			tmp_checksum[269]=Denc[ 7] ^ Denc[ 1] ^ checksum[310] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 5] ^ checksum[306] ^ checksum[261];
			tmp_checksum[270]=Denc[ 6] ^ Denc[ 0] ^ checksum[311] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 5] ^ Denc[ 1] ^ checksum[310] ^ checksum[306] ^ Denc[ 4] ^ checksum[307] ^ checksum[262];
			tmp_checksum[271]=Denc[ 7] ^ checksum[304] ^ Denc[ 5] ^ checksum[306] ^ Denc[ 0] ^ checksum[311] ^ checksum[263];
			tmp_checksum[272]=Denc[ 7] ^ Denc[ 1] ^ checksum[310] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 6] ^ checksum[305] ^ checksum[264];
			tmp_checksum[273]=Denc[ 7] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 0] ^ checksum[311] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 5] ^ checksum[306] ^ checksum[265];
			tmp_checksum[274]=Denc[ 7] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 0] ^ checksum[311] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 5] ^ checksum[306] ^ checksum[266];
			tmp_checksum[275]=Denc[ 7] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 6] ^ checksum[305] ^ Denc[ 5] ^ checksum[306] ^ checksum[267];
			tmp_checksum[276]=Denc[ 7] ^ Denc[ 1] ^ checksum[310] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 5] ^ checksum[306] ^ checksum[268];
			tmp_checksum[277]=Denc[ 7] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 0] ^ checksum[311] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 5] ^ checksum[306] ^ checksum[269];
			tmp_checksum[278]=Denc[ 6] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 5] ^ Denc[ 1] ^ checksum[310] ^ checksum[306] ^ Denc[ 4] ^ checksum[307] ^ checksum[270];
			tmp_checksum[279]=Denc[ 7] ^ checksum[304] ^ Denc[ 5] ^ checksum[306] ^ Denc[ 0] ^ checksum[311] ^ checksum[271];
			tmp_checksum[280]=Denc[ 7] ^ Denc[ 1] ^ checksum[310] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 6] ^ checksum[305] ^ checksum[272];
			tmp_checksum[281]=Denc[ 6] ^ Denc[ 0] ^ checksum[311] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 5] ^ checksum[306] ^ checksum[273];
			tmp_checksum[282]=Denc[ 7] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 5] ^ checksum[306] ^ checksum[274];
			tmp_checksum[283]=Denc[ 6] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 4] ^ checksum[307] ^ checksum[275];
			tmp_checksum[284]=Denc[ 7] ^ Denc[ 4] ^ checksum[307] ^ checksum[304] ^ Denc[ 5] ^ checksum[306] ^ checksum[276];
			tmp_checksum[285]=Denc[ 6] ^ Denc[ 3] ^ checksum[308] ^ checksum[305] ^ Denc[ 4] ^ checksum[307] ^ checksum[277];
			tmp_checksum[286]=Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ checksum[306] ^ Denc[ 3] ^ checksum[308] ^ checksum[278];
			tmp_checksum[287]=Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ Denc[ 2] ^ checksum[309] ^ checksum[279];
			tmp_checksum[288]=Denc[ 3] ^ Denc[ 0] ^ checksum[311] ^ checksum[308] ^ Denc[ 1] ^ checksum[310] ^ checksum[280];
			tmp_checksum[289]=Denc[ 2] ^ checksum[309] ^ Denc[ 0] ^ checksum[311] ^ checksum[281];
			tmp_checksum[290]=Denc[ 7] ^ Denc[ 4] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ checksum[282];
			tmp_checksum[291]=Denc[ 7] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ checksum[283];
			tmp_checksum[292]=Denc[ 6] ^ Denc[ 3] ^ Denc[ 0] ^ checksum[311] ^ checksum[308] ^ checksum[305] ^ Denc[ 5] ^ Denc[ 1] ^ checksum[310] ^ checksum[306] ^ checksum[284];
			tmp_checksum[293]=Denc[ 7] ^ Denc[ 1] ^ checksum[310] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ checksum[306] ^ Denc[ 0] ^ checksum[311] ^ checksum[285];
			tmp_checksum[294]=Denc[ 6] ^ Denc[ 0] ^ checksum[311] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ checksum[286];
			tmp_checksum[295]=Denc[ 5] ^ Denc[ 1] ^ checksum[310] ^ checksum[306] ^ Denc[ 3] ^ Denc[ 0] ^ checksum[311] ^ checksum[308] ^ checksum[287];
			tmp_checksum[296]=Denc[ 7] ^ Denc[ 1] ^ checksum[310] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 0] ^ checksum[311] ^ Denc[ 2] ^ checksum[309] ^ checksum[288];
			tmp_checksum[297]=Denc[ 7] ^ Denc[ 4] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 0] ^ checksum[311] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ checksum[289];
			tmp_checksum[298]=Denc[ 7] ^ Denc[ 4] ^ checksum[307] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ Denc[ 5] ^ checksum[306] ^ checksum[290];
			tmp_checksum[299]=Denc[ 7] ^ checksum[304] ^ Denc[ 6] ^ checksum[305] ^ Denc[ 5] ^ checksum[306] ^ checksum[291];
			tmp_checksum[300]=Denc[ 6] ^ checksum[305] ^ Denc[ 5] ^ checksum[306] ^ Denc[ 4] ^ checksum[307] ^ checksum[292];
			tmp_checksum[301]=Denc[ 7] ^ Denc[ 1] ^ checksum[310] ^ checksum[304] ^ Denc[ 5] ^ checksum[306] ^ checksum[293];
			tmp_checksum[302]=Denc[ 6] ^ Denc[ 0] ^ checksum[311] ^ checksum[305] ^ Denc[ 4] ^ checksum[307] ^ checksum[294];
			tmp_checksum[303]=Denc[ 7] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ checksum[304] ^ Denc[ 5] ^ checksum[306] ^ checksum[295];
			tmp_checksum[304]=Denc[ 6] ^ Denc[ 3] ^ Denc[ 0] ^ checksum[311] ^ checksum[308] ^ checksum[305] ^ Denc[ 4] ^ checksum[307] ^ checksum[296];
			tmp_checksum[305]=Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ checksum[306] ^ Denc[ 3] ^ checksum[308] ^ checksum[297];
			tmp_checksum[306]=Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ Denc[ 2] ^ checksum[309] ^ checksum[298];
			tmp_checksum[307]=Denc[ 3] ^ Denc[ 0] ^ checksum[311] ^ checksum[308] ^ Denc[ 1] ^ checksum[310] ^ checksum[299];
			tmp_checksum[308]=Denc[ 7] ^ Denc[ 4] ^ Denc[ 1] ^ checksum[310] ^ checksum[307] ^ Denc[ 3] ^ checksum[308] ^ checksum[304] ^ Denc[ 2] ^ checksum[309] ^ Denc[ 0] ^ checksum[311] ^ checksum[300];
			tmp_checksum[309]=Denc[ 7] ^ Denc[ 4] ^ checksum[307] ^ checksum[304] ^ Denc[ 6] ^ Denc[ 0] ^ checksum[311] ^ Denc[ 2] ^ checksum[309] ^ checksum[305] ^ checksum[301];
			tmp_checksum[310]=Denc[ 6] ^ Denc[ 3] ^ checksum[308] ^ checksum[305] ^ Denc[ 5] ^ Denc[ 1] ^ checksum[310] ^ checksum[306] ^ checksum[302];
			tmp_checksum[311]=Denc[ 5] ^ Denc[ 2] ^ checksum[309] ^ checksum[306] ^ Denc[ 4] ^ Denc[ 0] ^ checksum[311] ^ checksum[307] ^ checksum[303];
			break;
		} //end case(24)
	} // end switch

	// Here we take into consideration the fact that the checksum function length is 3*t*GF(in our case m=13)
	
	if (print_checksum_for_debug == 1)
	{
		if(Mode==ENCODE_MODE)
		{
			//fprintf(CHECKSUM_FILE,"Encode, Din[0-7]=%01d%01d%01d%01d%01d%01d%01d%01d Checksum[0-%d]=",Denc[0],Denc[1],Denc[2],Denc[3],Denc[4],Denc[5],Denc[6],Denc[7],(13*t)-1);
			fprintf(CHECKSUM_FILE,"Encode, Din[7-0]=%01d%01d%01d%01d%01d%01d%01d%01d Checksum[%d-0]=",Denc[7],Denc[6],Denc[5],Denc[4],Denc[3],Denc[2],Denc[1],Denc[0],(13*t)-1);
		}
		else
		{
			//fprintf(CHECKSUM_FILE,"Decode, Din[0-7]=%01d%01d%01d%01d%01d%01d%01d%01d Checksum[0-%d]=",Ddec[0],Ddec[1],Ddec[2],Ddec[3],Ddec[4],Ddec[5],Ddec[6],Ddec[7],(13*t)-1);
			fprintf(CHECKSUM_FILE,"Decode, Din[7-0]=%01d%01d%01d%01d%01d%01d%01d%01d Checksum[%d-0]=",Ddec[7],Ddec[6],Ddec[5],Ddec[4],Ddec[3],Ddec[2],Ddec[1],Ddec[0],(13*t)-1);
		}
	}
	//for(bit_index=0;bit_index<(t*m);bit_index++)
	for(bit_index=(t*m)-1;bit_index>=0;bit_index--)
	{
		checksum[bit_index] = tmp_checksum[bit_index];
		if (print_checksum_for_debug == 1)
		{
			fprintf(CHECKSUM_FILE,"%01d",checksum[bit_index]);
		}
	}

	if (print_checksum_for_debug == 1)
	{
		fprintf(CHECKSUM_FILE,"\n");
	}

} // function end

void syndrome_calc(int current_bit, int mode)
// This function calculates the Syndromes values based on pre-defined XOR funxtions
// The function only scan the minimal set of syndromes in order to match the needed error correction level
{
	int index;
	int bit_index;
	int max_index=1;
	int temp_S[13];
	int Din;

	// Configure the syndrome calculation engines operating modes
	if (mode == SYN_CALC)
	{
		max_index = 2*t;
		Din = current_bit;
	}
	else
	{
		if (mode == ERR_LOC_CALC)
		{
			max_index = t;
			Din = 0;
		}
	}
	// Update the syndrome variables according to the indexed equation
	for (index=0;index<=max_index;index++)
	{
		switch (index)
		{
		case(0):
			{
				temp_S[ 0]=Din ^ Syndromes[index].S[ 0];
				temp_S[ 1]=Syndromes[index].S[ 1];
				temp_S[ 2]=Syndromes[index].S[ 2];
				temp_S[ 3]=Syndromes[index].S[ 3];
				temp_S[ 4]=Syndromes[index].S[ 4];
				temp_S[ 5]=Syndromes[index].S[ 5];
				temp_S[ 6]=Syndromes[index].S[ 6];
				temp_S[ 7]=Syndromes[index].S[ 7];
				temp_S[ 8]=Syndromes[index].S[ 8];
				temp_S[ 9]=Syndromes[index].S[ 9];
				temp_S[10]=Syndromes[index].S[10];
				temp_S[11]=Syndromes[index].S[11];
				temp_S[12]=Syndromes[index].S[12];
				break;
			}
		case(1):
			{
				temp_S[ 0]=Syndromes[index].S[12] ^ Din;
				temp_S[ 1]=Syndromes[index].S[ 0] ^ Syndromes[index].S[12];
				temp_S[ 2]=Syndromes[index].S[ 1];
				temp_S[ 3]=Syndromes[index].S[ 2] ^ Syndromes[index].S[12];
				temp_S[ 4]=Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[ 5]=Syndromes[index].S[ 4];
				temp_S[ 6]=Syndromes[index].S[ 5];
				temp_S[ 7]=Syndromes[index].S[ 6];
				temp_S[ 8]=Syndromes[index].S[ 7];
				temp_S[ 9]=Syndromes[index].S[ 8];
				temp_S[10]=Syndromes[index].S[ 9];
				temp_S[11]=Syndromes[index].S[10];
				temp_S[12]=Syndromes[index].S[11];
				break;
			}
		case(2):
			{
				temp_S[ 0]=Syndromes[index].S[11] ^ Din;
				temp_S[ 1]=Syndromes[index].S[12] ^ Syndromes[index].S[11];
				temp_S[ 2]=Syndromes[index].S[ 0] ^ Syndromes[index].S[12];
				temp_S[ 3]=Syndromes[index].S[ 1] ^ Syndromes[index].S[11];
				temp_S[ 4]=Syndromes[index].S[ 2] ^ Syndromes[index].S[12] ^ Syndromes[index].S[11];
				temp_S[ 5]=Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[ 6]=Syndromes[index].S[ 4];
				temp_S[ 7]=Syndromes[index].S[ 5];
				temp_S[ 8]=Syndromes[index].S[ 6];
				temp_S[ 9]=Syndromes[index].S[ 7];
				temp_S[10]=Syndromes[index].S[ 8];
				temp_S[11]=Syndromes[index].S[ 9];
				temp_S[12]=Syndromes[index].S[10];
				break;
			}
		case(3):
			{
				temp_S[ 0]=Syndromes[index].S[10] ^ Din;
				temp_S[ 1]=Syndromes[index].S[11] ^ Syndromes[index].S[10];
				temp_S[ 2]=Syndromes[index].S[12] ^ Syndromes[index].S[11];
				temp_S[ 3]=Syndromes[index].S[ 0] ^ Syndromes[index].S[12] ^ Syndromes[index].S[10];
				temp_S[ 4]=Syndromes[index].S[ 1] ^ Syndromes[index].S[11] ^ Syndromes[index].S[10];
				temp_S[ 5]=Syndromes[index].S[ 2] ^ Syndromes[index].S[12] ^ Syndromes[index].S[11];
				temp_S[ 6]=Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[ 7]=Syndromes[index].S[ 4];
				temp_S[ 8]=Syndromes[index].S[ 5];
				temp_S[ 9]=Syndromes[index].S[ 6];
				temp_S[10]=Syndromes[index].S[ 7];
				temp_S[11]=Syndromes[index].S[ 8];
				temp_S[12]=Syndromes[index].S[ 9];
				break;
			}
		case(4):
			{
				temp_S[ 0]=Syndromes[index].S[ 9] ^ Din;
				temp_S[ 1]=Syndromes[index].S[10] ^ Syndromes[index].S[ 9];
				temp_S[ 2]=Syndromes[index].S[11] ^ Syndromes[index].S[10];
				temp_S[ 3]=Syndromes[index].S[12] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 9];
				temp_S[ 4]=Syndromes[index].S[ 0] ^ Syndromes[index].S[12] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 9];
				temp_S[ 5]=Syndromes[index].S[ 1] ^ Syndromes[index].S[11] ^ Syndromes[index].S[10];
				temp_S[ 6]=Syndromes[index].S[ 2] ^ Syndromes[index].S[12] ^ Syndromes[index].S[11];
				temp_S[ 7]=Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[ 8]=Syndromes[index].S[ 4];
				temp_S[ 9]=Syndromes[index].S[ 5];
				temp_S[10]=Syndromes[index].S[ 6];
				temp_S[11]=Syndromes[index].S[ 7];
				temp_S[12]=Syndromes[index].S[ 8];
				break;
			}
		case(5):
			{
				temp_S[ 0]=Syndromes[index].S[ 8] ^ Din;
				temp_S[ 1]=Syndromes[index].S[ 9] ^ Syndromes[index].S[ 8];
				temp_S[ 2]=Syndromes[index].S[10] ^ Syndromes[index].S[ 9];
				temp_S[ 3]=Syndromes[index].S[11] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8];
				temp_S[ 4]=Syndromes[index].S[12] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 8];
				temp_S[ 5]=Syndromes[index].S[ 0] ^ Syndromes[index].S[12] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 9];
				temp_S[ 6]=Syndromes[index].S[ 1] ^ Syndromes[index].S[11] ^ Syndromes[index].S[10];
				temp_S[ 7]=Syndromes[index].S[ 2] ^ Syndromes[index].S[12] ^ Syndromes[index].S[11];
				temp_S[ 8]=Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[ 9]=Syndromes[index].S[ 4];
				temp_S[10]=Syndromes[index].S[ 5];
				temp_S[11]=Syndromes[index].S[ 6];
				temp_S[12]=Syndromes[index].S[ 7];
				break;
			}
		case(6):
			{
				temp_S[ 0]=Syndromes[index].S[ 7] ^ Din;
				temp_S[ 1]=Syndromes[index].S[ 8] ^ Syndromes[index].S[ 7];
				temp_S[ 2]=Syndromes[index].S[ 9] ^ Syndromes[index].S[ 8];
				temp_S[ 3]=Syndromes[index].S[10] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7];
				temp_S[ 4]=Syndromes[index].S[11] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 7];
				temp_S[ 5]=Syndromes[index].S[12] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 8];
				temp_S[ 6]=Syndromes[index].S[ 0] ^ Syndromes[index].S[12] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 9];
				temp_S[ 7]=Syndromes[index].S[ 1] ^ Syndromes[index].S[11] ^ Syndromes[index].S[10];
				temp_S[ 8]=Syndromes[index].S[ 2] ^ Syndromes[index].S[12] ^ Syndromes[index].S[11];
				temp_S[ 9]=Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[10]=Syndromes[index].S[ 4];
				temp_S[11]=Syndromes[index].S[ 5];
				temp_S[12]=Syndromes[index].S[ 6];
				break;
			}
		case(7):
			{
				temp_S[ 0]=Syndromes[index].S[ 6] ^ Din;
				temp_S[ 1]=Syndromes[index].S[ 7] ^ Syndromes[index].S[ 6];
				temp_S[ 2]=Syndromes[index].S[ 8] ^ Syndromes[index].S[ 7];
				temp_S[ 3]=Syndromes[index].S[ 9] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6];
				temp_S[ 4]=Syndromes[index].S[10] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 6];
				temp_S[ 5]=Syndromes[index].S[11] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 7];
				temp_S[ 6]=Syndromes[index].S[12] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 8];
				temp_S[ 7]=Syndromes[index].S[ 0] ^ Syndromes[index].S[12] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 9];
				temp_S[ 8]=Syndromes[index].S[ 1] ^ Syndromes[index].S[11] ^ Syndromes[index].S[10];
				temp_S[ 9]=Syndromes[index].S[ 2] ^ Syndromes[index].S[12] ^ Syndromes[index].S[11];
				temp_S[10]=Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[11]=Syndromes[index].S[ 4];
				temp_S[12]=Syndromes[index].S[ 5];
				break;
			}
		case(8):
			{
				temp_S[ 0]=Syndromes[index].S[ 5] ^ Din;
				temp_S[ 1]=Syndromes[index].S[ 6] ^ Syndromes[index].S[ 5];
				temp_S[ 2]=Syndromes[index].S[ 7] ^ Syndromes[index].S[ 6];
				temp_S[ 3]=Syndromes[index].S[ 8] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5];
				temp_S[ 4]=Syndromes[index].S[ 9] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 5];
				temp_S[ 5]=Syndromes[index].S[10] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 6];
				temp_S[ 6]=Syndromes[index].S[11] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 7];
				temp_S[ 7]=Syndromes[index].S[12] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 8];
				temp_S[ 8]=Syndromes[index].S[ 0] ^ Syndromes[index].S[12] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 9];
				temp_S[ 9]=Syndromes[index].S[ 1] ^ Syndromes[index].S[11] ^ Syndromes[index].S[10];
				temp_S[10]=Syndromes[index].S[ 2] ^ Syndromes[index].S[12] ^ Syndromes[index].S[11];
				temp_S[11]=Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[12]=Syndromes[index].S[ 4];
				break;
			}
		case(9):
			{
				temp_S[ 0]=Syndromes[index].S[ 4] ^ Din;
				temp_S[ 1]=Syndromes[index].S[ 5] ^ Syndromes[index].S[ 4];
				temp_S[ 2]=Syndromes[index].S[ 6] ^ Syndromes[index].S[ 5];
				temp_S[ 3]=Syndromes[index].S[ 7] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4];
				temp_S[ 4]=Syndromes[index].S[ 8] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 4];
				temp_S[ 5]=Syndromes[index].S[ 9] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 5];
				temp_S[ 6]=Syndromes[index].S[10] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 6];
				temp_S[ 7]=Syndromes[index].S[11] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 7];
				temp_S[ 8]=Syndromes[index].S[12] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 8];
				temp_S[ 9]=Syndromes[index].S[ 0] ^ Syndromes[index].S[12] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 9];
				temp_S[10]=Syndromes[index].S[ 1] ^ Syndromes[index].S[11] ^ Syndromes[index].S[10];
				temp_S[11]=Syndromes[index].S[ 2] ^ Syndromes[index].S[12] ^ Syndromes[index].S[11];
				temp_S[12]=Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				break;
			}
		case(10):
			{
				temp_S[ 0]=Syndromes[index].S[ 3] ^ Syndromes[index].S[12] ^ Din;
				temp_S[ 1]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[ 2]=Syndromes[index].S[ 5] ^ Syndromes[index].S[ 4];
				temp_S[ 3]=Syndromes[index].S[ 6] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[ 4]=Syndromes[index].S[ 7] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[ 5]=Syndromes[index].S[ 8] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 4];
				temp_S[ 6]=Syndromes[index].S[ 9] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 5];
				temp_S[ 7]=Syndromes[index].S[10] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 6];
				temp_S[ 8]=Syndromes[index].S[11] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 7];
				temp_S[ 9]=Syndromes[index].S[12] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 8];
				temp_S[10]=Syndromes[index].S[ 0] ^ Syndromes[index].S[12] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 9];
				temp_S[11]=Syndromes[index].S[ 1] ^ Syndromes[index].S[11] ^ Syndromes[index].S[10];
				temp_S[12]=Syndromes[index].S[ 2] ^ Syndromes[index].S[12] ^ Syndromes[index].S[11];
				break;
			}
		case(11):
			{
				temp_S[ 0]=Syndromes[index].S[ 2] ^ Syndromes[index].S[12] ^ Syndromes[index].S[11] ^ Din;
				temp_S[ 1]=Syndromes[index].S[ 3] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11];
				temp_S[ 2]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[ 3]=Syndromes[index].S[ 5] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[12] ^ Syndromes[index].S[11];
				temp_S[ 4]=Syndromes[index].S[ 6] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11];
				temp_S[ 5]=Syndromes[index].S[ 7] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[ 6]=Syndromes[index].S[ 8] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 4];
				temp_S[ 7]=Syndromes[index].S[ 9] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 5];
				temp_S[ 8]=Syndromes[index].S[10] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 6];
				temp_S[ 9]=Syndromes[index].S[11] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 7];
				temp_S[10]=Syndromes[index].S[12] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 8];
				temp_S[11]=Syndromes[index].S[ 0] ^ Syndromes[index].S[12] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 9];
				temp_S[12]=Syndromes[index].S[ 1] ^ Syndromes[index].S[11] ^ Syndromes[index].S[10];
				break;
			}
		case(12):
			{
				temp_S[ 0]=Syndromes[index].S[ 1] ^ Syndromes[index].S[11] ^ Syndromes[index].S[10] ^ Din;
				temp_S[ 1]=Syndromes[index].S[ 2] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10];
				temp_S[ 2]=Syndromes[index].S[ 3] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11];
				temp_S[ 3]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[11] ^ Syndromes[index].S[10];
				temp_S[ 4]=Syndromes[index].S[ 5] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10];
				temp_S[ 5]=Syndromes[index].S[ 6] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11];
				temp_S[ 6]=Syndromes[index].S[ 7] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[ 7]=Syndromes[index].S[ 8] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 4];
				temp_S[ 8]=Syndromes[index].S[ 9] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 5];
				temp_S[ 9]=Syndromes[index].S[10] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 6];
				temp_S[10]=Syndromes[index].S[11] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 7];
				temp_S[11]=Syndromes[index].S[12] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 8];
				temp_S[12]=Syndromes[index].S[ 0] ^ Syndromes[index].S[12] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 9];
				break;
			}
		case(13):
			{
				temp_S[ 0]=Syndromes[index].S[ 0] ^ Syndromes[index].S[12] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 9] ^ Din;
				temp_S[ 1]=Syndromes[index].S[ 1] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 9];
				temp_S[ 2]=Syndromes[index].S[ 2] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10];
				temp_S[ 3]=Syndromes[index].S[ 3] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[12] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 9];
				temp_S[ 4]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 9];
				temp_S[ 5]=Syndromes[index].S[ 5] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10];
				temp_S[ 6]=Syndromes[index].S[ 6] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11];
				temp_S[ 7]=Syndromes[index].S[ 7] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[ 8]=Syndromes[index].S[ 8] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 4];
				temp_S[ 9]=Syndromes[index].S[ 9] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 5];
				temp_S[10]=Syndromes[index].S[10] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 6];
				temp_S[11]=Syndromes[index].S[11] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 7];
				temp_S[12]=Syndromes[index].S[12] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 8];
				break;
			}
		case(14):
			{
				temp_S[ 0]=Syndromes[index].S[12] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 8] ^ Din;
				temp_S[ 1]=Syndromes[index].S[ 0] ^ Syndromes[index].S[10] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 8];
				temp_S[ 2]=Syndromes[index].S[ 1] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 9];
				temp_S[ 3]=Syndromes[index].S[ 2] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 8];
				temp_S[ 4]=Syndromes[index].S[ 3] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8];
				temp_S[ 5]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 9];
				temp_S[ 6]=Syndromes[index].S[ 5] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10];
				temp_S[ 7]=Syndromes[index].S[ 6] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11];
				temp_S[ 8]=Syndromes[index].S[ 7] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[ 9]=Syndromes[index].S[ 8] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 4];
				temp_S[10]=Syndromes[index].S[ 9] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 5];
				temp_S[11]=Syndromes[index].S[10] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 6];
				temp_S[12]=Syndromes[index].S[11] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 7];
				break;
			}
		case(15):
			{
				temp_S[ 0]=Syndromes[index].S[11] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 7] ^ Din;
				temp_S[ 1]=Syndromes[index].S[12] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 7];
				temp_S[ 2]=Syndromes[index].S[ 0] ^ Syndromes[index].S[10] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 8];
				temp_S[ 3]=Syndromes[index].S[ 1] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 7];
				temp_S[ 4]=Syndromes[index].S[ 2] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7];
				temp_S[ 5]=Syndromes[index].S[ 3] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8];
				temp_S[ 6]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 9];
				temp_S[ 7]=Syndromes[index].S[ 5] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10];
				temp_S[ 8]=Syndromes[index].S[ 6] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11];
				temp_S[ 9]=Syndromes[index].S[ 7] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[10]=Syndromes[index].S[ 8] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 4];
				temp_S[11]=Syndromes[index].S[ 9] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 5];
				temp_S[12]=Syndromes[index].S[10] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 6];
				break;
			}
		case(16):
			{
				temp_S[ 0]=Syndromes[index].S[10] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 6] ^ Din;
				temp_S[ 1]=Syndromes[index].S[11] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 6];
				temp_S[ 2]=Syndromes[index].S[12] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 7];
				temp_S[ 3]=Syndromes[index].S[ 0] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 6];
				temp_S[ 4]=Syndromes[index].S[ 1] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6];
				temp_S[ 5]=Syndromes[index].S[ 2] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7];
				temp_S[ 6]=Syndromes[index].S[ 3] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8];
				temp_S[ 7]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 9];
				temp_S[ 8]=Syndromes[index].S[ 5] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10];
				temp_S[ 9]=Syndromes[index].S[ 6] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11];
				temp_S[10]=Syndromes[index].S[ 7] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[11]=Syndromes[index].S[ 8] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 4];
				temp_S[12]=Syndromes[index].S[ 9] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 5];
				break;
			}
		case(17):
			{
				temp_S[ 0]=Syndromes[index].S[ 9] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 5] ^ Din;
				temp_S[ 1]=Syndromes[index].S[10] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 5];
				temp_S[ 2]=Syndromes[index].S[11] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 6];
				temp_S[ 3]=Syndromes[index].S[12] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 5];
				temp_S[ 4]=Syndromes[index].S[ 0] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5];
				temp_S[ 5]=Syndromes[index].S[ 1] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6];
				temp_S[ 6]=Syndromes[index].S[ 2] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7];
				temp_S[ 7]=Syndromes[index].S[ 3] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8];
				temp_S[ 8]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 9];
				temp_S[ 9]=Syndromes[index].S[ 5] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10];
				temp_S[10]=Syndromes[index].S[ 6] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11];
				temp_S[11]=Syndromes[index].S[ 7] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[12]=Syndromes[index].S[ 8] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 4];
				break;
			}
		case(18):
			{
				temp_S[ 0]=Syndromes[index].S[ 8] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 4] ^ Din;
				temp_S[ 1]=Syndromes[index].S[ 9] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 4];
				temp_S[ 2]=Syndromes[index].S[10] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 5];
				temp_S[ 3]=Syndromes[index].S[11] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 4];
				temp_S[ 4]=Syndromes[index].S[12] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4];
				temp_S[ 5]=Syndromes[index].S[ 0] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5];
				temp_S[ 6]=Syndromes[index].S[ 1] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6];
				temp_S[ 7]=Syndromes[index].S[ 2] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7];
				temp_S[ 8]=Syndromes[index].S[ 3] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8];
				temp_S[ 9]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 9];
				temp_S[10]=Syndromes[index].S[ 5] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10];
				temp_S[11]=Syndromes[index].S[ 6] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11];
				temp_S[12]=Syndromes[index].S[ 7] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				break;
			}
		case(19):
			{
				temp_S[ 0]=Syndromes[index].S[ 7] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12] ^ Din;
				temp_S[ 1]=Syndromes[index].S[ 8] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[ 2]=Syndromes[index].S[ 9] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 4];
				temp_S[ 3]=Syndromes[index].S[10] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[ 4]=Syndromes[index].S[11] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[ 5]=Syndromes[index].S[12] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4];
				temp_S[ 6]=Syndromes[index].S[ 0] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5];
				temp_S[ 7]=Syndromes[index].S[ 1] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6];
				temp_S[ 8]=Syndromes[index].S[ 2] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7];
				temp_S[ 9]=Syndromes[index].S[ 3] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8];
				temp_S[10]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 9];
				temp_S[11]=Syndromes[index].S[ 5] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10];
				temp_S[12]=Syndromes[index].S[ 6] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11];
				break;
			}
		case(20):
			{
				temp_S[ 0]=Syndromes[index].S[ 6] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11] ^ Din;
				temp_S[ 1]=Syndromes[index].S[ 7] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11];
				temp_S[ 2]=Syndromes[index].S[ 8] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[ 3]=Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11];
				temp_S[ 4]=Syndromes[index].S[10] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11];
				temp_S[ 5]=Syndromes[index].S[11] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[ 6]=Syndromes[index].S[12] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4];
				temp_S[ 7]=Syndromes[index].S[ 0] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5];
				temp_S[ 8]=Syndromes[index].S[ 1] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6];
				temp_S[ 9]=Syndromes[index].S[ 2] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7];
				temp_S[10]=Syndromes[index].S[ 3] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8];
				temp_S[11]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 9];
				temp_S[12]=Syndromes[index].S[ 5] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10];
				break;
			}
		case(21):
			{
				temp_S[ 0]=Syndromes[index].S[ 5] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10] ^ Din;
				temp_S[ 1]=Syndromes[index].S[ 6] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10];
				temp_S[ 2]=Syndromes[index].S[ 7] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11];
				temp_S[ 3]=Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10];
				temp_S[ 4]=Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10];
				temp_S[ 5]=Syndromes[index].S[10] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11];
				temp_S[ 6]=Syndromes[index].S[11] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[ 7]=Syndromes[index].S[12] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4];
				temp_S[ 8]=Syndromes[index].S[ 0] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5];
				temp_S[ 9]=Syndromes[index].S[ 1] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6];
				temp_S[10]=Syndromes[index].S[ 2] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7];
				temp_S[11]=Syndromes[index].S[ 3] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8];
				temp_S[12]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 9];
				break;
			}
		case(22):
			{
				temp_S[ 0]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 9] ^ Din;
				temp_S[ 1]=Syndromes[index].S[ 5] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[12] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 9];
				temp_S[ 2]=Syndromes[index].S[ 6] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10];
				temp_S[ 3]=Syndromes[index].S[ 7] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 9];
				temp_S[ 4]=Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 9];
				temp_S[ 5]=Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10];
				temp_S[ 6]=Syndromes[index].S[10] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11];
				temp_S[ 7]=Syndromes[index].S[11] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[ 8]=Syndromes[index].S[12] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4];
				temp_S[ 9]=Syndromes[index].S[ 0] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5];
				temp_S[10]=Syndromes[index].S[ 1] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6];
				temp_S[11]=Syndromes[index].S[ 2] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7];
				temp_S[12]=Syndromes[index].S[ 3] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8];
				break;
			}
		case(23):
			{
				temp_S[ 0]=Syndromes[index].S[ 3] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8] ^ Din;
				temp_S[ 1]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8];
				temp_S[ 2]=Syndromes[index].S[ 5] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[12] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 9];
				temp_S[ 3]=Syndromes[index].S[ 6] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 8];
				temp_S[ 4]=Syndromes[index].S[ 7] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8];
				temp_S[ 5]=Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 9];
				temp_S[ 6]=Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10];
				temp_S[ 7]=Syndromes[index].S[10] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11];
				temp_S[ 8]=Syndromes[index].S[11] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[ 9]=Syndromes[index].S[12] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4];
				temp_S[10]=Syndromes[index].S[ 0] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5];
				temp_S[11]=Syndromes[index].S[ 1] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6];
				temp_S[12]=Syndromes[index].S[ 2] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7];
				break;
			}
		case(24):
			{
				temp_S[ 0]=Syndromes[index].S[ 2] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7] ^ Din;
				temp_S[ 1]=Syndromes[index].S[ 3] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7];
				temp_S[ 2]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8];
				temp_S[ 3]=Syndromes[index].S[ 5] ^ Syndromes[index].S[12] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 7];
				temp_S[ 4]=Syndromes[index].S[ 6] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7];
				temp_S[ 5]=Syndromes[index].S[ 7] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8];
				temp_S[ 6]=Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 9];
				temp_S[ 7]=Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10];
				temp_S[ 8]=Syndromes[index].S[10] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11];
				temp_S[ 9]=Syndromes[index].S[11] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[10]=Syndromes[index].S[12] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4];
				temp_S[11]=Syndromes[index].S[ 0] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5];
				temp_S[12]=Syndromes[index].S[ 1] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6];
				break;
			}
		case(25):
			{
				temp_S[ 0]=Syndromes[index].S[ 1] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6] ^ Din;
				temp_S[ 1]=Syndromes[index].S[ 2] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6];
				temp_S[ 2]=Syndromes[index].S[ 3] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7];
				temp_S[ 3]=Syndromes[index].S[ 4] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 6];
				temp_S[ 4]=Syndromes[index].S[ 5] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6];
				temp_S[ 5]=Syndromes[index].S[ 6] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7];
				temp_S[ 6]=Syndromes[index].S[ 7] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8];
				temp_S[ 7]=Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 9];
				temp_S[ 8]=Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10];
				temp_S[ 9]=Syndromes[index].S[10] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11];
				temp_S[10]=Syndromes[index].S[11] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[11]=Syndromes[index].S[12] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4];
				temp_S[12]=Syndromes[index].S[ 0] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5];
				break;
			}
		case(26):
			{
				temp_S[ 0]=Syndromes[index].S[ 0] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5] ^ Din;
				temp_S[ 1]=Syndromes[index].S[ 1] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5];
				temp_S[ 2]=Syndromes[index].S[ 2] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6];
				temp_S[ 3]=Syndromes[index].S[ 3] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 5];
				temp_S[ 4]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5];
				temp_S[ 5]=Syndromes[index].S[ 5] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6];
				temp_S[ 6]=Syndromes[index].S[ 6] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7];
				temp_S[ 7]=Syndromes[index].S[ 7] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8];
				temp_S[ 8]=Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 9];
				temp_S[ 9]=Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10];
				temp_S[10]=Syndromes[index].S[10] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11];
				temp_S[11]=Syndromes[index].S[11] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[12]=Syndromes[index].S[12] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4];
				break;
			}
		case(27):
			{
				temp_S[ 0]=Syndromes[index].S[12] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4] ^ Din;
				temp_S[ 1]=Syndromes[index].S[ 0] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[12] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4];
				temp_S[ 2]=Syndromes[index].S[ 1] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5];
				temp_S[ 3]=Syndromes[index].S[ 2] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 4];
				temp_S[ 4]=Syndromes[index].S[ 3] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4];
				temp_S[ 5]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5];
				temp_S[ 6]=Syndromes[index].S[ 5] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6];
				temp_S[ 7]=Syndromes[index].S[ 6] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7];
				temp_S[ 8]=Syndromes[index].S[ 7] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8];
				temp_S[ 9]=Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 9];
				temp_S[10]=Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10];
				temp_S[11]=Syndromes[index].S[10] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11];
				temp_S[12]=Syndromes[index].S[11] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				break;
			}
		case(28):
			{
				temp_S[ 0]=Syndromes[index].S[11] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12] ^ Din;
				temp_S[ 1]=Syndromes[index].S[10] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3];
				temp_S[ 2]=Syndromes[index].S[ 0] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[12] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4];
				temp_S[ 3]=Syndromes[index].S[ 1] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 3];
				temp_S[ 4]=Syndromes[index].S[ 2] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[ 5]=Syndromes[index].S[ 3] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4];
				temp_S[ 6]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5];
				temp_S[ 7]=Syndromes[index].S[ 5] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6];
				temp_S[ 8]=Syndromes[index].S[ 6] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7];
				temp_S[ 9]=Syndromes[index].S[ 7] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8];
				temp_S[10]=Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 9];
				temp_S[11]=Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10];
				temp_S[12]=Syndromes[index].S[10] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11];
				break;
			}
		case(29):
			{
				temp_S[ 0]=Syndromes[index].S[10] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11] ^ Din;
				temp_S[ 1]=Syndromes[index].S[ 9] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 2];
				temp_S[ 2]=Syndromes[index].S[10] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3];
				temp_S[ 3]=Syndromes[index].S[ 0] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 2];
				temp_S[ 4]=Syndromes[index].S[ 1] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11];
				temp_S[ 5]=Syndromes[index].S[ 2] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[ 6]=Syndromes[index].S[ 3] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4];
				temp_S[ 7]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5];
				temp_S[ 8]=Syndromes[index].S[ 5] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6];
				temp_S[ 9]=Syndromes[index].S[ 6] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7];
				temp_S[10]=Syndromes[index].S[ 7] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8];
				temp_S[11]=Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 9];
				temp_S[12]=Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10];
				break;
			}
		case(30):
			{
				temp_S[ 0]=Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10] ^ Din;
				temp_S[ 1]=Syndromes[index].S[ 8] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 1];
				temp_S[ 2]=Syndromes[index].S[ 9] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 2];
				temp_S[ 3]=Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1];
				temp_S[ 4]=Syndromes[index].S[ 0] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10];
				temp_S[ 5]=Syndromes[index].S[ 1] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11];
				temp_S[ 6]=Syndromes[index].S[ 2] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[ 7]=Syndromes[index].S[ 3] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4];
				temp_S[ 8]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5];
				temp_S[ 9]=Syndromes[index].S[ 5] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6];
				temp_S[10]=Syndromes[index].S[ 6] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7];
				temp_S[11]=Syndromes[index].S[ 7] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8];
				temp_S[12]=Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 9];
				break;
			}
		case(31):
			{
				temp_S[ 0]=Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 9] ^ Din;
				temp_S[ 1]=Syndromes[index].S[ 7] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 0];
				temp_S[ 2]=Syndromes[index].S[ 8] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 1];
				temp_S[ 3]=Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0];
				temp_S[ 4]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 9];
				temp_S[ 5]=Syndromes[index].S[ 0] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10];
				temp_S[ 6]=Syndromes[index].S[ 1] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11];
				temp_S[ 7]=Syndromes[index].S[ 2] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[ 8]=Syndromes[index].S[ 3] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4];
				temp_S[ 9]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5];
				temp_S[10]=Syndromes[index].S[ 5] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6];
				temp_S[11]=Syndromes[index].S[ 6] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7];
				temp_S[12]=Syndromes[index].S[ 7] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8];
				break;
			}
		case(32):
			{
				temp_S[ 0]=Syndromes[index].S[ 7] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8] ^ Din;
				temp_S[ 1]=Syndromes[index].S[ 6] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 1];
				temp_S[ 2]=Syndromes[index].S[ 7] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 0];
				temp_S[ 3]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[10];
				temp_S[ 4]=Syndromes[index].S[ 3] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8];
				temp_S[ 5]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 9];
				temp_S[ 6]=Syndromes[index].S[ 0] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10];
				temp_S[ 7]=Syndromes[index].S[ 1] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11];
				temp_S[ 8]=Syndromes[index].S[ 2] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[ 9]=Syndromes[index].S[ 3] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4];
				temp_S[10]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5];
				temp_S[11]=Syndromes[index].S[ 5] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6];
				temp_S[12]=Syndromes[index].S[ 6] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7];
				break;
			}
		case(33):
			{
				temp_S[ 0]=Syndromes[index].S[ 6] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7] ^ Din;
				temp_S[ 1]=Syndromes[index].S[ 5] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 0];
				temp_S[ 2]=Syndromes[index].S[ 6] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 1];
				temp_S[ 3]=Syndromes[index].S[ 3] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 9];
				temp_S[ 4]=Syndromes[index].S[ 2] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7];
				temp_S[ 5]=Syndromes[index].S[ 3] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8];
				temp_S[ 6]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 9];
				temp_S[ 7]=Syndromes[index].S[ 0] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10];
				temp_S[ 8]=Syndromes[index].S[ 1] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11];
				temp_S[ 9]=Syndromes[index].S[ 2] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[10]=Syndromes[index].S[ 3] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4];
				temp_S[11]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5];
				temp_S[12]=Syndromes[index].S[ 5] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6];
				break;
			}
		case(34):
			{
				temp_S[ 0]=Syndromes[index].S[ 5] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6] ^ Din;
				temp_S[ 1]=Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 3];
				temp_S[ 2]=Syndromes[index].S[ 5] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 0];
				temp_S[ 3]=Syndromes[index].S[ 2] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 8];
				temp_S[ 4]=Syndromes[index].S[ 1] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6];
				temp_S[ 5]=Syndromes[index].S[ 2] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7];
				temp_S[ 6]=Syndromes[index].S[ 3] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8];
				temp_S[ 7]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 9];
				temp_S[ 8]=Syndromes[index].S[ 0] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10];
				temp_S[ 9]=Syndromes[index].S[ 1] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11];
				temp_S[10]=Syndromes[index].S[ 2] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[11]=Syndromes[index].S[ 3] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4];
				temp_S[12]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5];
				break;
			}
		case(35):
			{
				temp_S[ 0]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5] ^ Din;
				temp_S[ 1]=Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[12];
				temp_S[ 2]=Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 3];
				temp_S[ 3]=Syndromes[index].S[ 1] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 7];
				temp_S[ 4]=Syndromes[index].S[ 0] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5];
				temp_S[ 5]=Syndromes[index].S[ 1] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6];
				temp_S[ 6]=Syndromes[index].S[ 2] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7];
				temp_S[ 7]=Syndromes[index].S[ 3] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8];
				temp_S[ 8]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 9];
				temp_S[ 9]=Syndromes[index].S[ 0] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10];
				temp_S[10]=Syndromes[index].S[ 1] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11];
				temp_S[11]=Syndromes[index].S[ 2] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[12]=Syndromes[index].S[ 3] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4];
				break;
			}
		case(36):
			{
				temp_S[ 0]=Syndromes[index].S[ 3] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4] ^ Din;
				temp_S[ 1]=Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[11];
				temp_S[ 2]=Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[12];
				temp_S[ 3]=Syndromes[index].S[ 0] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 6];
				temp_S[ 4]=Syndromes[index].S[ 0] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4];
				temp_S[ 5]=Syndromes[index].S[ 0] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5];
				temp_S[ 6]=Syndromes[index].S[ 1] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6];
				temp_S[ 7]=Syndromes[index].S[ 2] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7];
				temp_S[ 8]=Syndromes[index].S[ 3] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8];
				temp_S[ 9]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 9];
				temp_S[10]=Syndromes[index].S[ 0] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10];
				temp_S[11]=Syndromes[index].S[ 1] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11];
				temp_S[12]=Syndromes[index].S[ 2] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				break;
			}
		case(37):
			{
				temp_S[ 0]=Syndromes[index].S[ 2] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12] ^ Din;
				temp_S[ 1]=Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[10];
				temp_S[ 2]=Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[11];
				temp_S[ 3]=Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 5];
				temp_S[ 4]=Syndromes[index].S[ 1] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[ 5]=Syndromes[index].S[ 0] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4];
				temp_S[ 6]=Syndromes[index].S[ 0] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5];
				temp_S[ 7]=Syndromes[index].S[ 1] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6];
				temp_S[ 8]=Syndromes[index].S[ 2] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7];
				temp_S[ 9]=Syndromes[index].S[ 3] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8];
				temp_S[10]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 9];
				temp_S[11]=Syndromes[index].S[ 0] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10];
				temp_S[12]=Syndromes[index].S[ 1] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11];
				break;
			}
		case(38):
			{
				temp_S[ 0]=Syndromes[index].S[ 1] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11] ^ Din;
				temp_S[ 1]=Syndromes[index].S[ 0] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 9];
				temp_S[ 2]=Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[10];
				temp_S[ 3]=Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12];
				temp_S[ 4]=Syndromes[index].S[ 0] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11];
				temp_S[ 5]=Syndromes[index].S[ 1] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[ 6]=Syndromes[index].S[ 0] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4];
				temp_S[ 7]=Syndromes[index].S[ 0] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5];
				temp_S[ 8]=Syndromes[index].S[ 1] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6];
				temp_S[ 9]=Syndromes[index].S[ 2] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7];
				temp_S[10]=Syndromes[index].S[ 3] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8];
				temp_S[11]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 9];
				temp_S[12]=Syndromes[index].S[ 0] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10];
				break;
			}
		case(39):
			{
				temp_S[ 0]=Syndromes[index].S[ 0] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10] ^ Din;
				temp_S[ 1]=Syndromes[index].S[ 7] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 8];
				temp_S[ 2]=Syndromes[index].S[ 0] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 9];
				temp_S[ 3]=Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[12];
				temp_S[ 4]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10];
				temp_S[ 5]=Syndromes[index].S[ 0] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11];
				temp_S[ 6]=Syndromes[index].S[ 1] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[ 7]=Syndromes[index].S[ 0] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4];
				temp_S[ 8]=Syndromes[index].S[ 0] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5];
				temp_S[ 9]=Syndromes[index].S[ 1] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6];
				temp_S[10]=Syndromes[index].S[ 2] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7];
				temp_S[11]=Syndromes[index].S[ 3] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8];
				temp_S[12]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 9];
				break;
			}
		case(40):
			{
				temp_S[ 0]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 9] ^ Din;
				temp_S[ 1]=Syndromes[index].S[ 6] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 7];
				temp_S[ 2]=Syndromes[index].S[ 7] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 8];
				temp_S[ 3]=Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[11];
				temp_S[ 4]=Syndromes[index].S[ 3] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 9];
				temp_S[ 5]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10];
				temp_S[ 6]=Syndromes[index].S[ 0] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11];
				temp_S[ 7]=Syndromes[index].S[ 1] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[ 8]=Syndromes[index].S[ 0] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4];
				temp_S[ 9]=Syndromes[index].S[ 0] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5];
				temp_S[10]=Syndromes[index].S[ 1] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6];
				temp_S[11]=Syndromes[index].S[ 2] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7];
				temp_S[12]=Syndromes[index].S[ 3] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8];
				break;
			}
		case(41):
			{
				temp_S[ 0]=Syndromes[index].S[ 3] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 8] ^ Din;
				temp_S[ 1]=Syndromes[index].S[ 5] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 6];
				temp_S[ 2]=Syndromes[index].S[ 6] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 7];
				temp_S[ 3]=Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[11] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[10];
				temp_S[ 4]=Syndromes[index].S[ 2] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 8];
				temp_S[ 5]=Syndromes[index].S[ 3] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 9];
				temp_S[ 6]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10];
				temp_S[ 7]=Syndromes[index].S[ 0] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11];
				temp_S[ 8]=Syndromes[index].S[ 1] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[ 9]=Syndromes[index].S[ 0] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4];
				temp_S[10]=Syndromes[index].S[ 0] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5];
				temp_S[11]=Syndromes[index].S[ 1] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6];
				temp_S[12]=Syndromes[index].S[ 2] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7];
				break;
			}
		case(42):
			{
				temp_S[ 0]=Syndromes[index].S[ 2] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 7] ^ Din;
				temp_S[ 1]=Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 5];
				temp_S[ 2]=Syndromes[index].S[ 5] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 6];
				temp_S[ 3]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[10] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 9];
				temp_S[ 4]=Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 7];
				temp_S[ 5]=Syndromes[index].S[ 2] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 8];
				temp_S[ 6]=Syndromes[index].S[ 3] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 9];
				temp_S[ 7]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10];
				temp_S[ 8]=Syndromes[index].S[ 0] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11];
				temp_S[ 9]=Syndromes[index].S[ 1] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[10]=Syndromes[index].S[ 0] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4];
				temp_S[11]=Syndromes[index].S[ 0] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5];
				temp_S[12]=Syndromes[index].S[ 1] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6];
				break;
			}
		case(43):
			{
				temp_S[ 0]=Syndromes[index].S[ 1] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 6] ^ Din;
				temp_S[ 1]=Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 4];
				temp_S[ 2]=Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 5];
				temp_S[ 3]=Syndromes[index].S[ 3] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 8];
				temp_S[ 4]=Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 6];
				temp_S[ 5]=Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 7];
				temp_S[ 6]=Syndromes[index].S[ 2] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 8];
				temp_S[ 7]=Syndromes[index].S[ 3] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 9];
				temp_S[ 8]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10];
				temp_S[ 9]=Syndromes[index].S[ 0] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11];
				temp_S[10]=Syndromes[index].S[ 1] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[11]=Syndromes[index].S[ 0] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4];
				temp_S[12]=Syndromes[index].S[ 0] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5];
				break;
			}
		case(44):
			{
				temp_S[ 0]=Syndromes[index].S[ 0] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 5] ^ Din;
				temp_S[ 1]=Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 3];
				temp_S[ 2]=Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 4];
				temp_S[ 3]=Syndromes[index].S[12] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 7];
				temp_S[ 4]=Syndromes[index].S[10] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 5];
				temp_S[ 5]=Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 6];
				temp_S[ 6]=Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 7];
				temp_S[ 7]=Syndromes[index].S[ 2] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 8];
				temp_S[ 8]=Syndromes[index].S[ 3] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 9];
				temp_S[ 9]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10];
				temp_S[10]=Syndromes[index].S[ 0] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11];
				temp_S[11]=Syndromes[index].S[ 1] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[12]=Syndromes[index].S[ 0] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4];
				break;
			}
		case(45):
			{
				temp_S[ 0]=Syndromes[index].S[ 0] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 4] ^ Din;
				temp_S[ 1]=Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 2];
				temp_S[ 2]=Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 3];
				temp_S[ 3]=Syndromes[index].S[11] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 6];
				temp_S[ 4]=Syndromes[index].S[12] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 4];
				temp_S[ 5]=Syndromes[index].S[10] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 5];
				temp_S[ 6]=Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 6];
				temp_S[ 7]=Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 7];
				temp_S[ 8]=Syndromes[index].S[ 2] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 8];
				temp_S[ 9]=Syndromes[index].S[ 3] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 9];
				temp_S[10]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10];
				temp_S[11]=Syndromes[index].S[ 0] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11];
				temp_S[12]=Syndromes[index].S[ 1] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				break;
			}
		case(46):
			{
				temp_S[ 0]=Syndromes[index].S[ 1] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12] ^ Din;
				temp_S[ 1]=Syndromes[index].S[ 0] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[12];
				temp_S[ 2]=Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 2];
				temp_S[ 3]=Syndromes[index].S[10] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[12];
				temp_S[ 4]=Syndromes[index].S[11] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[ 5]=Syndromes[index].S[12] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 4];
				temp_S[ 6]=Syndromes[index].S[10] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 5];
				temp_S[ 7]=Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 6];
				temp_S[ 8]=Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 7];
				temp_S[ 9]=Syndromes[index].S[ 2] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 8];
				temp_S[10]=Syndromes[index].S[ 3] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 9];
				temp_S[11]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10];
				temp_S[12]=Syndromes[index].S[ 0] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11];
				break;
			}
		case(47):
			{
				temp_S[ 0]=Syndromes[index].S[ 0] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11] ^ Din;
				temp_S[ 1]=Syndromes[index].S[ 7] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[11];
				temp_S[ 2]=Syndromes[index].S[ 0] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[12];
				temp_S[ 3]=Syndromes[index].S[ 9] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[12] ^ Syndromes[index].S[11];
				temp_S[ 4]=Syndromes[index].S[10] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11];
				temp_S[ 5]=Syndromes[index].S[11] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[ 6]=Syndromes[index].S[12] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 4];
				temp_S[ 7]=Syndromes[index].S[10] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 5];
				temp_S[ 8]=Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 6];
				temp_S[ 9]=Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 7];
				temp_S[10]=Syndromes[index].S[ 2] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 8];
				temp_S[11]=Syndromes[index].S[ 3] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 9];
				temp_S[12]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10];
				break;
			}
		case(48):
			{
				temp_S[ 0]=Syndromes[index].S[ 4] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10] ^ Din;
				temp_S[ 1]=Syndromes[index].S[ 6] ^ Syndromes[index].S[12] ^ Syndromes[index].S[10];
				temp_S[ 2]=Syndromes[index].S[ 7] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[11];
				temp_S[ 3]=Syndromes[index].S[ 8] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[11] ^ Syndromes[index].S[10];
				temp_S[ 4]=Syndromes[index].S[ 9] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[12] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[10];
				temp_S[ 5]=Syndromes[index].S[10] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[11];
				temp_S[ 6]=Syndromes[index].S[11] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[12];
				temp_S[ 7]=Syndromes[index].S[12] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 5] ^ Syndromes[index].S[ 4];
				temp_S[ 8]=Syndromes[index].S[10] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 6] ^ Syndromes[index].S[ 5];
				temp_S[ 9]=Syndromes[index].S[11] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 7] ^ Syndromes[index].S[ 6];
				temp_S[10]=Syndromes[index].S[12] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 8] ^ Syndromes[index].S[ 7];
				temp_S[11]=Syndromes[index].S[ 2] ^ Syndromes[index].S[ 3] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 9] ^ Syndromes[index].S[ 8];
				temp_S[12]=Syndromes[index].S[ 3] ^ Syndromes[index].S[ 4] ^ Syndromes[index].S[ 1] ^ Syndromes[index].S[ 2] ^ Syndromes[index].S[10] ^ Syndromes[index].S[ 0] ^ Syndromes[index].S[ 9];
				break;
			}
		} // Switch case
		for (bit_index=0;bit_index<13;bit_index++)
		{
			Syndromes[index].S[bit_index]=temp_S[bit_index]; // Update the new value by the calculated one
		}
	} // for loop ends here
} //syndrome_calc ends here


void gen_poly(void)
/*
* Compute the generator polynomial	of a binary	BCH	code. Fist generate	the
* cycle sets modulo 2**m -	1, cycle[][] =	(i,	2*i, 4*i, ..., 2^l*i). Then
* determine those cycle sets that contain integers	in the set of (d-1)
* consecutive integers	{1..(d-1)}.	The	generator polynomial is	calculated
* as the product of linear	factors	of the form	(x+alpha^i), for every i in
* the above cycle sets.
*/
{
	register int	ii,	jj,	ll,	kaux;
	register int	test, aux, nocycles, root, noterms,	rdncy;
	int				cycle[1024][21], size[1024], min[1024],	zeros[1024];

	/* Generate	cycle sets modulo n, n = 2**m -	1 */
	cycle[0][0]	= 0;
	size[0]	= 1;
	cycle[1][0]	= 1;
	size[1]	= 1;
	jj = 1;			/* cycle set index */
	if (m >	9)	{
		printf("Computing cycle	sets modulo	%d\n", n);
		printf("(This may take some	time)...\n");
	}
	do {
		/* Generate	the	jj-th cycle	set	*/
		ii = 0;
		do {
			ii++;
			cycle[jj][ii] =	(cycle[jj][ii -	1] * 2)	% n;
			size[jj]++;
			aux	= (cycle[jj][ii] * 2) %	n;
		} while	(aux !=	cycle[jj][0]);
		/* Next	cycle set representative */
		ll = 0;
		do {
			ll++;
			test = 0;
			for	(ii	= 1; ((ii <= jj) &&	(!test)); ii++)	
				/* Examine previous	cycle sets */
				for (kaux	= 0; ((kaux	< size[ii])	&& (!test)); kaux++)
					if	(ll	== cycle[ii][kaux])
						test = 1;
		} while	((test)	&& (ll < (n	- 1)));
		if (!(test)) {
			jj++;	/* next	cycle set index	*/
			cycle[jj][0] = ll;
			size[jj] = 1;
		}
	} while	(ll	< (n - 1));
	nocycles = jj;		/* number of cycle sets	modulo n */

	printf("Enter the error	correcting capability, t: ");
	if (use_random_data	== 1)
	{
		t =	rand_t;
		printf("%d\n",t);
	}
	else
	{
		scanf("%d",	&t);
	}

	d =	2 *	t +	1;

	/* Search for roots	1, 2, ..., d-1 in cycle	sets */
	kaux = 0;
	rdncy =	0;
	for	(ii	= 1; ii	<= nocycles; ii++) {
		min[kaux] =	0;
		test = 0;
		for	(jj	= 0; ((jj <	size[ii]) && (!test)); jj++)
			for	(root =	1; ((root <	d) && (!test));	root++)
				if (root ==	cycle[ii][jj])	{
					test = 1;
					min[kaux] =	ii;
				}
				if (min[kaux]) {
					rdncy += size[min[kaux]];
					kaux++;
				}
	}
	noterms	= kaux;
	kaux = 1;
	for	(ii	= 0; ii	< noterms; ii++)
		for	(jj	= 0; jj	< size[min[ii]]; jj++) {
			zeros[kaux]	= cycle[min[ii]][jj];
			kaux++;
		}

		k =	length - rdncy;

		if (k<0)
		{
			printf("Parameters	invalid!\n");
			exit(0);
		}

		printf("This is	a (%d, %d, %d) binary BCH code\n", length, k, d);
		printf("The block length is %d\n",length);
		printf("The data length is %d\n",k);
		printf("The designed minimum distance is %d\n",d);

		/* Compute the generator polynomial	*/
		g[0] = alpha_to[zeros[1]];
		g[1] = 1;		/* g(x)	= (X + zeros[1]) initially */
		for	(ii	= 2; ii	<= rdncy; ii++)	{
			g[ii]	= 1;
			for (jj =	ii - 1;	jj > 0;	jj--)
				if (g[jj] != 0)
					g[jj]	= g[jj - 1]	^ alpha_to[(index_of[g[jj]]	+ zeros[ii]) % n];
				else
					g[jj]	= g[jj - 1];
			g[0] = alpha_to[(index_of[g[0]] +	zeros[ii]) % n];
		}
		printf("Generator polynomial:\ng(x)	= ");
		for	(ii	= 0; ii	<= rdncy; ii++)	{
			printf("%d", g[ii]);
			if (ii &&	((ii % 50) == 0))
				printf("\n");
		}
		printf("\n");
}


void calculate_hamming_checksum(int D[8], int Byte_index)
/* This function calculates the checksum of a Hamming algorithm */
{
	int tmp_checksum[14];
	int xor_all_bits;
	int i;
	xor_all_bits = D[7] ^ D[6] ^ D[5] ^ D[4] ^ D[3] ^ D[2] ^ D[1] ^ D[0];
	tmp_checksum[0] = checksum[0] ^ xor_all_bits;
	tmp_checksum[1] = checksum[1] ^ D[7] ^ D[5] ^ D[3] ^ D[1];
	tmp_checksum[2] = checksum[2] ^ D[7] ^ D[6] ^ D[3] ^ D[2];
	tmp_checksum[3] = checksum[3] ^ D[7] ^ D[6] ^ D[5] ^ D[4];

	if ((Byte_index & 1) == 1)
	{
		tmp_checksum[4] = checksum[4] ^ xor_all_bits;
	}
	else
	{
		tmp_checksum[4] = checksum[4];
	}

	if ((Byte_index & 2) == 2)
	{
		tmp_checksum[5] = checksum[5] ^ xor_all_bits;
	}
	else
	{
		tmp_checksum[5] = checksum[5];
	}

	if ((Byte_index & 4) == 4)
	{
		tmp_checksum[6] = checksum[6] ^ xor_all_bits;
	}
	else
	{
		tmp_checksum[6] = checksum[6];
	}

	if ((Byte_index & 8) == 8)
	{
		tmp_checksum[7] = checksum[7] ^ xor_all_bits;
	}
	else
	{
		tmp_checksum[7] = checksum[7];
	}

	if ((Byte_index & 16) == 16)
	{
		tmp_checksum[8] = checksum[8] ^ xor_all_bits;
	}
	else
	{
		tmp_checksum[8] = checksum[8];
	}

	if ((Byte_index & 32) == 32)
	{
		tmp_checksum[9] = checksum[9] ^ xor_all_bits;
	}
	else
	{
		tmp_checksum[9] = checksum[9];
	}

	if ((Byte_index & 64) == 64)
	{
		tmp_checksum[10] = checksum[10] ^ xor_all_bits;
	}
	else
	{
		tmp_checksum[10] = checksum[10];
	}

	if ((Byte_index & 128) == 128)
	{
		tmp_checksum[11] = checksum[11] ^ xor_all_bits;
	}
	else
	{
		tmp_checksum[11] = checksum[11];
	}

	if ((Byte_index & 256) == 256)
	{
		tmp_checksum[12] = checksum[12] ^ xor_all_bits;
	}
	else
	{
		tmp_checksum[12] = checksum[12];
	}
	
	if ((Byte_index & 512) == 512)
	{
		tmp_checksum[13] = checksum[13] ^ xor_all_bits;
	}
	else
	{
		tmp_checksum[13] = checksum[13];
	}
	/* Copy the newly calculated values into the checksum array */
	for (i=0; i<14; i++)
	{
		checksum[i] = tmp_checksum[i];
		if ((checksum[i] != 0) && (checksum[i] != 1))
		{
			printf("Hamming chaecksum calculation Error condition:\n");
			printf("Byte_index=%d, Checksum[%d]=%d, xor_all_bits = %d, Data=%d%d%d%d%d%d%d%d\n",
				Byte_index, i, checksum[i], xor_all_bits, D[7], D[6], D[5], D[4], D[3], D[2], D[1], D[0]);
			getchar();
		}
	}
}

// This function is used to identify the bytes in the files
// 0xD - Data
// 0x0 - n1_mask
// 0x1 - n1
// 0xC - checksum
// 0x2 - n2
int byte_type(int byte_index)
{
	if (byte_index < 512)
	{
		return(0xd);
	}
	else
	{
		if (byte_index < (512 + n1_mask))
		{
			return(0);
		}
		else
		{
			if (byte_index < (512 + n1))
			{
				return(1);
			}
			else
			{
				if (byte_index < (512 + n1 + ecc_checksum_size))
				{
					return(0xC);
				}
				else
				{
					return(2);
				}
			}
		}
	}
}

void encode_hamming(void)
/* This function calculates the checksum of a vector of data for single error correction and 2 errors detection */
{
	int encode_bit_index;
	int	encode_byte_index;
	int D[8]; /* this variable is used to store a byte as an array of 8 bits */
	int i; // yet another index parameter

	// Initialize the checksum array to 0
	for(i=0; i<HAMMING_REDUNDANCY_SIZE; i++)
		checksum[i] = 0;

	for(encode_byte_index=0; encode_byte_index < k - 1;encode_byte_index+=8)
		{
			//		printf("Byte %3d =",(decode_byte_index + 7) / 8);
			// Prepare the next byte
			for(encode_bit_index=0;encode_bit_index<8;encode_bit_index++)
			{
				// If the data is not in the masked area then use it to calculate the checksum
				if((encode_byte_index < 4096) | (encode_byte_index > (4095 + (8*n1_mask))))
				{
                    D[encode_bit_index]=data[k - (encode_bit_index+encode_byte_index) - 1];
				}
				else
				{
					D[encode_bit_index]= 0;
				}

				//			printf("%d",D[encode_bit_index]);
			}
			// Calculate the next byte checksum
			//		printf("\n");
			calculate_hamming_checksum(D, (int) (encode_byte_index/8));
		}
		// Once the HW calculation is done, copy the checksum into the bb[] array
		for (i=0;i<HAMMING_REDUNDANCY_SIZE;i++)
		{
			bb[HAMMING_REDUNDANCY_SIZE - i - 1] = checksum[i]; // Since the vector order is inverted, the LSB is on the highest index
		}
		printf("Calculated Encoding Hamming checksum is:");
		for (i=0; i<HAMMING_REDUNDANCY_SIZE; i++)
		{
			printf("%d",bb[i]); // Since the bits are flipped, this will print MSB --> LSB
		}
		printf("\n");
}

void decode_hamming(void)
/* This function calculates the error location (if realy exists) in Hamming algorithm */
{
	int decode_bit_index;
	int	decode_byte_index;
	int D[8]; /* this variable is used to store a byte as an array of 8 bits */
	int error_found = 0; /* A flag used to indicate when an error is found */
	int non_correctable_errors = 0; /* A flag used to indicate when non correctable errors are found */
	int error_location = 0;
	int i; // yet another index parameter

	char FILE_NAME[100];
	FILE *TEST_DECODE_FILE; // A File pointer for the test decoding sequence information
	FILE *ERRORS_LOC_FILE; // A file pointer for the reported error location

	// Initialize the checksum array to 0
	for(i=0; i<HAMMING_REDUNDANCY_SIZE; i++)
		checksum[i] = 0;

	if (generate_test_files == 1) // If needed create the test vector reference files
	{
		//sprintf(FILE_NAME,"BCH_test_%d_%d_%d_%d_ECC_CALC.txt",t,numerr,n1,n1_mask);
		sprintf(FILE_NAME,"%s_ECC_CALC.txt",test_name);
		TEST_DECODE_FILE = fopen(FILE_NAME,"w");
		sprintf(FILE_NAME,"%s_Errors_loc.txt",test_name);
		ERRORS_LOC_FILE = fopen(FILE_NAME,"w");
	}

	for(decode_byte_index=0; decode_byte_index < k - 1;decode_byte_index+=8)
	{
		//		printf("Byte %3d =",(decode_byte_index + 7) / 8);
		// Prepare the next byte
		for(decode_bit_index=0;decode_bit_index<8;decode_bit_index++)
		{
			// If the data is not in the masked area then use it to calculate the checksum
			if((decode_byte_index < 4096) | (decode_byte_index > (4095 + (8*n1_mask))))
			{
				D[decode_bit_index]=recd[length - (decode_bit_index+decode_byte_index) - 1];
			}
			else
			{
				D[decode_bit_index]= 0;
			}

			//			printf("%d",D[decode_bit_index]);
		}
		// Calculate the next byte checksum
		//		printf("\n");
		calculate_hamming_checksum(D, (int) (decode_byte_index/8));
	}

	/* first form the syndromes	*/
	if (generate_test_files == 1) // If needed close the test vector reference files
	{
		// Initially print the calculated checksum
		fprintf(TEST_DECODE_FILE,"Calculated checksum value [%d : 0] =\n",HAMMING_REDUNDANCY_SIZE-1);
		printf("Calculated Hamming Decode checksum is  :");
		for(i=0;i<HAMMING_REDUNDANCY_SIZE;i++)
		{ // Make sure that the report is done MAB --> LSB
			fprintf(TEST_DECODE_FILE,"%d",checksum[HAMMING_REDUNDANCY_SIZE - i - 1]);
			printf("%d",checksum[HAMMING_REDUNDANCY_SIZE - i - 1]);
		}
		fprintf(TEST_DECODE_FILE,"\n\n");
		printf("\n");
	}
	// Once the HW calculation is done, copy the checksum into the bb[] array
	        printf("Read back Hamming checksum is          :");
	for (i=0;i<HAMMING_REDUNDANCY_SIZE;i++)
	{
		printf("%d",recd[i]); // Print MSB (low index) to LSB (high index)
		checksum[i] = checksum[i] ^ recd[HAMMING_REDUNDANCY_SIZE - i - 1]; // Operate on LSB --> MSB
		if ((checksum[i] != 0 ) && (i > 0)) // The LSB is not an indication for all errors
		{
			/* Here we detect that any of the upper bits is set */
			error_location = error_location + (1 << (i-1)); // Set the error location to match the upper 13 bits value
			error_found = 1;
		}
	}
	
	printf("\n");
	if (generate_test_files == 1) // If needed close the test vector reference files
	{
		if (error_found != 0)
		{
			fprintf(TEST_DECODE_FILE,"Error location is %d\n",error_location);
		}
		else
		{
			fprintf(TEST_DECODE_FILE,"No error was found\n");
		}
	}
	if (error_found != 0)
	{
		if (checksum[0] == 0 )
		{
			printf("Non correctable errors detected\n");
			if (generate_test_files == 1) // If needed close the test vector reference files
			{
				fprintf(TEST_DECODE_FILE,"Non correctable errors detected\n");
			}
			non_correctable_errors = 1;
			numerr = 2;
		}
		else
		{
			// The following line was added to assure that if there are 2 errors but one is masked, that the data will 
			// be compared at the end of the program since we have a single error in practice as one is masked out
			numerr = 1;

			printf("Single error detected at bit location %d (=position %d) = Byte %d bit %d\n",error_location,length-error_location-1, (int) (error_location / 8), error_location % 8);
			if (generate_test_files == 1) // If needed close the test vector reference files
			{
				fprintf(TEST_DECODE_FILE,"Single error detected at bit location %d (=position %d) = Byte %d bit %d\n",error_location, length-error_location-1, (int) (error_location / 8), error_location % 8);
				fprintf(ERRORS_LOC_FILE,"%04X\n",(error_location<<1)+1);
			}
			printf("Correcting error\n");
			recd[ length - error_location - 1] ^=1; /* Correct the failing bit */
		}
	}
	else
	{
		printf("No error was found\n");
	}

	fclose(TEST_DECODE_FILE);
	fclose(ERRORS_LOC_FILE);
}

void encode_bch(void)
/*
* Compute redundacy bb[], the coefficients	of b(x). The redundancy
* polynomial b(x) is the remainder	after dividing x^(length-k)*data(x)
* by the generator	polynomial g(x).
*/
{
	register int	i, j;
	register int	feedback;
	int				encode_bit_index;
	int				encode_byte_index;
	int				D[8]; // A byte wide buffer
	if (emulate_HW_only == 0)
	{
		for	(i = 0;	i <	length - k;	i++)
			bb[i] =	0;
		for	(i = k - 1;	i >= 0;	i--) {
			feedback = data[i] ^ bb[length - k - 1];
			if (feedback !=	0) {
				for	(j = length	- k	- 1; j > 0;	j--)
					if (g[j] !=	0)
						bb[j] =	bb[j - 1] ^	feedback;
					else
						bb[j] =	bb[j - 1];
				bb[0] =	g[0] &&	feedback;
			} else {
				for	(j = length	- k	- 1; j > 0;	j--)
					bb[j] =	bb[j - 1];
				bb[0] =	0;
			}
		}
	} // if (emulate_HW_only == 0) ends here
	else
	{
		// This section is used to verify that the checksum calculation hardware 
		// is indeed equal to the single-bit LFSR implementation
		if (log_for_debug == 1)
		{
			printf("Initialize checksum buffer\n");
		}
		for(encode_bit_index=0;encode_bit_index<400;encode_bit_index++)
			checksum[encode_bit_index]=0;
		// Calculate the alignment buffer size
		for(encode_byte_index=0; encode_byte_index < k - 1;encode_byte_index+=8)
		{
			//		printf("Byte %3d =",(decode_byte_index + 7) / 8);
			// Prepare the next byte
			for(encode_bit_index=0;encode_bit_index<8;encode_bit_index++)
			{
				// If the data is not in the masked area then use it to calculate the checksum
				if(((encode_byte_index + encode_bit_index) < 4096) | ((encode_byte_index + encode_bit_index) > (4095 + (8*n1_mask))))
				{
                    D[encode_bit_index]=data[k - (encode_bit_index+encode_byte_index) - 1];
				}
				else
				{
					D[encode_bit_index]= 0;
						//printf("Index %d is ignored in encoding\n",encode_byte_index + encode_bit_index);
				}

				//			printf("%d",D[encode_bit_index]);
			}
			// Calculate the next byte checksum
			//		printf("\n");
			calculate_checksum(D,ENCODE_MODE);
		}
		// Once the HW calculation is done, copy the checksum into the bb[] array
		for (i=0;i<m*t;i++)
			bb[i] = checksum[i];
	} // if (emulate_HW_only == 1) ends here
}
char* remove_repeated_text(char source_string[300], char string_to_remove[20])
/* This function removes an element from a string. If the element does not exist it does not modify the original string
to remove, it removes only the 1st one */

{
	int index_of_element = 0;
	int element_size = 0;
	int flag = 0;
	char operation_string[300] = "";
	char search_string[50];

	// Check if this is a middle element and if so, concatenate before and after
	sprintf(search_string," ^ %s ^ ",string_to_remove);
	if (NULL != strstr(source_string,search_string))
	{
		//	printf("Found a middle element %s in %s\n",string_to_remove, source_string);
		flag = 1;
		index_of_element = (int) (strstr(source_string,search_string) - source_string);
		element_size = strlen(search_string);
		// Take the first part
		strncpy(operation_string,source_string,index_of_element);
		// Append the 2nd part skipping the element location
		strncpy(operation_string+index_of_element,source_string + ((int) index_of_element + element_size - 3), strlen(source_string) - index_of_element - element_size + 3);
	}
	else
	{
		// Check if it is the last element and then take before and add null at the end
		sprintf(search_string," ^ %s",string_to_remove);
		if (NULL != strstr(source_string,search_string)) // If not the last element
		{
			//			printf("Found a last element %s in %s\n",string_to_remove, source_string);
			flag = 1;
			index_of_element = (int) (strstr(source_string,search_string) - source_string);
			element_size = strlen(search_string);
			// Take the first part
			strncpy(operation_string,source_string,index_of_element);
			// Append the null at the end
			strncpy(operation_string + index_of_element,"\0",1);
		}
		else
		{
			// Check if it is the first element and then take from this element till the end
			sprintf(search_string,"%s ^ ",string_to_remove);
			if (NULL != strstr(source_string,search_string)) // This is not the first element
			{
				//			printf("Found a first element %s in %s\n",string_to_remove, source_string);
				flag = 1;
				index_of_element = (int) (strstr(source_string,search_string) - source_string);
				element_size = strlen(search_string);
				// Append the 2nd part skipping the element location
				strncpy(operation_string,source_string + ((int) index_of_element + element_size), strlen(source_string) - index_of_element - element_size);
			}
		}
	}
	if (flag == 1)
	{
		return(operation_string);
	}
	else
	{
		return(source_string);
	}
}





void encode_bch_calculate_per_byte(void)
/*
* Compute redundacy bb[], the coefficients	of b(x). The redundancy
* polynomial b(x) is the remainder	after dividing x^(length-k)*data(x)
* by the generator	polynomial g(x).
*/
{
	int		i, j;
	int		elements_count;
	char	feedback_string[300];
	char	net_feedback_string[300]; // This is the feedback after removing elements which appear in the cell
	char	net_cell_string[300];  // This is the cell string after removing elements which appear in the feedback
	char	bb_string[400][300];
	char	data_string[10];
	//char	*string_pointer;
	char	temp_string[400];
	char	element_string[20];

	for	(i = 0;	i <	length - k;	i++)
		sprintf(bb_string[i],"G[%3d]",i); // Prepare the string with the relevant content
	for (i = 0; i<=7; i++)
	{
		printf("Iteration %d\n",i);
		sprintf(data_string,"D[%2d]",i); // Prepare the input data string
		sprintf(feedback_string,"%s ^ %s",data_string,bb_string[length - k - 1]);
		printf("Feedbvack_string = %s\n",feedback_string);
		// strcpy(feedback_string = data[i] ^ bb[length - k - 1];

		for	(j = length	- k	- 1; j >= 0;	j--)
		{
			if (g[j] !=	0)
			{
				//bb[j] =	bb[j - 1] ^	feedback;
				if (j>0)
				{
					// At this stage we need to add a XOR with another element.
					// Before we can add the XOR, we need to see if one of the elements is common for both sides
					// If there is such an element we need to remove it from both sides using remove_repeated_text()

					// At first stage we copy the source string into a working string for both sides
					strcpy(net_feedback_string,feedback_string);
					strcpy(net_cell_string,bb_string[j - 1]);
					// At this stage for each element in the feedback search for a common element in the cell
					// If found remove from both
					elements_count = 0;
					if (NULL != strstr(net_cell_string," ^ ")) // If there are multiple elements in the current cell
					{
						elements_count +=1;
						strcpy(temp_string, net_cell_string); // Store the elements into a temporal buffer
						// Scan as long as there are multiple elements in the feedback string
						while(NULL != strstr(temp_string," ^ "))
						{
							elements_count +=1;                // Increment the elements count
							strncpy(element_string,temp_string,(int) (strstr(temp_string," ^ ") - temp_string));
							// Append end-of string to assure that we have properly terminated string
							strncpy(element_string + ((int) (strstr(temp_string," ^ ") - temp_string)),"\0",1); 
							//	printf("Found element: %s\n",element_string);
							// If the element is a common element, then remove it from both sides
							if (NULL != strstr(net_feedback_string,element_string))
							{
								// Remove the element from the cell string
								strcpy(net_cell_string,remove_repeated_text(net_cell_string,element_string));
								// Remove the element from the feedback string
								strcpy(net_feedback_string,remove_repeated_text(net_feedback_string,element_string));

							}
							// This is supposed to move the pointer to start after the current seperator
							strcpy(temp_string, strstr(temp_string," ^ ") + strlen(" ^ ")); 
						}
						//	printf("Found element: %s\n",temp_string);
						strcpy(element_string,temp_string); // We need to move the element into element_string for next lines
						// If the last element is a common element, then remove it from both sides
						if (NULL != strstr(net_feedback_string,element_string))
						{
							// Remove the element from the cell string
							strcpy(net_cell_string,remove_repeated_text(net_cell_string,element_string));
							// Remove the element from the feedback string
							strcpy(net_feedback_string,remove_repeated_text(net_feedback_string,element_string));

						}
					}
					//printf("It has %d elements\n",elements_count);

					sprintf(bb_string[j],"%s ^ %s",net_cell_string, net_feedback_string);
				}
				else
				{
					strcpy(bb_string[j],feedback_string);
				}
			}

			else
			{
				if (j>0)
				{
					//bb[j] =	bb[j - 1];
					strcpy(bb_string[j],bb_string[j - 1]);
				}
				else
				{
					strcpy(bb_string[j], data_string);
				}
			}
			printf("G[%3d]=%s\n",j,bb_string[j]);
		}
	}

	for	(j = 0; j<=length	- k	- 1;j++)
	{
		printf("g[%3d]=%s\n",j,bb_string[j]);
	}
}

void calc_galua_syndrome_registers(void)
/* This function calculates the polynomials of the syndrome calculations and ELP roots calculations.
For t errors we will need to have 2*t syndromes and t ELP machines 
the clculation is based on creating a shift register in which the MSB rotates into the LSB and to all the other
stages in the shift register in which the generating polynomial p[i] equals to 1.
the Sj function equals to j rotations of the initial value with the input data "xored" with the feedback for the LSB*/
{
	int bit_index;
	//	int index;
	int power_index;
	//	int		i;
	//	int		j;
	int		elements_count;
	char	feedback_string[300];
	char	net_feedback_string[300]; // This is the feedback after removing elements which appear in the cell
	char	net_cell_string[300];  // This is the cell string after removing elements which appear in the feedback
	char	bb_string[20][300];
	//	char	data_string[10];
	//	char	*string_pointer;
	char	temp_string[400];
	char	element_string[20];


	// Initialize the array
	printf("S0:\n");
	for(bit_index=0;bit_index<=(m-1);bit_index++)
	{
		sprintf(bb_string[bit_index],"S[%2d]",bit_index);
		if (bit_index > 0)
		{
			printf("S[%2d]=%s\n",bit_index,bb_string[bit_index]);
		}
		else
		{
			printf("S[%2d]=Din ^ %s\n",bit_index,bb_string[bit_index]);
		}
	}

	for(power_index=1;power_index<=(2*t);power_index++)
	{
		// Prepare the feedback string
		strcpy(feedback_string,bb_string[m-1]);
		for(bit_index=(m-1);bit_index>=0;bit_index--)
		{
			if((p[bit_index]==1) && (bit_index > 0))
			{
				// At this stage we need to add a XOR with another element.
				// Before we can add the XOR, we need to see if one of the elements is common for both sides
				// If there is such an element we need to remove it from both sides using remove_repeated_text()

				// At first stage we copy the source string into a working string for both sides
				strcpy(net_feedback_string,feedback_string);
				strcpy(net_cell_string,bb_string[bit_index - 1]);
				// At this stage for each element in the feedback search for a common element in the cell
				// If found remove from both
				elements_count = 0;
				if (NULL != strstr(net_cell_string," ^ ")) // If there are multiple elements in the current cell
				{
					elements_count +=1;
					strcpy(temp_string, net_cell_string); // Store the elements into a temporal buffer
					// Scan as long as there are multiple elements in the feedback string
					while(NULL != strstr(temp_string," ^ "))
					{
						elements_count +=1;                // Increment the elements count
						strncpy(element_string,temp_string,(int) (strstr(temp_string," ^ ") - temp_string));
						// Append end-of string to assure that we have properly terminated string
						strncpy(element_string + ((int) (strstr(temp_string," ^ ") - temp_string)),"\0",1); 
						//	printf("Found element: %s\n",element_string);
						// If the element is a common element, then remove it from both sides
						if (NULL != strstr(net_feedback_string,element_string))
						{
							// Remove the element from the cell string
							strcpy(net_cell_string,remove_repeated_text(net_cell_string,element_string));
							// Remove the element from the feedback string
							strcpy(net_feedback_string,remove_repeated_text(net_feedback_string,element_string));

						}
						// This is supposed to move the pointer to start after the current seperator
						strcpy(temp_string, strstr(temp_string," ^ ") + strlen(" ^ ")); 
					}
					//	printf("Found element: %s\n",temp_string);
					strcpy(element_string,temp_string); // We need to move the element into element_string for next lines
					// If the last element is a common element, then remove it from both sides
					if (NULL != strstr(net_feedback_string,element_string))
					{
						// Remove the element from the cell string
						strcpy(net_cell_string,remove_repeated_text(net_cell_string,element_string));
						// Remove the element from the feedback string
						strcpy(net_feedback_string,remove_repeated_text(net_feedback_string,element_string));

					}
				} // end of multi-element section
				//printf("It has %d elements\n",elements_count);

				sprintf(bb_string[bit_index],"%s ^ %s",net_cell_string,net_feedback_string);
			} // if(p[bit_index]==1)
			else
			{
				if(bit_index>0)
				{
					strcpy(bb_string[bit_index],bb_string[bit_index-1]);
				}
				else
				{
					strcpy(bb_string[bit_index],feedback_string);
				}
			} // else of (if(p[bit_index]==1))
		} // for(bit_index=(m-1);bit_index>=0;bit_index--)
		// Printout the current function
		printf("S%d:\n",power_index);
		for(bit_index=0;bit_index<=(m-1);bit_index++)
		{
			if (bit_index > 0)
			{
				printf("S[%2d]=%s\n",bit_index,bb_string[bit_index]);
			}
			else
			{
				printf("S[%2d]=%s ^ Din\n",bit_index,bb_string[bit_index]);
			}
		}
	}
}

void decode_bch(void)
/*
* Simon Rockliff's	implementation of Berlekamp's algorithm.
*
* Assume we have received bits	in recd[i],	i=0..(n-1).
*
* Compute the 2*t syndromes by	substituting alpha^i into rec(X) and
* evaluating, storing the syndromes in	s[i], i=1..2t (leave s[0] zero)	.
* Then	we use the Berlekamp algorithm to find the error location polynomial
* elp[i].
*
* If the degree of	the	elp	is >t, then	we cannot correct all the errors, and
* we have detected	an uncorrectable error pattern.	We output the information
* bits	uncorrected.
*
* If the degree of	elp	is <=t,	we substitute alpha^i ,	i=1..n into	the	elp
* to get the roots, hence the inverse roots, the error	location numbers.
* This	step is	usually	called "Chien's	search".
*
* If the number of	errors located is not equal	the	degree of the elp, then
* the decoder assumes that	there are more than	t errors and cannot	correct
* them, only detect them. We output the information bits uncorrected.
*/
{
	register int	i, j, u, q,	t2,	count =	0, syn_error = 0;
	int				elp[300][256], d[1026], l[1026], u_lu[1026], s[128];
	int				root[200], loc[200],	reg[201]; // , err[256];

	t2 = 2 * t;

	/* first form the syndromes	*/
	printf("The calculated syndrome is \n");
	printf("S(x) = ");
	for	(i = 1;	i <= t2; i++) {
		s[i] = 0;
		for	(j = 0;	j <	length;	j++)
			if (recd[j]	!= 0)
				s[i] ^=	alpha_to[(i	* j) % n];
		if (s[i] !=	0)
			syn_error =	1; /* set error	flag if	non-zero syndrome */
		/*
		* Note:	If the code	is used	only for ERROR DETECTION, then
		*			exit program here indicating the presence of errors.
		*/
		/* convert syndrome	from polynomial	form to	index form	*/
		s[i] = index_of[s[i]];
		printf("%3d	", s[i]);
	}
	printf("\n");

	if (syn_error) {	/* if there	are	errors,	try	to correct them	*/
		/*
		* Compute the error location polynomial via the Berlekamp
		* iterative algorithm.	Following the terminology of Lin and
		* Costello's book :   d[u]	is the 'mu'th discrepancy, where
		* u='mu'+1	and	'mu' (the Greek	letter!) is	the	step number
		* ranging from	-1 to 2*t (see L&C),  l[u] is the degree of
		* the elp at that step, and u_l[u]	is the difference between
		* the step	number and the degree of the elp. 
		*/
		/* initialise table	entries	*/
		printf("Errors are being detected\n");
		d[0] = 0;			/* index form */
		d[1] = s[1];		/* index form */
		elp[0][0] =	0;		/* index form */
		elp[1][0] =	1;		/* polynomial form */
		for	(i = 1;	i <	t2;	i++) {
			elp[0][i] =	-1;	/* index form */
			elp[1][i] =	0;	/* polynomial form */
		}
		l[0] = 0;
		l[1] = 0;
		u_lu[0]	= -1;
		u_lu[1]	= 0;
		u =	0;

		do {
			u++;
			if (d[u] ==	-1)	{
				l[u	+ 1] = l[u];
				for	(i = 0;	i <= l[u]; i++)	{
					elp[u +	1][i] =	elp[u][i];
					elp[u][i] =	index_of[elp[u][i]];
				}
			} else
				/*
				* search for words	with greatest u_lu[q] for
				* which d[q]!=0 
				*/
			{
				q =	u -	1;
				while ((d[q] ==	-1)	&& (q >	0))
					q--;
				/* have	found first	non-zero d[q]  */
				if (q >	0) {
					j	= q;
					do {
						j--;
						if ((d[j] != -1) &&	(u_lu[q] < u_lu[j]))
							q	= j;
					}	while (j > 0);
				}

				/*
				* have	now	found q	such that d[u]!=0 and
				* u_lu[q] is maximum 
				*/
				/* store degree	of new elp polynomial */
				if (l[u] > l[q]	+ u	- q)
					l[u	+ 1] = l[u];
				else
					l[u	+ 1] = l[q]	+ u	- q;

				/* form	new	elp(x) */
				for	(i = 0;	i <	t2;	i++)
					elp[u +	1][i] =	0;
				for	(i = 0;	i <= l[q]; i++)
					if (elp[q][i] != -1)
						elp[u +	1][i + u - q] =	
						alpha_to[(d[u] +	n -	d[q] + elp[q][i]) %	n];
				for	(i = 0;	i <= l[u]; i++)	{
					elp[u +	1][i] ^= elp[u][i];
					elp[u][i] =	index_of[elp[u][i]];
				}
			}
			u_lu[u + 1]	= u	- l[u +	1];

			/* form	(u+1)th	discrepancy	*/
			if (u <	t2)	{	
				/* no discrepancy computed on last iteration */
				if (s[u +	1] != -1)
					d[u	+ 1] = alpha_to[s[u	+ 1]];
				else
					d[u	+ 1] = 0;
				for	(i = 1;	i <= l[u + 1]; i++)
					if ((s[u + 1 - i]	!= -1) && (elp[u + 1][i] !=	0))
						d[u	+ 1] ^=	alpha_to[(s[u +	1 -	i] 
						+	index_of[elp[u + 1][i]]) % n];
						/* put d[u+1]	into index form	*/
						d[u +	1] = index_of[d[u +	1]];	
			}
		} while	((u	< t2) && (l[u +	1] <= t));

		u++;
		if (l[u] <=	t) {/* Can correct errors */
			/* put elp into	index form */
			printf("Number of errors %3d is <= t (%3d) so they can be corrected\n",l[u],t);
			for	(i = 0;	i <= l[u]; i++)
				elp[u][i] =	index_of[elp[u][i]];

			printf("sigma(x) = ");
			for	(i = 0;	i <= l[u]; i++)
				printf("%3d	", elp[u][i]);
			printf("\n");
			printf("Roots: ");

			/* Chien search: find roots	of the error location polynomial */
			for	(i = 1;	i <= l[u]; i++)
				reg[i] = elp[u][i];
			count =	0;
			for	(i = 1;	i <= n;	i++) {
				q =	1;
				for	(j = 1;	j <= l[u]; j++)
					if (reg[j] != -1) {
						reg[j] = (reg[j] + j) %	n;
						q ^= alpha_to[reg[j]];
					}
					if (!q)	{	/* store root and error
								* location	number indices */
						root[count]	= i;
						loc[count] = n - i;
						count++;
						printf("%3d	", n - i);
					}
			}
			printf("\n");
			if (count == l[u])	
			{
				/* no. roots = degree of elp hence <= t	errors */
				printf("Try to fix the located %3d errors\n",count);
				for	(i = 0;	i <	l[u]; i++)
				{
					printf("Correct error at location %d\n",loc[i]);
					recd[loc[i]] ^=	1;
				}
			}
			else	/* elp has degree >t hence cannot solve	*/
			{
				printf("Since the degree of the elp (%3d) > t (%3d) errors can not be corrected\n", count, l[u]);
				printf("Incomplete decoding: errors	detected\n");
			}
		}
	}
}

void dsp_decode_bch(void)
/*
* Simon Rockliff's	implementation of Berlekamp's algorithm.
*
* Assume we have received bits	in recd[i],	i=0..(n-1).
*
* Compute the 2*t syndromes by	substituting alpha^i into rec(X) and
* evaluating, storing the syndromes in	s[i], i=1..2t (leave s[0] zero)	.
* Then	we use the Berlekamp algorithm to find the error location polynomial
* elp[i].
*
* If the degree of	the	elp	is >t, then	we cannot correct all the errors, and
* we have detected	an uncorrectable error pattern.	We output the information
* bits	uncorrected.
*
* If the degree of	elp	is <=t,	we substitute alpha^i ,	i=1..n into	the	elp
* to get the roots, hence the inverse roots, the error	location numbers.
* This	step is	usually	called "Chien's	search".
*
* If the number of	errors located is not equal	the	degree of the elp, then
* the decoder assumes that	there are more than	t errors and cannot	correct
* them, only detect them. We output the information bits uncorrected.
*/
{
	register int	i, j, u, q,	q1, t2,	count =	0, syn_error = 0;
	int				elp[300][256], d[1026], l[1026], u_lu[1026], s[128];
	int				root[200], loc[200],	reg[201]; // , err[256];

	// These variables are for the SiBM/RiBM algorithm
	int             theta[100][50];
	int             delta[100][50];
	int             delta_zero;
	int             gamma[100];
	int				MC[100];
	int             k[100];
	int             PE_MULT1;
	int             PE_MULT2;


	// Just needed for debug printouts
	int             index;
	int             l_index;

	int				decode_bit_index;
	int				decode_byte_index;
	int				data_buffer_count; // indicates how many bits should be buffered to assure byte alignment 
	// during the calculation of the checksum for the syndrome calculations
	int				D[8]; // A byte wide buffer

	// these are needed to support more than a single decoding algorithm

	int		Elp_Min_Loop;
	int		q_init;
	char FILE_NAME[100];

	FILE *TEST_DECODE_FILE; // A File pointer for the test decoding sequence information
	FILE *ERRORS_LOC_FILE; // A File pointer for the errors locations (as read from the HW)



	if (generate_test_files == 1) // If needed create the test vector reference files
	{
		//sprintf(FILE_NAME,"BCH_test_%d_%d_%d_%d_ECC_CALC.txt",t,numerr,n1,n1_mask);
		sprintf(FILE_NAME,"%s_ECC_CALC.txt",test_name);
		TEST_DECODE_FILE = fopen(FILE_NAME,"w");
		sprintf(FILE_NAME,"%s_Errors_loc.txt",test_name);
		ERRORS_LOC_FILE = fopen(FILE_NAME,"w");
	}

	t2 = 2 * t;
	// The syndrome calculation is skipped if the HW is to be emulated
	if (emulate_HW_only == 0)
	{
		/* first form the syndromes	*/
		if (log_for_debug == 1)
		{
			printf("The calculated syndrome is \n");
			printf("S(x) = ");
		}
		for	(i = 0;	i <= t2; i++) { // Modified to start form 0 instead of 1 to allow calculation of S0 as well
			s[i] = 0;
			for	(j = 0;	j <	length;	j++)
				if (recd[j]	!= 0)
					s[i] ^=	alpha_to[(i	* j) % n];
			if ((s[i] !=	0) && (i>0)) // Modified to support the calculation of S0 as well
				syn_error =	1; /* set error	flag if	non-zero syndrome */
			/*
			* Note:	If the code	is used	only for ERROR DETECTION, then
			*			exit program here indicating the presence of errors.
			*/
			/* convert syndrome	from polynomial	form to	index form	*/
			// s[i] = index_of[s[i]]; Removed the conversion into the index. Now all works in polynomial form only

			if (log_for_debug == 1)
			{
				printf("%5d ", s[i]);
			}
		}
		if (log_for_debug == 1)
		{
			printf("\n");
		}
	} // if (emulate_HW_only == 0) ends here
	else
	{
		// At this point we calculate the syndromes based on 1st calculating the checksum then feeding the data to 
        // calculate the syndromes
        // initialize the checksum array to 0
		if (log_for_debug == 1)
		{
			printf("Initialize checksum buffer\n");
		}
		for(decode_bit_index=0;decode_bit_index<400;decode_bit_index++)
			checksum[decode_bit_index]=0;
		// Calculate the alignment buffer size
		data_buffer_count = (8-(length % 8)) % 8; 
		if (log_for_debug == 1)
		{
			printf("for decoding %d errors in %d bits, we need to buffer %d bits to aligne the data to Bytes\n",t,length,data_buffer_count);
		}
		if (data_buffer_count != 0)
		{
			//		printf("Byte   0 =");
			for(decode_bit_index=0;decode_bit_index<8;decode_bit_index++)
			{
				if(decode_bit_index < data_buffer_count)
				{
					D[decode_bit_index]=0;
				}
				else
				{
					if (((decode_bit_index < (4096 + data_buffer_count)) | (decode_bit_index > (4095 + (8*n1_mask) + data_buffer_count))))
					{
						D[decode_bit_index]=recd[length - (decode_bit_index-data_buffer_count) - 1];
					}
					else
					{
						D[decode_bit_index]= 0;
						//printf("Index %d is ignored in decoding\n",decode_bit_index);
					}
					//D[decode_bit_index]=recd[length - (decode_bit_index-data_buffer_count) - 1];
				}
				//			printf("%d",D[decode_bit_index]);
			}
			//		printf("\n");
			calculate_checksum(D,DECODE_MODE); // Calculate the 1st aligned byte checksum
		}

		for(decode_byte_index=data_buffer_count;decode_byte_index<length;decode_byte_index+=8)
		{
			//		printf("Byte %3d =",(decode_byte_index + 7) / 8);
			// Prepare the next byte
			for(decode_bit_index=0;decode_bit_index<8;decode_bit_index++)
			{
				if ( ((decode_bit_index + decode_byte_index) < 4096  ) 
					|((decode_bit_index + decode_byte_index) > (4095 + (8*n1_mask)) ) )
				{
					D[decode_bit_index]=recd[length - (decode_bit_index+decode_byte_index) - 1];
				}
				else
				{
					D[decode_bit_index]= 0;
					//printf("Index %d is ignored in decoding\n",decode_bit_index+decode_byte_index);
				}
				//D[decode_bit_index]=recd[length - (decode_bit_index+decode_byte_index) - 1];
				//			printf("%d",D[decode_bit_index]);
			}
			// Calculate the next byte checksum
			//		printf("\n");
			calculate_checksum(D,DECODE_MODE);
		}

		// Once the checksum is ready, we need to calculate the syndromes.
		// The syndrome calculation is performed by feeding the checksum data (MSB first) into the 
		// checksum calculation hardware. The process requires exactly the same number of cycles as the 
		// checksum length.
		// At first stage we need to initialize the syndrome hardware cells
		// Please note that we only initialize the needed cells. The number of syndromes equals to 2*t

		for(i=0;i<=(2*t);i++)
		{
			for(j=0;j<m;j++)
			{
				Syndromes[i].S[j]=0;
			}
		}

		// Errors are declared if the cjecksum is not equal to 0

		for(decode_bit_index=(m*t)-1;(decode_bit_index>=0) && (syn_error == 0);decode_bit_index--)
		{
			if (checksum[decode_bit_index]!=0)
				syn_error =	1;

		}
		if (syn_error == 1)
		{

			// Now calculate the syndromes values
			for(decode_bit_index=(m*t)-1;decode_bit_index>=0;decode_bit_index--)
			{
				syndrome_calc(checksum[decode_bit_index],SYN_CALC);
			}
			// Once done copy the calculated syndrome values to a regular integer array (convert from bits to int)
			for(i=1;i<=(2*t)-1;i++)
			{
				s[i]=0;
				for(j=m-1;j>=0;j--)
				{
					s[i]= (s[i] << 1) + Syndromes[i].S[j];
				}
			}
		}
		// Print the checksum and the syndromes values
		if (generate_test_files == 1) // If needed close the test vector reference files
		{
			// Initially print the calculated checksum
			fprintf(TEST_DECODE_FILE,"Calculated Checksum value [%d : 0] =\n",m*t-1);
			for(i=0;i<m*t;i++)
			{
				fprintf(TEST_DECODE_FILE,"%d",checksum[m*t-i-1]);
			}
			fprintf(TEST_DECODE_FILE,"\n\n");

			fprintf(TEST_DECODE_FILE,"Calculated Syndromes:\n");
			for (i=0;i<2*t;i++)
			{
				fprintf(TEST_DECODE_FILE,"S%d[%d-0]=",i,m-1);
				for(j=0;j<m;j++)
				{
					fprintf(TEST_DECODE_FILE,"%d",Syndromes[i].S[m-j-1]);
				}
				fprintf(TEST_DECODE_FILE,"\n");
			}
			fprintf(TEST_DECODE_FILE,"\n\n");
		}
	} // if (emulate_HW_only == 1) ends here

	if (syn_error) {	/* if there	are	errors,	try	to correct them	*/
		if (regular_BM_Enable == 1) // Use the BM algorithm only if configured to do so
		{
			/*
			* Compute the error location polynomial via the Berlekamp
			* iterative algorithm.	Following the terminology of Lin and
			* Costello's book :   d[u]	is the 'mu'th discrepancy, where
			* u='mu'+1	and	'mu' (the Greek	letter!) is	the	step number
			* ranging from	-1 to 2*t (see L&C),  l[u] is the degree of
			* the elp at that step, and u_l[u]	is the difference between
			* the step	number and the degree of the elp. 
			*/
			/* initialise table	entries	*/
			printf("Errors are being detected\n");
			d[0] = 0;			/* index form */
			d[1] = index_of[s[1]];		/* index form */ //*******************************************
			elp[0][0] =	1;		/* polynomial form */
			elp[1][0] =	1;		/* polynomial form */
			for	(i = 1;	i <	t2;	i++) {
				elp[0][i] =	0;	/* polynomial form */
				elp[1][i] =	0;	/* polynomial form */
			}
			l[0] = 0;
			l[1] = 0;
			u_lu[0]	= -1;
			u_lu[1]	= 0;
			u =	0;

			do {
				u++;
				if (d[u] ==	-1)	{
					l[u	+ 1] = l[u];
					for	(i = 0;	i <= l[u]; i++)	{
						elp[u +	1][i] =	elp[u][i];
						//elp[u][i] =	index_of[elp[u][i]];
					}
				} else
					/*
					* search for words	with greatest u_lu[q] for
					* which d[q]!=0 
					*/
				{
					q =	u -	1;
					while ((d[q] ==	-1)	&& (q >	0))
						q--;
					/* have	found first	non-zero d[q]  */
					if (q >	0) {
						j	= q;
						do {
							j--;
							if ((d[j] != -1) &&	(u_lu[q] < u_lu[j]))
							{
								printf("Found a case while u=%3d in which u_lu[q=%3d]=%3d < u_lu[j=%3d]=%3d\n",u, q,u_lu[q],j,u_lu[j]);
								for (index=0;index<u+1;index++)
								{
									printf("Index=%3d u_lu[index]=%3d d[index]=%5d l[index]=%3d\n",index,u_lu[index], d[index], l[index]);
								}
								for (index=0;index<u+1;index++)
								{
									for (l_index=0;l_index<l[index]+1;l_index++)
									{
										printf("elp[%2d][%2d]=%4d ", index, l_index, elp[index][l_index]);
									}
									printf("\n");
								}
								q	= j;
								getchar();
							}
						}	while (j > 0);
					}

					/*
					* have	now	found q	such that d[u]!=0 and
					* u_lu[q] is maximum 
					*/
					/* store degree	of new elp polynomial */
					if (l[u] > l[q]	+ u	- q)
						l[u	+ 1] = l[u];
					else
						l[u	+ 1] = l[q]	+ u	- q;

					/* form	new	elp(x) */
					for	(i = 0;	i <	t2;	i++)
						elp[u +	1][i] =	0;
					for	(i = 0;	i <= l[q]; i++)
						/*if (elp[q][i] != -1)
						elp[u +	1][i + u - q] =	
						alpha_to[(d[u] +	n -	d[q] + elp[q][i]) %	n]; */
						if (elp[q][i] != 0)
							elp[u +	1][i + u - q] =	galua_mult(elp[q][i],alpha_to[(d[u] +	n -	d[q]) % n]); //****************

					for	(i = 0;	i <= l[u]; i++)	{
						elp[u +	1][i] ^= elp[u][i];
						//elp[u][i] =	index_of[elp[u][i]];
					}
				}
				u_lu[u + 1]	= u	- l[u +	1];

				/* form	(u+1)th	discrepancy	*/
				if (u <	t2)	{	
					/* no discrepancy computed on last iteration */
					if (s[u +	1] != 0)
						d[u	+ 1] = s[u	+ 1]; // alpha_to[s[u	+ 1]];
					else
						d[u	+ 1] = 0;
					for	(i = 1;	i <= l[u + 1]; i++)
						if ((s[u + 1 - i]	!= 0) && (elp[u + 1][i] !=	0))
							/*d[u	+ 1] ^=	alpha_to[(s[u +	1 -	i] 
							+	index_of[elp[u + 1][i]]) % n];*/
							d[u	+ 1] ^=	galua_mult(s[u +	1 -	i] , elp[u + 1][i]);
					/* put d[u+1]	into index form	*/
					d[u +	1] = index_of[d[u +	1]];	// **************************************************
				}
				/*
				printf("BM ELP cycle=%d discrepancy=%5d\n",u, d[u+1]);
				for (i=0;i<2*t+1;i++)
				printf("%5d ",elp[u+1][i]);
				printf("\n");
				*/

			} while	((u	< t2) && (l[u +	1] <= t));

			/*
			printf("Berlekamp algorithm for elp calculation is done\n");
			for	(i = 0;	i <= (u+1); i++) {
			printf("Step %3d: d[]=%4d u_lu[]=%4d l[]=%3d\n",i, d[i], u_lu[i], l[i]);
			}
			*/

			printf("ELP(x) = ");
			for	(i = 0;	i <= l[u+1]; i++)
				printf("%3d	", elp[u+1][i]);
			printf("\n");

		}
		if (dsp_SiBM_Enable == 1)
		{
			// This code computes the error locating polyomial using the SiBM algorithm
			// Initialization
			delta[2*t][0]=1;
			delta[2*t-1][0]=0;
			theta[2*t-1][0]=0;
			k[0]=0;
			gamma[0]=1;
			for (i=0; i<=(2*t - 2); i++)
			{
				theta[i][0]=s[i];
				delta[i][0]=s[i];
			}

			// For debug only

			printf("Initial delta values: ");
			for (i=0; i<=(2*t); i++)
				printf("%5d ", delta[i][0]);
			printf("\n"); 
			for (u=0; u<=(t-1); u++) // u is used as r index in the algorithm
			{
				// Step SiBM.1
				delta[2*t+2][u]=0;
				delta[2*t+1][u]=0;
				theta[2*t+1][u]=0;
				theta[2*t][u]=0; // Missing in the pseudocode and in the diagram!!!
				for (i=0; i<=(2*t); i++)
				{
					delta[i][u+1]= galua_mult(gamma[u],delta[i+2][u]) ^ galua_mult(delta[0][u],theta[i+1][u]);
				}
				// Step SiBM.2
				if ((delta[0][u] != 0) & (k[u] >= 0))
				{
					for (i=0; i<( 2*t - 2); i++)
					{
						theta[i][u+1]=delta[i+1][u];
					}
					gamma[u+1] = delta[0][u];
					k[u+1] = -k[u];
				}
				else
				{
					for (i=1; i< (2*t - 2); i++)
					{
						theta[i][u+1]=theta[i][u];
					}
					gamma[u+1]=gamma[u];
					k[u+1]=k[u]+2;
				}
				theta[2*t - 2][u+1] = 0;
				theta[2*t - 1][u+1] = 0;
				// After every iteration print the current delta values
				printf("Iteration %3d, K=%4d, gamma=%5d\n",u,k[u],gamma[u]);
				printf("delta = ");
				for (i=0; i<(2*t+3); i++)
					printf("%5d ",delta[i][u]);
				printf("\n");;
				printf("theta = ");
				for (i=0; i<(2*t+2); i++)
					printf("%5d ",theta[i][u]);
				printf("\n");
			}

			// Once the calculation is done, we need to copy the values from theta to the elp for locating the errors
			for (i=0; i<t+1; i++)
			{
				elp[u][i] = delta[i][t]; /* note that the indexes are flipped ince in SiBM the 1st index is the 
										 polynomial position and the 2nd index is the iteration count, while in elp the 1st index
										 is the iteration and the 2nd index is the polynoial position */

			}
			// At this moment since there is no clear way to tell the number of errors, we set l to the maximal number
			l[u]= t;
		}

		if (dsp_RiBM_Enable == 1)
		{
			// This code computes the error locating polyomial using the SiBM algorithm
			// Initialization
			delta[3*t][0]=1;               // delta is the discrepancy polynomial
			delta[3*t+1][0]=0;             // This piece is critical. Indicated in the text but not in the pseudocode
			theta[3*t][0]=1;               // theta is the discrepancy polynomial
			k[0]=0;
			gamma[0]=1;
			for (i=(2*t); i<=(3*t-1); i++)
			{
				delta[i][0]=0;
				theta[i][0]=0;
			}
			theta[0][0]=0;
			delta[0][0]=0;
			for (i=1; i<=(2*t-1); i++)
			{
				theta[i][0]=s[i];
				delta[i][0]=s[i];
			}

			// For debug only
			if (log_for_debug == 1)
			{
				printf("Initial delta values: ");
				for (i=0; i<=(3*t); i++)
					printf("%5d ", delta[i][0]);
				printf("\n"); 
				printf("Initial theta values: ");
				for (i=0; i<=(3*t); i++)
					printf("%5d ", theta[i][0]);
				printf("\n"); 
			}
			for (u=0; u<=(t*2-1); u++) // u is used as r index in the algorithm
			{
				// Step RiBM.1
				for (i=0; i<=(3*t); i++)
				{
					if (i<(3*t))
					{
						if (emulate_HW_only == 1)
						{
							delta[i][u+1]= galua_mult(gamma[u],delta[i+1][u]) ^ galua_mult_single_cycle(delta[0][u],theta[i][u]);
						}
						else
						{
							delta[i][u+1]= galua_mult(gamma[u],delta[i+1][u]) ^ galua_mult(delta[0][u],theta[i][u]);
						}
					}
					else
					{
						if (emulate_HW_only == 1)
						{
							delta[i][u+1]= galua_mult(gamma[u],0) ^ galua_mult_single_cycle(delta[0][u],theta[i][u]);
						}
						else
						{
							delta[i][u+1]= galua_mult(gamma[u],0) ^ galua_mult(delta[0][u],theta[i][u]);
						}
					}

				}
				// Step RiBM.2
				if ((delta[0][u] != 0) && (k[u] >= 0))
				{
					MC[u]=1;
					//	printf("Update\n");
					for (i=0; i<= (3*t); i++)
					{
						if (i < (3*t))
						{
							theta[i][u+1]=delta[i+1][u];
						}
						else
						{
							theta[i][u+1]=0;   // delta[3*t][u]=== 0 
						}
					}

					gamma[u+1] = delta[0][u];
					k[u+1] = -k[u] - 1;
				}
				else
				{
					MC[u]=0;
					//	printf("Keep\n");
					for (i=0; i<=((3*t)); i++)
					{
						theta[i][u+1]=theta[i][u];
					}
					gamma[u+1] = gamma[u];
					k[u+1] = k[u]+1;
				}
				// After every iteration print the current delta values
				if (log_for_debug == 1)
				{
					printf("Iteration %3d, K=%4d, gamma=%5d, delta[0]=%5d\n",u,k[u],gamma[u], delta[0][u]);
					printf("delta = ");
					for (i=0; i<(3*t + 1); i++)
						printf("%5d ",delta[i][u+1]);
					printf("\n");;
					printf("theta = ");
					for (i=0; i<(3*t + 1); i++)
						printf("%5d ",theta[i][u+1]);
					printf("\n"); 
					printf("Gamma[%3d] = %4d\n", u, gamma[u]);
					printf("K[%3d] = %4d\n", u, k[u]);
					printf("MC[%3d] = %4d\n", u, MC[u]);
				}
			}

			// Once the calculation is done, we need to copy the values from theta to the elp for locating the errors
			printf("Calculated ELP = ");
			l[u] = 0; // Initialize
			for (i=0; i<(t+1); i++)
			{

				elp[u][i] = delta[t+i][2*t]; /* note that the indexes are flipped ince in RiBM the 1st index is the 
											 polynomial position and the 2nd index is the iteration count, while in elp the 1st index
											 is the iteration and the 2nd index is the polynoial position */
				printf("%5d ",elp[u][i]);
				if (elp[u][i] > 0)
				{
					l[u]=i;
				}

			}
			printf("\n");
		} // if (dsp_RiBM_Enable == 1) ends here

		if (dsp_DcRiBM_Enable == 1)
		{
			// This code computes the error locating polyomial using the SiBM algorithm
			// Initialization
			for(i=0;i<=(2*t-4);i++)
				delta[i][0]=s[i+3];

			for(i=0;i<=(2*t-4);i++)
				theta[i][0]=s[i+2];

			delta[2*t-3][0]=0;
			delta[2*t-2][0]=0;
			delta[2*t-1][0]=1;
			delta[2*t  ][0]=0;
			delta[0    ][0]=0;
			delta_zero = 1;     // The pseudo code differentiate between delta[i] and prev. delta0

			theta[2*t-2][0]=0;
			theta[2*t-1][0]=0;
			theta[2*t+1][0]=0;
			theta[2*t  ][0]=1;
			theta[0    ][0]=0;

			gamma[0]=1;

			// For debug only

			printf("Initial delta values: ");
			for (i=0; i<=(2*t); i++)
				printf("%5d ", delta[i][0]);
			printf("\n"); 
			printf("Initial theta values: ");
			for (i=0; i<=(2*t); i++)
				printf("%5d ", theta[i][0]);
			printf("\n"); 
			for (u=0; u<=(t*2 - 1); u++) // u is used as r index in the algorithm
			{
				for (i=0;i<=(2*t);i++)
				{
					PE_MULT1 = galua_mult(gamma[u],delta[i+1][u]);
					PE_MULT2 = galua_mult(theta[i][u],delta[0][u]);  // Equation 4 uses theta[0] instead of theta[i];

					delta[i][u+1] = galua_mult(delta[0][u],PE_MULT1 ^ PE_MULT2); // Pseudo-code uses delta[i] instead of delta[0] (equation 4)
				}
				// After every iteration print the current delta values
				gamma[u+1]=delta[0][u];  // Not written in the pseudo-code, but from RiBM and schematic it makes sense

				printf("Iteration %3d, , gamma=%5d, delta[0]=%5d\n",u,gamma[u], delta[0][u]);
				printf("delta = ");
				for (i=0; i<=(2*t); i++)
					printf("%5d ",delta[i][u+1]);
				printf("\n");;
				printf("theta = ");
				for (i=0; i<=(2*t); i++)
					printf("%5d ",theta[i][u+1]);
				printf("\n");

			}

			// Once the calculation is done, we need to copy the values from theta to the elp for locating the errors
			printf("Calculated ELP = ");
			l[u] = 0; // Initialize the number of found errors
			for (i=0; i<=t; i++)
			{
				elp[u][i] = delta[i][2*t]; /* note that the indexes are flipped ince in SiBM the 1st index is the 
										   polynomial position and the 2nd index is the iteration count, while in elp the 1st index
										   is the iteration and the 2nd index is the polynoial position */
				printf("%5d ",elp[u][i]);
				if (elp[u][i] !=0)
				{
					l[u]= i;
				}


			}
			printf("\n");
			// At this moment since there is no clear way to tell the number of errors, we set l to the maximal number
			//l[u]= t;
		}

		if ((dsp_SiBM_Enable == 0) && (dsp_RiBM_Enable == 0) && (dsp_DcRiBM_Enable == 0))
		{
			u++;
			Elp_Min_Loop = 1;
			q_init = 1;
		}
		else // For inversionless Chien search we need to sum from index 0 and to initialize q_init to 0
		{
			q_init = 0;
			Elp_Min_Loop = 0;
		}
		if (l[u] <=	t) 
		{   
			/* Can correct errors */
			/* put elp into	index form */
			printf("ELP degree %3d is <= t (%3d) so errors may be corrected\n",l[u],t);
			/*	for	(i = 0;	i <= l[u]; i++)
			elp[u][i] =	index_of[elp[u][i]]; */

			printf("sigma(x) = ");
			for	(i = 0;	i <= l[u]; i++)
				printf("%3d	", elp[u][i]);
			printf("\n");
			printf("Roots: ");

			// For HW emulation we use the Syndrome HW machines
			if (emulate_HW_only == 1)
			{

				if (debug_syndrome_chien_search == 1)
				{
					// Initialize the reg array
					for	(i = Elp_Min_Loop;	i <= l[u]; i++)
						reg[i] = elp[u][i];
				}

				count =	0;
				// Initialize the Syndroems values
				for (j=Elp_Min_Loop;j<=t; j++) // Although we can settle for l[u] we need to be regular
				{

					if (debug_syndrome_chien_search == 1)
					{
						printf("elp[%d][%d]=%d\n",u,j,elp[u][j]);
						printf("S[%d]=",j);
					}
					// Since the Syndromes operations are performed per-bit we need to convert numbers to bits
					for (i=0;i<m;i++)
					{
						Syndromes[j].S[i] = (elp[u][j] >> i) & 0x01;

						if (debug_syndrome_chien_search == 1)
							printf("%d",Syndromes[j].S[i]);
					}

					if (debug_syndrome_chien_search == 1)
						printf("\n");
				}
				// Now once the syndromes are initialized with the calculated elp values
				// we perform the Chien search for locating the errors

				for	(i = 1;	i <= n;	i++) 
				{
					q =	q_init;
					if (debug_syndrome_chien_search == 1)
						q1 = q_init;
					// Update the Syndromes values
					syndrome_calc(0,ERR_LOC_CALC); // Activate the Syndromes HW in error locating mode

					// Perform the Chien search syndromes sum for checking for roots coordinates
					for	(j = Elp_Min_Loop;	j <= t; j++) // Although we can settle for l[u] we need to be regular
					{
						// Perform the add operation per bit by shifting each bit field to the bit-field index
						for (decode_bit_index=0; decode_bit_index<m;decode_bit_index++)
						{
							q = q ^ (Syndromes[j].S[decode_bit_index] << decode_bit_index);
						}

						if (debug_syndrome_chien_search == 1)
						{
							if (reg[j] != 0) {

								reg[j] = galua_mult_single_cycle(reg[j],alpha_to[j]); 
							}
							q1 ^= reg[j];
							if (q1 != q)
							{
								printf("Found difference in Syndromes\n");
								printf("q=%d , q1 = %d, j=%d\n",q, q1, j);
								printf("reg[%d]=%d\n",j,reg[j]);
								printf("Syndrome[%d]=",j);
								for (decode_bit_index=0; decode_bit_index<m;decode_bit_index++)
								{
									printf("%d",Syndromes[j].S[decode_bit_index]);
								}
								printf("\n");
								getchar();
							}

						}
					}
					if (!q)	{	/* store root and error
								* location	number indices */
					//	root[count]	= i;
					//	printf("Found a root at index %d\n",i);
						loc[count] = n - i;
					//	loc[count] = i;
						count++;
						printf("%3d	", n - i);
					//	printf("%3d ",i);
					}
				}

			} // if (emulate_HW_only == 1) ends here
			else
			{
				/* Chien search: find roots	of the error location polynomial */
				for	(i = Elp_Min_Loop;	i <= l[u]; i++)
					reg[i] = elp[u][i];
				count =	0;
				for	(i = 1;	i <= n;	i++) {
					q =	q_init;
					for	(j = Elp_Min_Loop;	j <= l[u]; j++)
						if (reg[j] != 0) {
							reg[j] = galua_mult(reg[j],alpha_to[j]); 
							//reg[j] = (reg[j] + j) %	n;
							//q ^= alpha_to[reg[j]];
							q ^= reg[j];
						}
						if (!q)	{	/* store root and error
									* location	number indices */
							// printf("\nq=%d\n",q);
							root[count]	= i;
							loc[count] = n - i;
							count++;
							printf("%3d	", n - i);
						}
				}
			} // if (emulate_HW_only == 0) ends here
			printf("\n");
			if (count == l[u])	
			{
				/* no. roots = degree of elp hence <= t	errors */
				printf("Try to fix the located %3d errors\n",count);
				for	(i = 0;	i <	l[u]; i++)
				{
					printf("Correct error at location %d\n",loc[i]);
					recd[loc[i]] ^=	1;
				}
			}
			else	/* elp has degree >t hence cannot solve	*/
			{
				printf("The ELP degree %d is different than the number of found roots %d\n",l[u],count);
				printf("Errors can not be corrected\n");
				//printf("Since the degree of the elp (%3d) > t (%3d) errors can not be corrected\n", count, l[u]);
				//printf("Incomplete decoding: errors	detected\n");
				//getchar();
			}
		}
		else
		{
			printf("number of identified errors %d is larger than the possible correctable errors %d\n",l[u],t);
			printf("Errors can not be corrected!\n");
		}
	}
	
if (generate_test_files == 1) // If needed close the test vector reference files
	{
		fprintf(TEST_DECODE_FILE,"Calculating ELP values per iteration:");
		for (i=1;i<=(2*t);i++)
		{
			fprintf(TEST_DECODE_FILE,"\n\n");
			fprintf(TEST_DECODE_FILE,"Iteration %d\n",i);
			fprintf(TEST_DECODE_FILE,"Theta[0 - %d] ",3*t);
			for(j=0;j<=(3*t);j++)
			{
				fprintf(TEST_DECODE_FILE,"%3d ",theta[j][i]);
			}
			fprintf(TEST_DECODE_FILE,"\n");
			fprintf(TEST_DECODE_FILE,"Delta[0 - %d] ",3*t);
			for(j=0;j<=(3*t);j++)
			{
				fprintf(TEST_DECODE_FILE,"%3d ",delta[j][i]);
			}
			fprintf(TEST_DECODE_FILE,"\n");
			fprintf(TEST_DECODE_FILE,"Gamma[%2d] = %4d\n", i, gamma[i]);
			fprintf(TEST_DECODE_FILE,"K[%2d] = %3d\n", i, k[i]);
			fprintf(TEST_DECODE_FILE,"MC[%2d] = %d\n", i, MC[i]);
			fprintf(TEST_DECODE_FILE,"\n");
		}
		fprintf(TEST_DECODE_FILE,"\n\n");

		fprintf(TEST_DECODE_FILE,"Calculated ELP [0 - %d]=",t);
		for (i=0;i<(t+1);i++)
		{
				fprintf(TEST_DECODE_FILE," %d ",elp[u][i]);
		}
		fprintf(TEST_DECODE_FILE,"\n\n");

		fprintf(TEST_DECODE_FILE,"Detected errors locations are:\n");
		for	(i = 0;	i <	l[u]; i++)
		{
			fprintf(TEST_DECODE_FILE,"Cycle=%4d, Index=%4d, address = %3d, Bit index = %d\n",n-loc[i],loc[i],((length - loc[i] - 1) / 8), ((length - loc[i] - 1) % 8));
			fprintf(ERRORS_LOC_FILE,"%04X\n",loc[i]);
		}

		fclose(TEST_DECODE_FILE);
		fclose(ERRORS_LOC_FILE);
	}
}


void gen_data(int vp)
{
	/// If there is no input file to read data from, we simply generate random data
	if (!input_file)
	{
		for	(int i = 0;	i <	k; i++)
			data[i]	= (	RANDOM() & 0x7FFF ) >> 14; // In Microsoft RAND_MAX == 0x7FFF so the MSB is in bit 14
		for (int i=0; i < (8*(n2 - ecc_checksum_size)); i++)
			n2_data[i]	= (	RANDOM() & 0x7FFF ) >> 14; // In Microsoft RAND_MAX == 0x7FFF so the MSB is in bit 14
		return;
	}
	
	unsigned char* file_data = 0;
	int	page_size = k/8;		/// page size in bytes
	int	total_size = page_size + n2;

	file_data = new unsigned char[total_size + 1];
	
	int bytes_read = fread(file_data, 1, total_size, input_file);
	if( bytes_read != total_size) {
			printf("Failed to read all data from input file %s\n", input_data_file_name);
			exit(0);
	}

	// Note: The data[] array is in bits, not bytes
	//for	(int byte = page_size;	byte >	0; byte--)
	for	(int byte = 0;	byte < page_size; byte++)
	{	
		unsigned char the_byte = file_data[ byte ];
		
		for	(int bit = 0; bit<8; bit++)
		{
			data[ (page_size-byte)*8 - 1 - bit]	= the_byte & 0x01;
			the_byte >>= 1;
		}
	}

	for (int i = 0; i < n2 - ecc_checksum_size; i++) {
		unsigned char the_byte = file_data[page_size + ecc_checksum_size + i];

		for (int bit = 0; bit < 8; bit++) {
			n2_data[i*8 + bit] = the_byte & 1;
			the_byte >>= 1;
		}
	}

	delete[] file_data;
}

int	main(int argc, char* argv[])
{
	int				i;
	int byte_count_value; // temporal variable used to format output vector files nicely
	int temp_byte_value;
	int Params_error;
	int iteration;
	int iterations_count;
	int errnum_histogram[100]; // This array is used to allow to verify the test coverage
	FILE *ALPHA_FILE;
	FILE *DATA_WITH_ERRORS_FILE; // A File pointer for the corrupted input data 
	FILE *DATA_OUT_FIXED_FILE;   // A File pointer for the corrected output data
	FILE *TEST_CONFIGURATION_FILE; // A File pointer for the test log information
	FILE *SOURCE_DATA_FILE;     // A File pointer for the source data before errors are added
	char parameter_string[100]; // A temporal string used to process command line parameters
	char FILE_NAME[100];

	// Initialize the errors numbers histogram.
	for (i=0; i< 100; i++)
	{
		errnum_histogram[i] = 0;
	}


	// If command line parameters were provided then scan them
	if (argc > 1)
	{
		Params_error = 0;
		if (argc < 7) 
		{
			Params_error = 1;
		}
		else
		{
			// By default set all the relevant parameters to normal single iteration random run
			emulate_HW_only = 1; // This is a must to assure that the SW actually behaves as the HW implementation
			debug_syndrome_chien_search = 0; // Not needed
			use_random_data = 1; // All the command line invoked runs uses random data
			random_iterations = 1; // In command line mode we iterate only once
			debug_gal_mult = 0; // Not needed
			only_create_mult_equations = 0; // Not needed
			calculate_encoding_polynomial = 0; // Not needed
			calculate_syndrome_polynomials = 0; // Not needed
			print_data_in_random_run = 0; // Not needed
			limit_errors_count = 1; // Needed to allow the user to set the number of errors
			rand_m = 13; // Mandatory to assure that the used field is indeed GF13
			//k = 1 << (rand_m - 1);
			dsp_decode_mode = 1; // Needed to assure that the SW uses DSP implementation of the ECC

			input_data_file_name[0] = 0;  /// Initialize this variable

			strcpy(parameter_string,argv[1]); // Set the number of detectable errors
			sscanf(parameter_string,"%d",&rand_t);
			if ((rand_t != 1) && (rand_t != 4) && (rand_t != 8) && (rand_t != 12) && (rand_t != 24))
			{
				Params_error = 1;
			}
			else
			{
				// Define the value of ecc_checksum_size
				switch(rand_t)
				{
				case(1):
					{
						ecc_checksum_size = 2; // In Hamming we have 14 bits spanned over 2 Bytes
						break;
					}
				case(4):
					ecc_checksum_size = 7; // In BCH 4 we have 4*13=52 bits spanned over 7 Bytes
					break;
				case(8):
					{
						ecc_checksum_size = 13; // In BCH 8 we have 8*13=104 bits spanned over 13 Bytes
						break;
					}
				case(12):
					{
						ecc_checksum_size = 20; // In BCH 12 we have 12*13=156 bits spanned over 20 Bytes
						break;
					}
				}
			}

			strcpy(parameter_string,argv[2]);
			sscanf(parameter_string,"%d",&n1);
			//n1 = Val(argv[2]);
			if ((n1 > 50) || (n1 < 0))
			{
				Params_error = 1;
			}


			strcpy(parameter_string,argv[3]);
			sscanf(parameter_string,"%d",&n1_mask);
			//n1_mask = Val(argv[3]);
			if ((n1_mask > n1) || (n1_mask < 0))
			{
				Params_error = 1;
			}

			strcpy(parameter_string,argv[4]);
			sscanf(parameter_string,"%d",&n2);
			if ((ecc_checksum_size > n2) || (n2 < 0))
			{
				Params_error = 1;
			}

			strcpy(parameter_string,argv[5]);
			sscanf(parameter_string,"%d",&errors_count_value);
			//errors_count_value = Val(argv[4]);
			if ((errors_count_value > (rand_t + 1)) || (errors_count_value < 0))
			{
				Params_error = 1;
			}
			if (rand_t != 1)
			{
				rand_block_size = (4096 + (n1*8)) + (rand_t * 13);
			}
			else
			{
				rand_block_size = (4096 + (n1*8)) + (rand_t * 14);
			}
			k=(4096 + (n1*8));
			if (argc > 7)
			{
				strcpy(parameter_string,argv[7]); // Get the 7th parameter to select between on/off for checksum log file
				sscanf(parameter_string,"%d",&print_checksum_for_debug);
			}
			else
			{
				print_checksum_for_debug = 0;
			}
			if (argc > 8)
			{
				use_determinitstic_seed = 1;
				strcpy(parameter_string,argv[8]); // Get the 8th parameter to force a seed value in a random run
				sscanf(parameter_string,"%d",&seed);
			}
			else
			{
				use_determinitstic_seed = 0;
			}
		}
		strcpy(prefix_string,argv[6]); // The last mandatory parameter is the test name prefix

		/// Crude way to check if there a '-i <input_filename>' switch
		/// The assumption is that if '-i' is the 8 param, then <input_filename> will follow it
		if (argc == 9)  
		{
			if ( strstr( argv[7], "-i") == argv[7] )
				strcpy(input_data_file_name , argv[8]);
		}

		if (Params_error == 1)
		{
			printf("Wrong parameters provided / ");
			printf("Wrong number of parameters\n");
			printf("Please invoke with:\n\n");
			printf("BCH_Implementation <ECC_MODE> <n1> <n1_mask> <n2> <Errors> <Prefix> [Show_checksum] [Seed Value]\n");
			printf("<ECC_MODE> - should be one of 1 4 8 12 or 24\n");
			printf("<n1> - should be an integer number of bytes of protected data in the spare area\n");
			printf("<n1_mask> - should be  number of bytes (always <= n1) to ignore in n1 (treated as 00)\n");
			printf("<n2> - should be number of n2 bytes (always >= checksum size)\n");
			printf("<Errors> - is the number of actual errors (Errors<=ECC_MODE+1)\n");
			printf("<prefix> - is the test name unique prefix string\n");
			printf("[Show_checksum] - is an OPTIONAL parameter which when set to 1, generates trace file for checksum calculations\n");
			printf("[Seed Value] - is an OPTIONAL parameter which when provided, is used to re-create a sequence\n");
			exit(-1); // If error happened in the command line exit with error code -1
		}
		else
		{
			printf("BCH_Implementation: Start running from command line parameters as follows\n");
			printf("\t ECC_MODE = %d\n\t n1=%d\n\t n1_mask=%d\n\t n2=%d\n\t Num_OF_Errors=%d\n\t Test_Name_Prefix=%s\n\t Show_checksum=%d",rand_t,n1,n1_mask,n2,errors_count_value,prefix_string,print_checksum_for_debug);
			generate_test_files = 1; // Mark to thw whole program that files should be created
			// This line defines the common part of all the files used in the test case
			sprintf(test_name,"BCH_%s_%d_%d_%d_%d_%d",prefix_string,rand_t,n1, n1_mask, n2, errors_count_value);
		}
	}
	
	read_p();				/* Read	m */
	if (calculate_encoding_polynomial == 1)
	{
		generate_gf();			/* Construct the Galois	Field GF(2**m) */
		gen_poly(); // This call is required to initialize some variables such as k which are used later in the code
	}

	if ((emulate_HW_only == 0) || (debug_syndrome_chien_search == 1))
	{
		generate_gf();			/* Construct the Galois	Field GF(2**m) */
		gen_poly();				/* Compute the generator polynomial	of BCH code	*/
		if (create_alpha_file == 1)
		{
		ALPHA_FILE = fopen("alpha_to.csv","w");
		for(i=0;i<=8192;i++)
		{
			fprintf(ALPHA_FILE,"%d,%d\n",i,alpha_to[i]);
		}
		fclose(ALPHA_FILE);
		}
	}
	else
	{
		t =	rand_t;
		d =	2 *	t +	1;

	}

	if (use_random_data == 1)
	{
		iterations_count = random_iterations;
	}
	else
	{
		iterations_count = 1;
	}

	/* Randomly	generate DATA */
	if(use_random_data == 0)
	{
		/* If constant data is needed then */
		seed = 131073;
	}
	else
	{
		/* If random data is needed then */
		if (use_determinitstic_seed == 0)
		{
			seed = 	(unsigned) time(NULL);
		}
	}
	SEED(seed);

	if (debug_gal_mult == 1)
	{
		// Only for testing the multiplier
		int mul_a; 
		int mul_b;
		for (mul_a = 1; mul_a < n+1; mul_a++)
			for (mul_b = 1; mul_b < n+1; mul_b++)
			{
				//if (alpha_to[(index_of[mul_a] + index_of[mul_b]) % n] != galua_mult(mul_a, mul_b))
				if	 (galua_mult_single_cycle(mul_a, mul_b) != galua_mult(mul_a, mul_b))
				{
					printf("(mul_a = %d) * (mil_b = %d)\n", mul_a, mul_b);
					printf("galua_mult(mul_a, mul_b) = %d\n",galua_mult(mul_a, mul_b));
					//printf("alpha_to[((index_of[mul_a]=%d) + (index_of[mul_b])=%d) % (n=%d)] = %d\n",index_of[mul_a],index_of[mul_b], n, alpha_to[(index_of[mul_a] + index_of[mul_b]) % n]);
					printf("galua_mult_single_cycle(mul_a, mul_b) = %d\n",galua_mult_single_cycle(mul_a, mul_b));

				}
			}
			// End for the test section
			printf("Done checking multipliers\n");
			getchar(); 
			exit(0);
	}
	if (only_create_mult_equations == 1)
	{
		galua_mult_calc();
		exit(0); // Once done exit the program
	}

	if (calculate_encoding_polynomial == 1)
	{
		encode_bch_calculate_per_byte();
		getchar();
		exit(0);
	}
	if (calculate_syndrome_polynomials == 1)
	{
		calc_galua_syndrome_registers();
		getchar();
		exit(0);
	}

	if (strlen(input_data_file_name)) {
		long file_size;
		long vp_size = k/8 + n2;

		input_file = fopen(input_data_file_name, "rb");
		if (!input_file) {
			printf("Failed opening data input file %s\n", input_data_file_name);
			return EXIT_FAILURE;
		}

		fseek(input_file, 0, SEEK_END);
		file_size = ftell(input_file);
		rewind(input_file);

		if (file_size % vp_size) {
			printf("Input file must be multiple of page+spare size (%ld)\n", vp_size);
			return EXIT_FAILURE;
		}

		iterations_count = file_size / vp_size;
	}

	for (iteration = 0; iteration < iterations_count; iteration++)
	{
		printf("\n\nStarting iteration %d\n\n",iteration);

		if ( print_checksum_for_debug == 1)
		{
			sprintf(FILE_NAME,"%s_checksum_Data.txt",test_name);
			CHECKSUM_FILE=fopen(FILE_NAME,"w");
		}
		
		gen_data(iteration);
		
		if (t != 1)
		{
            encode_bch();			/* encode data using BCH algorithm */
		}
		else
		{
			encode_hamming();       /* encode data using Hamming algorithm */
		}

		/*
		* recd[] are the coefficients of c(x) = x**(length-k)*data(x) + b(x)
		*/
		// At first copy the redundancy data bb[] into the received data recd[];
		// Note that the location of the redundancy is actually index 0..length-k-1
		for	(i = 0;	i <	length - k;	i++)
			recd[i]	= bb[i];
		// Then append to the redundancy the actual data from data[];
		// Note that the location of the actual data is index length-k..k-1
		for	(i = 0;	i <	k; i++)
			recd[i + length	- k] = data[i];
	


		if ((print_data_in_random_run == 1) || (use_random_data == 0))
		{
			printf("Code polynomial:\nc(x) = ");
			for	(i = 0;	i <	length;	i++) {
				printf("%1d", recd[i]);
				if (i && ((i % 50) == 0))
					printf("\n");
			}
			printf("\n");
		}

		if (use_random_data == 1)
		{
			if (limit_errors_count == 0)
			{
				numerr =  (int) ((RANDOM() * (rand_t + 1))/RAND_MAX);
			}
			else
			{
				numerr = errors_count_value;
			}
			printf("Number of errors = %d\n",numerr);
			// At this point we know the number of errors in the test which is needed for the test name.
			// Before we create the errors in the read data, we save the original data for a reference


			// Create a source data file which holds the data and the checksum before errors are added
			if (generate_test_files == 1) // If needed create the test vector reference files
			{
				//sprintf(FILE_NAME,"BCH_test_%d_%d_%d_%d_Source_Data.txt",t,numerr,n1,n1_mask);
				sprintf(FILE_NAME,"%s_Source_Data_vp%04d.txt",test_name,iteration);
				SOURCE_DATA_FILE=fopen(FILE_NAME,"w");
				byte_count_value = 0; // This counter is used to detect the field in use

				temp_byte_value = 0;
				for (i=0; i< length; i++)
				{
					temp_byte_value = recd[length - i - 1]*128 + (temp_byte_value >> 1);
					//fprintf(SOURCE_DATA_FILE,"%d",recd[length - i]);

					if (((i+1) % 8)==0)
					{
						fprintf(SOURCE_DATA_FILE,"%02X",temp_byte_value);
						temp_byte_value = 0;
						fprintf(SOURCE_DATA_FILE," %X\n",byte_type(byte_count_value));
						byte_count_value++;
				//	}
				//	if (byte_count_value == 32)
				//	{
				//		byte_count_value = 0;
				//		fprintf(SOURCE_DATA_FILE,"\n");
					}
				}
				if ((length % 8) != 0)
				{
					temp_byte_value = temp_byte_value >> (8 - (length % 8));
					fprintf(SOURCE_DATA_FILE,"%02X",temp_byte_value);
					fprintf(SOURCE_DATA_FILE," %X\n",byte_type(byte_count_value));
					byte_count_value++;
				}
				// Append n2 data
				temp_byte_value = 0;
				for (i=0; i< (8*(n2 - ecc_checksum_size)); i++)
				{
					temp_byte_value = n2_data[i]*128 + (temp_byte_value >> 1);
					//fprintf(SOURCE_DATA_FILE,"%d",recd[length - i]);

					if (((i+1) % 8)==0)
					{
						fprintf(SOURCE_DATA_FILE,"%02X",temp_byte_value);
						temp_byte_value = 0;
						fprintf(SOURCE_DATA_FILE," %X\n",byte_type(byte_count_value));
						byte_count_value++;
					}
				}
				
				//fprintf(SOURCE_DATA_FILE,"\n");
				fclose(SOURCE_DATA_FILE);

			} // End of reference file creation section

			if(numerr > 0)
			{
				printf("Errors positions are: ");
				for(i=0;i<numerr;i++) // Iterate according to the drawed errors count
				{
					errpos[i] = (int) ((RANDOM() * rand_block_size)/RAND_MAX); // draw a random error location
					printf("%3d ",errpos[i]);
					recd[errpos[i]]	^= 1; // Corrupt the relevant data position
				}
				printf("\n");
			}

		}
		else
		{
			printf("Enter the number of	errors:\n");
			scanf("%d",	&numerr);	/* CHANNEL errors */
			printf("Enter error	locations (integers	between");
			printf(" 0 and %d):	", length-1);

			/*
			* recd[] are the coefficients of r(x) = c(x) +	e(x)
			*/
			for	(i = 0;	i <	numerr;	i++)
				scanf("%d",	&errpos[i]);
			
			// Create a source data file which holds the data and the checksum before errors are added
			if (generate_test_files == 1) // If needed create the test vector reference files
			{
				//sprintf(FILE_NAME,"BCH_test_%d_%d_%d_%d_Source_Data.txt",t,numerr,n1,n1_mask);
				sprintf(FILE_NAME,"%s_Source_Data.txt",test_name);
				SOURCE_DATA_FILE=fopen(FILE_NAME,"w");

				temp_byte_value = 0;
				for (i=0; i< length; i++)
				{
					temp_byte_value = recd[length - i - 1]*128 + (temp_byte_value >> 1);
					//fprintf(SOURCE_DATA_FILE,"%d",recd[length - i]);

					if (((i+1) % 8)==0)
					{
						fprintf(SOURCE_DATA_FILE,"%02X",temp_byte_value);
						temp_byte_value = 0;
						byte_count_value++;
						//fprintf(SOURCE_DATA_FILE,"\n");
					}
					if (byte_count_value == 32)
					{
						byte_count_value = 0;
						fprintf(SOURCE_DATA_FILE,"\n");
					}
				}
				if ((length % 8) != 0)
				{
					temp_byte_value = temp_byte_value >> (8 - (length % 8));
					fprintf(SOURCE_DATA_FILE,"%02X",temp_byte_value);
				}
				fprintf(SOURCE_DATA_FILE,"\n");
				fclose(SOURCE_DATA_FILE);

			} // End of reference file creation section

			if (numerr)
				for	(i = 0;	i <	numerr;	i++)
					recd[errpos[i]]	^= 1;
		}
		errnum_histogram[numerr] +=1; // increment the errors tracing histogram
		if ((print_data_in_random_run == 1) || (use_random_data == 0))
		{
			printf("r(x) = ");
			for	(i = 0;	i <	length;	i++) {
				printf("%1d", recd[i]);
				if (i && ((i % 50) == 0))
					printf("\n");
			}
			printf("\n");
		}
		if (generate_test_files == 1) // If needed create the test vector reference files
		{
			//sprintf(FILE_NAME,"BCH_test_%d_%d_%d_%d_Setup.txt",t,numerr,n1,n1_mask);
			sprintf(FILE_NAME,"%s_Setup.txt",test_name);
			TEST_CONFIGURATION_FILE=fopen(FILE_NAME,"w");
			fprintf(TEST_CONFIGURATION_FILE,"Test name Prefix = %s\n",prefix_string);
			fprintf(TEST_CONFIGURATION_FILE,"Number of correctable errors = %d\n",t);
			fprintf(TEST_CONFIGURATION_FILE,"Number of errors = %d\n",numerr);
			fprintf(TEST_CONFIGURATION_FILE,"Number of protected spare bytes (n1) = %d\n",n1);
			fprintf(TEST_CONFIGURATION_FILE,"Number of masked spare bytes (n1_mask) = %d\n",n1_mask);
			fprintf(TEST_CONFIGURATION_FILE,"Number of non protected spare bytes (n2) = %d\n",n2);
			if (use_determinitstic_seed == 0)
			{
				fprintf(TEST_CONFIGURATION_FILE,"Random seed value = %d\n",seed);
			}
			else
			{
				fprintf(TEST_CONFIGURATION_FILE,"Command line seed value = %d\n",seed);
			}
			fprintf(TEST_CONFIGURATION_FILE,"Errors locations (bits position in the Vector):\n\t ");
			for(i=0;i<numerr;i++)
			{
				fprintf(TEST_CONFIGURATION_FILE,"%d ",errpos[i]);
			}
			fprintf(TEST_CONFIGURATION_FILE,"\n");
			fclose(TEST_CONFIGURATION_FILE);

			//sprintf(FILE_NAME,"BCH_test_%d_%d_%d_%d_Read_Data.txt",t,numerr,n1,n1_mask);
			sprintf(FILE_NAME,"%s_Read_Data.txt",test_name);
			DATA_WITH_ERRORS_FILE=fopen(FILE_NAME,"w");
			temp_byte_value = 0;
			byte_count_value = 0; // An index used to format the reference file
			for(i=0;i<length;i++)
			{
				temp_byte_value = recd[length - i - 1]*128 + (temp_byte_value >> 1);
				//fprintf(DATA_WITH_ERRORS_FILE,"%d",recd[length - i]);

				if (((i + 1) % 8)==0)
				{
					fprintf(DATA_WITH_ERRORS_FILE,"%02X",temp_byte_value);
					temp_byte_value = 0;
					fprintf(DATA_WITH_ERRORS_FILE," %X\n",byte_type(byte_count_value));
					byte_count_value++;
				}
				//if (byte_count_value == 32)
				//{
				//	byte_count_value = 0;
				//	fprintf(DATA_WITH_ERRORS_FILE,"\n");
				//}
			}
			if ((length % 8) != 0)
			{
				temp_byte_value = temp_byte_value >> (8 - (length % 8));
				fprintf(DATA_WITH_ERRORS_FILE,"%02X",temp_byte_value);
				fprintf(DATA_WITH_ERRORS_FILE," %X\n",byte_type(byte_count_value));
				byte_count_value++;
			}
			// Append n2 data
			temp_byte_value = 0;
			for (i=0; i< (8*(n2 - ecc_checksum_size)); i++)
			{
				temp_byte_value = n2_data[i]*128 + (temp_byte_value >> 1);
				//fprintf(DATA_WITH_ERRORS_FILE,"%d",recd[length - i]);

					if (((i+1) % 8)==0)
					{
						fprintf(DATA_WITH_ERRORS_FILE,"%02X",temp_byte_value);
						temp_byte_value = 0;
						fprintf(DATA_WITH_ERRORS_FILE," %X\n",byte_type(byte_count_value));
						byte_count_value++;
					}
				}
			//fprintf(DATA_WITH_ERRORS_FILE,"\n");
			fclose(DATA_WITH_ERRORS_FILE);

		} // End of reference file creation section

		if (dsp_decode_mode == 1)
		{
			if (t != 1)
			{
                dsp_decode_bch(); /* Decode using DSPG method */
			//decode_bch();
			}
			else
			{
				decode_hamming();
			}
		}
		else
		{	
			if (t != 1)
			{

				decode_bch();			  /* DECODE	received codeword recv[] */
			}
			else
			{
				decode_hamming();
			}
		}

		if (generate_test_files == 1) // If needed create the test vector reference files
		{
			//sprintf(FILE_NAME,"BCH_test_%d_%d_%d_%d_Corrected_Data.txt",t,numerr,n1,n1_mask);
			sprintf(FILE_NAME,"%s_Corrected_Data.txt",test_name);
			DATA_OUT_FIXED_FILE=fopen(FILE_NAME,"w");
			temp_byte_value = 0;
			byte_count_value = 0; // An index used to format the reference file
			for(i=0;i<length;i++)
			{
				temp_byte_value = recd[length - i - 1]*128 + (temp_byte_value >> 1);
				//fprintf(DATA_OUT_FIXED_FILE,"%d",recd[length - i]);

				if (((i + 1) % 8)==0)
				{
					fprintf(DATA_OUT_FIXED_FILE,"%02X",temp_byte_value);
					temp_byte_value = 0;
					fprintf(DATA_OUT_FIXED_FILE," %X\n",byte_type(byte_count_value));
					byte_count_value++;
				}
				//if (byte_count_value == 32)
				//{
				//	byte_count_value = 0;
				//	fprintf(DATA_OUT_FIXED_FILE,"\n");
				//}
			}
			if ((length % 8) != 0)
			{
				temp_byte_value = temp_byte_value >> (8 - (length % 8));
				fprintf(DATA_OUT_FIXED_FILE,"%02X",temp_byte_value);
				fprintf(DATA_OUT_FIXED_FILE," %X\n",byte_type(byte_count_value));
				byte_count_value++;
			}
				// Append n2 data
				temp_byte_value = 0;
				for (i=0; i< (8*(n2 - ecc_checksum_size)); i++)
				{
					temp_byte_value = n2_data[i]*128 + (temp_byte_value >> 1);
					//fprintf(DATA_OUT_FIXED_FILE,"%d",recd[length - i]);

					if (((i+1) % 8)==0)
					{
						fprintf(DATA_OUT_FIXED_FILE,"%02X",temp_byte_value);
						temp_byte_value = 0;
						fprintf(DATA_OUT_FIXED_FILE," %X\n",byte_type(byte_count_value));
						byte_count_value++;
					}
				}
			//fprintf(DATA_OUT_FIXED_FILE,"\n");
			fclose(DATA_OUT_FIXED_FILE);
		}
		if ((print_data_in_random_run == 1) || (use_random_data == 0))
		{
			/*
			* print out original and decoded data
			*/
			printf("Results:\n");
			printf("original data  = ");
			for	(i = 0;	i <	k; i++)	{
				printf("%1d", data[i]);
				if (i && ((i % 50) == 0))
					printf("\n");
			}
			printf("\nrecovered	data = ");
			for	(i = length	- k; i < length; i++) {
				printf("%1d", recd[i]);
				if ((i-length+k) &&	(((i-length+k) % 50) ==	0))
					printf("\n");
			}
			printf("\n");
		}
		/*
		* DECODING	ERRORS?	we compare only	the	data portion
		*/
		if (numerr <= t)
		{
			for	(i = length	- k; i < length; i++) 
			{
				if (((length -1 - i) < 4096) | ((length - 1 - i) > (4095 + (8*n1_mask)))) // Only check for errors outside the masked area
				{
					if (data[i - length	+ k] !=	recd[i])
					{
						decerror++;
						printf("Decoding error was found at location %d which is Byte %d\n", i, ((length - i - 1) / 8));
					}
				}
				else
				{
					if (data[i - length	+ k] !=	recd[i])
					{
						printf("Masked bit %d is not checked as it resides on Byte %d\n", i, ((length - i - 1) / 8));
					}
				}
			}
			if (decerror)
			{
				printf("There were %d decoding errors in	message	positions\n", decerror);
			}
			else
				printf("Succesful decoding\n");
		}
		if ((Pause_Between_Iterations == 1) || ((pause_on_Errors == 1) && (decerror !=0 )))
		{
			getchar();              // just for debug stop every iteration
			decerror = 0;

		}
		
		if ( print_checksum_for_debug == 1)
		{
			fclose(CHECKSUM_FILE);
		}
	}
	if (generate_test_files == 0)
	{
		if (use_random_data == 1)
		{
			for(i=0; i< (rand_t+2); i++)
			{
				printf("err_num_histogram[%2d]=%d\n",i,errnum_histogram[i]);
			}
		}
		printf("Press Enter to continue");
		getchar(); 
	}
	return 0;
}
