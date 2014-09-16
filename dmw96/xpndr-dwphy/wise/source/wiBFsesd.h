/*
 **************************************************************************
 * WIBFSESD.H
 * created: Mon Feb  7 14:46:28 JST 2011
 *-------------------------------------------------------------------------
 * class "Wsesd" declarations.
 * Schnorr-Euchner Sphere Decoder.
 * SE tree search of max-likelihood hypothesis and and counter-hypotheses
 * for closest lattice point.
 * (put more comments here)
 *-------------------------------------------------------------------------
 * Author: Leonardo Vainsencher
 *-------------------------------------------------------------------------
 * Copyright (c) 2011 DSP Group, Inc. All rights reserved.
 **************************************************************************
 */

#ifndef _WIBFSESD_H_
#define _WIBFSESD_H_

#include <cstdlib>
#include <vector>

namespace WiBF {

class Wgslmat;

class Wsesd
{
    typedef std::vector<int> ivector_t;
    typedef std::vector<double> dvector_t;

public:
    Wsesd(void);
    ~Wsesd(void);
    void operator() (
	    Wgslmat& uhat, ivector_t* unitr, Wgslmat* xhat,
	    const Wgslmat& X, const Wgslmat& G, const int L,
	    const Wgslmat* U=NULL, int bitp=-1) const;
    void operator() (
	    Wgslmat& uhat, Wgslmat* chat, ivector_t* unitr, Wgslmat* xhat,
	    const Wgslmat& X, const Wgslmat& G, const int L) const;

private:
    static int sedecode2(const Wgslmat& H, const Wgslmat& xH,
	    const Wgslmat& A, int maxit, Wgslmat& uhat);
    static void fixsign(Wgslmat& G3, Wgslmat& Q);
    static void nearestu(int& u, int& s, const double& e,
	    const Wgslmat& A, int k);
    static bool nextus2(int& u, int& s, const Wgslmat& A, int k);
    static void nextus(int& u, int& s, int L);
    static bool ismember(int u, int L);
    static bool ismember(int u, const Wgslmat& A, int k);
    static void graycodegen(const int b, ivector_t& C);
};

}

#endif	//_WIBFSESD_H_
