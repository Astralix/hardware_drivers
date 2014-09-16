/*
 **************************************************************************
 * WIBFFIXPT.H
 * created: Sun Apr  3 12:03:23 JDT 2011
 *-------------------------------------------------------------------------
 * class "Wfixpt" declarations.
 * by default, all numbers are double/wiComplex.
 * inline operators are used to improve speed.
 * light fixed point: limits fraction precision of numbers.
 * heavy fixed point: limits overall precision of numbers.
 * heavier fixed point: limits precision and range of numbers.
 * initial implementation is "light".
 * (put more comments here)
 *-------------------------------------------------------------------------
 * Author: Leonardo Vainsencher
 *-------------------------------------------------------------------------
 * Copyright (c) 2011 DSP Group, Inc. All rights reserved.
 **************************************************************************
 */

#ifndef _WIBFFIXPT_H_
#define _WIBFFIXPT_H_

#include "wiBFmissing.h"
#include "wiMath.h"
#include <string>
#include <vector>
#include <map>
#include <cmath>

namespace WiBF {

class Wfixpt
{
    typedef std::vector<double> dvect_t;

public:
    typedef enum {REAL=0, CPLX=1} vtype_t;
    struct stat_s;
    typedef std::map<std::string,stat_s> smap_t;

public:
    Wfixpt(void);
    Wfixpt(const Wfixpt&);
    Wfixpt(const wiComplex&);
    Wfixpt(const double&);
    Wfixpt(vtype_t, int);
    Wfixpt(const char*, vtype_t, int, bool=true);
    Wfixpt(const char*, vtype_t, bool=true);
    ~Wfixpt(void);

    inline vtype_t type(void) const {return vtype;};
    inline int getfrac(void) const {return frac;};
    inline double real(void) const {return a.Real;}
    inline double imag(void) const {return a.Imag;}
    inline bool isreal(void) const {return vtype==REAL;}
    inline wiComplex toComplex(void) const {return a;}
    inline double toReal(void) const {
	if (!isreal())
	    _error(__FILE__, __LINE__, "invalid conversion");
	return real();
    }
    bool isfixpt(void) const;
    void setfrac(int);
    void real(const double&);
    void imag(const double&);
    static wiComplex limit(const wiComplex&, double hi);
    static double limit(const double&, double hi);

    Wfixpt operator- (void) const;
    Wfixpt& operator= (const Wfixpt&);
    Wfixpt& operator+= (const Wfixpt&);
    Wfixpt& operator-= (const Wfixpt&);
    Wfixpt& operator*= (const Wfixpt&);
    Wfixpt& operator/= (const Wfixpt&);

private:
    void _inits(void);
    void _setfrac(int);
    inline void _updates(void);
    inline void _rounds(const Wfixpt& p) {
	if (frac < 0) a=p.a;
	else if (p.frac < 0 || frac < p.frac)
	    rounds(a,p.a);
	else a=p.a;
    }
    inline double rounds(const double& x) const {
	if (frac < 0) return x;
	return rounds(x,scl);
    }
    inline void rounds(wiComplex& w) const {
	w.Real = rounds(w.Real);
	w.Imag = rounds(w.Imag);
    }
    inline void rounds(wiComplex& z, const wiComplex& w) const {
	z.Real = rounds(w.Real);
	z.Imag = rounds(w.Imag);
    }
    static inline double rounds(const double& x, unsigned s) {
	return round(x*s)/s;
    }
    static inline void rounds(wiComplex& w, unsigned s) {
	w.Real = rounds(w.Real,s);
	w.Imag = rounds(w.Imag,s);
    }
    static int _getfracbyname(const char*);
    static void _error(const char*, int, const char*);
    static smap_t* getsmap(void);

private:
    const std::string name;
    const vtype_t vtype;
    const bool enable;
    stat_s* stats;
    int frac, scl;
    wiComplex a;
};

#define WIBFFIXPT_OP_DECL(_op_) \
    Wfixpt operator _op_ (const Wfixpt&, const Wfixpt&); \
    Wfixpt operator _op_ (const Wfixpt&, const double&); \
    Wfixpt operator _op_ (const double&, const Wfixpt&)
WIBFFIXPT_OP_DECL(+);
WIBFFIXPT_OP_DECL(-);
WIBFFIXPT_OP_DECL(*);
WIBFFIXPT_OP_DECL(/);
#undef WIBFFIXPT_OP_DECL

struct Wfixpt::stat_s {
    const Wfixpt* varp;
    bool fresh;
    int frac, updc;
    double maxval, minval;
    double hmax, hmin;
    //dvect_t hist;
    stat_s(void) {
	varp=NULL;
	maxval= minval= hmax= hmin= frac= updc= 0;
	fresh=true;
    }
    inline void update(const Wfixpt* z) {
	update(z->real());
	if (!z->isreal())
	    update(z->imag(), false);
    }
    inline void update(const double& x, bool c=true) {
	if (fresh) {
	    fresh=false;
	    minval= maxval= x;
	} else if (minval > x)
	    minval = x;
	else if (maxval < x)
	    maxval = x;
	if (c) updc++;
    }
};

}

#endif	//_WIBFFIXPT_H_
