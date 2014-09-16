/*
 **************************************************************************
 * WIBFFIXPT.CPP
 * created: Sun Apr  3 12:03:23 JDT 2011
 *-------------------------------------------------------------------------
 * class "Wfixpt" definitions.
 * light fixed point: limits fraction/overall precision of numbers.
 * (put more comments here)
 *-------------------------------------------------------------------------
 * Author: Leonardo Vainsencher
 *-------------------------------------------------------------------------
 * Copyright (c) 2011 DSP Group, Inc. All rights reserved.
 **************************************************************************
 */

#include "wiBFfixpt.h"
#include "wiBFutil.h"
#include "wiProcess.h"

#include "Phy11n_BFDemapper.h"
#include "Phy11n.h"

#ifdef	ERROR
#undef	ERROR
#endif
#define	ERROR(s) _error(__FILE__,__LINE__,(s))

using namespace std;

namespace WiBF {

Wfixpt::Wfixpt(void) : vtype(REAL), enable(true)
{
    frac= -1; scl=0;
    a.Real= a.Imag=0;
    _inits();
}

Wfixpt::Wfixpt(const Wfixpt& p) : vtype(p.vtype), enable(p.enable)
{
    frac=p.frac; scl=p.scl; a=p.a;
    _inits();
}

Wfixpt::Wfixpt(const wiComplex& w) : vtype(CPLX), enable(true)
{
    frac= -1; scl=0;
    a = w;
    _inits();
}

Wfixpt::Wfixpt(const double& x) : vtype(REAL), enable(true)
{
    frac= -1; scl=0;
    a.Real = x;
    a.Imag = 0;
    _inits();
}

Wfixpt::Wfixpt(vtype_t t, int p) : vtype(t), enable(true)
{
    _setfrac(p);
    a.Real= a.Imag= 0;
    _inits();
}

Wfixpt::Wfixpt(const char* n, vtype_t t, int p, bool e) :
    name(n), vtype(t), enable(e)
{
    _setfrac(p);
    a.Real= a.Imag= 0;
    _inits();
}

Wfixpt::Wfixpt(const char* n, vtype_t t, bool e) :
    name(n), vtype(t), enable(e)
{
    _setfrac(_getfracbyname(n));
    a.Real= a.Imag= 0;
    _inits();
}

Wfixpt::~Wfixpt(void) {}

Wfixpt::smap_t*
Wfixpt::getsmap(void)
{
    const Phy11n_RxState_t* pRX= Phy11n_RxState();
    BFextra_s* bx= (BFextra_s*)(pRX->BFDemapper.bfextra);
    if (bx==NULL) return NULL;
    return &bx->smap;
}

// register named variables for stats
void Wfixpt::_inits(void)
{
    stats = NULL;
    if (name.empty()) return;
    const int tid= wiProcess_ThreadIndex();
    if (tid < 0 || tid >= WISE_MAX_THREADS)
	ERROR("unexpected error");
    smap_t* sm= getsmap();
    smap_t::iterator sk= sm->find(name);
    if (sk==sm->end()) {
	stats = &(*sm)[name];
	stats->frac = frac;
    } else {
	stats = &sk->second;
	if (stats->frac != frac)
	    ERROR("unexpected error");
    }
}

int Wfixpt::_getfracbyname(const char* n)
{
    const int tid= wiProcess_ThreadIndex();
    if (tid < 0 || tid >= WISE_MAX_THREADS)
	ERROR("unexpected error");
    const smap_t* sm= getsmap();
    smap_t::const_iterator sk= sm->find(n);
    if (sk==sm->end())
	return -1;
    return sk->second.frac;
}

void Wfixpt::_setfrac(int p)
{
    if (!enable) p= -1;
    if (p < 0 || p > 30) {
	frac= -1; scl=0;
    } else {
	frac=p; scl=1<<p;
    }
}

inline void Wfixpt::_updates(void)
{
    if (stats!=NULL)
	stats->update(this);
}

bool Wfixpt::isfixpt(void) const {return frac >= 0;}

void Wfixpt::setfrac(int p)
{
    _setfrac(p);
    rounds(a);
}

void Wfixpt::real(const double& x) {a.Real=rounds(x);}
void Wfixpt::imag(const double& y) {a.Imag=rounds(y);}

wiComplex Wfixpt::limit(const wiComplex& x, double hi)
{
    double lo= -hi;
    wiComplex z= x;
    if (z.Real > hi) z.Real=hi;
    else if (z.Real < lo) z.Real=lo;
    if (z.Imag > hi) z.Imag=hi;
    else if (z.Imag < lo) z.Imag=lo;
    return z;
}

double Wfixpt::limit(const double& a, double hi)
{
    double lo= -hi;
    double b= a;
    if (b >  hi) b=hi;
    else if (b < lo) b=lo;
    return b;
}

Wfixpt& Wfixpt::operator= (const Wfixpt& p)
{
    if (isreal() && !p.isreal())
	ERROR("invalid assignment");
    _rounds(p);
    _updates();
    return *this;
}

#define WIBFFIXPT_SELF_OP_DEF(_op_, _fun_) \
    Wfixpt& Wfixpt::operator _op_ ## = (const Wfixpt& p) { \
	if (isreal() && p.isreal()) \
	    a.Real = rounds(real() _op_ p.real()); \
	else \
	    rounds(a, wiComplex ## _fun_ (a, p.a)); \
	_updates(); \
	return *this; \
    }
WIBFFIXPT_SELF_OP_DEF(+,Add)
WIBFFIXPT_SELF_OP_DEF(-,Sub)
WIBFFIXPT_SELF_OP_DEF(*,Mul)
WIBFFIXPT_SELF_OP_DEF(/,Div)
#undef WIBFFIXPT_SELF_OP_DEF

Wfixpt Wfixpt::operator- (void) const {
    Wfixpt p(vtype, frac);
    p.real(-real());
    p.imag(-imag());
    return p;
}

#define WIBFFIXPT_OP_DEF(_op_, _fun_) \
    Wfixpt operator _op_ (const Wfixpt& p, const Wfixpt& r) { \
	const int pf= p.getfrac(); \
	const int rf= r.getfrac(); \
	int f; \
	if (pf < 0 || rf < 0) f= -1; \
	else f= std::max(pf,rf); \
	if (p.isreal() && r.isreal()) { \
	    Wfixpt q(Wfixpt::REAL, f); \
	    q.real(p.real() _op_ r.real()); \
	    return q; \
	} \
	Wfixpt q(Wfixpt::CPLX, f); \
	q = wiComplex ## _fun_ (p.toComplex(), r.toComplex()); \
	return q; \
    } \
    Wfixpt operator _op_ (const Wfixpt& p, const double& x) { \
	if (p.isreal()) { \
	    Wfixpt q(Wfixpt::REAL, p.getfrac()); \
	    q.real(p.real() _op_ x); \
	    return q; \
	} \
	Wfixpt q(Wfixpt::CPLX, p.getfrac()); \
	wiComplex w={x,0}; \
	q = wiComplex ## _fun_ (p.toComplex(), w); \
	return q; \
    } \
    Wfixpt operator _op_ (const double& x, const Wfixpt& p) { \
	if (p.isreal()) { \
	    Wfixpt q(Wfixpt::REAL, p.getfrac()); \
	    q.real(x _op_ p.real()); \
	    return q; \
	} \
	Wfixpt q(Wfixpt::CPLX, p.getfrac()); \
	wiComplex w={x,0}; \
	q = wiComplex ## _fun_ (w, p.toComplex()); \
	return q; \
    }
WIBFFIXPT_OP_DEF(+,Add)
WIBFFIXPT_OP_DEF(-,Sub)
WIBFFIXPT_OP_DEF(*,Mul)
WIBFFIXPT_OP_DEF(/,Div)
#undef WIBFFIXPT_OP_DEF

void Wfixpt::_error(const char* f, int l, const char* m) {
    throw error(f,l,m);
}

}
