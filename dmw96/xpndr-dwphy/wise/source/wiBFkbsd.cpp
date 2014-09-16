/*
 **************************************************************************
 * WIBFKBSD.CPP
 * created: Thu Feb 10 19:01:36 JST 2011
 *-------------------------------------------------------------------------
 * class "Wkbsd" definitions.
 * (put more comments here)
 *-------------------------------------------------------------------------
 * Author: Leonardo Vainsencher
 *-------------------------------------------------------------------------
 * Copyright (c) 2011 DSP Group, Inc. All rights reserved.
 **************************************************************************
 */

#include "wiBFkbsd.h"
#ifdef	USE_WGSLMAT
#include "wgslmat.h"
#else
#include "wiBFmatrix.h"
#endif
#include "wiBFutil.h"
#include "wiBFmissing.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <exception>
#include <cstdlib>
#include <cmath>
#include <cassert>

#ifdef	ERROR
#undef	ERROR
#endif
#define ERROR(s) throw error(__FILE__,__LINE__,(s))

using namespace std;

namespace WiBF {

struct Wkbsd::priv_s
{
    priv_s(int k, int n, int l) : K(k), N(n), L(l),
	bps((int)round(log2((double)l))),
	dist(N,K), traceb(N,K), u(N,K), y(N,K),
	u1(L,K), y1(L,K), d1(L,K), E(K,N), E1(K,N)
    {
	sidx= new size_t[K];
	graygen(bps,gc);
    }
    ~priv_s(void) {delete sidx;}
    bool compatible(int k, int n, int l) const {
	return k==K && n==N && l==L;
    }
    static void graygen(int b, ivector_t& C);
    const int K, N, L, bps;
    ivector_t gc;
    size_t* sidx;
    Wgslmat dist, traceb, u, y, u1, y1, d1, E, E1;
private:
    priv_s(const priv_s&) : K(0), N(0), L(0), bps(0) {}
    priv_s& operator= (const priv_s&) {return *this;}
};

// generate gray-code sequence of b bits
void Wkbsd::priv_s::graygen(const int b, ivector_t& C)
{
    if (b < 1) {C.clear(); return;}
    const int n= 1<<b;
    C.resize(n);
    C[0]=0; C[1]=1;
    for (int c=1; c < b; c++) {
	int k= 1<<c;
	assert(k <= n/2);
	for (int j=0; j < k; j++)
	    C[k+j] = C[k-j-1] | (1<<c);
    }
}

// NOTE: default pivotting mode is reverse (aka Wubben)
// NOTE: if pivotting mode is set to reverse2 (aka complex Wubben)
//	it assumed G is partitioned as follows:
//	[  real(H)  imag(H);
//	  -imag(H)  real(H) ]
Wkbsd::Wkbsd(void) {priv=NULL;}

Wkbsd::~Wkbsd(void) {delete priv;}

/*
 * uhat = kbsd(x, G, L, K, [U, p])
 * schnorr-euchner method to find the closest lattice point,
 * k-best breadth-first variant with support for counter-hypothesis
 * search.
 * input arguments:
 *   x= target points, r x M matrix.
 *   G= lattice generator, N x M matrix (M >= N).
 *   L= constellation levels (e.g. 8 for 3b PAM).
 *   K= number of paths to keep
 *   U= reference for counter-hypothesis search
 *   p= bit position for counter-hypothesis search
 * output arguments:
 *   uhat= lattice closest point coordinates, r x N matrix.
 * actual closest point can be obtained by xhat= uhat*G.
 * for each input point x, this function finds the estimated
 * lattice coordinate uhat corresponding to the lattice point
 * xhat with shortest euclidean distance to x. xhat may or may
 * not be the true closest point to x.
 */
void Wkbsd::operator() (Wgslmat& uhat, const Wgslmat& X,
	const Wgslmat& G, const int L, const int K,
	const Wgslmat* U, int bitp)
{
    int j,k;

    const int npoints= X.rows();
    const int M= G.cols();
    const int N= G.rows();
    if (L < 2)
	ERROR("L < 2 or not an integer power of 2");
    const int bitsps= (int)round(log2((double)L));
    if ((1 << bitsps) != L)
	ERROR("L < 2 or not an integer power of 2");
    if (M < N)
	ERROR("G has invalid size");
    if (X.cols() != M)
	ERROR("X, G size mismatch");
    if (U!=NULL && (bitp < 0 || bitp >= bitsps*N))
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

    ivector_t Uk;
    int bk=0, bp=0;
    if (U!=NULL) {
	U3 = U->permute_cols(P);
	bk = bitp / bitsps;
	for (j=0; j < N; j++)
	    if (P(j)==bk) {bk=j; break;}
	assert(j < N);
	bp = bitp % bitsps;
	Uk.resize(N);
    }

    // temporary storage for kseclosest2()
    if (priv==NULL)
	priv = new priv_s(K,N,L);
    else if (!priv->compatible(K,N,L)) {
	delete priv;
	priv = new priv_s(K,N,L);
    }

    uhat.resize(npoints,N);

    for (k=0; k < npoints; k++) {
	if (!U3.empty()) {
	    for (j=0; j < N; j++) {
		int a= Uk[j]= (int)U3(k,j);
		if (a < -L+1 || a > L-1 || (std::abs(a) % 2)==0)
		    ERROR("U: illegal value");
	    }
	}
	int st= ksedecode2(u3hat, X3(k,0,1,N)*H3, H3, Uk, bk, bp);
	if (st==0)
	    ERROR("error calling ksedecode2()");
	for (j=0; j < N; j++) {
	    int p= (int)P(j);
	    uhat(k,p) = u3hat(0,j);
	}
    }
}

static inline unsigned bitget(unsigned w, int b) {
    return (w >> b) & 1u;
}

// k-best schnorr-euchner lattice decoder for finite constellation
int Wkbsd::ksedecode2(Wgslmat& uhat, const Wgslmat& xH,
	const Wgslmat& H, const ivector_t& U, int bitk, int bitp)
{
    priv_s& S= *priv;
    const int K= S.K;
    const int L= S.L;
    const int N= S.N;
    double x,x1;
    int i,j,k,n,s,utmp,kstep,mk;
    ivector_t mK(N), A(L);

    // initialize
    for (i=0; i < K; i++) S.E(xH,i,0);
    uhat=0.0; S.dist=0.0;
    S.u=0.0; S.y=0.0;
    S.u1=0.0; S.y1=0.0;
    S.traceb=0.0;
    k = N-1;
    kstep = 0;

    for (i=0; i < L; i++) A[i]= 2*i-L+1;
    int L2= L;
    if (!U.empty() && k==bitk) {
	// reduce constellation
	unsigned ub= bitget(S.gc[(U[k]+L-1)/2], bitp);
	for (i=0; i < L; i++)
	    if (ub == bitget(S.gc[i],bitp)) A[i]=0;
	L2 = L/2;
    }

    // top of the tree
    for (n=0; n < L2; n++) {
	if (n==0) {
	    utmp = (int)S.u1(n,0);
	    nearestu(utmp, kstep, S.E(0,k), A);
	} else {
	    utmp = (int)S.u1(n-1,0);
	    nextus2(utmp, kstep, A);
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
	if (!U.empty() && k==bitk) {
	    // reduce constellation
	    unsigned ub= bitget(S.gc[(U[k]+L-1)/2], bitp);
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
		    nearestu(utmp, kstep, S.E(s,k), A);
		} else {
		    utmp = (int)S.u1(n-1,s);
		    nextus2(utmp, kstep, A);
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
    uhat(0,0) = S.u(0,s);
    for (k=1; k < N; k++) {
	s = (int)S.traceb(k-1,s);
	uhat(0,k) = S.u(k,s);
    }
    return 1;
}


// check if u is in -L+1 : 2 : L-1
bool Wkbsd::ismember(int u, int L)
{
    if (L < 2 || L % 2)
	ERROR("unexpected error");
    for (int j= -L+1; j < L; j+=2)
	if (j==u) return true;
    return false;
}

// check if u is in A(k,:)
bool Wkbsd::ismember(int u, const ivector_t& A)
{
    if (u==0) return false;
    for (unsigned j=0; j < A.size(); j++)
	if (A[j]==u) return true;
    return false;
}

// return nearest lattice coordinate value,
// adjusting for null constellation point in A(k,:),
// and return the next step value.
void Wkbsd::nearestu(int& u, int& s, const double& e, const ivector_t& A)
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
bool Wkbsd::nextus2(int& u, int& s, const ivector_t& A)
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
void Wkbsd::nextus(int& u, int& s, int L)
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
void Wkbsd::fixsign(Wgslmat& G3, Wgslmat& Q)
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

}
