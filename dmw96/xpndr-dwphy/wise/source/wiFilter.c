//--< wiFilter.c >---------------------------------------------------------------------------------
//=================================================================================================
//
//  WiSE Digital Filter Library
//  Copyright 2001 Bermai, Inc. All rights reserved.
//      Also Copyright 1998 Barrett Brickner. All Rights Reserved.
//
//=================================================================================================

#include <math.h>
#include "wiFilter.h"
#include "wiUtil.h"

static const double pi = 3.14159265358979323846;

/*===========================================================================*/
/*--  LOCAL FUNCTION PROTOTYPES                                              */
/*===========================================================================*/
wiStatus wiFilter_Digital_ImpulseInvariance(int N, int M, double fc, double T, double Gs, wiComplex Ps[], wiComplex Zs[], double Bz[], double Az[]);
wiStatus wiFilter_Digital_BilinearTransform(int N, int M, double fc, double T, double Gs, wiComplex Ps[], wiComplex Zs[], double Bz[], double Az[]);
wiStatus wiFilter_FrequencyScale_s_NRP(double fc, int Nz, wiComplex z[], int  Np, wiComplex p[], double G[]);
wiStatus wiFilter_PartialFractionExpansion_NRP(int Nz, wiComplex z[], int Np, wiComplex p[], double G, wiComplex C[]);
wiStatus wiFilter_PoleZero_to_Polynomial_NRP(int N, wiComplex C[], wiComplex Q[], wiComplex P[], wiComplex B[], wiComplex A[]);
wiStatus wiFilter_DigitalFilterDesign(int  N, int  M,  double fc, double T, double Gs, wiComplex Ps[], wiComplex Zs[], double Bz[], double Az[], int method);
wiStatus wiFilter_ComplexDivision(wiComplex a, wiComplex b, wiComplex *c);

/*===========================================================================*/
/*--  INTERNAL ERROR HANDLING                                                */
/*===========================================================================*/
#define STATUS(arg)  WICHK(arg)
#define XSTATUS(arg) if (STATUS(arg)<0) return -1;

/*****************************************************************************/
/*=== GLOBAL (Exported) FUNCTIONS ===========================================*/
/*****************************************************************************/

/*============================================================================+
|  Function  : wiFilter_IIR()                                                 |
|-----------------------------------------------------------------------------|
|  Purpose   : Implement a digital IIR filter,                                |
|  Parameters: N     -- Filter order                                          |
|              b     -- Numerator polynomial B(D), must contain N+1 terms     |
|              a     -- Denominator polynomial A(D), must contain N+1 terms   |
|              Nx    -- Number of samples in x to process                     |
|              x     -- Array of filter input samples (length Nx)             |
|              y     -- Array for filter output samples (length Nx)           |
|              w0    -- Optional initial conditions for the internal shift    |
|                       register of a canonic direct for implementation. The  |
|                       length must be N if used; otherwise pass NULL.        |
+============================================================================*/
wiStatus wiFilter_IIR(int N, double b[], double a[], int Nx, double x[], double y[], double w0[])
{
    int    n, k;       // sample and loop indices
    int    Nw;         // number of delays
    int    Na = N+1;   // number of terms in the denominator (a)
    int    Nb = N+1;   // number of terms in the numerator (b)
    double w[32];      // filter delay line

    // ------------------------
    // Error and Range Checking
    // ------------------------
    if (InvalidRange(N,1,99)) return WI_ERROR_PARAMETER1;
    if (b == NULL)            return WI_ERROR_PARAMETER2;
    if (a == NULL)            return WI_ERROR_PARAMETER3;
    if (Nx < 1)               return WI_ERROR_PARAMETER4;
    if (x == NULL)            return WI_ERROR_PARAMETER5;
    if (y == NULL)            return WI_ERROR_PARAMETER6;

    Nw = ((Na>Nb)? Na:Nb);  // order of the longest delay element (D^Nw)

    // -----------------------------------------------
    // Initialize the filter's internal shift register
    // -----------------------------------------------
    if (w0) 
    {
        for (k=1; k<=Nw; k++) w[k] = w0[k];  // specified initial conditions
    }
    else 
    {
        for (k=1; k<=Nw; k++) w[k] = 0.0;    // zeros for unspecified initial cond.
    }
    // ------------------------------------------------------------------
    // Decrease polynomial lengths to skip high order terms equal to zero
    // ------------------------------------------------------------------
    while ((Nb>0) && (b[Nb-1]==0.0)) Nb--;
    while ((Na>0) && (a[Na-1]==0.0)) Na--;

    // ---------------------------------------------------
    // Verify that the individual polynomials are not zero
    // ---------------------------------------------------
    if (Nb == 0) return WI_ERROR_PARAMETER2;
    if (Na == 0) return WI_ERROR_PARAMETER3;

    // ---------------------------------------------------
    // Normalize so A[0]=1; this eliminates one multiplier
    // ---------------------------------------------------
    if (a[0] != 1.0) 
    {
        for (k=0; k<Nb; k++) b[k] = b[k] / a[0];
        for (k=1; k<Na; k++) a[k] = a[k] / a[0];
        a[0] = 1.0;
    }
    // -------------------------------------------------------------
    // IIR filter using the direct form II implementation [1, p.296]
    //
    //              N-1      
    //    w[n] = SUMMATION { a[n]*w[n-k] + x[n] }
    //              k=0
    //
    //              N-1
    //    y[n] = SUMMATION { b[n]*w[n-k] }
    //              k=0
    //
    // -------------------------------------------------------------
    for (n=0; n<Nx; n++)
    {
        w[0] = x[n];
        y[n] = 0.0;

        for (k=1; k<Na; k++)
            w[0] -= a[k] * w[k];

        for (k=0; k<Nb; k++)
            y[n] += b[k] * w[k];

        for (k=Nw; k>0; k--)
            w[k] = w[k-1];
    }
    // --------------------------------------------------
    // If an initial state was provided, return the final
    // shift-register state in the w0[] array.
    // --------------------------------------------------
    if (w0) 
    {
        for (k=1; k<=Nw; k++) w0[k] = w[k];
    }
    return WI_SUCCESS;
}
// end of wiFilter_IIR()

/*============================================================================+
|  Function  : wiFilter_IIR_ComplexInput()                                    |
|-----------------------------------------------------------------------------|
|  Purpose   : Implement a digital IIR filter for complex input               |
|  Parameters: N     -- Filter order                                          |
|              b     -- Numerator polynomial B(D), must contain N+1 terms     |
|              a     -- Denominator polynomial A(D), must contain N+1 terms   |
|              Nx    -- Number of samples in x to process                     |
|              x     -- Array of filter input samples (length Nx)             |
|              y     -- Array for filter output samples (length Nx)           |
|              w0    -- Optional initial conditions for the internal shift    |
|                       register of a canonic direct for implementation. The  |
|                       length must be N if used; otherwise pass NULL.        |
+============================================================================*/
wiStatus wiFilter_IIR_ComplexInput(int N, double b[], double a[], int Nx, 
                                         wiComplex x[], wiComplex y[], wiComplex w0[])
{
    int       n, k;     // sample and loop indices
    int       Nw;       // number of delays
    int       Na = N+1; // number of terms in the denominator (a)
    int       Nb = N+1; // number of terms in the numerator (b)
    wiComplex w[32];    // filter delay line

    // ------------------------
    // Error and Range Checking
    // ------------------------
    if (InvalidRange(N,1,99)) return WI_ERROR_PARAMETER1;
    if (b == NULL)            return WI_ERROR_PARAMETER2;
    if (a == NULL)            return WI_ERROR_PARAMETER3;
    if (Nx < 1)               return WI_ERROR_PARAMETER4;
    if (x == NULL)            return WI_ERROR_PARAMETER5;
    if (y == NULL)            return WI_ERROR_PARAMETER6;

    Nw = ((Na>Nb)? Na:Nb);  // order of the longest delay element (D^Nw)

    // -----------------------------------------------
    // Initialize the filter's internal shift register
    // -----------------------------------------------
    if (w0) 
    {
        for (k=1; k<=Nw; k++) {
            w[k].Real = w0[k].Real;
            w[k].Imag = w0[k].Imag;
        }
    }
    else 
    {
        for (k=1; k<=Nw; k++) 
            w[k].Real = w[k].Imag = 0.0;    // zeros for unspecified initial cond.
    }
    // ------------------------------------------------------------------
    // Decrease polynomial lengths to skip high order terms equal to zero
    // ------------------------------------------------------------------
    while ((Nb>0) && (b[Nb-1]==0.0)) Nb--;
    while ((Na>0) && (a[Na-1]==0.0)) Na--;

    // ---------------------------------------------------
    // Verify that the individual polynomials are not zero
    // ---------------------------------------------------
    if (Nb == 0) return WI_ERROR_PARAMETER2;
    if (Na == 0) return WI_ERROR_PARAMETER3;

    // ---------------------------------------------------
    // Normalize so A[0]=1; this eliminates one multiplier
    // ---------------------------------------------------
    if (a[0] != 1.0) 
    {
        for (k=0; k<Nb; k++) b[k] = b[k] / a[0];
        for (k=1; k<Na; k++) a[k] = a[k] / a[0];
        a[0] = 1.0;
    }
    // -------------------------------------------------------------
    // IIR filter using the direct form II implementation [1, p.296]
    //
    //              N-1      
    //    w[n] = SUMMATION { a[n]*w[n-k] + x[n] }
    //              k=0
    //
    //              N-1
    //    y[n] = SUMMATION { b[n]*w[n-k] }
    //              k=0
    //
    // -------------------------------------------------------------
    for (n=0; n<Nx; n++)
    {
        w[0].Real = x[n].Real;
        w[0].Imag = x[n].Imag;

        y[n].Real = y[n].Imag = 0.0;

        for (k=1; k<Na; k++)
        {
            w[0].Real -= a[k] * w[k].Real;
            w[0].Imag -= a[k] * w[k].Imag;
        }
        for (k=0; k<Nb; k++)
        {
            y[n].Real += b[k] * w[k].Real;
            y[n].Imag += b[k] * w[k].Imag;
        }
        for (k=Nw; k>0; k--)
        {
            w[k].Real = w[k-1].Real;
            w[k].Imag = w[k-1].Imag;
        }
    }
    // --------------------------------------------------
    // If an initial state was provided, return the final
    // shift-register state in the w0[] array.
    // --------------------------------------------------
    if (w0) 
    {
        for (k=1; k<=Nw; k++) 
        {
            w0[k].Real = w[k].Real;
            w0[k].Imag = w[k].Imag;
        }
    }
    return WI_SUCCESS;
}
// end of wiFilter_IIR_ComplexInput()

/*============================================================================+
|  Function  : wiFilter_Butterworth()                                         |
|-----------------------------------------------------------------------------|
|  Purpose   : Generate the transfer function for a digital Butterworth low   |
|              pass filter.                                                   |
|  Parameters: N     -- Order of the filter                                   |
|              fc    -- Cutoff relative to the sample frequency (1.0 <-->1/T).|
|              b     -- Numerator polynomial (must contain n+1 terms)         |
|              a     -- Denominator polynomal (must contain n+1 terms)        |
|              method-- {BFILTER_IMPULSE_INVARIANCE,*_BILINEAR_TRANSFORM}     |
+============================================================================*/
wiStatus wiFilter_Butterworth(int N, double fc, double *b, double *a, int method)
{
    int       i, k;     // pole indices
    double    T = 2*pi; // sample so normalized cutoff is at 1/T
    double    g = 1;    // gain term
    double    angle;    // angle on unit circle for pole location
    wiComplex p[32];    // pole locations
    wiComplex z[32];    // zero locations
    int Nz = 0;         // number of zeros
 
    // ----------------------
    // Error / Range Checking
    // ----------------------
    if (InvalidRange( N,      1,  30)) return WI_ERROR_PARAMETER1;
    if (InvalidRange(fc, 1.0E-6, 0.5)) return WI_ERROR_PARAMETER2;

    // ------------------------------------------------------------------------
    // Find the analog prototype for a Butterworth filter with cutoff frequency
    // of 1 radian/sec. The poles of the magnitude squared function lie on a
    // unit circle in the s-plane. For a stable/causal filter, only those poles
    // on the left-hand side of the j-axis are chosen. See [1, pp. 845-846]
    // ------------------------------------------------------------------------
    for (k=i=0; k<2*N; k++) 
    {
        angle = (double)(pi/(2*N))*(2*k + N - 1);
        if ((angle > pi/2) && (angle < 3*pi/2))
        {
            p[i].Real = cos(angle);
            p[i].Imag = sin(angle);
            i++;
        }
    }
    // ------------------------------
    // No zeros...clear the z[] array
    // ------------------------------
    for (k=0; k<N; k++) 
        z[k].Real = z[k].Imag = 0.0;

    // --------------------------------------------------------------
    // Transform from the analog prototype to the sampled-time filter
    // --------------------------------------------------------------
    switch (method)
    {
        case WIFILTER_IMPULSE_INVARIANCE: XSTATUS(wiFilter_Digital_ImpulseInvariance(N, Nz, fc, T, g, p, z, b, a)); break;
        case WIFILTER_BILINEAR_TRANSFORM: XSTATUS(wiFilter_Digital_BilinearTransform(N, Nz, fc, T, g, p, z, b, a)); break;
    }
    return WI_SUCCESS;
}
// end of wiFilter_Butterworth()

/*****************************************************************************/
/*=== LOCAL (Non-Exported) FUNCTIONS ========================================*/
/*****************************************************************************/

/*============================================================================+
|  Function  : wiFilter_Digital_ImpulseInvariance()                           |
|-----------------------------------------------------------------------------|
|  Purpose   : Generate the z-Domain transfer function for the poles, zeros,  |
|              and gain term specifying a continuous-time prototype.          |
|  Parameters: n     -- Order of the filter (number of poles)                 |
|              M     -- Number of zeros                                       |
|              fc    -- Cutoff relative to the sample frequency (1.0 <--> 1T).|
|              T     -- Sample period                                         |
|              Gs    -- s-Domain gain term                                    |
|              Ps    -- s-Domain poles                                        |
|              Zs    -- s-Domain zeros                                        |
|              Bz    -- Numerator polynomial (must contain n+1 terms)         |
|              Az    -- Denominator polynomal (must contain n+1 terms)        |
+============================================================================*/
wiStatus wiFilter_Digital_ImpulseInvariance(int n, int M, double fc, double T, double Gs, 
                                                                    wiComplex Ps[], wiComplex Zs[], double Bz[], double Az[])
{
    int       k;      // loop index
    double    G = Gs; // gain term of the s-domain filter
    wiComplex Cs[32]; // coefficients of the s-domain partial fraction expansion
    wiComplex Cz[32]; // coefficients of the z-domain partial fraction expansion
    wiComplex Pz[32]; // poles of the z-domain partial fraction expansion
    wiComplex A[32];  // complex polynomial A(z)
    wiComplex B[32];  // complex polynomial B(z)
    wiComplex X;      // induction variable

    // ----------------------
    // Error / Range Checking
    // ----------------------
    if (InvalidRange(n,  1,       31)) return WI_ERROR_PARAMETER1;
    if (InvalidRange(fc, 1.0E-6, 0.5)) return WI_ERROR_PARAMETER2;
    if (InvalidRange(T,  1.0, 2.0*pi)) return WI_ERROR_PARAMETER3;
    if (Gs==0.0)                       return WI_ERROR_PARAMETER4;

    // ----------------------------
    // Scale the frequency response
    // ----------------------------
    XSTATUS( wiFilter_FrequencyScale_s_NRP(fc, M, Zs, n, Ps, &G) );

    // ----------------------------------
    // Partial fraction expansion of H(s)
    // ----------------------------------
    XSTATUS( wiFilter_PartialFractionExpansion_NRP(M, Zs, n, Ps, G, Cs) );

    // -----------------------------------------------------------------------
    // IMPULSE INVARIANE TRANSFORM
    //
    // Convert from
    //               H[k](s) =  Cs[k] / (s - Ps[k])
    // into
    //               H[k](z) = zCs[k] / (z - exp{Ps[k]T})
    // -----------------------------------------------------------------------
    for (k=0; k<n; k++)
    {
        X.Real = T * Ps[k].Real;               
        X.Imag = T * Ps[k].Imag;

        Pz[k].Real = exp(X.Real) * cos(X.Imag);
        Pz[k].Imag = exp(X.Real) * sin(X.Imag);

        Cz[k].Real = T * Cs[k].Real;           
        Cz[k].Imag = T * Cs[k].Imag;
    }
    // -----------------------------------------------------
    // Convert the pole-zero form into A(z)/B(z) polynomials
    // -----------------------------------------------------
    XSTATUS( wiFilter_PoleZero_to_Polynomial_NRP(n, Cz, NULL, Pz, B, A ) );

    // --------------------------------------------------------------------
    // Clean up the polynomials
    // For the filters provided in this module, the poles should be complex
    // conjugates or reals; therefore, the polynomials will be real.
    // --------------------------------------------------------------------
    for (k=0; k<=n; k++) 
    {
        Bz[k] = B[k].Real;
        Az[k] = A[k].Real;
    }
    return WI_SUCCESS;
}
// end of wiFilter_Digital_ImpulseInvariance()

/*============================================================================+
|  Function  : wiFilter_Digital_BilinearTransform()                           |
|-----------------------------------------------------------------------------|
|  Purpose   : Generate the z-Domain transfer function for the poles, zeros,  |
|              and gain term specifying a continuous-time prototype.          |
|  Parameters: N     -- Order of the filter (number of poles)                 |
|              M     -- Number of zeros                                       |
|              fc    -- Cutoff relative to the sample frequency (1.0 <--> 1T).|
|              T     -- Sample period                                         |
|              Gs    -- s-Domain gain term                                    |
|              Ps    -- s-Domain poles                                        |
|              Zs    -- s-Domain zeros                                        |
|              Bz    -- Numerator polynomial (must contain N+1 terms)         |
|              Az    -- Denominator polynomal (must contain N+1 terms)        |
+============================================================================*/
wiStatus wiFilter_Digital_BilinearTransform(int N, int M, double fc, double T, double Gs, 
                                                                    wiComplex Ps[], wiComplex Zs[], double Bz[], double Az[])
{
    int       k;      // loop index
    double    G = Gs; // gain term of the s-domain filter
    wiComplex Cs[32]; // coefficients of the s-domain partial fraction expansion
    wiComplex Cz[32]; // coefficients of the z-domain partial fraction expansion
    wiComplex Qz[32]; // zeros of the z-domain partial fraction expansion
    wiComplex Pz[32]; // poles of the z-domain partial fraction expansion
    wiComplex A[32];  // complex polynomial A(z)
    wiComplex B[32];  // complex polynomial B(z)
    wiComplex *Qptr;  // ptr -> z-domain zeros
    wiComplex X,Y;
    wiComplex Tck;
    wiComplex Tpk;
    double Fap;       // prewarped analog frequency

    // ----------------------
    // Error / Range Checking
    // ----------------------
    if (InvalidRange(N,  1,       31)) return WI_ERROR_PARAMETER1;
    if (InvalidRange(fc, 1.0E-6, 0.5)) return WI_ERROR_PARAMETER2;
    if (InvalidRange(T,  1.0, 2.0*pi)) return WI_ERROR_PARAMETER3;
    if (Gs==0.0)                       return WI_ERROR_PARAMETER4;

    // ------------------------------------------------
    // Calculate the required analog frequency
    // "Pre-warping" is done for the Bilinear Transform
    // ------------------------------------------------
    Fap = 2.0/T * tan(pi * fc);

    // ----------------------------
    // Scale the frequency response
    // ----------------------------
    XSTATUS( wiFilter_FrequencyScale_s_NRP( Fap, M, Zs, N, Ps, &G ) );

    // ----------------------------------
    // Partial fraction expansion of H(s)
    // ----------------------------------
    XSTATUS( wiFilter_PartialFractionExpansion_NRP( M, Zs, N, Ps, G, Cs ) );

    // -----------------------------------------------------------------------
    // BILINEAR TRANSFORM
    //              
    //                                             c[k]*T
    //                                          ------------ ( z + 1 )
    //                 c[k]                     (2 - T*p[k])
    //     H[k](s) = -------- ===> H[k](z) = --------------------------
    //               s - p[k]                         (2 + T*p[k])
    //                                          z  -  ------------
    //                                                (2 - T*p[k])
    // -----------------------------------------------------------------------
    for (k=0; k<N; k++)
    {
        Tck.Real = T * Cs[k].Real;
        Tck.Imag = T * Cs[k].Imag;

        Tpk.Real = T * Ps[k].Real;
        Tpk.Imag = T * Ps[k].Imag;

        X.Real = 2.0+Tpk.Real;
        X.Imag =     Tpk.Imag;

        Y.Real = 2.0-Tpk.Real;
        Y.Imag =    -Tpk.Imag;

        wiFilter_ComplexDivision(Tck, Y, &(Cz[k]));
        wiFilter_ComplexDivision(X,   Y, &(Pz[k]));

        Qz[k].Real = -1.0;
        Qz[k].Imag =  0.0;
    }
    Qptr = Qz;

    // -----------------------------------------------------
    // Convert the pole-zero form into A(z)/B(z) polynomials
    // -----------------------------------------------------
    XSTATUS( wiFilter_PoleZero_to_Polynomial_NRP( N, Cz, Qptr, Pz, B, A ) );

    // --------------------------------------------------------------------
    // Clean up the polynomials
    // For the filters provided in this module, the poles should be complex
    // conjugates or reals; therefore, the polynomials will be real.
    // --------------------------------------------------------------------
    for (k=0; k<=N; k++) 
    {
        Bz[k] = B[k].Real;
        Az[k] = A[k].Real;
    }
    return WI_SUCCESS;
}
// end of wiFilter_Digital_BilinearTransform()

/*===========================================================================+
|  Function  : wiFilter_FrequencyScale_s_NRP()                               |
|----------------------------------------------------------------------------|
|  Purpose   : Frequency scaling for an s-Domain transfer function expressed |
|              in pole, zero, gain form. The array of poles are assumed to be|
|              unique (no repeated poles). The frequency transform is a LPF  |
|              to LPF transformation characterized by                        |
|                                                                            |
|                        s --> s / fc                                        |
|                                                                            |
|  Parameters: fc -- Frequency scaling relative to the current scale         |
|              Nz -- Number of zeros                                         |
|              z  -- Array of zeros                                          |
|              Np -- Number of poles                                         |
|              p  -- Number of poles                                         |
|              G  -- Constant gain term                                      |
+===========================================================================*/
wiStatus wiFilter_FrequencyScale_s_NRP(double fc, int  Nz, wiComplex z[], int  Np, wiComplex p[], double G[])
{
    int k;
    
    // ---------------
    // Scale the zeros
    // ---------------
    for (k=0; k<Nz; k++) 
    {
        z[k].Real *= fc;
        z[k].Imag *= fc;
    }
    // ---------------
    // Scale the poles
    // ---------------
    for (k=0; k<Np; k++) 
    {
        p[k].Real *= fc;
        p[k].Imag *= fc;
    }
    // -------------------
    // Scale the gain term
    // -------------------
    (*G) *= exp((Np-Nz) * log(fc));

    return WI_SUCCESS;
}
// end of wiFilter_FrequencyScale_s_NRP()

/*============================================================================+
|  Function  : wiFilter_PartialFractionExpansion_NRP()                        |
|-----------------------------------------------------------------------------|
|  Purpose   : The transfer function H(x) specified by Nz zeros, Np poles and |
|              gain term G is expanded via Heaviside's partial fraction       |
|              expansion theorem to give coefficients C[]. This function is   |
|              limited to the case where there are no repeated poles (NRP).   |
|                                                                             |
|              The input                                                      |
|                                 Nz-1                                        |
|                               PRODUCT ( x - z[k] )                          |
|                                 k=1                                         |
|                    H(x) = G * --------------------                          |
|                                 Np-1                                        |
|                               PRODUCT ( x - p[k] )                          |
|                                 k=0                                         |
|                                                                             |
|              is expanded into                                               |
|                                                                             |
|                              N-1        C[k]                                |
|                    H(x) = SUMMATION ( -------- )                            |
|                              k=0      x - p[k]                              |
|                                                                             |
|              where p[i] != p[j], i != j.                                    |
|                                                                             |
|  Reference : [1, pp. 166-170]                                               |
|                                                                             |
|  Parameters: Nz -- Number of zeros                                          |
|              z  -- Array of zeros                                           |
|              Np -- Number of poles                                          |
|              p  -- Pole values                                              |
|              G  -- Constant gain term                                       |
|              C  -- Coefficients of partial franction expansion. The length  |
|                    of this array must be at least Np elements.              |
+============================================================================*/
wiStatus wiFilter_PartialFractionExpansion_NRP(int Nz, wiComplex z[], 
                                                                        int Np, wiComplex p[], double G, wiComplex C[])
{
    int i, k;
    wiComplex A, B, T, X;

    // ----------------------
    // Error / Range Checking
    // ----------------------
    if (InvalidRange(Nz, 0, 128)) return WI_ERROR_PARAMETER1;
    if (InvalidRange(Np, 1, 128)) return WI_ERROR_PARAMETER3;

    // --------------------------------------
    // Form k expansion terms for the k poles
    // --------------------------------------
    for (k=0; k<Np; k++)
    {
        A.Real = 1.0; A.Imag = 0.0;  // Q and R are accumulators for C = A/B
        B.Real = 1.0; B.Imag = 0.0;

        // --------------------------------------
        // NUMERATOR (A)
        // Evaluate the numerator at the kth pole
        // --------------------------------------
        for (i=0; i<Nz; i++)
        {
            X.Real = (p[k].Real - z[i].Real);
            X.Imag = (p[k].Imag - z[i].Imag);

            T.Real = (X.Real * A.Real) - (X.Imag * A.Imag);
            T.Imag = (X.Real * A.Imag) + (X.Imag * A.Real);

            A.Real = T.Real;
            A.Imag = T.Imag;
        }
        // ------------------------------------------------------------------
        // DENOMINATOR (B)
        // Evaluate the denominator at the kth pole, with that factor removed
        // ------------------------------------------------------------------
        for (i=0; i<Np; i++)
        {
            if (i==k) continue; // skip the kth pole

            X.Real = (p[k].Real - p[i].Real);
            X.Imag = (p[k].Imag - p[i].Imag);

            T.Real = (X.Real * B.Real) - (X.Imag * B.Imag);
            T.Imag = (X.Real * B.Imag) + (X.Imag * B.Real);

            B.Real = T.Real;
            B.Imag = T.Imag;
        }
        // ------------------------------
        // Compute C[k] = G * A[k] / B[k]
        // ------------------------------
        wiFilter_ComplexDivision(A, B, &(C[k]) );
        C[k].Real *= G;
        C[k].Imag *= G; 
    }
    return WI_SUCCESS;
}
// end of wiFilter_PartialFractionExpansion_NRP()

/*============================================================================+
|  Function  : wiFilter_PoleZero_to_Polynomial_NRP()                          |
|-----------------------------------------------------------------------------|
|  Purpose   : Convert a pole-zero representation to a polynomial form of the |
|              transfer function for the case where there are No Repeated     |
|              Poles (NRP). There are N poles with corresponding terms        |
|                                                                             |
|                      H[k](z) = C[k](z - Q[k]) / (z - P[k])                  |
|                                                                             |
|              The summation of H[0](z)...H[N-1](z) yields                    |
|                                                                             |
|                      H(z) = B(z) / A(z)                                     |
|                                                                             |
|              where A(z) = A[0] + A[1]z^-1 + A[2]z^-2 + ... + A[N]z^-N, and  |
|              B(z) has the same form.                                        |
|                                                                             |
|  Parameters: N  -- Order of the transfer function (number of poles)         |
|              C  -- Coefficients of the partial fraction expansion terms     |
|              Q  -- Zeros of the partial fraction expansion terms (can be    |
|                    NULL if there are no zeros in the partial fraction       |
|                    expansion)                                               |
|              P  -- Poles of the partial fraction expansion terms            |
|              A  -- Numerator polynomial coefficients (N+1 terms)            |
|              B  -- Denominator polynomial coefficients (N+1 terms)          |
+============================================================================*/
wiStatus wiFilter_PoleZero_to_Polynomial_NRP(int N, wiComplex C[], wiComplex Q[],
                                                    wiComplex P[], wiComplex B[], wiComplex A[])
{
    wiComplex T[32], U[32];
    int i, j, k;

    // ----------------------
    // Error / Range Checking
    // ----------------------
    if (InvalidRange(N, 1, 32)) return WI_ERROR_PARAMETER1;

    // ---------------------------------
    // Clear the polynomial coefficients
    // ---------------------------------
    for (k=0; k<=N; k++) 
    {
        B[k].Real = B[k].Imag = 0.0;
        A[k].Real = A[k].Imag = 0.0;
    }
    // -------------------
    // Build the numerator
    // -------------------
    for (k=0; k<N; k++)
    {
        // -------------------------
        // Clear the T(z) polynomial
        // -------------------------
        for (j=0; j<=N; j++)
            T[j].Real = T[j].Imag = 0.0;

        // -----------------------------------
        // Initialize T(z) with the first term
        // For k=0, i=1; otherwise, i=0
        // -----------------------------------
        i = (k==0)? 1:0;
        T[0].Real = 1.0;        T[0].Imag = 0.0;
        if (N>1)
        {
            T[1].Real = -P[i].Real; T[1].Imag = -P[i].Imag;
        }
        // ------------------------------------------------------------
        // Finish the convolution to form the polynomial portion due to
        // terms from the partial fraction denominators.
        // ------------------------------------------------------------
        for (i++; i<N; i++)
        {
            if (i==k) continue;  // skip the kth root

            for (j=1; j<=N; j++)
            {
                U[j].Real = T[j].Real - (P[i].Real*T[j-1].Real - P[i].Imag*T[j-1].Imag);
                U[j].Imag = T[j].Imag - (P[i].Real*T[j-1].Imag + P[i].Imag*T[j-1].Real);
            }
            for (j=1; j<=N; j++)
            {
                T[j].Real = U[j].Real;
                T[j].Imag = U[j].Imag;
            }
        }
        // --------------------------------------------------------
        // Convolution to account for the numerator term (z - Q[k])
        // --------------------------------------------------------
        if ( Q )
        {
            for (j=1; j<=N; j++)
            {
                U[j].Real = T[j].Real - (Q[k].Real*T[j-1].Real - Q[k].Imag*T[j-1].Imag);
                U[j].Imag = T[j].Imag - (Q[k].Real*T[j-1].Imag + Q[k].Imag*T[j-1].Real);
            }
            for (j=1; j<=N; j++)
            {
                T[j].Real = U[j].Real;
                T[j].Imag = U[j].Imag;
            }
        }
        // --------------------------------------------
        // Multiply by the partial fraction coefficient
        // and add into the polynomial
        // --------------------------------------------
        for (i=0; i<=N; i++)
        {
            B[i].Real += (C[k].Real * T[i].Real) - (C[k].Imag * T[i].Imag);
            B[i].Imag += (C[k].Real * T[i].Imag) + (C[k].Imag * T[i].Real);
        }
    }
    // ---------------------
    // Build the denominator
    // ---------------------
    A[0].Real = 1.0;         A[0].Imag = 0.0;
    A[1].Real = -P[0].Real;  A[1].Imag = -P[0].Imag;
    for (i=2; i<=N; i++)
        A[i].Real = A[i].Imag = 0.0;

    for (k=1; k<N; k++)
    {
        for (j=1; j<=N; j++)
        {
            T[j].Real = A[j].Real - (P[k].Real*A[j-1].Real - P[k].Imag*A[j-1].Imag);
            T[j].Imag = A[j].Imag - (P[k].Real*A[j-1].Imag + P[k].Imag*A[j-1].Real);
        }
        for (j=1; j<=N; j++)
        {
            A[j].Real = T[j].Real;
            A[j].Imag = T[j].Imag;
        }
    }
    return WI_SUCCESS;
}
// end of wiFilter_PoleZero_to_Polynomial_NRP()

/*============================================================================+
|  Function  : wiFilter_ComplexDivision()                                     |
|-----------------------------------------------------------------------------|
|  Purpose   : Divide two complex number to give c = a / b.                   |
|  Parameters: a -- numerator                                                 |
|              b -- denominator                                               |
|              c -- quotient                                                  |
+============================================================================*/
wiStatus wiFilter_ComplexDivision(wiComplex a, wiComplex b, wiComplex *c)
{
    double magnitude, phase;

    // ------------------------
    // Check for divide-by-zero
    // ------------------------
    if ((b.Real==0.0) && (b.Imag==0.0)) return WI_ERROR_PARAMETER2;

    // ----------------------------
    // Compute the complex quotient
    // ----------------------------
    magnitude = sqrt((a.Real*a.Real + a.Imag*a.Imag) / (b.Real*b.Real + b.Imag*b.Imag));
    phase     = atan2(a.Imag,a.Real) - atan2(b.Imag,b.Real);
    c->Real = magnitude * cos(phase);
    c->Imag = magnitude * sin(phase);

    return WI_SUCCESS;
}
// end of wiFilter_ComplexDivision()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------
