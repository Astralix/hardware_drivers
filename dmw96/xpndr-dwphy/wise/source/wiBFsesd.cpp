/*
 **************************************************************************
 * WIBFSESD.CPP
 * created: Mon Feb  7 14:46:28 JST 2011
 *-------------------------------------------------------------------------
 * class "Wsesd" definitions.
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

#include "wiBFsesd.h"
#ifdef	USE_WGSLMAT
#include "wgslmat.h"
#else
#include "wiBFmatrix.h"
#endif
#include "wiBFutil.h"
#include "wiBFmissing.h"

#include <iostream>
#include <sstream>
#include <cmath>
#include <cassert>

#ifdef	ERROR
#undef	ERROR
#endif
#define ERROR(s) throw error(__FILE__,__LINE__,(s))

using namespace std;

namespace WiBF {

Wsesd::Wsesd(void) {}
Wsesd::~Wsesd(void) {}

/*
 * [uhat, niter, xhat] = sesd(x, G, A)
 * schnorr-euchner method to find the closest lattice point,
 * with support for skipping null constellation points.
 * input arguments:
 *   x= target points, k x M matrix.
 *   G= lattice generator, N x M matrix (M >= N).
 *   A= lattice coordinate alphabet; this argument can be a scalar,
 *	a 1 x L vector or a N x L matrix.
 * output arguments:
 *   uhat= resulting lattice coordinates, k x N matrix.
 *   niter= number of iterations required to find results, k x 1 vector.
 *   xhat= closest lattice points, k x M matrix.
 * xhat and niter output arguments are optional.
 * for each input point x, this function finds the lattice
 * coordinate uhat corresponding to the lattice point xhat
 * with shortest euclidean distance to x.
 * the number of search iterations for each point is also returned.
 * for each coordinate c=1..N A(c,:) is used as dictionary of valid
 * coordinate values; null (0) values in A are skipped.
 */

void Wsesd::operator() (
	Wgslmat& uhat, ivector_t* pNitr, Wgslmat* pXhat,
	const Wgslmat& X, const Wgslmat& G, const int L,
	const Wgslmat* U, int bitp) const
{
    const int N= G.rows();
    const int M= G.cols();
    const int npt= X.rows();
    if (M < N)
	ERROR("G: invalid size");
    if (X.cols()!=M)
	ERROR("X: invalid size");

    if (L < 2)
	ERROR("L: illegal value");
    const int bitsps= (int)round(log2(L));
    if ((1 << bitsps) != L)
	ERROR("L: illegal value");

    if (bitp >= bitsps*N)
	ERROR("bit_pos: outside range");
    if (U!=NULL && (U->rows()!=npt || U->cols()!=N))
	ERROR("U: invalid size");
    if (U!=NULL && bitp < 0)
	ERROR("missing/illegal argument(s)");

    double x=L;
    const int maxit= int( x * (std::pow(x,N) - 1) / (x-1) );
    int i,j,k;

    Wgslmat Q,R,P;
    G.transpose().qrp(Q,R,P);

    Wgslmat G2= G.permute_rows(P);
    Wgslmat G3= R(0,0,N,N).transpose();
    fixsign(G3,Q);

    Wgslmat H3= G3.ltinverse();
    Wgslmat X3= X*Q(0,0,M,N);
    Wgslmat u3hat(1,N);
    Wgslmat x3hat(1,M);
    ivector_t grayc;

    graycodegen(bitsps, grayc);
    assert((int)grayc.size() == L);

    Wgslmat A0(N,L), A1;

    for (i=0; i < N; i++)
	for (j=0; j < L; j++) A0(i,j)= -L+1 + 2*j;

    if (U==NULL) bitp= -1;

    if (bitp >= 0) {
	// counter-hypothesis search
	A1 = A0;
	k = bitp/bitsps;
	for (j=0; j < N; j++)
	    if (P(j)==k) break;
	assert(j < N);
	k = bitp % bitsps;
	for (i=0; i < L; i++) {
	    int bi= (grayc[i] >> k) & 1;
	    if (bi) A0(j,i)=0;
	    else A1(j,i)=0;
	}
    } else {
	// ML search
	j= k= -1;
    }
    const int bit_n= j;
    const int bit_b= k;

    if (pNitr!=NULL) pNitr->resize(npt);
    if (pXhat!=NULL) pXhat->resize(X);

    uhat.resize(npt,N);

    for (k=0; k < npt; k++) {
	const Wgslmat* A= &A0;
	if (bit_n >= 0) {
	    // change search constellation depending on bit value at
	    // selected bit position
	    j = (int)(*U)(k, bitp/bitsps);
	    j += L-1;
	    if (j < 0 || j % 2 || j > 2*L-2)
		ERROR("illegal symbol value");
	    j = grayc[j/2];
	    int bitv= (j >> bit_b) & 1;
	    if (bitv==0) A= &A1;
	}
	int n1= sedecode2(H3, X3(k,0,1,N)*H3, *A, maxit, u3hat);
	if (n1==0)
	    ERROR("error in sedecode2()");
	for (j=0; j < N; j++) {
	    int p= (int)P(j);
	    uhat(k,p) = u3hat(0,j);
	}
	if (pNitr!=NULL) (*pNitr)[k]= n1;
	if (pXhat!=NULL) {
	    x3hat = u3hat*G2;
	    for (j=0; j < M; j++)
		(*pXhat)(k,j) = x3hat(0,j);
	}
    }
}

static inline unsigned bitget(unsigned w, int b) {return (w >> b) & 1u;}

// alternate calling sequence: calculates ML hypothesis along with
// all counter-hypotheses.
void Wsesd::operator() (
	Wgslmat& uhat, Wgslmat* pChat, ivector_t* pNitr, Wgslmat* pXhat,
	const Wgslmat& X, const Wgslmat& G, const int L) const
{
    if (pChat==NULL) {
	// fall back to original calling sequence
	(*this)(uhat, pNitr, pXhat, X, G, L);
	return;
    }

    const int N= G.rows();
    const int M= G.cols();
    const int npt= X.rows();
    if (M < N)
	ERROR("G: invalid size");
    if (X.cols()!=M)
	ERROR("X: invalid size");

    if (L < 2)
	ERROR("L: illegal value");
    const int bitsps= (int)round(log2(L));
    if ((1 << bitsps) != L)
	ERROR("L: illegal value");

    double x=L;
    const int maxit= int( x * (std::pow(x,N) - 1) / (x-1) );
    const int numch= N * bitsps;
    int i,j,k,p,n1;

    Wgslmat Q,R,P;
    G.transpose().qrp(Q,R,P);

    Wgslmat G2= G.permute_rows(P);
    Wgslmat G3= R(0,0,N,N).transpose();
    fixsign(G3,Q);

    Wgslmat H3= G3.ltinverse();
    Wgslmat X3= X*Q(0,0,M,N);
    Wgslmat u3hat(1,N), c3hat(1,N);
    Wgslmat xH3(1,N);
    Wgslmat x3hat(1,M);
    Wgslmat A0(N,L), A1;
    ivector_t grayc;

    graycodegen(bitsps, grayc);
    assert((int)grayc.size() == L);

    for (i=0; i < N; i++)
	for (j=0; j < L; j++) A0(i,j)= 2*j - (L-1);

    if (pNitr!=NULL) pNitr->resize(npt);
    if (pXhat!=NULL) pXhat->resize(X);
    if (pChat!=NULL) pChat->resize(npt*N*bitsps, N);

    uhat.resize(npt,N);

    for (p=0; p < npt; p++) {
	xH3 = X3.row(p)*H3;

	// find ML hypothesis
	n1 = sedecode2(H3, xH3, A0, maxit, u3hat);
	if (n1==0)
	    ERROR("error in sedecode2()");
	for (j=0; j < N; j++) {
	    k = (int)P(j);
	    uhat(p,k) = u3hat(j);
	}

	if (pNitr!=NULL) (*pNitr)[p]= n1;

	if (pXhat!=NULL) {
	    x3hat = u3hat*G2;
	    for (j=0; j < M; j++)
		(*pXhat)(p,j) = x3hat(j);
	}

	if (pChat==NULL) continue;

	// find counter-hypotheses
	const int chbase= numch*p;

	for (j=0; j < N; j++) {
	    const int s= chbase + (int)P(j) * bitsps;
	    const int uj= (int)u3hat(j) + L-1;
	    assert(0 <= uj && uj <= 2*L-2 && (uj & 1)==0);
	    const int cj= grayc[uj >> 1];
	    for (i=0; i < bitsps; i++) {
		A1 = A0;
		for (k=0; k < L; k++)
		    if (bitget(grayc[k] ^ cj, i) == 0)
			A1(j,k) = 0;
		n1 = sedecode2(H3, xH3, A1, maxit, c3hat);
		if (n1==0)
		    ERROR("error in sedecode2()");
		pChat->setrow(s+i, c3hat.inverse_permute_cols(P));
		if (pNitr!=NULL) (*pNitr)[p] += n1;
	    }
	}
    }
}

int Wsesd::sedecode2(const Wgslmat& H, const Wgslmat& xH,
	const Wgslmat& A, int maxit, Wgslmat& uhat)
{
    const int N= H.rows();
    int i, nit=0, k=N-1;

    if (H.cols()!=N) return 0;
    if (xH.rows()!=1 || xH.cols()!=N) return 0;
    if (uhat.rows()!=1 || uhat.cols()!=N) return 0;

    Wgslmat E(N,N);
    dvector_t dist(N);
    ivector_t u(N), step(N);
    dvector_t h1(N);
    double bestdist= HUGE_VAL; //INFINITY;
    double y, newdist, Ekk;

    for (i=0; i < N; i++)
	if (H(i,i) <= 0)
	    ERROR("unexpected error");

    E(xH,k,0);
    Ekk = E(k,k);
    nearestu(u[k], step[k], Ekk, A, k);
    y = (Ekk-u[k]) / H(k,k);

    while(true) {
	if (nit > maxit) {
	    ostringstream o;
	    o << "sedecode2() cannot converge\n";
	    o << "H=\n" << H;
	    o << "\nxH=\n" << xH;
	    ERROR(o.str().c_str());
	}
	newdist = dist[k] + y*y;
	if (newdist < bestdist) {
	    if (k > 0) {
		// move DOWN
		for (i=0; i < k; i++)
		    E(k-1,i) = E(k,i) - y*H(k,i);
		k--;
		dist[k] = newdist;
		Ekk = E(k,k);
		nearestu(u[k], step[k], Ekk, A, k);
		y = (Ekk-u[k]) / H(k,k);
		if (nit || k==0) nit++;
		continue;
	    }
	    // move UP
	    if (k >= N-1) return 0; // should NEVER happen
	    for (i=0; i < N; i++) uhat(0,i)=u[i];
	    bestdist = newdist;
	    k++;
	    if (nextus2(u[k], step[k], A, k)) {
		y = (E(k,k)-u[k]) / H(k,k);
		nit++;
	    } else
		y = HUGE_VAL; //INFINITY;
	    continue;
	}
	// move UP
	k++;
	if (k >= N) break;
	if (nextus2(u[k], step[k], A, k)) {
	    y = (E(k,k)-u[k]) / H(k,k);
	    nit++;
	} else
	    y = HUGE_VAL; //INFINITY;
    }

    return nit;
}

// check if u is in -L+1 : 2 : L-1
bool Wsesd::ismember(int u, int L)
{
    if (L < 2 || L % 2)
	ERROR("unexpected error");
    for (int j= -L+1; j < L; j+=2)
	if (j==u) return true;
    return false;
}

// check if u is in A(k,:)
bool Wsesd::ismember(int u, const Wgslmat& A, int k)
{
    if (u==0) return false;
    for (int j=0; j < A.cols(); j++)
	if (A(k,j)==u) return true;
    return false;
}

// return nearest lattice coordinate value,
// adjusting for null constellation point in A(k,:),
// and return the next step value.
void Wsesd::nearestu(int& u, int& s, const double& e,
	const Wgslmat& A, int k)
{
    const int L= A.cols();
    double dmin=0;
    int j;
    for (j=0; j < L; j++) {
	int l= -L+1 + 2*j;
	double d= std::abs(e-l);
	if (j==0 || d < dmin) {dmin=d; u=l;}
    }
    s=  (e > u) ? 1 : -1;
    while (!ismember(u,A,k)) {
	nextus(u,s,L);
	if (u==0)
	    ERROR("unexpected error");
    }
}

// return next lattice coordinate according to the given step,
// adjusting for null constellation point in A(k,:),
// and return the next step value.
bool Wsesd::nextus2(int& u, int& s, const Wgslmat& A, int k)
{
    const int L= A.cols();
    nextus(u,s,L);
    while (u && !ismember(u,A,k))
	nextus(u,s,L);
    return u!=0;
}

// return next lattice coordinate according to the given step;
// if a constellation point cannot be found after 2 iterations,
// then u and s are both set to 0.
void Wsesd::nextus(int& u, int& s, int L)
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
void Wsesd::fixsign(Wgslmat& G3, Wgslmat& Q)
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

// generate gray-code sequence of b bits
void Wsesd::graycodegen(const int b, ivector_t& C)
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

}
