/*
 **************************************************************************
 * WIBFKVSD.CPP
 * created: Thu Feb 10 19:01:36 JST 2011
 *-------------------------------------------------------------------------
 * class "Wkvsd" definitions.
 * (put more comments here)
 *-------------------------------------------------------------------------
 * Author: Leonardo Vainsencher
 *-------------------------------------------------------------------------
 * Copyright (c) 2011 DSP Group, Inc. All rights reserved.
 **************************************************************************
 */

#include "wiBFkvsd.h"
#ifdef	USE_WGSLMAT
#include "wgslmat.h"
#include "wgslmatx.h"
#else
#include "wiBFmatrix.h"
#endif
#include "wiBFfixpt.h"
#include "wiBFutil.h"
#include "wiBFmissing.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>
#include <exception>
#include <cstdlib>
#include <cassert>

#ifdef	ERROR
#undef	ERROR
#endif
#define ERROR(s) throw error(__FILE__,__LINE__,(s))

using namespace std;

namespace WiBF {

struct Wkvsd::priv_s
{
    priv_s(int k, int n, int l) :
	K(k), N(n), L(l),
	bps((int)round(log2((double)l))),
	dist(N,K), traceb(N,K), u(N,K), y(N,K),
	u1(L,K), y1(L,K), d1(L,K), E(K,N), E1(K,N)
    {
	sidx= new size_t[K];
	Wkvsd::graygen(bps,gc);
    }
    ~priv_s(void) {delete sidx;}
    bool is_compatible(int k, int n, int l) const {
	return k==K && n==N && l==L;
    }
    const int K, N, L, bps;
    ivector_t gc;
    size_t* sidx;
    Wgslmat dist, traceb, u, y, u1, y1, d1, E, E1;
private:
    priv_s(const priv_s&) : K(0), N(0), L(0), bps(0) {}
    priv_s& operator= (const priv_s&) {return *this;}
};

struct Wkvsd::prix_s
{
    prix_s(int k, int n, int l) :
	K(k), N(n), L(l), L2(l*l),
	bps((int)round(log2((double)l))),
	cspan(K), gc(L), A(L), ustep(K), vstep(K), 
	dist(1,K), d2(1,L2), u1(N,L2), y1(1,L2), u2(1,K), E(1,K)
    {
	Wkvsd::graygen(bps,gc);
	for (int i=0; i < L; i++) A[i]= 2*i-L+1;
	sidx= new size_t[K];
    }
    ~prix_s(void) {delete sidx;}
    bool is_compatible(int k, int n, int l) const {
	return k==K && n==N && l==L;
    }
    const int K,N,L,L2,bps;
    vector<bool> cspan;
    ivector_t gc,A,ustep,vstep;
    Wgslmat dist,d2;
    Xgslmat u1,y1,u2,E;
    size_t* sidx;
private:
    prix_s(const prix_s&) : K(0), N(0), L(0), L2(0), bps(0) {}
    prix_s& operator= (const prix_s&) {return *this;}
};

Wkvsd::Wkvsd(int nt, int fxp, int dlim) :
    norm_type(nt==1?1:2),	// norm type (L1 or L2)
    fixpt(fxp > 0),		// enable/disable fixed-point arithmetic
    intp((fixpt && norm_type==1 && dlim > 0) ? dlim : 0),
				// >0 enables dist range limiting (csesd3 only)
    frac(fixpt ? fxp : 0)	// number of bits in fraction (csesd3 only)
{
    prv=NULL; prx=NULL; chit=cmiss=0;
}

Wkvsd::~Wkvsd(void) {delete prv; delete prx;}

/*
 * [uhat, chat, ncyc] = kvsd(x, G, L, K, m);
 * schnorr-euchner method to find the closest lattice point,
 * k-best breadth-first variant with support for counter-hypothesis
 * search.
 * input arguments:
 *   x= target points, r x M matrix.
 *   G= lattice generator, N x M matrix (M >= N).
 *   L= constellation levels (e.g. 8 for 3b PAM).
 *   K= number of paths to keep
 *   m= 0: use full search for c/h
 *      1: return only c/h found among hypothesis survivors
 *      2: like 1 but fill-in missing c/h with full search
 * output arguments:
 *   uhat= lattice closest point coordinates, r x N matrix.
 *   chat= vector of conter-hypotheses, h x N matrix, where
 *		h = r * N * log2(L)
 * actual closest point can be obtained by xhat= uhat*G.
 * for each input point x, this function finds the estimated
 * lattice coordinate uhat corresponding to the lattice point
 * xhat with shortest euclidean distance to x. xhat may or may
 * not be the true closest point to x.
 */

void Wkvsd::operator() (Wgslmat& uhat, Wgslmat* pchat, ivector_t* pcyc,
	const Wgslmat& X, const Wgslmat& G,
	const int L, const int K, int cmode)
{
    int i,j,p;

    const int npoints= X.rows();
    const int M= G.cols();
    const int N= G.rows();
    if (L < 2)
	ERROR("L < 2 or not an integer power of 2");
    const int bps= (int)round(log2((double)L));
    if ((1 << bps) != L)
	ERROR("L < 2 or not an integer power of 2");
    if (M < N)
	ERROR("G has invalid size");
    if (X.cols() != M)
	ERROR("X, G size mismatch");
    if (K < 2)
	ERROR("K out of range");

    const int numch= N * bps;
    Wgslmat Q,R,P;
    G.transpose().sqrp(Q,R,P);
    Wgslmat G2(G.permute_rows(P));

    Wgslmat G3= R(0,0,N,N).transpose();
    fixsign(G3,Q);
    Wgslmat H3= G3.ltinverse();
    Wgslmat X3= X*Q(0,0,M,N);
    Wgslmat u3hat(1,N), c3hat(1,N);
    Wgslmat xH3;

    // temporary storage for kseclosest2()
    if (prv==NULL)
	prv = new priv_s(K,N,L);
    else if (!prv->is_compatible(K,N,L)) {
	delete prv;
	prv = new priv_s(K,N,L);
    }

    uhat.resize(npoints,N);
    if (pchat!=NULL) {
	pchat->resize(npoints * numch, N);
	*pchat = 0.0;
    }
    if (pcyc!=NULL) {
	pcyc->resize(npoints);
	std::fill(pcyc->begin(), pcyc->end(), 0);
    }

    for (p=0; p < npoints; p++) {
	xH3 = X3.row(p)*H3;
	ksedecode2(u3hat, xH3, H3, Wgslmat(), -1, -1);
	uhat.setrow(p, u3hat.inverse_permute_cols(P));

	if (pcyc!=NULL) (*pcyc)[p]=1;
	if (pchat==NULL) continue;

	const int chbase= numch*p;

	switch(cmode) {
	    case 0:
		// full separate search for each c/h
		for (j=0; j < N; j++) {
		    const int s= chbase + (int)P(j)*bps;
		    for (i=0; i < bps; i++) {
			ksedecode2(c3hat, xH3, H3, u3hat, j, i);
			pchat->setrow(s+i, c3hat.inverse_permute_cols(P));
		    }
		}
		if (pcyc!=NULL) (*pcyc)[p] += numch;
		break;
	    case 1:
		// search c/h only among hypothesis survivors
		for (j=0; j < N; j++) {
		    const int s= chbase + (int)P(j)*bps;
		    for (i=0; i < bps; i++) {
			chsurv(c3hat, u3hat, j, i);
			pchat->setrow(s+i, c3hat.inverse_permute_cols(P));
		    }
		}
		break;
	    case 2:
		// search c/h among hypothesis survivors
		for (j=0; j < N; j++) {
		    const int s= chbase + (int)P(j)*bps;
		    for (i=0; i < bps; i++) {
			chsurv(c3hat, u3hat, j, i);
			pchat->setrow(s+i, c3hat.inverse_permute_cols(P));
		    }
		}
		// do full search for each missing c/h 
		for (j=0; j < N; j++) {
		    const int s= chbase + (int)P(j)*bps;
		    for (i=0; i < bps; i++) {
			if (pchat->row(s+i) != 0.0) continue;
			// missing c/h
			ksedecode2(c3hat, xH3, H3, u3hat, j, i);
			pchat->setrow(s+i, c3hat.inverse_permute_cols(P));
			if (pcyc!=NULL) (*pcyc)[p]++;
		    }
		}
		break;
	    default:
		ERROR("unexpected error");
	}
    }
}

// CSESD algorithm for 2xM channels (2 transmit antennas)
// based on complex SE enumeration
void Wkvsd::csesd3(Wgslmat& uhat, Wgslmat* pchat,
	const Wgslmat& X_Re, const Wgslmat& G_Re, const int L, const int K)
{
    int i,j,k,p;

    if (G_Re.cols() % 2 || G_Re.rows() % 2)
	ERROR("G has invalid size");
    const int npoints= X_Re.rows();
    const int M= G_Re.cols()/2;
    const int N= G_Re.rows()/2;
    if (L < 2)
	ERROR("L out of range");
    const int bps= (int)round(log2((double)L));
    if ((1 << bps) != L)
	ERROR("L not a power of 2");
    if (M < N)
	ERROR("G has invalid size");
    if (X_Re.cols() != 2*M)
	ERROR("X, G size mismatch");
    if (K < 2 || K > L*L)
	ERROR("K out of range");
    if (N!=2)
	ERROR("not 2xM MIMO");

    Xgslmat X(npoints,M), G(N,M), Q, R;
    Wgslmat P, p1(1,2*N);
    G.format_long(true);

    // recover complex representation
    for (i=0; i < N; i++)
	for (j=0; j < M; j++)
	    G(i, j, _complex(G_Re(i,j), G_Re(i,j+M)));

    for (i=0; i < npoints; i++)
	for (j=0; j < M; j++)
	    X(i, j, _complex(X_Re(i,j), X_Re(i,j+M)));

    const int numch= 2*N*bps;
    G.ctranspose().sqrp(Q,R,P);
    Xgslmat G2= G.permute_rows(P);
    Xgslmat G3= R(0,0,N,N).ctranspose();
    fixsign(G3,Q);
    Xgslmat X3= X*Q(0,0,M,N);
    Wgslmat u3hat(1,2*N), c3hat(numch,2*N);

    // temporary storage for kseclosest2()
    if (prx==NULL)
	prx = new prix_s(K,N,L);
    else if (!prx->is_compatible(K,N,L)) {
	delete prx;
	prx = new prix_s(K,N,L);
    }

    uhat.resize(npoints,2*N);
    if (pchat!=NULL) {
	pchat->resize(npoints * numch, 2*N);
	*pchat = 0.0;
    }

    // complex perm -> real perm
    for (k=0; k < N; k++) {
	p1(k  ) = P(k);
	p1(k+N) = P(k)+N;
    }

    chit= cmiss= 0;

    for (p=0; p < npoints; p++) {
	csesd3_hypothesis_decode(u3hat, X3.row(p), G3);
	uhat.setrow(p, u3hat.inverse_permute_cols(p1));

	if (pchat==NULL) continue;

	const int chbase= numch*p;

	for (j=0; j < 2*N; j++) {
	    const int s= chbase + (int)p1(j)*bps;
	    for (i=0; i < bps; i++) {
		csesd3_counter_decode(c3hat, u3hat, j, i);
		pchat->setrow(s+i, c3hat.inverse_permute_cols(p1));
	    }
	}
    }
}

// CSESD algorithm for 2xM channels (2 transmit antennas)
// based on complex SE enumeration
// (old implementation)
void Wkvsd::csesd2(Wgslmat& uhat, Wgslmat* pchat,
	const Wgslmat& X_Re, const Wgslmat& G_Re, const int L, const int K)
{
    int i,j,k,p;

    if (G_Re.cols() % 2 || G_Re.rows() % 2)
	ERROR("G has invalid size");
    const int npoints= X_Re.rows();
    const int M= G_Re.cols()/2;
    const int N= G_Re.rows()/2;
    if (L < 2)
	ERROR("L out of range");
    const int bps= (int)round(log2((double)L));
    if ((1 << bps) != L)
	ERROR("L not a power of 2");
    if (M < N)
	ERROR("G has invalid size");
    if (X_Re.cols() != 2*M)
	ERROR("X, G size mismatch");
    if (K < 2 || K > L*L)
	ERROR("K out of range");
    if (N!=2)
	ERROR("not 2xM MIMO");

    Xgslmat X(npoints,M), G(N,M), Q, R;
    Wgslmat P, p1(1,2*N);

    // recover complex representation
    for (i=0; i < N; i++)
	for (j=0; j < M; j++)
	    G(i, j, _complex(G_Re(i,j), G_Re(i,j+M)));

    for (i=0; i < npoints; i++)
	for (j=0; j < M; j++)
	    X(i, j, _complex(X_Re(i,j), X_Re(i,j+M)));

    const int numch= 2*N*bps;
    G.ctranspose().sqrp(Q,R,P);
    Xgslmat G2= G.permute_rows(P);
    Xgslmat G3= R(0,0,N,N).ctranspose();
    fixsign(G3,Q);
    Xgslmat H3= G3.ltinverse();
    Xgslmat X3= X*Q(0,0,M,N);
    Wgslmat u3hat(1,2*N), c3hat(numch,2*N);
    Xgslmat xH3;

    // temporary storage for kseclosest2()
    if (prx==NULL)
	prx = new prix_s(K,N,L);
    else if (!prx->is_compatible(K,N,L)) {
	delete prx;
	prx = new prix_s(K,N,L);
    }

    uhat.resize(npoints,2*N);
    if (pchat!=NULL) {
	pchat->resize(npoints * numch, 2*N);
	*pchat = 0.0;
    }

    // complex perm -> real perm
    for (k=0; k < N; k++) {
	p1(k  ) = P(k);
	p1(k+N) = P(k)+N;
    }

    chit= cmiss= 0;

    for (p=0; p < npoints; p++) {
	double dh1;
	xH3 = X3.row(p)*H3;
	csesd2_hypothesis_decode(u3hat, xH3, H3, dh1);
	uhat.setrow(p, u3hat.inverse_permute_cols(p1));

	if (pchat==NULL) continue;

	const int chbase= numch*p;

	for (j=0; j < 2*N; j++) {
	    const int s= chbase + (int)p1(j)*bps;
	    for (i=0; i < bps; i++) {
		csesd2_counter_decode(c3hat, u3hat, j, i, dh1);
		pchat->setrow(s+i, c3hat.inverse_permute_cols(p1));
	    }
	}
    }
}

/* legacy algorithm */
void Wkvsd::operator() (Wgslmat& uhat, const Wgslmat& X,
	const Wgslmat& G, const int L, const int K,
	const Wgslmat* U, int bitp)
{
    int j,k;

    const int npoints= X.rows();
    const int M= G.cols();
    const int N= G.rows();
    if (L < 2)
	ERROR("L < 2 or not an integer power of 2");
    const int bps= (int)round(log2((double)L));
    if ((1 << bps) != L)
	ERROR("L < 2 or not an integer power of 2");
    if (M < N)
	ERROR("G has invalid size");
    if (X.cols() != M)
	ERROR("X, G size mismatch");
    if (U!=NULL && (bitp < 0 || bitp >= bps*N))
	ERROR("bitp out of range");
    if (U!=NULL && (U->rows()!=npoints || U->cols()!=N))
	ERROR("X, U size mismatch");
    if (K < 2)
	ERROR("K out of range");

    Wgslmat Q,R,P,U3;
    G.transpose().sqrp(Q,R,P);
    Wgslmat G2(G.permute_rows(P));

    Wgslmat G3= R(0,0,N,N).transpose();
    fixsign(G3,Q);
    Wgslmat H3= G3.ltinverse();
    Wgslmat X3= X*Q(0,0,M,N);
    Wgslmat u3hat(1,N);

    int bk= -1, bp= -1;
    if (U!=NULL) {
	U3 = U->permute_cols(P);
	bk = bitp / bps;
	for (j=0; j < N; j++)
	    if (P(j)==bk) {bk=j; break;}
	assert(j < N);
	bp = bitp % bps;
    }

    // temporary storage for kseclosest2()
    if (prv==NULL)
	prv = new priv_s(K,N,L);
    else if (!prv->is_compatible(K,N,L)) {
	delete prv;
	prv = new priv_s(K,N,L);
    }

    uhat.resize(npoints,N);

    for (k=0; k < npoints; k++) {
	ksedecode2(u3hat, X3.row(k)*H3, H3, U3, bk, bp);
	for (j=0; j < N; j++) {
	    int p= (int)P(j);
	    uhat(k,p) = u3hat(j);
	}
    }
}

static inline
unsigned bitget(unsigned w, int b) {
    return (w >> b) & 1u;
}

// k-best schnorr-euchner lattice decoder for finite constellation
void Wkvsd::ksedecode2(Wgslmat& uhat, const Wgslmat& xH,
	const Wgslmat& H, const Wgslmat& U, int bitk, int bitp)
{
    priv_s& S= *prv;
    const int K= S.K;
    const int L= S.L;
    const int N= S.N;
    double x,x1;
    int i,j,k,n,s,utmp,kstep,mk;
    ivector_t mK(N), A(L);

    // initialize
    for (i=0; i < K; i++) S.E.setrow(i,xH);
    uhat=0.0; S.dist=0.0;
    S.u=0.0; S.y=0.0;
    S.u1=0.0; S.y1=0.0;
    S.traceb=0.0;
    k = N-1;
    kstep = 0;

    for (i=0; i < L; i++) A[i]= 2*i-L+1;
    int L2= L;
    if (k==bitk) {
	// reduce constellation
	const int a= (int)U(k)+L-1;
	if (a < 0 || a > 2*L-2 || (a & 1))
	    ERROR("unexpected error");
	unsigned ub= bitget(S.gc[a >> 1], bitp);
	for (i=0; i < L; i++)
	    if (ub == bitget(S.gc[i],bitp)) A[i]=0;
	L2 = L/2;
    }

    // top of the tree
    for (n=0; n < L2; n++) {
	if (n==0) {
	    utmp = (int)S.u1(n,0);
	    nearestus(utmp, kstep, S.E(0,k), A);
	} else {
	    utmp = (int)S.u1(n-1,0);
	    nextus(utmp, kstep, A);
	}
	if (utmp==0)
	    ERROR("unexpected error");
	S.u1(n,0) = utmp;
	S.y1(n,0) = (S.E(0,k) - utmp) / H(k,k);
    }

    // number of survivors
    mK[k] = mk = std::min(L2,K);

    // get best survivors
    for (i=0; i < mk; i++) S.y(k,i)= S.y1(i,0);
    while (i < K) S.y(k,i++)=HUGE_VAL;
    // verify assumption
    //assert(min(abs(y(k,:))) <= min(abs(y1(:,0))));
    for (x=x1=i=0; i < mk; i++) {
	double t;
	t=std::abs(S.y(k,i));  if (i==0 || x  > t) x=t;
	t=std::abs(S.y1(i,0)); if (i==0 || x1 > t) x1=t;
    }
    if (x > x1)
	ERROR("unexpected error");

    // corresponding coordinates
    for (i=0; i < mk; i++) S.u(k,i)= S.u1(i,0);
    while (i < K) S.u(k,i++)= S.u(k,mk-1);

    // initial distances
    for (i=0; i < K; i++) {x= S.y(k,i); S.dist(k,i)= x*x;}

    // move down the tree
    while (k > 0) {
	k--;
	for (i=0; i < L; i++) A[i]= 2*i-L+1;
	int L2=L;
	if (k==bitk) {
	    // reduce constellation
	    const int a= (int)U(k)+L-1;
	    if (a < 0 || a > 2*L-2 || (a & 1))
		ERROR("unexpected error");
	    unsigned ub= bitget(S.gc[a >> 1], bitp);
	    for (i=0; i < L; i++)
		if (ub == bitget(S.gc[i],bitp)) A[i]=0;
	    L2 = L/2;
	}

	// compute L x K children
	mK[k] = std::min(K, L2*mK[k+1]);
	mk = mK[k+1];
	for (s=0; s < mk; s++) {
	    double ys= S.y(k+1,s);
	    for (i=0; i <= k; i++) S.E(s,i) -= ys*H(k+1,i);
	    for (n=0; n < L2; n++) {
		if (n==0) {
		    utmp = (int)S.u1(n,s);
		    nearestus(utmp, kstep, S.E(s,k), A);
		} else {
		    utmp = (int)S.u1(n-1,s);
		    nextus(utmp, kstep, A);
		}
		if (utmp==0)
		    ERROR("unexpected error");
		S.u1(n,s) = utmp;
		S.y1(n,s) = (S.E(s,k) - utmp) / H(k,k);
	    }
	}

	// select K best among L2 x K branch metrics
	for (i=0; i < L; i++) {
	    for (j=0; j < mk; j++) {
		if (i < L2) {
		    x = S.y1(i,j);
		    S.d1(i,j) = S.dist(k+1,j) + x*x;
		} else
		    S.d1(i,j) = HUGE_VAL;
	    }
	    for (j=mk; j < K; j++) S.d1(i,j)= HUGE_VAL;
	}
	// note: sidx contains row-wise indices (unlike matlab!)
	i = S.d1.sort(S.sidx, K);
	if (i < K)
	    ERROR("unexpected error");

	// update buffers for K-best survivors
	for (i=0; i < K; i++) {
	    j = S.sidx[i];
	    S.y(k,i)= S.y1(j);
	    S.u(k,i)= S.u1(j);
	    S.dist(k,i)= S.d1(j);
	    S.traceb(k,i) = s = j % K;
	    for (n=0; n < N; n++) S.E1(i,n)= S.E(s,n);
	}
	S.E(S.E1,0,0);
    }

    // traceback
    s = 0;
    uhat(0) = S.u(0,s);
    for (k=1; k < N; k++) {
	s = (int)S.traceb(k-1,s);
	uhat(k) = S.u(k,s);
    }
}

// CSESD hypothesis decoder for 2xM MIMO
void Wkvsd::csesd3_hypothesis_decode(Wgslmat& uhat, const Xgslmat& X,
	const Xgslmat& H)
{
    prix_s& T= *prx;
    const int K= T.K;
    const int L= T.L;
    const int N= T.N;
    const ivector_t& A= T.A;
    ivector_t& ustep= T.ustep;
    ivector_t& vstep= T.vstep;
    double yr,yi,dmin,d0;
    int i,j,s,smin;
    int utmp,vtmp,ustp,vstp;
    wiComplex S, W, Z;

    if (N!=2)
	ERROR("unexpected error");

    // initialize
    uhat=0.0; T.dist=0.0;
    T.u1=0.0;
    utmp= vtmp= ustp= vstp= 0;

    // top of the tree
    // for now we expand all L^2 combinations, then select K best if K < L^2

    Wfixpt h2_f ("h2", Wfixpt::REAL, frac, fixpt);
    Wfixpt F2_f ("F2", Wfixpt::CPLX, frac, fixpt);
    Wfixpt d2_f ("d2", Wfixpt::REAL, frac, fixpt);

    const double dh1 = 1.0 / _real(H(0,0));
    h2_f = _real(H(1,1)) * dh1;
    F2_f = X(1) * dh1;

    for (s=j=0; j < L; j++) {
	vtmp = 2*j-L+1;

	for (i=0; i < L; i++, s++) {
	    utmp = 2*i-L+1;
	    T.u1(1,s) = S = _complex(utmp,vtmp);
	    W = (F2_f - h2_f * S).toComplex();
	    yr = _real(W);
	    yi = _imag(W);
	    double d2= (norm_type==1) ? norm1(yr, yi) : norm2(yr, yi);
	    if (intp > 0)
		d2 = Wfixpt::limit(d2, 4*L-1);
	    d2_f = d2;
	    T.d2(s) = d2_f.toReal();
	}
    }

    if (K < T.L2) {
	i = T.d2.sort(T.sidx,K);
	if (i < K)
	    ERROR("unexpected error");

	// using T.dist as temp
	for (s=0; s < K; s++) T.dist(s)= T.d2(T.sidx[s]);
	T.d2(T.dist,0,0);
	for (s=K; s < T.L2; s++) T.d2(s)= HUGE_VAL;

	// using T.u2 as temp
	for (s=0; s < K; s++) T.u2(s)= T.u1(1,T.sidx[s]);
	T.u1(T.u2,1,0);
	for (s=K; s < T.L2; s++) T.u1(1,s)=_complex(0,0);
    }

    // bottom of the tree

    Wfixpt  H21_f ("H21",  Wfixpt::CPLX, frac, fixpt);
    Wfixpt   F1_f ("F1",   Wfixpt::CPLX, frac, fixpt);
    Wfixpt   E1_f ("E1",   Wfixpt::CPLX, frac, fixpt);
    Wfixpt dist_f ("dist", Wfixpt::REAL, frac, fixpt);

    H21_f = H(1,0) * dh1;
    F1_f = X(0) * dh1;
    smin=0; dmin=0.0;

    for (s=0; s < K; s++) {
	S = T.u1(1,s);
	Z = (F1_f - H21_f*S).toComplex();
	if (intp > 0)
	    Z = Wfixpt::limit(Z, 4*L-1);
	E1_f = Z;
	T.E(s) = E1_f.toComplex();

	nearestus(utmp, ustep[s], E1_f.real(), A);
	nearestus(vtmp, vstep[s], E1_f.imag(), A);
	T.u1(0,s) = S = _complex(utmp,vtmp);

	Z = (E1_f - S).toComplex();
	yr = _real(Z);
	yi = _imag(Z);

	d0 = (norm_type==1) ? norm1(yr, yi) : norm2(yr, yi);
	d0 += T.d2(s);
	dist_f = d0;
	T.dist(s) = dist_f.toReal();

	if (s==0 || dmin > d0) {smin=s; dmin=d0;}
    }

    for (i=0; i < N; i++) {
	uhat(i  ) = _real(T.u1(i,smin));
	uhat(i+N) = _imag(T.u1(i,smin));
    }
}

// decode counter-hypothesis by further spanning hypothesis
// survivors as needed (for "fast" 2xM algorithm).
void Wkvsd::csesd3_counter_decode(Wgslmat& chat, const Wgslmat& U,
	int bk, int bp)
{
    // dispatch
    switch(bk) {
	case 1:
	    csesd3_counter_top(chat,U,bk,bp); break;
	case 3:
	    csesd3_counter_top(chat,U,bk,bp); break;
	case 0:
	    csesd3_counter_bot(chat,U,bk,bp); break;
	case 2:
	    csesd3_counter_bot(chat,U,bk,bp); break;
	default:
	    ERROR("unexpected error");
    }
}

// find counter-hypothesis for top bits
void Wkvsd::csesd3_counter_top(Wgslmat& chat, const Wgslmat& U, int bk, int bp)
{
    prix_s& S= *prx;
    const int K= S.K;
    const int L= S.L;
    const int N= S.N;
    int a,k,s;

    if (N!=2)
	ERROR("unexpected error");

    if (bp < 0 || bp >= S.bps)
	ERROR("unexpected error");

    a = (int)U(bk) + L-1;
    if (a < 0 || a > 2*L-2 || (a & 1))
	ERROR("unexpected error");
    const unsigned hb= bitget(S.gc[a >> 1], bp);

    int smin= -1;
    double dmin= 0;
    for (s=0; s < K; s++) {
	switch(bk) {
	    case 1:
		a = (int)_real(S.u1(1,s)); break;
	    case 3:
		a = (int)_imag(S.u1(1,s)); break;
	    default:
		ERROR("unexpected error");
	}
	a += L-1;
	if (a < 0 || a > 2*L-2 || (a & 1))
	    ERROR("unexpected error");
	const unsigned cb= bitget(S.gc[a >> 1], bp);
	if (cb == hb) continue;
	// got c/h candidate
	if (smin < 0 || dmin > S.dist(s)) {smin=s; dmin= S.dist(s);}
    }

    chat.resize(1,2*N);

    if (smin < 0) {
	if (K >= S.L2)
	    ERROR("unexpected error");
	chat=0.0;
	cmiss++;
	return;
    } else if (K < S.L2)
	chit++;

    for (k=0; k < N; k++) {
	chat(k  ) = _real(S.u1(k,smin));
	chat(k+N) = _imag(S.u1(k,smin));
    }
}

// find counter-hypothesis for bottom bits
void Wkvsd::csesd3_counter_bot(Wgslmat& chat, const Wgslmat& U, int bk, int bp)
{
    prix_s& S= *prx;
    const int K= S.K;
    const int L= S.L;
    const int N= S.N;
    double er,ei,yr,yi,dist;
    int a,s,utmp,vtmp,step;
    int* t;

    if (N!=2)
	ERROR("unexpected error");

    if (bp < 0 || bp >= S.bps)
	ERROR("unexpected error");

    a = (int)U(bk) + L-1;
    if (a < 0 || a > 2*L-2 || (a & 1))
	ERROR("unexpected error");
    const unsigned hb= bitget(S.gc[a >> 1], bp);

    int smin= -1;
    double dmin= 0;

    for (s=0; s < K; s++) {
	utmp = (int)_real(S.u1(0,s));
	vtmp = (int)_imag(S.u1(0,s));
	t=NULL; step=0;

	switch(bk) {
	    case 0:
		t = &utmp;
		step = S.ustep[s];
		break;
	    case 2:
		t = &vtmp;
		step = S.vstep[s];
		break;
	    default:
		ERROR("unexpected error");
	}

	// make sure survivor is c/h candidate
	// note that a c/h miss here is not the same as plain k-best c/h miss
	S.cspan[s]=false;	// for counting hits/misses
	while (*t) {
	    a = *t + L-1;
	    if (a < 0 || a > 2*L-2 || (a & 1))
		ERROR("unexpected error");
	    const unsigned cb= bitget(S.gc[a >> 1], bp);
	    if (cb != hb) break;
	    // not a c/h: further span children
	    nextus(*t, step, S.A);
	    // flag this survivor was created by further spanning
	    S.cspan[s]=true;
	}
	if (*t == 0)
	    ERROR("unexpected error");

	// got c/h candidate for this survivor
	S.u2(s) = _complex(utmp,vtmp);
	er = _real(S.E(s));
	ei = _imag(S.E(s));
	yr = er - utmp;
	yi = ei - vtmp;
	dist = (norm_type==1) ? norm1(yr,yi) : norm2(yr,yi);
	dist += S.d2(s);
	if (s==0 || dmin > dist) {smin=s; dmin=dist;}
    }

    chat.resize(1,2*N);
    chat(0) = _real(S.u2(smin));
    chat(1) = _real(S.u1(1,smin));
    chat(2) = _imag(S.u2(smin));
    chat(3) = _imag(S.u1(1,smin));
    if (K == S.L2) {
	if (S.cspan[smin]) cmiss++;
	else chit++;
	//if (S.cspan[smin]) chat=0.0;	// XXX: what if...
    }
}

// CSESD hypothesis decoder for 2xM MIMO
// (old implementation)
void Wkvsd::csesd2_hypothesis_decode(Wgslmat& uhat, const Xgslmat& xH,
	const Xgslmat& H, double& dh1)
{
    prix_s& T= *prx;
    const int K= T.K;
    const int L= T.L;
    const int N= T.N;
    const ivector_t& A= T.A;
    ivector_t& ustep= T.ustep;
    ivector_t& vstep= T.vstep;
    double yr,yi,dmin,d0;
    int i,j,s,smin;
    int utmp,vtmp,ustp,vstp;
    wiComplex W, Z;

    if (N!=2)
	ERROR("unexpected error");

    // initialize
    uhat=0.0; T.dist=0.0;
    T.u1=0.0; T.y1=0.0;
    utmp= vtmp= ustp= vstp= 0;

    // top of the tree
    // for now we expand all L^2 combinations, then select K best if K < L^2

    Wfixpt h2_f ("h2", Wfixpt::REAL, 12, fixpt);
    Wfixpt F2_f ("F2", Wfixpt::CPLX,  8, fixpt);
    Wfixpt Y1_f ("Y1", Wfixpt::CPLX, 11, fixpt);
    Wfixpt d2_f ("d2", Wfixpt::REAL,  6, fixpt);

    h2_f = 1.0 / _real(H(1,1));
    F2_f = xH(1);

    // pruning experiments: XXX
    const bool prune_test= (L==8 && (K==4*4 || K==5*5 || K==6*6));
    const int K1= (int)sqrt((double)K);
    const int L1= prune_test ? K1 : L;
    assert(L1 <= L);
    int u_nearest, v_nearest, vstp0;
    u_nearest= v_nearest= vstp0= 0;

    if (prune_test) {
	assert(K1*K1==K);
	nearestus(u_nearest, ustp, F2_f.real(), A);
	nearestus(v_nearest, vstp0, F2_f.imag(), A);
	// note: although we are using nearestus/nextus (true SE zig-zag)
	// in the loop below, in a practical implementation ordering
	// can be arbitrary.
    }

    for (s=i=0; i < L1; i++) {
	if (!prune_test)
	    utmp = 2*i-L+1;
	else if (i==0)
	    utmp = u_nearest;
	else
	    nextus(utmp, ustp, A);

	for (j=0; j < L1; j++, s++) {
	    if (!prune_test)
		vtmp = 2*j-L+1;
	    else if (j==0) {
		vtmp = v_nearest;
		vstp = vstp0;
	    } else
		nextus(vtmp, vstp, A);

	    W = _complex(utmp,vtmp);
	    Z = ((F2_f - W) * h2_f).toComplex();
	    Y1_f = Z;
	    T.y1(s) = Y1_f.toComplex();
	    yr = Y1_f.real();
	    yi = Y1_f.imag();
	    d2_f = (norm_type==1) ? norm1(yr, yi) : norm2(yr, yi);
	    T.d2(s) = d2_f.toReal();
	    T.u1(1,s) = _complex(utmp,vtmp);
	}
    }

    if (K < T.L2 && !prune_test) {
	i = T.d2.sort(T.sidx,K);
	if (i < K)
	    ERROR("unexpected error");

	// using T.u2 as temp
	for (s=0; s < K; s++) T.u2(s)= T.y1(T.sidx[s]);
	T.y1(T.u2,0,0);
	for (s=K; s < T.L2; s++) T.y1(s)=_complex(HUGE_VAL,HUGE_VAL);

	// using T.dist as temp
	for (s=0; s < K; s++) T.dist(s)= T.d2(T.sidx[s]);
	T.d2(T.dist,0,0);
	for (s=K; s < T.L2; s++) T.d2(s)= HUGE_VAL;

	// using T.u2 as temp
	for (s=0; s < K; s++) T.u2(s)= T.u1(1,T.sidx[s]);
	T.u1(T.u2,1,0);
	for (s=K; s < T.L2; s++) T.u1(1,s)=_complex(0,0);
    }

    // bottom of the tree
    Wfixpt   h1_f ("h1",   Wfixpt::REAL, 8, fixpt);
    Wfixpt  H21_f ("H21",  Wfixpt::CPLX, 6, fixpt);
    Wfixpt   E1_f ("E1",   Wfixpt::CPLX, 6, fixpt);
    Wfixpt   F1_f ("F1",   Wfixpt::CPLX, 6, fixpt);
    Wfixpt dist_f ("dist", Wfixpt::REAL, 6, fixpt);

    h1_f = dh1 = 1.0 / _real(H(0,0));
    H21_f = H(1,0);
    F1_f = xH(0);
    smin=0; dmin=0.0;

    for (s=0; s < K; s++) {
	E1_f = F1_f - T.y1(s)*H21_f;
	T.E(s) = E1_f.toComplex();

	nearestus(utmp, ustep[s], E1_f.real(), A);
	nearestus(vtmp, vstep[s], E1_f.imag(), A);
	W = _complex(utmp,vtmp);
	T.u1(0,s) = W;

	Z = ((E1_f - W) * h1_f).toComplex();
	yr = _real(Z);
	yi = _imag(Z);

	d0 = (norm_type==1) ? norm1(yr, yi) : norm2(yr, yi);
	d0 += T.d2(s);
	dist_f = d0;
	T.dist(s) = dist_f.toReal();

	if (s==0 || dmin > d0) {smin=s; dmin=d0;}
    }

    for (i=0; i < N; i++) {
	uhat(i  ) = _real(T.u1(i,smin));
	uhat(i+N) = _imag(T.u1(i,smin));
    }
}

// find counter-hypothesis among hypothesis survivors
void Wkvsd::chsurv(Wgslmat& chat, const Wgslmat& U, int bk, int bp)
{
    priv_s& S= *prv;
    const int K= S.K;
    const int L= S.L;
    const int N= S.N;
    int a,k,r,s;

    assert(0 <= bk && bk < N);

    a = (int)U(bk) + L-1;
    if (a < 0 || a > 2*L-2 || (a & 1))
	ERROR("unexpected error");
    const unsigned ub= bitget(S.gc[a >> 1], bp);

    ivector_t V(N);
    chat.resize(1,N);
    chat = 0.0;

    // verify hypothesis (XXX: for debugging purposes)
    s = 0;
    for (k=0; k < N; k++) {
	if (S.u(k,s) != U(k))
	    ERROR("unexpected error");
	s = (int)S.traceb(k,s);
    }

    // check if a counter-hypothesis is available
    for (r=1; r < K; r++) {
	s = r;
	V[0] = (int)S.u(0,s);
	for (k=1; k < N; k++) {
	    s = (int)S.traceb(k-1,s);
	    V[k] = (int)S.u(k,s);
	}
	a = V[bk] + L-1;
	if (a < 0 || a > 2*L-2 || (a & 1))
	    ERROR("unexpected error");
	const unsigned vb= bitget(S.gc[a >> 1], bp);
	if (vb != ub) {
	    // found C/H
	    chat=V; break;
	}
    }
}

// decode counter-hypothesis by further spanning hypothesis
// survivors as needed (for "fast" 2xM algorithm).
// (old implementation)
void Wkvsd::csesd2_counter_decode(Wgslmat& chat, const Wgslmat& U,
	int bk, int bp, const double& dh1)
{
    // dispatch
    switch(bk) {
	case 1:
	    csesd2_counter_top(chat,U,bk,bp); break;
	case 3:
	    csesd2_counter_top(chat,U,bk,bp); break;
	case 0:
	    csesd2_counter_bot(chat,U,bk,bp,dh1); break;
	case 2:
	    csesd2_counter_bot(chat,U,bk,bp,dh1); break;
	default:
	    ERROR("unexpected error");
    }
}

// find counter-hypothesis for top bits
// (old implementation)
void Wkvsd::csesd2_counter_top(Wgslmat& chat, const Wgslmat& U, int bk, int bp)
{
    prix_s& T= *prx;
    const int K= T.K;
    const int L= T.L;
    const int N= T.N;
    int a,k,s;

    if (N!=2)
	ERROR("unexpected error");

    if (bp < 0 || bp >= T.bps)
	ERROR("unexpected error");

    a = (int)U(bk) + L-1;
    if (a < 0 || a > 2*L-2 || (a & 1))
	ERROR("unexpected error");
    const unsigned hb= bitget(T.gc[a >> 1], bp);

    int smin= -1;
    double dmin= 0;
    for (s=0; s < K; s++) {
	switch(bk) {
	    case 1:
		a = (int)_real(T.u1(1,s)); break;
	    case 3:
		a = (int)_imag(T.u1(1,s)); break;
	    default:
		ERROR("unexpected error");
	}
	a += L-1;
	if (a < 0 || a > 2*L-2 || (a & 1))
	    ERROR("unexpected error");
	const unsigned cb= bitget(T.gc[a >> 1], bp);
	if (cb == hb) continue;
	// got c/h candidate
	if (smin < 0 || dmin > T.dist(s)) {smin=s; dmin= T.dist(s);}
    }

    chat.resize(1,2*N);

    if (smin < 0) {
	if (K >= T.L2)
	    ERROR("unexpected error");
	chat=0.0;
	cmiss++;
	return;
    } else if (K < T.L2)
	chit++;

    for (k=0; k < N; k++) {
	chat(k  ) = _real(T.u1(k,smin));
	chat(k+N) = _imag(T.u1(k,smin));
    }
}

// find counter-hypothesis for bottom bits
// (old implementation)
void Wkvsd::csesd2_counter_bot(Wgslmat& chat, const Wgslmat& U,
	int bk, int bp, const double& dh1)
{
    prix_s& T= *prx;
    const int K= T.K;
    const int L= T.L;
    const int N= T.N;
    double er,ei,yr,yi,d2;
    int a,s,utmp,vtmp,step;
    int* t;

    Wfixpt h1_f ("h1", Wfixpt::REAL, fixpt);
    h1_f = dh1;

    if (N!=2)
	ERROR("unexpected error");

    if (bp < 0 || bp >= T.bps)
	ERROR("unexpected error");

    a = (int)U(bk) + L-1;
    if (a < 0 || a > 2*L-2 || (a & 1))
	ERROR("unexpected error");
    const unsigned hb= bitget(T.gc[a >> 1], bp);

    int smin= -1;
    double dmin= 0;

    for (s=0; s < K; s++) {
	utmp = (int)_real(T.u1(0,s));
	vtmp = (int)_imag(T.u1(0,s));
	t=NULL; step=0;

	switch(bk) {
	    case 0:
		t = &utmp;
		step = T.ustep[s];
		break;
	    case 2:
		t = &vtmp;
		step = T.vstep[s];
		break;
	    default:
		ERROR("unexpected error");
	}

	// make sure survivor is c/h candidate
	// note that a c/h miss here is not the same as plain k-best c/h miss
	T.cspan[s]=false;	// for counting hits/misses
	while (*t) {
	    a = *t + L-1;
	    if (a < 0 || a > 2*L-2 || (a & 1))
		ERROR("unexpected error");
	    const unsigned cb= bitget(T.gc[a >> 1], bp);
	    if (cb != hb) break;
	    // not a c/h: further span children
	    nextus(*t, step, T.A);
	    // flag this survivor was created by further spanning
	    T.cspan[s]=true;
	}
	if (*t == 0)
	    ERROR("unexpected error");

	// got c/h candidate for this survivor
	T.u2(s) = _complex(utmp,vtmp);
	er = _real(T.E(s));
	ei = _imag(T.E(s));
	yr = ((er - utmp) * h1_f).toReal();
	yi = ((ei - vtmp) * h1_f).toReal();
	d2 = (norm_type==1) ? norm1(yr,yi) : norm2(yr,yi);
	d2 += T.d2(s);
	if (s==0 || dmin > d2) {smin=s; dmin=d2;}
    }

    chat.resize(1,2*N);
    chat(0) = _real(T.u2(smin));
    chat(1) = _real(T.u1(1,smin));
    chat(2) = _imag(T.u2(smin));
    chat(3) = _imag(T.u1(1,smin));
    if (K == T.L2) {
	if (T.cspan[smin]) cmiss++;
	else chit++;
	//if (T.cspan[smin]) chat=0.0;	// XXX: what if...
    }
}

// check if u is in -L+1 : 2 : L-1
bool Wkvsd::ismember(int u, const int L)
{
    if (L < 2 || L % 2)
	ERROR("unexpected error");
    for (int j= -L+1; j < L; j+=2)
	if (j==u) return true;
    return false;
}

// check if u is in A
bool Wkvsd::ismember(int u, const ivector_t& A)
{
    if (u==0) return false;
    for (unsigned j=0; j < A.size(); j++)
	if (A[j]==u) return true;
    return false;
}

// return nearest lattice coordinate value,
// adjusting for null constellation point in A,
// and return the next step value.
void Wkvsd::nearestus(int& u, int& s, const double& e, const ivector_t& A)
{
    const int L= A.size();
    double dmin=0;
    for (int j=0; j < L; j++) {
	int l= 2*j-L+1;
	double d= std::abs(e-l);
	if (j==0 || d < dmin) {dmin=d; u=l;}
    }
    s=  (e > u) ? 1 : -1;
    while (!ismember(u,A)) {
	nextus(u,s,L);
	if (u==0)
	    ERROR("unexpected error");
    }
}

// compute next lattice coordinate according to the given step,
// adjusting for null constellation point in A,
// and return the next step value.
bool Wkvsd::nextus(int& u, int& s, const ivector_t& A)
{
    const int L= A.size();
    nextus(u,s,L);
    while (u && !ismember(u,A))
	nextus(u,s,L);
    return u!=0;
}

// return next lattice coordinate according to the given step;
// if a constellation point cannot be found after 2 iterations,
// then u and s are both set to 0.
void Wkvsd::nextus(int& u, int& s, const int L)
{
    if (!ismember(u,L))
	ERROR("unexpected error");
    for (int i=0; i < 2; i++) {
	u += 2*s;
	s = -s - (s > 0 ? 1 : -1);
	if (ismember(u,L)) return;
    }
    u=s=0;
}

// fix sign of G3 diagonal
void Wkvsd::fixsign(Wgslmat& G3, Wgslmat& Q)
{
    const int N= G3.rows();
    const int M= Q.rows();
    int i,j;
    if (G3.cols() != N || Q.cols() < N)
	ERROR("unexpected error");
    for (j=0; j < N; j++) {
	double x= G3(j,j);
	if (x >= 0) continue;
	for (i=j; i < N; i++)
	    G3(i,j) = -G3(i,j);
	for (i=0; i < M; i++)
	    Q(i,j) = -Q(i,j);
    }
}

// complex case
void Wkvsd::fixsign(Xgslmat& G3, Xgslmat& Q)
{
    const int N= G3.rows();
    const int M= Q.rows();
    int i,j;
    if (G3.cols() != N || Q.cols() < N)
	ERROR("unexpected error");
    for (j=0; j < N; j++) {
	assert(_imag(G3(j,j))==0);
	double x= _real(G3(j,j));
	if (x >= 0) continue;
	for (i=j; i < N; i++)
	    G3(i,j) = -G3(i,j);
	for (i=0; i < M; i++)
	    Q(i,j) = -Q(i,j);
    }
}

// generate gray-code sequence of nbits
void Wkvsd::graygen(const int nbits, ivector_t& C)
{
    if (nbits < 1) {C.clear(); return;}
    const int n= 1<<nbits;
    C.resize(n);
    C[0]=0; C[1]=1;
    for (int c=1; c < nbits; c++) {
	int k= 1<<c;
	assert(k <= n/2);
	for (int j=0; j < k; j++)
	    C[k+j] = C[k-j-1] | (1<<c);
    }
}

}
