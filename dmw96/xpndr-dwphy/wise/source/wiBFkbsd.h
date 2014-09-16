/*
 **************************************************************************
 * WIBFKBSD.H
 * created: Thu Feb 10 19:01:36 JST 2011
 *-------------------------------------------------------------------------
 * class "Wkbsd" declarations.
 * (put more comments here)
 *-------------------------------------------------------------------------
 * Author: Leonardo Vainsencher
 *-------------------------------------------------------------------------
 * Copyright (c) 2011 DSP Group, Inc. All rights reserved.
 **************************************************************************
 */

#ifndef _WIBFKBSD_H_
#define _WIBFKBSD_H_

#include <vector>
#include <string>

namespace WiBF {

class Wgslmat;

class Wkbsd
{
    typedef std::vector<int> ivector_t;
    typedef std::vector<double> dvector_t;
    struct priv_s;

public:
    Wkbsd(void);
    ~Wkbsd(void);
    void operator() (Wgslmat& uhat, const Wgslmat& X,
	    const Wgslmat& G, const int L, const int K,
	    const Wgslmat* U, int bitp);

private:
    int ksedecode2(Wgslmat& uhat, const Wgslmat& xH,
	    const Wgslmat& H, const ivector_t& U, int bitk, int bitp);
    static void nearestu(int& u, int& s, const double& e, const ivector_t& A);
    static bool nextus2(int& u, int& s, const ivector_t& A);
    static void nextus(int& u, int& s, int L);
    static bool ismember(int u, int L);
    static bool ismember(int u, const ivector_t& A);
    static void fixsign(Wgslmat& G3, Wgslmat& Q);

private:
    priv_s* priv;
};

}

#endif	//_WIBFKBSD_H_
