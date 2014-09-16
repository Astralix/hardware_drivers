//--< Nevada_dpdBlocks.c >-------------------------------------------------------------------------
//=================================================================================================
//
//  Nevada - DFS Radar Detection
//           
//  Developed by Igor Rudakov and Ariel Feldman
//  Copyright 2010 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include "Nevada_dpdBlocks.h"
#include <stdio.h>
#include <stdlib.h>

#include "Nevada.h"
#include "wiUtil.h"

#define NUM_OF_FIRS 3


static dpdBlock **DMW_DPD = (dpdBlock **)NULL;

static dpdInt FIR1[] = {1476, -789, 231, -88, 923, -483, 1634, -1019, -834, 527};
static dpdInt FIR3[] = {681, -1701, 397, -514, -254, -1054};
static dpdInt FIR5[] = {89, 503, 104, 101, 32, 812};

static dpdInt *FIR_BANK[] = {FIR1, FIR3, FIR5};



#define VERBOSE_MODE 0
#if VERBOSE_MODE
#define NUM_OF_FILES 6
#define NUM_OF_FILES4 4
char *filenames[] = {"dpdvc_p1_r.dat", "dpdvc_p1_i.dat", "dpdvc_p3_r.dat", "dpdvc_p3_i.dat", "dpdvc_p5_r.dat", "dpdvc_p5_i.dat"};
char *filenames2[] = {"dpdvc_f1_r.dat", "dpdvc_f1_i.dat", "dpdvc_f3_r.dat", "dpdvc_f3_i.dat", "dpdvc_f5_r.dat", "dpdvc_f5_i.dat"};
char *filenames3[] = {"dpdvc_shf1_r.dat", "dpdvc_shf1_i.dat", "dpdvc_shf3_r.dat", "dpdvc_shf3_i.dat", "dpdvc_shf5_r.dat", "dpdvc_shf5_i.dat"};
char *filenames4[] = {"dpdvc_bfout_r.dat", "dpdvc_bfout_i.dat","dpdvc_out_r.dat", "dpdvc_out_i.dat"};
#endif // VERBOSE_MODE



dpdComplexInt Nevada_dpdRealMultInt(dpdComplexInt a, dpdRealInt b)
{
	dpdComplexInt result;

	result.Precision = a.Precision + b.Precision;
	result.Real = a.Real * b.Value;
	result.Imag = a.Imag * b.Value;

	// restrictPrecComplex(&result);
	return result;
}



dpdRealInt Nevada_dpdRealInit(dpdInt a, dpdUInt b)
{
    dpdRealInt result = {{a, b}};

	// restrictPrecReal(&result);
	return result;
}


dpdInt Nevada_convert_to_signed(dpdInt indata, int bits)
{
	dpdUInt NORM, HNORM;
	dpdInt out;

	NORM = 1<<bits;
	HNORM = NORM>>1;

	out = indata + HNORM;
	out = out%NORM;
	out -= HNORM;

	return out;

}


int Nevada_SATURATE(int a, int LIMIT)
{
	a = a>LIMIT? LIMIT : a;
	a = a<-LIMIT? -LIMIT : a;
	return a;
}

int Nevada_SATURATE2(int a, int LIMIT)
{
	a = a>LIMIT? LIMIT : a;
	a = a<-(LIMIT+1)? -(LIMIT+1) : a;
	return a;
}

/*
static void print_bcoeff(DELAY_LINE_COMPLEX *filter)
{
    int i;
    printf("\n==================\n");
    printf("filter size: %d\n\n", filter->size);
    
    for (i=0; i<filter->size; i++)
    {
	//	stam = filter->b_coeff[0].dat[0];
        printf("\tb_coeff[%d].re = %d\n", i, DPD_COMPLEX_REAL(filter->b_coeff[i]));
        printf("\tb_coeff[%d].im = %d\n", i, DPD_COMPLEX_IMAG(filter->b_coeff[i]));
        printf("\tb_coeff[%d].pr = %d\n", i, DPD_COMPLEX_PREC(filter->b_coeff[i]));
        printf("\n");
    }
}
*/

dpdStatus Nevada_dpdBlock_CleanFilters(int dpd_block_id)
{
	int i;
	dpdBlock *cdpd;
	DELAY_LINE_COMPLEX *delay_line;

	// Getting a pointer the current DPD block
	cdpd = DMW_DPD[dpd_block_id];

	for (i=0; (unsigned)i<cdpd->num_of_firs; i++)
	{
		delay_line = cdpd->VFP.vfir[i]->fir;
		Nevada_clean_delay_line(delay_line);
	}

	return dpd_success;

}

dpdStatus Nevada_dpdBlock_Close(int dpd_block_id)
{
	unsigned int i;
	dpdBlock *cdpd;

	cdpd = DMW_DPD[dpd_block_id];


	for (i=0; i<cdpd->num_of_firs; i++)
	{
		wiFree(cdpd->VFP.vfir[i]->fir->b_coeff);
		wiFree(cdpd->VFP.vfir[i]->fir->delay_line);
		wiFree(cdpd->VFP.vfir[i]->fir);
		wiFree(cdpd->VFP.vfir[i]);
	}
    wiFree(cdpd->VFP.vfir); // [100309 BB] added missing free of this allocated memory
	wiFree(cdpd);
	wiFree(DMW_DPD);

	
	return dpd_success;

}

dpdStatus Nevada_dpdBlock_SetFilterCoeff(int dpd_block_id, int bank_set)
{
	unsigned int i, j;
	int coeffr, coeffi;
	int precision;
	dpdComplexInt *pb_coeff;
	dpdBlock *cdpd;
	wiWORD *FILTER_POINTER;
    NevadaRegisters_t *pReg = Nevada_Registers();

    // [Barrett-110214] Changed the type of FILTER_POINTER from wiUWORD to wiWORD to match
    // the type in the registers structure. However, I think the pointer arithmetic below
    // is a little dangerous because the registers are contained in a structure and not an
    // array. It is possible for the compiler to put space between elements to align with,
    // for example, 64 bit boundaries.
    //
	FILTER_POINTER = (wiWORD *)&pReg->P1_coef_t0_r_s0;
	FILTER_POINTER += 22*bank_set;
	cdpd = DMW_DPD[dpd_block_id];

	for (i=0; i<cdpd->num_of_firs; i++)
	{
		precision = cdpd->VFP.vfir[i]->precision;
		for (j=0; j<cdpd->VFP.vfir[i]->taps; j++)
		{
			coeffr = FILTER_POINTER->word;
			coeffr = Nevada_convert_to_signed(coeffr, precision);
			FILTER_POINTER++;
			coeffi = FILTER_POINTER->word;
			coeffi = Nevada_convert_to_signed(coeffi, precision);
			FILTER_POINTER++;
			
			pb_coeff = &(cdpd->VFP.vfir[i]->fir->b_coeff[j]);
			DPD_SET_COMPLEX_INT_PR(pb_coeff, coeffr, coeffi, precision);
		}
	}

	return dpd_success;
}

dpdStatus Nevada_dpdBlock_Init(int dpd_block_id)
{
	unsigned int i;
	dpdBlock *cdpd;
//	void *tmp;

	// allocate memory to keep DPD control structures
	DMW_DPD = (dpdBlock **)wiRealloc(DMW_DPD, (dpd_block_id+1)*sizeof(dpdBlock **));
	cdpd= (dpdBlock *)wiMalloc(sizeof(dpdBlock));
	DMW_DPD[dpd_block_id] = cdpd;

	// init Higher Order Block
	cdpd->VFP.dpdHigherOrder.dpd_in_precision = 10;
	cdpd->VFP.dpdHigherOrder.shift_right1 = 11;
	cdpd->VFP.dpdHigherOrder.shift_right2 = 9;
	cdpd->VFP.dpdHigherOrder.shift_right3 = 7;
	cdpd->VFP.dpdHigherOrder.shift_right4 = 5;

	cdpd->VFP.firNormalization.shift_right[0] = 12;
	cdpd->VFP.firNormalization.shift_right[1] = 14;
	cdpd->VFP.firNormalization.shift_right[2] = 11;


	// init FIRs
	cdpd->num_of_firs = NUM_OF_FIRS;
	cdpd->VFP.vfir = (dpdFIR **)wiCalloc(cdpd->num_of_firs, sizeof(dpdFIR *));
	for (i=0; i<cdpd->num_of_firs; i++)	cdpd->VFP.vfir[i] = (dpdFIR *)wiMalloc(sizeof(dpdFIR));

	INIT_VFP_FIR(cdpd->VFP.vfir[0], 5, 13);			// 1st order FIR: 5 taps, precision 13
	INIT_VFP_FIR(cdpd->VFP.vfir[1], 3, 15);			// 3rd order FIR: 3 taps, precision 15
	INIT_VFP_FIR(cdpd->VFP.vfir[2], 3, 12);			// 5th order FIR: 3 taps, precision 12
	Nevada_dpdVFIRs_Init(cdpd);

	return dpd_success;
}

dpdBlock *Nevada_get_dpdBlock(int dpd_block_id)
{
	return DMW_DPD[dpd_block_id];
}

dpdStatus Nevada_dpdVFIRs_Init(dpdBlock *cdpd)
{
	unsigned int i;
	dpdComplexInt *dl;
	dpdComplexInt *coeff;
	DELAY_LINE_COMPLEX *tmp_delay_line;

	for (i=0; i<cdpd->num_of_firs; i++)
	{
		// cdpd->VFP.vfir[i]->coeff = (dpdComplexInt *)calloc(cdpd->VFP.vfir[i]->taps, sizeof(dpdComplexInt));
		tmp_delay_line = (DELAY_LINE_COMPLEX *)wiMalloc(sizeof(DELAY_LINE_COMPLEX));
		dl = (dpdComplexInt *)wiCalloc(cdpd->VFP.vfir[i]->taps, sizeof(dpdComplexInt));
		coeff = (dpdComplexInt *)wiCalloc(cdpd->VFP.vfir[i]->taps, sizeof(dpdComplexInt));


		DPD_SET_BUFFER(coeff, FIR_BANK[i], cdpd->VFP.vfir[i]->precision, cdpd->VFP.vfir[i]->taps);
		INIT_DELAY_LINE(tmp_delay_line, dl, coeff, (dpdComplexInt *)NULL, cdpd->VFP.vfir[i]->taps);
		cdpd->VFP.vfir[i]->fir = tmp_delay_line;

#if VERBOSE_MODE
		print_bcoeff(cdpd->VFP.vfir[i]->fir);
		print_delay_line(cdpd->VFP.vfir[i]->fir);
#endif // VERBOSE_MODE

	}

	return dpd_success;
}




dpdStatus Nevada_dpdVFP(int dpd_block_id, dpdComplexInt *dpd_in, int plen, dpdComplexInt *dpd_out)
{
	int i, j;
//	int block_size = 1;
//	dpdUInt precision = 0;
	dpdBlock *cdpd;
	dpdComplexInt **dpd_in_bank;
	// dpdComplexInt *fir_output;
	// dpdComplexInt fir_output[NUM_OF_FIRS][23000];
	dpdComplexInt **fir_output;
	dpdComplexInt fir_output_shifted[NUM_OF_FIRS];
	DELAY_LINE_COMPLEX *dline;
    dpdComplexInt fir_sum = {{0, 0, 0}};  //  Real = 0; Imag = 0; Prec = 0
	dpdComplexInt *tmpbuffer;

#if VERBOSE_MODE
	int k;
	FILE *fid[NUM_OF_FILES];
	FILE *fid2[NUM_OF_FILES];
	FILE *fid3[NUM_OF_FILES];
	FILE *fid4[NUM_OF_FILES4];
	for (k=0; k<NUM_OF_FILES; k++)
	{
		fid[k] = fopen(filenames[k], "w");
		fid2[k] = fopen(filenames2[k], "w");
		fid3[k] = fopen(filenames3[k], "w");
	}

	for (k=0; k<NUM_OF_FILES4; k++)
	{
		fid4[k] = fopen(filenames4[k], "w");
	}



#endif // VERBOSE_MODE

	// Getting a pointer the current DPD block
	cdpd = DMW_DPD[dpd_block_id];

	// array to keep the output of the firs (currently 3)
	// fir_output = (dpdComplexInt *)calloc(cdpd->num_of_firs, sizeof(dpdComplexInt));
	fir_output = (dpdComplexInt **)wiCalloc(cdpd->num_of_firs, sizeof(dpdComplexInt *));



	// dpd_in_bank[0] - 1st order; dpd_in_bank[1] - 3rd order, dpd_in_bank[2] - 5th order
	dpd_in_bank = (dpdComplexInt **)wiCalloc(cdpd->num_of_firs, sizeof(dpdComplexInt *));
	dpd_in_bank[0] = dpd_in;
	fir_output[0] = (dpdComplexInt *)wiCalloc(plen, sizeof(dpdComplexInt));
	for (i=1; (unsigned int)i<cdpd->num_of_firs; i++)
	{
		// initialize dpd_in_p3 & dpd_in_p5 storage buffers
		dpd_in_bank[i] = (dpdComplexInt *)wiCalloc(plen, sizeof(dpdComplexInt));
		fir_output[i] = (dpdComplexInt *)wiCalloc(plen, sizeof(dpdComplexInt));

	}



	// processing the block of length PLEN
	// for (i=0; i<plen; i++)
	// {
		// internal loop is done by block_size of 1 - step by step processing
		// although vector processing is supported
		// calculating the higher order inputs
		Nevada_dpdHighOrder(cdpd, dpd_in_bank, plen, 0);
	// }
	//for (i=0; i</*plen*/1; i++)
	//{
		for (j=0; (unsigned)j<cdpd->num_of_firs; j++)
		{
			dline = Nevada_get_dpdBlock(dpd_block_id)->VFP.vfir[j]->fir;		// pointer to the delay line of corresponeded filter
			tmpbuffer = dpd_in_bank[j];									// pointer to the input data
			Nevada_complex_fir(dline, tmpbuffer, fir_output[j], plen );			// filter
		}
			for (i=0; i<plen; i++)
			{
				for (j=0; (unsigned)j<cdpd->num_of_firs; j++)
				{
			// store the result
			// fir_output_shifted[j] = dpdComplexShiftR(fir_output[j][i], cdpd->VFP.firNormalization.shift_right[j]);
			fir_output_shifted[j].Real = fir_output[j][i].Real >> cdpd->VFP.firNormalization.shift_right[j];
			fir_output_shifted[j].Imag = fir_output[j][i].Imag >> cdpd->VFP.firNormalization.shift_right[j];
			// fir_sum = dpdComplexAddInt(fir_sum, fir_output_shifted[j]);			// calculate the final result
			fir_sum.Real += fir_output_shifted[j].Real;
			fir_sum.Imag += fir_output_shifted[j].Imag;
				
			//dpd_out[i] = fir_sum;
			dpd_out[i].Real = Nevada_SATURATE2(fir_sum.Real, (1<<(cdpd->VFP.dpdHigherOrder.dpd_in_precision-1))-1);
			dpd_out[i].Imag = Nevada_SATURATE2(fir_sum.Imag, (1<<(cdpd->VFP.dpdHigherOrder.dpd_in_precision-1))-1);

#if VERBOSE_MODE
			// high order output
			fprintf(fid[2*j], "%d\n", tmpbuffer[i].Real);
			fprintf(fid[2*j+1], "%d\n", tmpbuffer[i].Imag);

			// filters output
			fprintf(fid2[2*j], "%d\n", fir_output[j].Real);
			fprintf(fid2[2*j+1], "%d\n", fir_output[j].Imag);

			// shifted output
			fprintf(fid3[2*j], "%d\n", fir_output_shifted[j].Real);
			fprintf(fid3[2*j+1], "%d\n", fir_output_shifted[j].Imag);
#endif // VERBOSE_MODE
		}

#if VERBOSE_MODE
		// output
		fprintf(fid4[0], "%d\n", fir_sum.Real);
		fprintf(fid4[1], "%d\n", fir_sum.Imag);
#endif // VERBOSE_MODE

		// 10-bit restriction
		// precision = fir_sum.Precision - cdpd->num_of_firs + (int)ceil(log(cdpd->num_of_firs)/log(2)) + 1;
//		precision = cdpd->VFP.dpdHigherOrder.dpd_in_precision;
		// setComplexIntPrecision(&fir_sum, precision);
		
#if VERBOSE_MODE
		// output
		fprintf(fid4[2], "%d\n", fir_sum.Real);
		fprintf(fid4[3], "%d\n", fir_sum.Imag);
#endif // VERBOSE_MODE

		DPD_SET_COMPLEX_INT_PR(&fir_sum, 0, 0, 0);

		
	}



	for (i=1; (unsigned int)i<cdpd->num_of_firs; i++)
	{
		// free the memory
		wiFree((void *)dpd_in_bank[i]);          
		wiFree(fir_output[i]);
	}
    wiFree(fir_output[0]);
	wiFree(dpd_in_bank);
	wiFree(fir_output);
	return dpd_success;

}




void Nevada_clean_delay_line(DELAY_LINE_COMPLEX *filter)
{
    int i;
    for (i=0; i<filter->size; i++)
    {
       DPD_SET_COMPLEX_INT_PR(&filter->delay_line[i], 0, 0, 0);
    }
    filter->pointer = 0;
}



/*
static void print_delay_line(DELAY_LINE_COMPLEX *filter)
{
    int i;
    printf("\n==================\n");
    printf("filter size: %d\n", filter->size);
    printf("Delay line position: %d\n\n", filter->pointer);
    
    for (i=0; i<filter->size; i++)
    {
        if (i==filter->pointer) printf("DELAYLINE START =>\n");
        printf("\tdelay_line[%d].re = %d\n", i, DPD_COMPLEX_REAL(filter->delay_line[i]));
        printf("\tdelay_line[%d|.im = %d\n", i, DPD_COMPLEX_IMAG(filter->delay_line[i]));
        printf("\tdelay_line[%d].pr = %d\n", i, DPD_COMPLEX_PREC(filter->delay_line[i]));
        printf("\n");
    }
}
*/



dpdStatus Nevada_complex_fir(DELAY_LINE_COMPLEX *localdl, dpdComplexInt *p_newsample, dpdComplexInt *outbuffer, int block_size)
{
    int i;
    int j;
    int ptr;
//	unsigned int precision = 0;
    dpdComplexInt tmp1;
	    dpdComplexInt *p_output;
    //DPD_SET_COMPLEX_INT_PR(p_output, 0, 0, 0);

	
 for (j=0; j<block_size; j++)
    {


    localdl->delay_line[localdl->pointer] = (*(p_newsample+j)); 
	ptr = localdl->pointer;
   
	p_output = outbuffer + j;
			//DPD_SET_COMPLEX_INT_PR(p_output, 0, 0, 0);
	p_output->Real = 0;
	p_output->Imag = 0;

    for(i=0; i<localdl->size; i++)
    {
        //tmp1 = dpdComplexMultInt(localdl->b_coeff[i], localdl->delay_line[ptr]);
		//  tmp1.Real = COMPLEX_MULT_REAL(localdl->b_coeff[i], localdl->delay_line[ptr]);
		tmp1.Real = localdl->b_coeff[i].Real*localdl->delay_line[ptr].Real - localdl->b_coeff[i].Imag*localdl->delay_line[ptr].Imag;
		tmp1.Imag = localdl->b_coeff[i].Real*localdl->delay_line[ptr].Imag + localdl->b_coeff[i].Imag*localdl->delay_line[ptr].Real;
        // *p_output = dpdComplexAddInt(*p_output, tmp1);
		p_output->Real = p_output->Real + tmp1.Real;
		p_output->Imag = p_output->Imag + tmp1.Imag;
        ptr--;
        if (ptr<0) ptr = localdl->size-1;

		// precision = MAX(precision, tmp1.Precision);
    }

	// precision = p_output->Precision - localdl->size + (int)ceil(log(localdl->size)/log(2)) + 1;
	//setComplexIntPrecision(p_output, precision);
        
    localdl->pointer++;
    if (localdl->pointer==localdl->size) localdl->pointer = 0;
    }

	return dpd_success;
}



dpdStatus Nevada_dpdHighOrder(dpdBlock *DPD, dpdComplexInt **dpd_in_bank, int plen, int start_index)
{
	int i;

	dpdComplexInt *DPD_IN, *DPD_IN_P3, *DPD_IN_P5;
	dpdComplexInt quad, ci1, ci2, ci3;
	dpdRealInt ri1, ri2, ri3;

	DPD_IN = dpd_in_bank[0];
	DPD_IN_P3 = dpd_in_bank[1];
	DPD_IN_P5 = dpd_in_bank[2];

	for (i=start_index; i<(start_index+plen); i++)
	{
		// quad = dpdComplexQuadInt(DPD_IN[i]);
		quad.Real = DPD_IN[i].Real * DPD_IN[i].Real;
		quad.Imag = DPD_IN[i].Imag * DPD_IN[i].Imag;
		//setComplexIntPrecision(&quad, quad.Precision-2);			// input data in the range [-511;511]

		// ci1 = dpdComplexShiftR(quad, DPD->VFP.dpdHigherOrder.shift_right1);
		ci1.Real = quad.Real >> DPD->VFP.dpdHigherOrder.shift_right1;
		ci1.Imag = quad.Imag >> DPD->VFP.dpdHigherOrder.shift_right1;

		ri1 = Nevada_dpdRealInit(DPD_COMPLEX_REAL(ci1) + DPD_COMPLEX_IMAG(ci1), 8 /*ci1.Precision+1*/);
		// ri2 = dpdRealRealMultInt(ri1, ri1);
		ri2.Value = ri1.Value * ri1.Value;
		// ri3 = dpdRealShiftR(ri2, DPD->VFP.dpdHigherOrder.shift_right2);
		ri3.Value = ri2.Value >> DPD->VFP.dpdHigherOrder.shift_right2;
		ri3.Precision = 7;


		ci2 = Nevada_dpdRealMultInt(DPD_IN[i], ri1);

		// DPD_IN_P3[i] = dpdComplexShiftR(ci2, DPD->VFP.dpdHigherOrder.shift_right3);
		DPD_IN_P3[i].Real = ci2.Real >> DPD->VFP.dpdHigherOrder.shift_right3;
		DPD_IN_P3[i].Imag = ci2.Imag >> DPD->VFP.dpdHigherOrder.shift_right3;

		
		ci3 = Nevada_dpdRealMultInt(DPD_IN[i], ri3);
		// DPD_IN_P5[i] = dpdComplexShiftR(ci3, DPD->VFP.dpdHigherOrder.shift_right4);
		DPD_IN_P5[i].Real  = ci3.Real >> DPD->VFP.dpdHigherOrder.shift_right4;
		DPD_IN_P5[i].Imag  = ci3.Imag >> DPD->VFP.dpdHigherOrder.shift_right4;
	}

	return dpd_success;
}


// dpdComplextInt fir1
