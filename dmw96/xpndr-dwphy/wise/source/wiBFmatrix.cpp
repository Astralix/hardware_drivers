/*
 **************************************************************************
 * wiMatrixpp.cpp
 * created: Sun Jan 23 12:43:14 JST 2011
 *-------------------------------------------------------------------------
 * class "Xgslmat" definitions.
 * class "Wgslmat" definitions.
 * adapter class for wiCMat.
 * note: class names contain "gsl" for compatibility reasons.
 * this code does not depend on gsl libs.
 *-------------------------------------------------------------------------
 * Author: Leonardo Vainsencher
 *-------------------------------------------------------------------------
 * Copyright (c) 2011 DSP Group, Inc. All rights reserved.
 **************************************************************************
 */

#include "wiBFmatrix.h"
#include "wiBFutil.h"
#include "wiBFformat.h"
#include <complex>
#include <cmath>

#ifdef	_MSC_VER
#include <malloc.h>
#endif

#ifdef	ERROR
#undef	ERROR
#endif
#define ERROR(s) throw error(__FILE__,__LINE__,(s))
#define CHECK(s) if ((s)!=WI_SUCCESS) \
		    throw error(__FILE__,__LINE__,"unexpected error")

using namespace std;
using WiBF::error;

namespace WiBF {

// matrix base class
// null matrix ctor
Xgslmat::Xgslmat(void) : g(NULL), long_fmt(false) {}

// basic constructor ctor
Xgslmat::Xgslmat(int nrows, int ncols) :
    g(NULL), long_fmt(false)
{
    init(nrows,ncols,true);
}

// copy ctor
Xgslmat::Xgslmat(const Xgslmat& m) :
    g(NULL), long_fmt(m.long_fmt)
{
    if (m.empty()) return;
    init(m.rows(), m.cols());
    CHECK(wiCMatCopy(m.g,g));
}

// construct from CMat
Xgslmat::Xgslmat(const wiCMat* m) :
    g(NULL), long_fmt(false)
{
    init(m->row, m->col);
    CHECK(wiCMatCopy(m,g));
}

// construct from or take ownership of wiCMat
Xgslmat::Xgslmat(wiCMat* m, bool _take) :
    g(NULL), long_fmt(false)
{
    if (_take)
	g = m;
    else {
	init(m->row, m->col);
	CHECK(wiCMatCopy(m,g));
    }
}

// construct 1x1 matrix
Xgslmat::Xgslmat(const wiComplex& w) :
    g(NULL), long_fmt(false)
{
    init(1,1);
    g->val[0][0]=w;
}

Xgslmat::Xgslmat(const double& x) :
    g(NULL), long_fmt(false)
{
    init(1,1);
    g->val[0][0].Real=x;
    g->val[0][0].Imag=0;
}

template <typename T>
Xgslmat::Xgslmat(const vector<T>& v) :
    g(NULL), long_fmt(false)
{
    if (v.empty()) return;
    init(1,v.size());
    wiComplex w={0,0};
    for (unsigned k=0; k < v.size(); k++) {
	w.Real = v[k];
	CHECK(wiCMatSetElement(g, &w, 0, k));
    }
}

template <typename T>
Xgslmat::Xgslmat(const vector<T>& v, int nr, int nc) :
    g(NULL), long_fmt(false)
{
    if ((int)v.size() != nr*nc)
	ERROR("size mismatch");
    if (v.empty()) return;
    init(nr,nc);
    wiComplex w={0,0};
    int i,j,k;
    for (k=i=0; i < nr; i++)
	for (j=0; j < nc; j++, k++) {
	    w.Real = v[k];
	    CHECK(wiCMatSetElement(g, &w, i, j));
	}
}

template Xgslmat::Xgslmat(const ivector_t&);
template Xgslmat::Xgslmat(const ivector_t&, int, int);
template Xgslmat::Xgslmat(const dvector_t&);
template Xgslmat::Xgslmat(const dvector_t&, int, int);

// destroy
Xgslmat::~Xgslmat(void) {free(NULL);}

void Xgslmat::init(int nrows, int ncols, bool clrf)
{
    wiStatus st= WI_SUCCESS;

    if (nrows < 1 || ncols < 1) {
	free(); return;
    }

    if (g==NULL) {
	g = new wiCMat;
	st = wiCMatInit(g, nrows, ncols);
    } else if (nrows != g->row || ncols != g->col)
	st = wiCMatReInit(g,nrows,ncols);
    else if (clrf)
	st = wiCMatSetZero(g);

    if (st!=WI_SUCCESS)
	ERROR("unexpected error");
}

void Xgslmat::free(wiCMat* h)
{
    if (g!=NULL) {
	CHECK(wiCMatFree(g));
	delete g;
    }
    g = h;
}

// return pointer to underlying wiCMat
const wiCMat* Xgslmat::getg(void) const {return g;}

// take ownership of external wiCMat
void Xgslmat::take(wiCMat* m) {free(m);}

int Xgslmat::rows(void) const {return g!=NULL ? g->row : 0;}

int Xgslmat::cols(void) const {return g!=NULL ? g->col : 0;}

int Xgslmat::numel(void) const {return rows()*cols();}

bool Xgslmat::empty(void) const {return g==NULL;}

Xgslmat::ivector_t
Xgslmat::size(void) const
{
    ivector_t s(2);
    s[0]=rows(); s[1]=cols();
    return s;
}

// resize matrix
void Xgslmat::clear(void) {resize(0,0);}

void Xgslmat::resize(const Xgslmat& m) {resize(m.rows(), m.cols());}

void Xgslmat::resize(int ncols) {resize(1,ncols);}

void Xgslmat::resize(int nrows, int ncols)
{
    if (rows()==nrows && cols()==ncols) return;
    if (nrows < 1 || ncols < 1) {
	free(); return;
    }
    if (empty()) {
	init(nrows,ncols,true); return;
    }
    wiCMat* h= new wiCMat;
    CHECK(wiCMatInit(h, nrows, ncols));
    const int nr= std::min(rows(),nrows);
    const int nc= std::min(cols(),ncols);
    for (int i=0; i < nr; i++)
	for (int j=0; j < nc; j++) {
	    wiComplex w= (*this)(i,j);
	    CHECK(wiCMatSetElement(h, &w, i, j));
	}
    free(h);
}

Xgslmat& Xgslmat::format_long(bool f) {long_fmt=f; return *this;}

double Xgslmat::norm2(void) const
{
    const int nr= rows();
    const int nc= cols();
    double x=0;
    for (int i=0; i < nr; i++)
	for (int j=0; j < nc; j++)
	    x += wiComplexNormSquared(g->val[i][j]);
    return x;
}

double Xgslmat::minabs(void) const
{
    const int nr=rows();
    const int nc=cols();
    int i,j;
    double amin=0;
    for (i=0; i < nr; i++) {
	for (j=0; j < nc; j++) {
	    double x= wiComplexNormSquared(g->val[i][j]);
	    if ((i==0 && j==0) || amin > x) amin=x;
	}
    }
    return sqrt(amin);
}

double Xgslmat::maxabs(void) const
{
    const int nr=rows();
    const int nc=cols();
    int i,j;
    double amax=0;
    for (i=0; i < nr; i++) {
	for (j=0; j < nc; j++) {
	    double x= wiComplexNormSquared(g->val[i][j]);
	    if ((i==0 && j==0) || amax < x) amax=x;
	}
    }
    return sqrt(amax);
}

Xgslmat Xgslmat::row(int r) const
{
    if (!indexok(r,0))
	ERROR("matrix index out of range");
    Xgslmat res(1, cols());
    for (int c=0; c < cols(); c++) res(0,c)= (*this)(r,c);
    return res;
}

Xgslmat Xgslmat::col(int c) const
{
    if (!indexok(0,c))
	ERROR("matrix index out of range");
    Xgslmat res(rows(), 1);
    for (int r=0; r < rows(); r++) res(r,0)= (*this)(r,c);
    return res;
}

void Xgslmat::setrow(int r, const Xgslmat& rv)
{
    if (!indexok(r,0))
	ERROR("matrix index out of range");
    const int nc= cols();
    if (rv.numel()!=nc)
	ERROR("argument size mismatch");
    for (int c=0; c < nc; c++) (*this)(r,c)= rv(c);
}

void Xgslmat::setcol(int c, const Xgslmat& cv)
{
    if (!indexok(0,c))
	ERROR("matrix index out of range");
    const int nr= rows();
    if (cv.numel()!=nr)
	ERROR("argument size mismatch");
    for (int r=0; r < nr; r++) (*this)(r,c)= cv(r);
}

// return transpose
Xgslmat Xgslmat::transpose(void) const
{
    Xgslmat t;
    transpose(t);
    return t;
}

Xgslmat Xgslmat::ctranspose(void) const
{
    Xgslmat t;
    ctranspose(t);
    return t;
}

void Xgslmat::transpose(Xgslmat& t) const
{
    if (empty())
	t.clear();
    else {
	t.resize(cols(), rows());
	CHECK(wiCMatTrans(t.g, g));
    }
}

void Xgslmat::ctranspose(Xgslmat& t) const
{
    if (empty())
	t.clear();
    else {
	t.resize(cols(), rows());
	CHECK(wiCMatConjTrans(t.g, g));
    }
}

// compute inverse of LT matrix using back-substitution
Xgslmat Xgslmat::ltinverse(void) const
{
    if (g==NULL)
	ERROR("invalid matrix op");
    const int n= rows();
    if (cols()!=n)
	ERROR("not a square matrix");
    Xgslmat res(n,n);
    wiComplex x;
    int i,j,k;
    for (i=0; i < n; i++)
	res(i,i) = wiComplexInv((*this)(i,i));
    for (j=0; j < n-1; j++) {
	for (i=j+1; i < n; i++) {
	    x.Real= x.Imag= 0;
	    for (k=0; k < i; k++)
		x = x + res(k,j) * (*this)(i,k);
	    res(i,j) = -x * res(i,i);
	}
    }
    return res;
}

// perform QR decomposition with pivoting
void Xgslmat::qrp(Xgslmat& Q, Xgslmat& R, Wgslmat& P) const
{
    qrp(Q, R, P, SORT_REGULAR);
}

void Xgslmat::sqrp(Xgslmat& Q, Xgslmat& R, Wgslmat& P) const
{
    qrp(Q, R, P, SORT_REVERSE);
}

void Xgslmat::qrp(Xgslmat& Q, Xgslmat& R, Wgslmat& P, sort_mode_t sm) const
{
    const int M= rows();
    const int N= cols();
    int i;
    if (g==NULL)
	ERROR("invalid matrix op");
    if (M < N)
	ERROR("invalid QRP op");
    Q.resize(M,N);
    R.resize(N,N);
    P.resize(1,N);
    if (sm==SORT_REGULAR) {
	// note: in this implementation SORT_REGULAR means NO sorting!
	for (i=0; i < N; i++) P(i)=i;
	CHECK(wiCMatQR(g, Q.g, R.g));
    } else if (sm==SORT_REVERSE) {
#ifdef _MSC_VER
	wiInt* _p= (wiInt*)_malloca(N*sizeof(wiInt));
#else
	wiInt _p[N];
#endif
	CHECK(wiCMatAscendingSortQR(g, Q.g, R.g, _p));
	for (i=0; i < N; i++) P(i)=_p[i];
    } else
	ERROR("invalid QR pivoting mode");
}

// perform row "gathering".
// output matrix is formed by gathering rows from this.
// p contains indices of source rows to pick from this.
Xgslmat Xgslmat::permute_rows(const Wgslmat& p) const
{
    Xgslmat res;
    permute_rows(res,p);
    return res;
}

void Xgslmat::permute_rows(Xgslmat& res, const Wgslmat& p) const
{
    res.resize(p.numel(), cols());
    for (int r=0; r < p.numel(); r++) {
	const int k= (int)p(r);
	for (int c=0; c < cols(); c++)
	    res(r,c) = (*this)(k,c);
    }
}


// perform column "gathering".
// output matrix is formed by gathering columns from this.
// p contains indices of source columns to pick from this.
Xgslmat Xgslmat::permute_cols(const Wgslmat& p) const
{
    Xgslmat res;
    permute_cols(res,p);
    return res;
}

void Xgslmat::permute_cols(Xgslmat& res, const Wgslmat& p) const
{
    res.resize(rows(), p.numel());
    for (int c=0; c < p.numel(); c++) {
	const int k= (int)p(c);
	for (int r=0; r < rows(); r++)
	    res(r,c) = (*this)(r,k);
    }
}

// perform row "scattering".
// output matrix is formed by scattering rows from this.
// p contains indices of destination rows.
// only the first rows() elements of p are considered.
void Xgslmat::getrows(const Wgslmat& p, Xgslmat& res) const
{
    const int nr= std::min(p.numel(), rows());
    const int nc= cols();
    int c,k,r;
    if (res.empty()) {
	int rmax=0;
	for (r=0; r < nr; r++) {
	    k = (int)p(r);
	    if (rmax < k) rmax=k;
	}
	res.resize(rmax+1, nc);
    }
    for (r=0; r < nr; r++) {
	k = (int)p(r);
	for (c=0; c < nc; c++)
	    res(k,c) = (*this)(r,c);
    }
}

// perform column "scattering".
// output matrix is formed by scattering columns from this.
// p contains indices of destination columns.
// only the first cols() elements of p are considered.
void Xgslmat::getcols(const Wgslmat& p, Xgslmat& res) const
{
    const int nr= rows();
    const int nc= std::min(p.numel(), cols());
    int c,k,r;
    if (res.empty()) {
	int cmax=0;
	for (c=0; c < nc; c++) {
	    k = (int)p(c);
	    if (cmax < k) cmax=k;
	}
	res.resize(nr, cmax+1);
    }
    for (c=0; c < nc; c++) {
	k = (int)p(c);
	for (r=0; r < nr; r++)
	    res(r,k) = (*this)(r,c);
    }
}

Xgslmat Xgslmat::inverse_permute_rows(const Wgslmat& p) const
{
    Xgslmat res;
    getrows(p,res);
    return res;
}

Xgslmat Xgslmat::inverse_permute_cols(const Wgslmat& p) const
{
    Xgslmat res;
    getcols(p,res);
    return res;
}

// return indices of k smallest elements
// indices are row-wise (unlike matlab)!
size_t Xgslmat::sort(size_t* idx, size_t _k) const
{
    if (g==NULL) return 0;
    const size_t n= g->row * g->col;
    const size_t k= std::min(_k,n);
    if (k < 1) return 0;
    size_t i,i1,j;
    double xbound, xi;
    const Xgslmat& src= *this;
    i=j=1; idx[0]=0;
    xbound = wiComplexNormSquared(src(0));
    for (i=1; i < n; i++) {
	xi = wiComplexNormSquared(src(i));
	if (j < k) j++;
	else if (xi >= xbound) continue;

	for (i1=j-1; i1 > 0 ; i1--) {
	    int p1= idx[i1-1];
	    double x= wiComplexNormSquared(src(p1));
	    if (xi > x) break;
	    idx[i1] = p1;
	}

	idx[i1] = i;
	xbound = wiComplexNormSquared(src(idx[j-1]));
    }

    return k;
}

// retrieve element
wiComplex Xgslmat::operator() (int m, int n) const
{
    wiComplex w;
    if (g==NULL || wiCMatGetElement(g, &w, m, n) != WI_SUCCESS)
	ERROR("matrix index out of range");
    return w;
}

// retrieve element (as ref)
wiComplex& Xgslmat::operator() (int m, int n)
{
    if (!indexok(m,n))
	ERROR("matrix index out of range");
    return g->val[m][n];
}

// retrieve element from single index
// index is row-wise (unlike matlab)!
wiComplex Xgslmat::operator() (int k) const
{
    const int nc= cols();
    const int c= k % nc;
    const int r= k / nc;
    return (*this)(r,c);
}

// retrieve element (as ref) from single index
// index is row-wise (unlike matlab)!
wiComplex& Xgslmat::operator() (int k)
{
    const int nc= cols();
    const int c= k % nc;
    const int r= k / nc;
    return (*this)(r,c);
}

// set element
void Xgslmat::operator() (int m, int n, const wiComplex& w)
{
    wiComplex z=w;
    if (g==NULL || wiCMatSetElement(g, &z, m, n) != WI_SUCCESS)
	ERROR("matrix index out of range");
}

void Xgslmat::operator() (int m, int n, const double& x)
{
    (*this)(m,n,_complex(x,0));
}

// set submatrix
void Xgslmat::operator() (const Xgslmat& x, int r, int c)
{
    if (x.empty()) return;
    if (empty())
	ERROR("matrix index out of range");
    CHECK(wiCMatSetSubMtx(g, x.g, r, c, r+x.rows()-1, c+x.cols()-1));
}

// get submatrix
Xgslmat Xgslmat::operator() (int r, int c, int nr, int nc) const
{
    if (empty())
	ERROR("invalid submatrix");
    if (r < 0 || c < 0 || nr < 1 || nc < 1)
	ERROR("invalid submatrix");
    if (rows() < r+nr || cols() < c+nc)
	ERROR("invalid submatrix");
    Xgslmat t(nr,nc);
    CHECK(wiCMatGetSubMtx(g, t.g, r, c, r+nr-1, c+nc-1));
    return t;
}

// assignment
Xgslmat& Xgslmat::operator= (const Xgslmat& m)
{
    if (m.empty()) {
	free();
	return *this;
    }
    init(m.rows(), m.cols(), true);
    CHECK(wiCMatCopy(m.g,g));
    return *this;
}

Xgslmat& Xgslmat::operator= (const double& x)
{
    if (g==NULL)
	init(1,1);
    for (int i=0; i < rows(); i++)
	for (int j=0; j < cols(); j++)
	    (*this)(i,j,x);
    return *this;
}

template<typename T>
Xgslmat& Xgslmat::operator= (const vector<T>& v)
{
    if (v.empty()) {
	free(); return *this;
    }
    const int nel= v.size();
    const bool keepsize=
	(rows()==1 && cols()==nel) || (cols()==1 && rows()==nel);
    if (!keepsize)
	init(1,v.size());
    for (int i=0; i < nel; i++) (*this)(i) = _complex(v[i],0);
    return *this;
}

template Xgslmat& Xgslmat::operator= (const ivector_t&);
template Xgslmat& Xgslmat::operator= (const dvector_t&);

// matrix in-place addition
Xgslmat& Xgslmat::operator+= (const Xgslmat& m)
{
    if (g==NULL)
	init(m.rows(), m.cols(), true);
    else if (!sizematch(m))
	ERROR("matrix size mismatch");
    if (!empty()) {
	CHECK(wiCMatAddSelf(g,m.g));
    }
    return *this;
}

// matrix ops
#define XGSLMAT_OP(_op_, _fun_) \
    Xgslmat Xgslmat::operator _op_ (const Xgslmat& m) const \
    { \
	if (empty() || m.empty()) \
	    throw error(__FILE__,__LINE__,"invalid null matrix op"); \
	Xgslmat res(rows(),cols()); \
	if ((wiCMat ## _fun_ (res.g, g, m.g)) != WI_SUCCESS) \
	    throw error(__FILE__,__LINE__,"unexpected error"); \
	return res; \
    }
XGSLMAT_OP(+,Add)
XGSLMAT_OP(-,Sub)
//XGSLMAT_OP(*,Mul)
#undef XGSLMAT_OP
Xgslmat Xgslmat::operator*(const Xgslmat& m) const
{
    if (empty() || m.empty())
	throw error(__FILE__,__LINE__,"invalid null matrix op"); \
    Xgslmat res(rows(),m.cols());
    if ((wiCMatMul(res.g, g, m.g)) != WI_SUCCESS)
	throw error(__FILE__,__LINE__,"unexpected error");
    return res;
}

bool Xgslmat::operator== (const Xgslmat& m) const
{
    if (empty())
	ERROR("invalid null matrix op");
    if (this->size() != m.size())
	ERROR("matrix size mismatch");
    const int nr= rows();
    const int nc= cols();
    for (int i=0; i < nr; i++)
	for (int j=0; j < nc; j++)
	    if ((*this)(i,j) != m(i,j)) return false;
    return true;
}

bool Xgslmat::operator== (const wiComplex& w) const
{
    if (empty())
	ERROR("invalid null matrix op");
    const int nr= rows();
    const int nc= cols();
    for (int i=0; i < nr; i++)
	for (int j=0; j < nc; j++)
	    if ((*this)(i,j) != w) return false;
    return true;
}

bool Xgslmat::operator== (const double& x) const
{
    if (g==NULL)
	ERROR("invalid null matrix op");
    const int nr= rows();
    const int nc= cols();
    for (int i=0; i < nr; i++)
	for (int j=0; j < nc; j++)
	    if ((*this)(i,j) != x) return false;
    return true;
}

bool Xgslmat::operator!= (const Xgslmat& m) const {return !(*this == m);}
bool Xgslmat::operator!= (const wiComplex& w) const {return !(*this == w);}
bool Xgslmat::operator!= (const double& x) const {return !(*this == x);}

bool Xgslmat::sizematch(const Xgslmat& m) const
{
    if (g==NULL) return false;
    if (m.g==NULL) return false;
    if (rows() != m.rows()) return false;
    if (cols() != m.cols()) return false;
    return true;
}

bool Xgslmat::indexok(int m, int n) const
{
    if (g==NULL) return false;
    if (m < 0 || n < 0) return false;
    if (m >= rows() || n >= cols()) return false;
    return true;
}

// matrix output to stream
void Xgslmat::puts(ostream& o) const
{
    if (numel()==0) return;
    const int nr= rows();
    const int nc= cols();
    bool fx=true,fr=true;
    string fm;
    wiComplex w;
    double x;
    int r,c;
    for (r=0; r < nr; r++) {
	for (c=0; c < nc; c++) {
	    w = (*this)(r,c);
	    x = _real(w);
	    if (std::floor(x)!=x) fx=false;
	    x = _imag(w);
	    if (std::floor(x)!=x) fx=false;
	    if (x!=0) fr=false;
	}
	if (!fx && !fr) break;
    }
    if (fx) fm=" %2.0f";
    else if (long_fmt) fm=" %18.15g";
    else fm=" %8.4f";
    for (r=0; r < nr; r++) {
	for (c=0; c < nc; c++) {
	    w = (*this)(r,c);
	    o << Wformat(fm.c_str(), _real(w));
	    if (!fr) {
		x = _imag(w);
		if (x < 0) o << "-";
		else o << "+";
		o << Wformat(fm.c_str(), std::abs(x)) << "i";
	    }
	}
	if (r < nr-1) o << endl;
    }
}

void Xgslmat::puts(void) const {puts(cout); cout << endl;}

// matrix input from stream
void Xgslmat::gets(istream& i)
{
    const int nr= rows();
    const int nc= cols();
    complex<double> w;
    for (int r=0; r < nr; r++)
	for (int c=0; c < nc; c++) {
	    i >> w;
	    (*this)(r,c,_complex(w.real(),w.imag()));
	}
}

// some useful complex ops
bool operator== (const wiComplex& w, const wiComplex& z)
    { return _real(w)==_real(z) && _imag(w)==_imag(z); }

bool operator== (const wiComplex& w, const double& x)
    { return _real(w)==x && _imag(w)==0; }

wiComplex operator- (const wiComplex& z) {return wiComplexNeg(z);}

#define WCOMPLEX_COP(_op_, _fun_) \
    wiComplex operator _op_ (const wiComplex& w, const wiComplex& z) \
	{ return wiComplex ## _fun_ (w,z); } \
    wiComplex operator _op_ (const wiComplex& w, const double& x) \
	{ wiComplex z={x,0}; return wiComplex ## _fun_ (w,z); } \
    wiComplex operator _op_ (const double& x, const wiComplex& w) \
	{ return w _op_ x; }

#define WCOMPLEX_NCOP(_op_, _fun_) \
    wiComplex operator _op_ (const wiComplex& w, const wiComplex& z) \
	{ return wiComplex ## _fun_ (w,z); }

WCOMPLEX_COP(+,Add)
WCOMPLEX_COP(*,Mul)
WCOMPLEX_NCOP(-,Sub)
WCOMPLEX_NCOP(/,Div)
wiComplex operator- (const wiComplex& w, const double& x) {
    return _complex(w.Real-x, w.Imag);
}
wiComplex operator- (const double& x, const wiComplex& w) {
    return _complex(x-w.Real, -w.Imag);
}
wiComplex operator/ (const wiComplex& w, const double& x) {
    return _complex(w.Real/x, w.Imag/x);
}
wiComplex operator/ (const double& x, const wiComplex& w) {
    wiComplex z= wiComplexInv(w);
    return _complex(x*z.Real, x*z.Imag);
}
#undef WCOMPLEX_COP
#undef WCOMPLEX_NCOP

///////////////////////////////////////////////////////////////////////
// real matrices
// null matrix
Wgslmat::Wgslmat(void) : Xgslmat() {}

// basic constructor
Wgslmat::Wgslmat(int nrows, int ncols) : Xgslmat(nrows,ncols) {}

// copy constructor
Wgslmat::Wgslmat(const Wgslmat& m) : Xgslmat(m) {}

// construct from CMat
Wgslmat::Wgslmat(const wiCMat* m) : Xgslmat(m) {}

// construct from or take ownership of wiCMat
Wgslmat::Wgslmat(wiCMat* m, bool _take) : Xgslmat(m,_take) {}

// construct 1x1 matrix
Wgslmat::Wgslmat(const double& x) : Xgslmat(x) {}

template <typename T>
Wgslmat::Wgslmat(const vector<T>& v) : Xgslmat(v) {}

template <typename T>
Wgslmat::Wgslmat(const vector<T>& v, int nr, int nc) : Xgslmat(v,nr,nc) {}

template Wgslmat::Wgslmat(const ivector_t&);
template Wgslmat::Wgslmat(const ivector_t&, int, int);
template Wgslmat::Wgslmat(const dvector_t&);
template Wgslmat::Wgslmat(const dvector_t&, int, int);

// destroy
Wgslmat::~Wgslmat(void) {}

Wgslmat Wgslmat::row(int r) const
{
    if (!indexok(r,0))
	ERROR("matrix index out of range");
    Wgslmat res(1, cols());
    for (int c=0; c < cols(); c++) res(0,c)= (*this)(r,c);
    return res;
}

Wgslmat Wgslmat::col(int c) const
{
    if (!indexok(0,c))
	ERROR("matrix index out of range");
    Wgslmat res(rows(), 1);
    for (int r=0; r < rows(); r++) res(r,0)= (*this)(r,c);
    return res;
}

Wgslmat& Wgslmat::format_long(bool f) {long_fmt=f; return *this;}

// return transpose
Wgslmat Wgslmat::transpose(void) const
{
    Wgslmat t; Xgslmat::transpose(t); return t;
}

// compute inverse of LT matrix using back-substitution
Wgslmat Wgslmat::ltinverse(void) const
{
    if (empty())
	ERROR("invalid matrix op");
    const int n= rows();
    if (cols()!=n)
	ERROR("invalid matrix inversion");
    Wgslmat res(n,n);
    double x;
    int i,j,k;
    for (i=0; i < n; i++)
	res(i,i) = 1.0/(*this)(i,i);
    for (j=0; j < n-1; j++) {
	for (i=j+1; i < n; i++) {
	    for (x=k=0; k < i; k++)
		x += res(k,j) * (*this)(i,k);
	    res(i,j) = -x * res(i,i);
	}
    }
    return res;
}

// perform row "gathering".
// output matrix is formed by gathering rows from this.
// p contains indices of source rows to pick from this.
Wgslmat Wgslmat::permute_rows(const Wgslmat& p) const
{
    Wgslmat res;
    Xgslmat::permute_rows(res,p);
    return res;
}

// perform column "gathering".
// output matrix is formed by gathering columns from this.
// p contains indices of source columns to pick from this.
Wgslmat Wgslmat::permute_cols(const Wgslmat& p) const
{
    Wgslmat res;
    Xgslmat::permute_cols(res,p);
    return res;
}

Wgslmat Wgslmat::inverse_permute_rows(const Wgslmat& p) const
{
    Xgslmat res;
    getrows(p,res);
    return *(static_cast<Wgslmat*>(&res));
}

Wgslmat Wgslmat::inverse_permute_cols(const Wgslmat& p) const
{
    Xgslmat res;
    getcols(p,res);
    return *(static_cast<Wgslmat*>(&res));
}

// return indices of k smallest elements
// indices are row-wise (unlike matlab)!
size_t Wgslmat::sort(size_t* idx, size_t _k) const
{
    if (empty()) return 0;
    const size_t n= numel();
    const size_t k= std::min(_k,n);
    if (k < 1) return 0;
    size_t i,i1,j;
    double xbound, xi;
    const Wgslmat& src= *this;
    i=j=1; idx[0]=0;
    xbound = src(0);
    for (i=1; i < n; i++) {
	xi = src(i);
	if (j < k) j++;
	else if (xi >= xbound) continue;

	for (i1=j-1; i1 > 0 ; i1--) {
	    int p1= idx[i1-1];
	    double x= src(p1);
	    if (xi > x) break;
	    idx[i1] = p1;
	}

	idx[i1] = i;
	xbound = src(idx[j-1]);
    }

    return k;
}

// retrieve element
double Wgslmat::operator() (int m, int n) const
{
    return _real(Xgslmat::operator()(m,n));
}

// retrieve element (as ref)
double& Wgslmat::operator() (int m, int n)
{
    if (!indexok(m,n))
	ERROR("matrix index out of range");
    return g->val[m][n].Real;
}

// retrieve element from single index
// index is row-wise (unlike matlab)!
double Wgslmat::operator() (int k) const
{
    const int nc= cols();
    const int c= k % nc;
    const int r= k / nc;
    return (*this)(r,c);
}

// retrieve element (as ref) from single index
// index is row-wise (unlike matlab)!
double& Wgslmat::operator() (int k)
{
    const int nc= cols();
    const int c= k % nc;
    const int r= k / nc;
    return (*this)(r,c);
}

// get submatrix
Wgslmat Wgslmat::operator() (int r, int c, int nr, int nc) const
{
    if (empty())
	ERROR("invalid submatrix");
    if (r < 0 || c < 0 || nr < 1 || nc < 1)
	ERROR("invalid submatrix");
    if (rows() < r+nr || cols() < c+nc)
	ERROR("invalid submatrix");
    Wgslmat t(nr,nc);
    CHECK(wiCMatGetSubMtx(g, t.g, r, c, r+nr-1, c+nc-1));
    return t;
}

// set submatrix
void Wgslmat::operator() (const Wgslmat& m, int r, int c)
{
    Xgslmat::operator()(m,r,c);
}

// assignment
Wgslmat& Wgslmat::operator= (const Wgslmat& m)
{
    Xgslmat::operator=(m);
    return *this;
}

Wgslmat& Wgslmat::operator= (const double& x)
{
    Xgslmat::operator=(x);
    return *this;
}

template<typename T>
Wgslmat& Wgslmat::operator= (const vector<T>& v)
{
    Xgslmat::operator=(v);
    return *this;
}

template Wgslmat& Wgslmat::operator= (const ivector_t&);
template Wgslmat& Wgslmat::operator= (const dvector_t&);

// matrix in-place addition
Wgslmat& Wgslmat::operator+= (const Wgslmat& m)
{
    Xgslmat::operator+=(m);
    return *this;
}

// matrix ops
#define WGSLMAT_OP(_op_) \
    Wgslmat Wgslmat::operator _op_ (const Wgslmat& m) const \
    { \
	Xgslmat res= Xgslmat::operator _op_(m); \
	return *(static_cast<Wgslmat*>(&res)); \
    }

WGSLMAT_OP(+)
WGSLMAT_OP(-)
WGSLMAT_OP(*)
#undef WGSLMAT_OP

bool Wgslmat::operator== (const Wgslmat& m) const
{
    return Xgslmat::operator==(m);
}

bool Wgslmat::operator== (const double& x) const
{
    return Xgslmat::operator==(x);
}

bool Wgslmat::operator!= (const Wgslmat& m) const {return !(*this == m);}
bool Wgslmat::operator!= (const double& x) const {return !(*this == x);}

double Wgslmat::norm2(void) const
{
    const int nr= rows();
    const int nc= cols();
    double x=0;
    for (int i=0; i < nr; i++)
	for (int j=0; j < nc; j++) {
	    double a= (*this)(i,j);
	    x += a*a;
	}
    return x;
}

double Wgslmat::minabs(void) const
{
    const int nr=rows();
    const int nc=cols();
    int i,j;
    double amin=0;
    for (i=0; i < nr; i++) {
	for (j=0; j < nc; j++) {
	    double x= std::abs((*this)(i,j));
	    if ((i==0 && j==0) || amin > x) amin=x;
	}
    }
    return amin;
}

double Wgslmat::maxabs(void) const
{
    const int nr=rows();
    const int nc=cols();
    int i,j;
    double amax=0;
    for (i=0; i < nr; i++) {
	for (j=0; j < nc; j++) {
	    double x= std::abs((*this)(i,j));
	    if ((i==0 && j==0) || amax < x) amax=x;
	}
    }
    return amax;
}

// matrix output to stream
void Wgslmat::puts(ostream& o) const
{
    if (empty()) return;
    const int nr= rows();
    const int nc= cols();
    bool fx=true;
    string fm;
    int r,c;
    for (r=0; r < nr; r++) {
	for (c=0; c < nc; c++) {
	    double x= (*this)(r,c);
	    if (std::floor(x)!=x) {fx=false; break;}
	}
	if (!fx) break;
    }
    if (fx) fm=" %2.0f";
    else if (long_fmt) fm=" %18.15g";
    else fm=" %8.4f";
    for (r=0; r < nr; r++) {
	for (c=0; c < nc; c++)
	    o << Wformat(fm.c_str(), (*this)(r,c));
	if (r < nr-1) o << endl;
    }
}

void Wgslmat::puts(void) const {puts(cout); cout << endl;}

// matrix input from stream
void Wgslmat::gets(istream& i)
{
    const int nr= rows();
    const int nc= cols();
    for (int r=0; r < nr; r++)
	for (int c=0; c < nc; c++)
	    i >> (*this)(r,c);
}

ostream& operator<< (ostream& o, const Wgslmat& m) {m.puts(o); return o;}
istream& operator>> (istream& i, Wgslmat& m) {m.gets(i); return i;}

}
