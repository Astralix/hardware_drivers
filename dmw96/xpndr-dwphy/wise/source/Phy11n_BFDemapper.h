/*
 **************************************************************************
 * Phy11n_BFDemapper.h
 *
 * created: Thu Jun 23 11:25:00 JST 2011
 *-------------------------------------------------------------------------
 * Phy11n BF Demapper related declarations
 *-------------------------------------------------------------------------
 * Author: Leonardo Vainsencher
 *-------------------------------------------------------------------------
 * Copyright (c) 2011 DSP Group, Inc. All rights reserved.
 **************************************************************************
 */

#ifndef	_PHY11N_BFDEMAPPER_H_
#define	_PHY11N_BFDEMAPPER_H_

#include "wiBFfixpt.h"
#ifdef	USE_WGSLMAT
#include "wgslmat.h"
#else
#include "wiBFmatrix.h"
#endif
#include "wiMatrix.h"
#include <string>
#include <vector>
#include <complex>

namespace WiBF {

// struct used to collect BF demapper stats
struct BFextra_s
{
    typedef std::complex<double> dcplx_t;
    typedef std::complex<int> icplx_t;

    const int thread_id;
    int ch_miss, ch_hit;
    int ucnt;
    int nrx, nss, nsc, maxsym;
    double SNR;
    std::string tvn;
    std::vector<dcplx_t> H, Y;
    std::vector<int> U, C;
    Wfixpt::smap_t smap;	// used to collect fixpt stats

    BFextra_s(int id);

    // update c/h misses
    void chmiss(int c=1);
    // update c/h hits
    void chhit(int c=1);
    void chkmaxsym(void);
    void addH(int nsc, const wiCMat H[]);
    void addY(double K, const wiCMat* Y);
    void addUC(int nsc, const Wgslmat& U, const Wgslmat& C);
};

}

#endif	// _PHY11N_BFDEMAPPER_H_
