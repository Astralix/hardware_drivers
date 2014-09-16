/*
 **************************************************************************
 * WIBFMATRIX.H
 * created: Sun Jan 23 12:43:14 JST 2011
 *-------------------------------------------------------------------------
 * class "Xgslmat" declarations: complex matrices, base class for Wgslmat.
 * class "Wgslmat" declarations: real matrices.
 * adapter class for wiCMat.
 * note: class names contain "gsl" for compatibility reasons.
 * this code does not depend on gsl libs.
 *-------------------------------------------------------------------------
 * Author: Leonardo Vainsencher
 *-------------------------------------------------------------------------
 * Copyright (c) 2011 DSP Group, Inc. All rights reserved.
 **************************************************************************
 */

#ifndef _WIBFMATRIX_H_
#define _WIBFMATRIX_H_

#include "wiMatrix.h"
#include "wiMath.h"
#include <vector>
#include <iostream>

namespace WiBF {

class Wgslmat;

class Xgslmat {
protected:
    typedef std::vector<int> ivector_t;
    typedef std::vector<double> dvector_t;

public:
    typedef enum {
	SORT_REGULAR=0, SORT_REVERSE=1
    } sort_mode_t;

    Xgslmat(void);
    Xgslmat(int nrows, int ncols);
    Xgslmat(const Xgslmat& m);
    Xgslmat(const wiCMat* m);
    Xgslmat(wiCMat* m, bool _take=false);
    Xgslmat(const wiComplex& w);
    Xgslmat(const double& x);
    template <typename T>
	Xgslmat(const std::vector<T>& v);
    template <typename T>
	Xgslmat(const std::vector<T>& v, int nr, int nc);
    virtual ~Xgslmat(void);

    const wiCMat* getg(void) const;
    void take(wiCMat* m);

    int rows(void) const;
    int cols(void) const;
    int numel(void) const;
    bool empty(void) const;
    ivector_t size(void) const;
    void clear(void);
    void resize(const Xgslmat& g);
    void resize(int ncols);
    void resize(int nrows, int ncols);
    Xgslmat& format_long(bool f);

    double norm2(void) const;
    double minabs(void) const;
    double maxabs(void) const;

public:
    // get element
    wiComplex operator() (int r, int c) const;
    wiComplex operator() (int k) const;
    wiComplex& operator() (int r, int c);
    wiComplex& operator() (int k);

    // set element
    void operator() (int r, int c, const wiComplex& x);
    void operator() (int r, int c, const double& x);

    // get sub-matrix
    Xgslmat row(int r) const;
    Xgslmat col(int c) const;
    void getrows(const Wgslmat& perm, Xgslmat& dest) const;
    void getcols(const Wgslmat& perm, Xgslmat& dest) const;
    Xgslmat operator() (int r, int c, int nr, int nc) const;

    // set sub-matrix
    void setrow(int r, const Xgslmat& rv);
    void setcol(int c, const Xgslmat& cv);
    virtual void operator() (const Xgslmat& x, int r, int c);

    // assignment
    Xgslmat& operator= (const Xgslmat& m);
    Xgslmat& operator= (const wiComplex& x);	// set all elements to x
    Xgslmat& operator= (const double& x);	// set all elements to x
    template <typename T>
	Xgslmat& operator= (const std::vector<T>& v);

    // self ops
    Xgslmat& operator+= (const Xgslmat& m);

    // dyadic ops
    Xgslmat operator+ (const Xgslmat& m) const;
    Xgslmat operator- (const Xgslmat& m) const;
    Xgslmat operator* (const Xgslmat& m) const;

    // comparison ops
    bool operator== (const Xgslmat& m) const;
    bool operator!= (const Xgslmat& m) const;
    bool operator== (const wiComplex& x) const;
    bool operator!= (const wiComplex& x) const;
    bool operator== (const double& x) const;
    bool operator!= (const double& x) const;

public:
    Xgslmat ctranspose(void) const;
    Xgslmat transpose(void) const;
    Xgslmat ltinverse(void) const;

    Xgslmat permute_rows(const Wgslmat& p) const;
    Xgslmat permute_cols(const Wgslmat& p) const;
    Xgslmat inverse_permute_rows(const Wgslmat& p) const;
    Xgslmat inverse_permute_cols(const Wgslmat& p) const;

    void qrp(Xgslmat& Q, Xgslmat& R, Wgslmat& P) const;
    void sqrp(Xgslmat& Q, Xgslmat& R, Wgslmat& P) const;

    size_t sort(size_t* p, size_t k) const;

    // matrix i/o
    void puts(std::ostream&) const;
    void gets(std::istream&);

protected:
    void permute_rows(Xgslmat&, const Wgslmat& p) const;
    void permute_cols(Xgslmat&, const Wgslmat& p) const;

    void transpose(Xgslmat&) const;
    void ctranspose(Xgslmat&) const;

    void qrp(Xgslmat& Q, Xgslmat& R, Wgslmat& P, sort_mode_t) const;

    bool sizematch(const Xgslmat& m) const;
    bool indexok(int m, int n) const;
    void puts(void) const;
    void init(int nrows, int ncols, bool clrf=false);
    void free(wiCMat* h=NULL);

protected:
    wiCMat* g;
    bool long_fmt;
};

class Wgslmat : public Xgslmat {
public:
    Wgslmat(void);
    Wgslmat(int nrows, int ncols);
    Wgslmat(const Wgslmat& m);
    Wgslmat(const wiCMat* m);
    Wgslmat(wiCMat* m, bool _take=false);
    Wgslmat(const double& x);
    template <typename T>
	Wgslmat(const std::vector<T>& v);
    template <typename T>
	Wgslmat(const std::vector<T>& v, int nr, int nc);
    virtual ~Wgslmat(void);

    Wgslmat& format_long(bool f);

    double norm2(void) const;
    double minabs(void) const;
    double maxabs(void) const;

    // get element
    double operator() (int r, int c) const;
    double operator() (int k) const;
    double& operator() (int r, int c);
    double& operator() (int k);

    // get sub-matrix
    Wgslmat row(int r) const;
    Wgslmat col(int c) const;
    Wgslmat operator() (int r, int c, int nr, int nc) const;

    // set sub-matrix
    virtual void operator() (const Wgslmat& x, int r, int c);

    // assignment
    Wgslmat& operator= (const Wgslmat& m);
    Wgslmat& operator= (const double& x);	// set all elements to x
    template <typename T>
	Wgslmat& operator= (const std::vector<T>& v);

    // self ops
    Wgslmat& operator+= (const Wgslmat& m);

    // dyadic ops
    Wgslmat operator+ (const Wgslmat& m) const;
    Wgslmat operator- (const Wgslmat& m) const;
    Wgslmat operator* (const Wgslmat& m) const;

    // comparison ops
    bool operator== (const Wgslmat& m) const;
    bool operator!= (const Wgslmat& m) const;
    bool operator== (const double& x) const;
    bool operator!= (const double& x) const;

    // matrix i/o
    void puts(std::ostream&) const;
    void gets(std::istream&);

public:
    Wgslmat transpose(void) const;
    Wgslmat ltinverse(void) const;

    Wgslmat permute_rows(const Wgslmat& p) const;
    Wgslmat permute_cols(const Wgslmat& p) const;
    Wgslmat inverse_permute_rows(const Wgslmat& p) const;
    Wgslmat inverse_permute_cols(const Wgslmat& p) const;

    size_t sort(size_t* p, size_t k) const;

private:
    void puts(void) const;
};

// some useful complex ops
wiComplex operator- (const wiComplex& z);
#define	WCOMPLEX_OP(_op_) \
    wiComplex operator _op_ (const wiComplex& w, const wiComplex& z); \
    wiComplex operator _op_ (const wiComplex& w, const double& x); \
    wiComplex operator _op_ (const double& x, const wiComplex& w);
WCOMPLEX_OP(+)
WCOMPLEX_OP(*)
WCOMPLEX_OP(-)
WCOMPLEX_OP(/)
#undef	WCOMPLEX_OP

bool operator== (const wiComplex& w, const wiComplex& z);
bool operator== (const wiComplex& w, const double& x);
bool operator!= (const wiComplex& w, const wiComplex& z);
bool operator!= (const wiComplex& w, const double& x);
inline bool operator!= (const wiComplex& w, const wiComplex& z) {
    return !(w==z);
}
inline bool operator!= (const wiComplex& w, const double& x) {
    return !(w==x);
}

inline double _real(const wiComplex& w) {return wiComplexReal(w);}
inline double _imag(const wiComplex& w) {return wiComplexImag(w);}
inline wiComplex _complex(const double& x, const double& y) {
    wiComplex w={x,y}; return w;
}

std::ostream& operator<< (std::ostream& o, const Xgslmat& g);
std::istream& operator>> (std::istream& i, Xgslmat& g);

std::ostream& operator<< (std::ostream& o, const Wgslmat& g);
std::istream& operator>> (std::istream& i, Wgslmat& g);
}

#endif	//_WIBFMATRIX_H_
