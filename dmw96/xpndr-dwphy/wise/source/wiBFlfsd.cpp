/*
 **************************************************************************
 * WIBFLFSD.CPP
 * created: Thu Jan 27 22:54:17 IST 2011
 *-------------------------------------------------------------------------
 * class "Wlfsd" definitions.
 * (put more comments here)
 *-------------------------------------------------------------------------
 * Author: Leonardo Vainsencher
 *-------------------------------------------------------------------------
 * Copyright (c) 2011 DSP Group, Inc. All rights reserved.
 **************************************************************************
 */

#include "wiBFlfsd.h"
#ifdef	USE_WGSLMAT
#include "wgslmat.h"
#else
#include "wiBFmatrix.h"
#endif
#include "wiBFutil.h"
#include "wiBFformat.h"
#include "wiBFmissing.h"

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cmath>
#include <cassert>

#ifdef	ERROR
#undef	ERROR
#endif
#define ERROR(s) throw error(__FILE__,__LINE__,(s))

using namespace std;

namespace WiBF {

// private struct
struct Wlfsd::priv_s {
    struct node_s {
	Wgslmat E;
	int u, step, parent;
	double ped,y;
	node_s(void) {clear();}
	void clear(void) {ped=y=u=step=0; parent= -1;}
    };
    typedef vector<node_s> nvector_t;
    typedef vector<nvector_t> lvector_t;
    priv_s(const ivector_t& k, int l, int nl);
    ~priv_s(void) {delete sidx;}
    bool compat(const ivector_t& k, int l, int nl) const;
    int graycode(int) const;
    int graysymb(int) const;
    const ivector_t K;
    const int N,L,NL;
    ivector_t K1;
    lvector_t lev;
    size_t* sidx;
private:
    vector<int> _gray2sym, _sym2gray;
    void _graymap(void);
    priv_s(const priv_s&) : N(0), L(0), NL(0) {}
    priv_s& operator= (const priv_s&) {return *this;}
};

Wlfsd::priv_s::priv_s(const ivector_t& ndist, int l, int nl) :
    K(ndist), N(ndist.size()), L(l), NL(nl)
{
    int k,n,s;
    assert(!K.empty());
    assert(N > 1 && L > 1);
    assert(NL > 0);
    sidx= new size_t[NL];
    for (n=0; n < NL; n++) sidx[n]=0;
    K1.resize(N);
    lev.resize(N+1);
    lev[N].resize(1);
    lev[N][0].E.resize(1,N);
    for (k=N-1; k >= 0; k--) {
	n = K[k];
	assert(0 < n && n <= L);
	n *= lev[k+1].size();
	K1[k] = n;
	lev[k].resize(n);
	for (s=0; s < n; s++)
	    lev[k][s].E.resize(1,N);
    }
    _graymap();
}

void Wlfsd::priv_s::_graymap(void)
{
    _sym2gray.resize(L);
    _gray2sym.resize(L);
    _sym2gray[0] = 0;
    _sym2gray[1] = 1;
    _gray2sym[0] = -L+1;
    _gray2sym[1] = _gray2sym[0]+2;
    int l=2;
    while (l < L) {
	for (int i=0; i < l; i++) {
	    int c= _sym2gray[l-1-i] | l;
	    _sym2gray[l+i] = c;
	    _gray2sym[c] = -L+1 + 2*(l+i);
	}
	l *= 2;
    }
    assert(l==L);
}

bool Wlfsd::priv_s::compat(const ivector_t& k, int l, int nl) const
{
    return (k==K && l==L && nl==NL);
}

int Wlfsd::priv_s::graycode(int symb) const
{
    const int u= symb+L-1;
    assert(0 <= u && u <= 2*L-2 && (u & 1)==0);
    return _sym2gray[u >> 1];
}

int Wlfsd::priv_s::graysymb(int code) const
{
    assert(0 <= code && code < L);
    return _gray2sym[code];
}

Wlfsd::Wlfsd(void) : priv(NULL) {}

Wlfsd::~Wlfsd(void) {delete priv;}

/*
 * [uhat, chat] = lfsd(x, G, L, K)
 * LFSD method to find the closest lattice point,
 * input arguments:
 *   x= target points, n x M matrix.
 *   G= lattice generator, N x M matrix (M >= N).
 *   L= PAM alphabet size
 *   K= node configuration
 * output arguments:
 *   uhat= coordinates of closest lattice point, n x N matrix.
 *   chat= coordinates of closest counter hypotheses for each bit,
 *	c x N matrix, where c= n * N * log2(L).
 * actual closest lattice point can be obtained with xhat= uhat*G.
 * for each input point x, this function finds the estimated
 * lattice coordinate uhat corresponding to the lattice point
 * xhat with shortest euclidean distance to x. xhat may or may
 * not be the true closest point to x.
 */
void Wlfsd::operator() (Wgslmat& uhat, Wgslmat* pchat, const Wgslmat& X,
	const Wgslmat& G, const int L, const ivector_t& K, const bool pca)
{
    int i,j,p;

    const int npoints= X.rows();
    const int M= X.cols();
    const int N= G.rows();
    if (G.cols() != M)
	ERROR("X, G size mismatch");
    if (M < N)
	ERROR("G has invalid size");

    const int bps= (int)round(log2((double)L));
    if (L < 2 || L != (1 << bps))
	ERROR("invalid L");

    Wgslmat Q,R,P;
    G.transpose().sqrp(Q,R,P);
    Wgslmat G2(G.permute_rows(P));

    Wgslmat G3= R(0,0,N,N).transpose();
    fixsign(G3,Q);
    Wgslmat H3= G3.ltinverse();
    Wgslmat X3= X*Q(0,0,M,N);
    const int numchyp= bps*N;
    Wgslmat u3hat(1,N), c3hat(numchyp,N);
    Wgslmat* pc3hat= pchat!=NULL ? &c3hat : NULL;
    Wgslmat xH3;

    // temp storage for lfsdecode()
    if (priv==NULL)
	priv = new priv_s(K, L, 1);
    if (!priv->compat(K, L, 1)) {
	delete priv;
	priv = new priv_s(K, L, 1);
    }

    uhat.resize(npoints,N);
    if (pchat!=NULL)
	pchat->resize(npoints*numchyp, N);

    for (p=0; p < npoints; p++) {

	xH3 = X3.row(p)*H3;

	const int uidx= lfsdecode(u3hat, H3, xH3, -1, 0, 0);

	for (j=0; j < N; j++) {
	    const int r= (int)P(j);
	    uhat(p,r) = u3hat(j);
	}

	if (pc3hat==NULL) continue;

	if (pca)
	    pchsea(c3hat, u3hat, H3, xH3, p);
	else
	    chsurv(c3hat, u3hat, uidx);
	const int pc= p*numchyp;
	for (i=0; i < numchyp; i++) {
	    for (j=0; j < N; j++) {
		const int r= (int)P(j);
		const int s= (int)P(i/bps);
		const int k= s*bps + i%bps;
		(*pchat)(pc+k,r) = c3hat(i,j);
	    }
	}
    }
}

// find bit counter-hypotheses among hypothesis survivors
// returns list of counter-hypotheses (one for each bit), log2(L) x N matrix.
// if a bit counter-hypothesis cannot be found, a zero vector is returned
// for the corresponding bit.
void Wlfsd::chsurv(Wgslmat& chat, const Wgslmat& uhat, const int uidx) const
{
    const priv_s& S= *priv;
    const priv_s::lvector_t& lev= S.lev;
    const int L= S.L;
    const int N= S.N;
    const int bps= (int)round(log2((double)L));
    const int numsurv = lev[0].size();
    int i,j,k;

    assert(chat.rows() == bps*N && chat.cols() == N);
    Wgslmat usurv(numsurv,N);

    // collect all survivor paths
    for (i=0; i < numsurv; i++) {
	int h=i;
	for (k=0; k < N; k++) {
	    usurv(i,k) = lev[k][h].u;
	    h = lev[k][h].parent;
	}
    }

    // for each level
    for (k=0; k < N; k++) {
	// hyp symbol at this level
	const int uk= (int)uhat(k);
	const int gk= S.graycode(uk);

	// for each bit at this level
	for (i=0; i < bps; i++) {
	    // find "best" c-hyp in list
	    double dmin=0;
	    int jmin= -1;

	    for (j=0; j < numsurv; j++) {
		const int ujk= (int)usurv(j,k);
		const int gjk= S.graycode(ujk);
		if (((gjk ^ gk) & (1 << i)) == 0) continue;
		assert(j!=uidx);
		// found eligible c-hyp
		double dj= lev[0][j].ped;
		if (jmin < 0 || (dj < dmin)) {jmin=j; dmin=dj;}
	    }

	    const int ki= k*bps+i;
	    if (jmin >= 0) {
		for (j=0; j < N; j++) chat(ki,j)=usurv(jmin,j);
	    } else {
		// no eligible c-hyp - flag it by setting to zero
		for (j=0; j < N; j++) chat(ki,j)=0;
	    }
	}
    }
}

// search bit counter-hypotheses using additional candidates
// returns list of counter-hypotheses (one for each bit), log2(L) x N matrix.
void Wlfsd::pchsea(Wgslmat& chat, const Wgslmat& uhat, const Wgslmat& H,
	const Wgslmat& xH, const int idx)
{
    const priv_s& S= *priv;
    const int L= S.L;
    const int N= S.N;
    const int bps= (int)round(log2((double)L));
    Wgslmat temp(1,N);
    int k,n,p;
    for (p=k=0; k < N; k++) {
	const int xk= (int)uhat(k);
	const int gk= S.graycode(xk);
	for (n=0; n < bps; n++, p++) {
	    const unsigned b= (gk >> n) & 1;
	    lfsdecode(temp, H, xH, k, n, b);
	    //cout << Wformat("%3d: k=%d n=%d chat=",idx,k,n) << temp;
	    chat(temp,p,0);
	}
    }
}

// LFSD detector with schnorr-euchner enumeration for finite constellations.
// return the ML hypothesis, and its survivor index.
int Wlfsd::lfsdecode(Wgslmat& uhat, const Wgslmat& H, const Wgslmat& xH,
	const int bitk, const int bitn, const int bith)
{
    priv_s& S= *priv;
    priv_s::lvector_t& lev= S.lev;
    const int numsurv = lev[0].size();
    const int L= S.L;
    const int N= S.N;
    int i,k,n,p,s,u,step;
    double  y;

    lev[N][0].E = xH;

    for (k=N-1; k >= 0; k--) {

	const double h_kk= H(k,k);
	const int K= (bitk==k) ? std::min(L/2,S.K[k]) : S.K[k];

	assert(h_kk > 0);
	for (n=0; n < (int)lev[k].size(); n++) lev[k][n].clear();
	y=u=step=0;

	// for each parent
	const int numpar= lev[k+1].size();
	for (s=p=0; p < numpar; p++) {
	    const priv_s::node_s& parent= lev[k+1][p];
	    if (k < N-1 && parent.u==0) continue;

	    // for each child
	    for (n=0; n < K; n++, s++) {
		priv_s::node_s& child= lev[k][s];
		child.parent = p;

		if (k == N-1)
		    child.E = parent.E;
		else if (n==0) {
		    for (i=0; i <= k; i++)
			child.E(i) = parent.E(i) - parent.y * H(k+1,i);
		} else
		    child.E = lev[k][s-1].E;

		double e= child.E(k);

		if (n==0) {
		    u = nearestu(e, L);
		    step = (e > u) ? 1 : -1;
		} else
		    nextus(u, step, L);

		if (k == bitk)
		    while( u!=0 && (((i=S.graycode(u)) >> bitn) & 1) == bith )
			nextus(u, step, L);

		if (u==0)
		    ERROR("unexpected error");

		y = (e - u) / h_kk;

		child.u = u;
		child.y = y;
		child.ped = parent.ped + y*y;
		child.step = step;
	    }

	    for ( ; n < S.K[k]; n++, s++) {
		priv_s::node_s& child= lev[k][s];
		child.clear();
	    }
	}
    }

    double edmin=0; int smin= -1;
    for (s=0; s < numsurv; s++) {
	const priv_s::node_s& child= lev[0][s];
	if (child.u==0) {
	    if (0 > bitk || bitk >= N)
		ERROR("unexpected error");
	    continue;
	}
	if (smin < 0 || edmin > child.ped) {smin=s; edmin=child.ped;}
    }

    for (s=smin, k=0; k < N; k++) {
	if ((unsigned)s >= lev[k].size())
	    ERROR("unexpected error");
	const priv_s::node_s& child= lev[k][s];
	if (child.u==0)
	    ERROR("unexpected error");
	uhat(k) = child.u;
	s = child.parent;
    }

    return smin;
}

int Wlfsd::nearestu(double e, const int L) const
{
    double dmin=0;
    int lmin=0;
    for (int l= -L+1; l <= L-1; l+=2) {
	double d= std::abs(e-l);
	if (lmin==0 || d < dmin) {dmin=d; lmin=l;}
    }
    return lmin;
}

void Wlfsd::nextus(int& u, int& s, const int L) const
{
    for (int i=0; i < 2; i++) {
	u += 2*s;
	s = -s - (s > 0 ? 1 : -1);
	if (-L+1 <= u && u <= L-1) return;
    }
    u=s=0;
    return;
}

// fix sign of G3 diagonal
void Wlfsd::fixsign(Wgslmat& G3, Wgslmat& Q) const
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
