/*
 **************************************************************************
 * WIBFKVSD.H
 * created: Thu Feb 10 19:01:36 JST 2011
 *-------------------------------------------------------------------------
 * class "Wkvsd" declarations.
 * (put more comments here)
 *-------------------------------------------------------------------------
 * Author: Leonardo Vainsencher
 *-------------------------------------------------------------------------
 * Copyright (c) 2011 DSP Group, Inc. All rights reserved.
 **************************************************************************
 */

#ifndef _WIBFKVSD_H_
#define _WIBFKVSD_H_

#include <vector>
#include <cmath>

namespace WiBF {

class Wgslmat;
class Xgslmat;

class Wkvsd
{
    typedef std::vector<int> ivector_t;
    typedef std::vector<double> dvector_t;
    struct priv_s;
    struct prix_s;

public:
    Wkvsd(int _normtype, int _fixpt, int _dlim);
    ~Wkvsd(void);
    // return hypothesis if U==NULL, else return counter-hypothesis for given
    // bit bitp and given hypothesis U:
    void operator() (Wgslmat& uhat, const Wgslmat& X,
	    const Wgslmat& G, const int L, const int K,
	    const Wgslmat* U, int bitp);
    // return hypothesis and, if C!=NULL, counter-hypotheses
    // for all hypothesis bits:
    void operator() (Wgslmat& uhat, Wgslmat* chat, ivector_t* ncyc,
	    const Wgslmat& X, const Wgslmat& G, const int L, const int K,
	    int cmode);
    // return hypothesis and, if C!=NULL, counter-hypotheses
    // for all hypothesis bits, using complex expansion (CSESD);
    // works only for special case of 2xM MIMO:
    void csesd3(Wgslmat& uhat, Wgslmat* chat,
	    const Wgslmat& X, const Wgslmat& G, const int L, const int K);
    // old implementation:
    void csesd2(Wgslmat& uhat, Wgslmat* chat,
	    const Wgslmat& X, const Wgslmat& G, const int L, const int K);
    int cmisses(void) const {return cmiss;}
    int chits(void) const {return chit;}
    int normtype(void) const {return norm_type;}
    int isfixpt(void) const {return fixpt;}

private:
    void ksedecode2(Wgslmat& uhat, const Wgslmat& xH, const Wgslmat& H,
	    const Wgslmat& U, int bitk, int bitp);
    void chsurv(Wgslmat& chat, const Wgslmat& U, int bitk, int bitp);

private:
    void csesd3_hypothesis_decode(Wgslmat& uhat, const Xgslmat& x,
	    const Xgslmat& H);
    void csesd3_counter_decode(Wgslmat& chat, const Wgslmat& U,
	    int bitk, int bitp);
    void csesd3_counter_top(Wgslmat& chat, const Wgslmat& U,
	    int bitk, int bitp);
    void csesd3_counter_bot(Wgslmat& chat, const Wgslmat& U,
	    int bitk, int bitp);

private:
    // old implementation:
    void csesd2_hypothesis_decode(Wgslmat& uhat, const Xgslmat& xH,
	    const Xgslmat& H, double& dh0);
    void csesd2_counter_decode(Wgslmat& chat, const Wgslmat& U,
	    int bitk, int bitp, const double& dh0);
    void csesd2_counter_top(Wgslmat& chat, const Wgslmat& U,
	    int bitk, int bitp);
    void csesd2_counter_bot(Wgslmat& chat, const Wgslmat& U,
	    int bitk, int bitp, const double& dh0);

private:
    static void nearestus(int& u, int& s, const double& e, const ivector_t& A);
    static void nextus(int& u, int& s, const int L);
    static bool nextus(int& u, int& s, const ivector_t& A);
    static bool ismember(int u, const int L);
    static bool ismember(int u, const ivector_t& A);
    static void fixsign(Wgslmat& G3, Wgslmat& Q);
    static void fixsign(Xgslmat& G3, Xgslmat& Q);
    static void graygen(const int nbits, ivector_t& C);

private:
    static inline double norm1(const double& x, const double& y) {
	return std::abs(x) + std::abs(y);
    }
    static inline double norm2(const double& x, const double& y) {
	return x*x + y*y;
    }

private:
    const int norm_type;
    const bool fixpt;
    const int intp, frac;
    priv_s* prv;
    prix_s* prx;
    int chit, cmiss;
};

}

#endif	//_WIBFKVSD_H_
