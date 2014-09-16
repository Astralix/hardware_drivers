/*
 **************************************************************************
 * WIBFLFSD.H
 * created: Thu Jan 27 22:54:17 IST 2011
 *-------------------------------------------------------------------------
 * class "Wlfsd" declarations.
 * (put more comments here)
 *-------------------------------------------------------------------------
 * Author: Leonardo Vainsencher
 *-------------------------------------------------------------------------
 * Copyright (c) 2011 DSP Group, Inc. All rights reserved.
 **************************************************************************
 */

#ifndef _WIBFLFSD_H_
#define _WIBFLFSD_H_

#include <vector>

namespace WiBF {

class Wgslmat;

class Wlfsd
{
    typedef std::vector<int> ivector_t;
    typedef std::vector<double> dvector_t;
    struct priv_s;
public:
    Wlfsd(void);
    ~Wlfsd(void);
    void operator() (Wgslmat& uhat, Wgslmat* chat, const Wgslmat& X,
	    const Wgslmat& G, const int L, const ivector_t& K,
	    const bool pca=false);
private:
    int lfsdecode(Wgslmat& uhat, const Wgslmat& H, const Wgslmat& xH,
	    const int bitk, const int bitn, const int bith);
    void chsurv(Wgslmat& chat, const Wgslmat& uhat, const int uidx) const;
    void pchsea(Wgslmat& chat, const Wgslmat& uhat, const Wgslmat& H,
	    const Wgslmat& xH, const int idx);
    void fixsign(Wgslmat& G3, Wgslmat& Q) const;
    int nearestu(double e, const int L) const;
    void nextus(int& u, int& s, const int L) const;
private:
    priv_s* priv;
};

}

#endif	//_WIBFLFSD_H_
