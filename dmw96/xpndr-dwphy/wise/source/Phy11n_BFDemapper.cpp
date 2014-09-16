//--< Phy11n_BFDetector.cpp >-----------------------------------------------
//=============================================================================
//
//  Phy11n_BFDemapper - Phy11n Receiver Demapper Models (Sphere Decoding)
//  (includes: breadth-first detector models, reference ML detector model)
//  Copyright 2011 DSP Group, Inc. All rights reserved.
//
//=============================================================================

#include "Phy11n_BFDemapper.h"

#include "wiBFkvsd.h"
#include "wiBFkbsd.h"
#include "wiBFlfsd.h"
#include "wiBFsesd.h"
#include "wiBFutil.h"
#include "wiBFformat.h"

#include "Phy11n.h"
#include "wiProcess.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <cmath>
#include <cstdlib>
#include <cassert>
#include <exception>

#include "wiBFmissing.h"

using namespace WiBF;
using namespace std;

BFextra_s::BFextra_s(int id) : thread_id(id)
{
    ch_hit= ch_miss= 0;
    ucnt= nrx= nss= nsc= maxsym= 0;
    SNR = 0;
}

// update c/h misses
void BFextra_s::chmiss(int c) {ch_miss += c;}

// update c/h hits
void BFextra_s::chhit(int c) {ch_hit += c;}

void BFextra_s::chkmaxsym(void)
{
    if (maxsym != 0) return;
    const Phy11n_RxState_t* pRX = Phy11n_RxState();
    maxsym = pRX->BFDemapper.TestVecSymbols;
    if (maxsym < 1) maxsym= -1;
    tvn = pRX->BFDemapper.TestVecFile;
}

void BFextra_s::addH(int nsc, const wiCMat H[])
{
    chkmaxsym();
    if (maxsym < 1) return;
    if (nsc < 1) return;
    const int tsc= nsc * maxsym;
    const int hmax= H[0].row * H[0].col * tsc;
    if ((int)this->H.size() >= hmax) return;
    if (this->H.empty()) {
	this->H.reserve(hmax);
	nsc = nsc;
	nrx = H[0].row;
	nss = H[0].col;
    }
    int p,i,j;
    for (p=0; p < nsc; p++) {
	assert(H[p].row==nrx && H[p].col==nss);
	for (i=0; i < nrx; i++)
	    for (j=0; j < nss; j++) {
		const wiComplex& z= H[p].val[i][j];
		this->H.push_back(dcplx_t(z.Real, z.Imag));
	    }
    }
}

void BFextra_s::addY(double K, const wiCMat* Y)
{
    chkmaxsym();
    if (maxsym < 1) return;
    const int nrx= Y->row;
    const int nsc= Y->col;
    const int tsc= nsc * maxsym;
    const int ymax= tsc*nrx;
    if ((int)this->Y.size() >= ymax) return;
    int p,i;
    if (this->Y.empty())
	this->Y.reserve(ymax);
    for (p=0; p < nsc; p++)
	for (i=0; i < nrx; i++) {
	    const wiComplex& z= Y->val[i][p];
	    this->Y.push_back(dcplx_t(z.Real*K, z.Imag*K));
	}
}

void BFextra_s::addUC(int nsc, const Wgslmat& U, const Wgslmat& C)
{
    chkmaxsym();
    if (maxsym < 1) return;
    const int tsc= nsc * maxsym;
    if (ucnt >= tsc) return;
    const int nu= U.numel();
    const int nc= C.numel();
    if (this->U.empty())
	this->U.reserve(nu*tsc);
    if (this->C.empty())
	this->C.reserve(nc*tsc);
    int i;
    for (i=0; i < nu; i++)
	this->U.push_back((int)U(i));
    for (i=0; i < nc; i++)
	this->C.push_back((int)C(i));
    ucnt++;
}

static const int testvec_thidx= 1;

static const bool per_thread_stats= false;
static const bool raw_stats= false;

static void _PrintBFStats(wiMessageFn MsgFun)
{
    const Phy11n_RxState_t* pRX= Phy11n_RxState();
    const wiInt nth= pRX->BFDemapper.nthreads;
    const BFextra_s* pBF;
    int thit=0, tmiss=0;
    int i,j,k,t;

    if (nth > 1) pRX++;

    for (k=t=0; t < nth; t++) {
	pBF = (const BFextra_s*)(pRX[t].BFDemapper.bfextra);
	if (pBF==NULL) continue;
	thit += pBF->ch_hit;
	tmiss += pBF->ch_miss;
	k++;
    }

    if (k==0) return; // no stats

    ostringstream sout;
    sout << "\n";
    if (tmiss || thit) {
	sout << Wformat("[BF] number of c/h misses= %9d (%0.1f%%)\n",
		tmiss, 100.0*tmiss/(thit+tmiss));
	sout << Wformat("[BF] number of c/h   hits= %9d (%0.1f%%)\n",
		thit, 100.0*thit/(thit+tmiss));
    }

    // merge variable stats
    Wfixpt::smap_t tmap;
    Wfixpt::smap_t::iterator tk;
    Wfixpt::smap_t::const_iterator sk;
    size_t mlen=0;

    for (t=0; t < nth; t++) {
	pBF = (const BFextra_s*)(pRX[t].BFDemapper.bfextra);
	if (pBF==NULL) continue;
	const Wfixpt::smap_t& sm= pBF->smap;
	for (sk=sm.begin(); sk != sm.end(); sk++) {
	    Wfixpt::stat_s* dt= &tmap[sk->first];
	    const Wfixpt::stat_s* st= &sk->second;
	    dt->update(st->minval, false);
	    dt->update(st->maxval, false);
	    dt->updc += st->updc;
	    dt->frac = st->frac;
	    if (mlen < sk->first.size()) mlen= sk->first.size();
	}
    }

    mlen += 2;

    for (tk=tmap.begin(); tk != tmap.end(); tk++) {
	const Wfixpt::stat_s* s1= &tk->second;
	string n= "\"" + tk->first + "\"";
	if (n.size() < mlen) n.append(mlen-n.size(), ' ');
	if (raw_stats)
	    sout << Wformat("[BF] var %s frac= %2d, range= [%12.6f, %12.6f]"
			    ", %9d updates\n", n.c_str(), s1->frac,
			    s1->minval, s1->maxval, s1->updc);
	else if (s1->frac < 0) {
	    sout << Wformat("[BF] var %s range= [%12.6f, %12.6f]"
			    ", %9d updates\n", n.c_str(),
			    s1->minval, s1->maxval, s1->updc);
	} else {
	    int sbits=0, ibits=0;
	    double mabs= std::max(std::abs(s1->minval), std::abs(s1->maxval));
	    if (s1->minval < 0) sbits=1;
	    if (mabs >= 1.0) ibits= (int)ceil(log2(mabs));
	    sout << Wformat("[BF] var %s %d.%d.%d\n", n.c_str(),
			    sbits, ibits, s1->frac);
	}

	if (!per_thread_stats) continue;

	// XXX: print per-thread stats
	const string sp(n.size(),' ');
	for (t=0; t < nth; t++) {
	    pBF = (const BFextra_s*)(pRX[t].BFDemapper.bfextra);
	    if (pBF==NULL) continue;
	    const Wfixpt::smap_t& sm= pBF->smap;
	    if (sm.empty()) continue;
	    Wfixpt::stat_s s2;
	    sk = sm.find(tk->first);
	    if (sk!=sm.end())
		s2 = sk->second;
	    sout << Wformat(
		    "[BF] %s #%02d frac= %2d, range= [%12.6f, %12.6f]"
		    ", %9d updates\n", sp.c_str(), t, s2.frac, s2.minval,
		    s2.maxval, s2.updc);
	}
    }

    cerr << sout.str();
    MsgFun("%s\n", sout.str().c_str());

    if (testvec_thidx < 1) return;

    // generate test vectors for pipeline model
    pBF = (const BFextra_s*)(pRX[testvec_thidx-1].BFDemapper.bfextra);

    if (pBF==NULL) return;
    if (pBF->maxsym < 1) return;

    const char* tvname= "testvec.m";

    if (pBF->tvn.empty())
	cerr << "[BF] test vector filename not set, using default" << endl;
    else
	tvname = pBF->tvn.c_str();

    // output test vectors as m-file
    ifstream tf(tvname);
    if (tf.is_open()) {
	tf.close();
	cerr << "[BF] " << tvname
	    << ": file already exists, won't overwrite it" << endl;
	return;
    }
    ofstream of(tvname);

    of << Wformat("%% WISE test vectors for NRx=%d, NSS=%d, NSc=%dx%d (tid%d)\n",
	    pBF->nrx, pBF->nss, pBF->nsc, pBF->ucnt/pBF->nsc, testvec_thidx);

    of << Wformat("Y = zeros(%d,%d);\n", pBF->nrx, pBF->ucnt);
    for (t=i=0; i < pBF->ucnt; i++) {
	of << Wformat("Y(:,%4d) = [",i+1);
	for (j=0; j < pBF->nrx; j++, t++) {
	    assert(t < (int)pBF->Y.size());
	    const BFextra_s::dcplx_t& z= pBF->Y[t];
	    if (j > 0) of << "; ";
	    of << Wformat("%g+(%gi)", z.real(), z.imag());
	}
	of << "];\n";
	if (t >= (int)pBF->Y.size()) break;
    }

    of << Wformat("H = zeros(%d,%d,%d);\n", pBF->nrx, pBF->nss, pBF->ucnt);
    for (t=i=0; i < pBF->ucnt; i++) {
	of << Wformat("H(:,:,%4d) = [",i+1);
	for (j=0; j < pBF->nrx; j++) {
	    if (j > 0) of << "; ";
	    for (k=0; k < pBF->nss; k++, t++) {
		assert(t < (int)pBF->H.size());
		const BFextra_s::dcplx_t& z= pBF->H[t];
		if (k > 0) of << ", ";
		of << Wformat("%g+(%gi)", z.real(), z.imag());
	    }
	}
	of << "];\n";
	if (t >= (int)pBF->H.size()) break;
    }

    of << Wformat("U = zeros(%d,%d);\n", pBF->nss*2, pBF->ucnt);
    for (t=i=0; i < pBF->ucnt; i++) {
	of << Wformat("U(:,%4d) = [",i+1);
	for (j=0; j < pBF->nss*2; j++, t++) {
	    assert(t < (int)pBF->U.size());
	    of << Wformat(" %2d", pBF->U[t]);
	}
	of << " ]';\n";
	if (t >= (int)pBF->U.size()) break;
    }

    k = pBF->C.size()/(pBF->nss*2);
    of << Wformat("C = zeros(%d,%d);\n", pBF->nss*2, k);
    for (t=i=0; i < k; i++) {
	of << Wformat("C(:,%5d) = [",i+1);
	for (j=0; j < pBF->nss*2; j++, t++) {
	    assert(t < (int)pBF->C.size());
	    of << Wformat(" %2d", pBF->C[t]);
	}
	of << " ]';\n";
	if (t >= (int)pBF->C.size()) break;
    }

    of.close();
    cerr << Wformat("[BF] %s: saved test vectors (%d/%d ofdm symbols)",
		tvname, pBF->ucnt/pBF->nsc, pBF->maxsym) << endl;
}

static int tobits(int symb, int L)
{
    const static int gc[]= {0, 1, 3, 2, 6, 7, 5, 4};
    const static int nc= sizeof(gc)/sizeof(gc[0]);
    assert(1 < L && L <= nc);
    const int s0= symb + L-1;
    assert(0 <= s0 && s0 < 2*L && (s0 & 1)==0);
    return gc[s0 >> 1];
}

static inline int bitget(unsigned w, unsigned n) {return (w >> n) & 1u;}

/*============================================================================+
|  Function  : Phy11n_KbvtDemapper()                                          |
|-----------------------------------------------------------------------------|
|  Purpose   : K-best (variable-throughput) near-ML detector for              |
|              Y = HX + N,                                                    |
|              assume QAM constellations are normalized to have average       |
|              unit energy.                                                   |
|              assume equal modulation (MCS no larger than 32).               |
|  Parameters: Y: received signal matrix after RX FFT, N_RX-by-len.           |
|              H: array of channel matrices; each element corresponds to a    |
|                 MIMO channel.                                               |
|              prior_llr: a-priori soft bit information matrix.               |
|              extr_llr:  output extrinsic soft bit information matrix.       |
|              N_BPSC:    array of number of bits per symbol per TX.          |
|              noise_std: white noise standard deviation (complex)            |
+============================================================================*/
extern "C"
wiStatus Phy11n_KbvtDemapper(wiCMat* Y, wiCMat H[],
	Phy11nSymbolLLR_t extr_LLR, Phy11nSymbolLLR_t /*prior_LLR*/,
	wiInt N_BPSC[], wiReal noise_std)
{
    const int thidx		= wiProcess_ThreadIndex();
    // receiver params
    Phy11n_RxState_t* pRX	= Phy11n_RxState();
    BFextra_s* pBF		= (BFextra_s*)(pRX->BFDemapper.bfextra);
    const int K			= pRX->BFDemapper.K;
    const int bfmode		= pRX->BFDemapper.mode;
    const int bfnorm		= pRX->BFDemapper.normtype;
    const int bffixpt		= std::max(pRX->BFDemapper.fixpt, 0);
    const int bfdistlim		= std::max(pRX->BFDemapper.distlim, 0);
    // receiver mode
    const wiInt N_SS		= H[0].col; // # of transmit antennas
    const wiInt NumRx		= H[0].row; // # of receive antennas
    const wiInt NumSubCarriers	= Y->col;
    const int NBPSC2		= N_BPSC[0]/2;
    const int NSS2		= N_SS*2;
    const int NRX2		= NumRx*2;
    const int L			= 1 << NBPSC2;
    const int numch		= N_BPSC[0] * N_SS;

    // instantiate backend
    Wkvsd kvsd(bfnorm, bffixpt, bfdistlim);

    // local vars
    Wgslmat X1(1, NRX2);
    Wgslmat G1(NSS2, NRX2);
    Wgslmat U1(1, NSS2);
    Wgslmat C1(numch, NSS2);
    Wgslmat c1(1, NSS2);
    Wgslmat E1(1, NRX2);
    Wgslmat LLR(NSS2, NBPSC2);
    wiReal Kmod, kmod2;
    wiInt i,j,k,p;

    if (pBF==NULL)
	pRX->BFDemapper.bfextra = pBF = new BFextra_s(thidx);

    if (pBF->SNR==0) {
	pBF->SNR = noise_std;
	const bool csesdf= (bfmode==3 || bfmode==4);
	const char* dname= csesdf ? "CSESDDemapper" : "KbvtDemapper";
	if (csesdf && bfnorm!=kvsd.normtype())
	    cerr << Wformat("[BF] %s: warning: norm type=%d not supported,"
		    " using norm%d", dname, bfnorm, kvsd.normtype()) << endl;
    }

    switch(NBPSC2) {
	case 1:	Kmod= sqrt( 2.0); break;	// QPSK
	case 2:	Kmod= sqrt(10.0); break;	// 16-QAM
	case 3:	Kmod= sqrt(42.0); break;	// 64-QAM
	default: return WI_ERROR_UNDEFINED_CASE;
    }

    if (testvec_thidx==thidx) {
	pBF->addH(NumSubCarriers, H);
	pBF->addY(Kmod, Y);
    }

    kmod2 = 1/(Kmod*Kmod);

    for (p=0; p < NumSubCarriers; p++) {
	// extract channel matrix
	for (i=0; i < NumRx; i++) {
	    const int i2= i+NumRx;
	    for (j=0; j < N_SS; j++) {
		const int j2= j+N_SS;
		G1(j ,i ) =  H[p].val[i][j].Real;
		G1(j2,i2) =  H[p].val[i][j].Real;
		G1(j ,i2) =  H[p].val[i][j].Imag;
		G1(j2,i ) = -H[p].val[i][j].Imag;
	    }
	}

	// extract rx vector
	for (i=0; i < NumRx; i++) {
	    const int i2= i+NumRx;
	    X1(i ) = Y->val[i][p].Real * Kmod;
	    X1(i2) = Y->val[i][p].Imag * Kmod;
	}

	// hypothesis + counter-hypotheses
	try {
	    switch(bfmode) {
		case 3:	// old implementation
		    kvsd.csesd2(U1, &C1, X1, G1, L, K);
		    break;
		case 4:
		    kvsd.csesd3(U1, &C1, X1, G1, L, K);
		    break;
		default:
		    kvsd(U1, &C1, NULL, X1, G1, L, K, bfmode);
		    break;
	    }
	} catch(const std::exception& e) {
	    cerr << "[BF] " << e.what() << endl;
	    return WI_ERROR_RETURNED;
	}
	E1 = X1 - U1*G1;
	const double n_ml= E1.norm2();
	double n_ch, n_ee, llr;

	if (testvec_thidx==thidx)
	    pBF->addUC(NumSubCarriers, U1, C1);

	try {
	    // counter-hypotheses
	    for (k=i=0; i < NSS2; i++) {
		const int hbits= tobits((int)U1(i), L);
		for (j=0; j < NBPSC2; j++, k++) {
		    const int bij= bitget(hbits, j);
		    c1 = C1.row(k);
		    if (c1==0) {
			n_ee=1.0;
			if (bfmode==1)
			    pBF->chmiss();
		    } else {
			E1 = X1 - c1*G1;
			n_ch = E1.norm2();
			n_ee = std::abs(n_ch - n_ml);
			if (bfmode==1)
			    pBF->chhit();
		    }
		    llr = (2*bij-1) * n_ee * kmod2;
		    LLR(i,j) = llr;
		}
	    }
	    if (bfmode==3 || bfmode==4) {
		pBF->chhit(kvsd.chits());
		pBF->chmiss(kvsd.cmisses());
	    }
	} catch(const std::exception& e) {
	    cerr << "[BF] " << e.what() << endl;
	    return WI_ERROR_RETURNED;
	}

	// copy LLR results
	for (i=0; i < N_SS; i++) {
	    if (N_BPSC[i] != N_BPSC[0])
		return WI_ERROR_PARAMETER5;
	    const wiInt p1= p*N_BPSC[i];
	    wiReal* xllr= extr_LLR[i];
	    for (j=0; j < N_BPSC[i]; j++) {
		const int i2= (j/NBPSC2)*N_SS + i;
		const int j2= NBPSC2-1 - (j % NBPSC2);
		xllr[p1 + j] = LLR(i2,j2);
	    }
	}
    }

    return WI_SUCCESS;
}
// end of Phy11n_KbestDemapper()

/*============================================================================+
|  Function  : Phy11n_KbestDemapper()                                         |
|-----------------------------------------------------------------------------|
|  Purpose   : K-best near-ML detector for                                    |
|              Y = HX + N,                                                    |
|              assume QAM constellations are normalized to have average       |
|              unit energy.                                                   |
|              assume equal modulation (MCS no larger than 32).               |
|  Parameters: Y: received signal matrix after RX FFT, N_RX-by-len.           |
|              H: array of channel matrices; each element corresponds to a    |
|                 MIMO channel.                                               |
|              prior_llr: a-priori soft bit information matrix.               |
|              extr_llr:  output extrinsic soft bit information matrix.       |
|              N_BPSC:    array of number of bits per symbol per TX.          |
|              noise_std: white noise standard deviation (complex)            |
+============================================================================*/
// XXX: this version is based on external back-end functions
extern "C"
wiStatus Phy11n_KbestDemapper(wiCMat* Y, wiCMat H[],
	Phy11nSymbolLLR_t extr_LLR, Phy11nSymbolLLR_t /*prior_LLR*/,
	wiInt N_BPSC[], wiReal /*noise_std*/)
{
    // receiver params
    const Phy11n_RxState_t* pRX	= Phy11n_RxState();
    const int K			= pRX->BFDemapper.K;
    // receiver mode
    const wiInt N_SS		= H[0].col; // # of transmit antennas
    const wiInt NumRx		= H[0].row; // # of receive antennas
    const wiInt NumSubCarriers	= Y->col;
    const int NBPSC2		= N_BPSC[0]/2;
    const int NSS2		= N_SS*2;
    const int NRX2		= NumRx*2;
    const int L			= 1 << NBPSC2;
    // instantiate backend
    Wkbsd kbsd;
    // local vars
    Wgslmat X1(1, NRX2);
    Wgslmat G1(NSS2, NRX2);
    Wgslmat U1(1, NSS2);
    Wgslmat C1(1, NSS2);
    Wgslmat E1(1, NRX2);
    Wgslmat LLR(NSS2, NBPSC2);
    wiInt i,j,k,p;
    wiReal Kmod, kmod2;

    switch(NBPSC2) {
	case 1:	Kmod= sqrt( 2.0); break;	// QPSK
	case 2:	Kmod= sqrt(10.0); break;	// 16-QAM
	case 3:	Kmod= sqrt(42.0); break;	// 64-QAM
	default: return WI_ERROR_UNDEFINED_CASE;
    }

    kmod2 = 1/(Kmod*Kmod);

    for (p=0; p < NumSubCarriers; p++) {
	// extract channel matrix
	for (i=0; i < NumRx; i++) {
	    const int i2= i+NumRx;
	    for (j=0; j < N_SS; j++) {
		const int j2= j+N_SS;
		G1(j ,i ) =  H[p].val[i][j].Real;
		G1(j2,i2) =  H[p].val[i][j].Real;
		G1(j ,i2) =  H[p].val[i][j].Imag;
		G1(j2,i ) = -H[p].val[i][j].Imag;
	    }
	}

	// extract rx vector
	for (i=0; i < NumRx; i++) {
	    const int i2= i+NumRx;
	    X1(i ) = Y->val[i][p].Real * Kmod;
	    X1(i2) = Y->val[i][p].Imag * Kmod;
	}

	// ML solution
	try {
	    kbsd(U1, X1, G1, L, K, NULL, 0);
	} catch(const std::exception& e) {
	    cerr << "[BF] " << e.what() << endl;
	    return WI_ERROR_RETURNED;
	}
	E1 = X1 - U1*G1;
	const double n_ml= E1.norm2();

	try {
	    // counter-hypotheses
	    for (k=i=0; i < NSS2; i++) {
		const int hbits= tobits((int)U1(i), L);
		for (j=0; j < NBPSC2; j++, k++) {
		    const int bij= bitget(hbits, j);
		    kbsd(C1, X1, G1, L, K, &U1, k);
		    E1 = X1 - C1*G1;
		    const double n_ch= E1.norm2();
		    const double n_ee= n_ch - n_ml;
		    const double llr= (2*bij-1) * n_ee * kmod2;
		    LLR(i,j) = llr;
		}
	    }
	} catch(const std::exception& e) {
	    cerr << "[BF] " << e.what() << endl;
	    return WI_ERROR_RETURNED;
	}

	// copy LLR results
	for (i=0; i < N_SS; i++) {
	    if (N_BPSC[i] != N_BPSC[0])
		return WI_ERROR_PARAMETER5;
	    const wiInt p1= p*N_BPSC[i];
	    wiReal* xllr= extr_LLR[i];
	    for (j=0; j < N_BPSC[i]; j++) {
		const int i2= (j/NBPSC2)*N_SS + i;
		const int j2= NBPSC2-1 - (j % NBPSC2);
		xllr[p1 + j] = LLR(i2,j2);
	    }
	}
    }

    return WI_SUCCESS;
}
// end of Phy11n_KbestDemapper()

/*============================================================================+
|  Function  : Phy11n_MLDemapper()                                            |
|-----------------------------------------------------------------------------|
|  Purpose   : Schnorr-Euchner ML detector for                                |
|              Y = HX + N,                                                    |
|              assume QAM constellations are normalized to have average       |
|              unit energy.                                                   |
|              assume equal modulation (MCS no larger than 32).               |
|  Parameters: Y: received signal matrix after RX FFT, N_RX-by-len.           |
|              H: array of channel matrices; each element corresponds to a    |
|                 MIMO channel.                                               |
|              prior_llr: a-priori soft bit information matrix.               |
|              extr_llr:  output extrinsic soft bit information matrix.       |
|              N_BPSC:    array of number of bits per symbol per TX.          |
|              noise_std: white noise standard deviation (complex)            |
+============================================================================*/
// XXX: this version is based on external functions
extern "C"
wiStatus Phy11n_MLDemapper(wiCMat* Y, wiCMat H[],
	Phy11nSymbolLLR_t extr_LLR, Phy11nSymbolLLR_t /*prior_LLR*/,
	wiInt N_BPSC[], wiReal /*noise_std*/)
{
    // receiver mode
    const wiInt N_SS		= H[0].col; // # of transmit antennas
    const wiInt NumRx		= H[0].row; // # of receive antennas
    const wiInt NumSubCarriers	= Y->col;
    const int NBPSC2		= N_BPSC[0]/2;
    const int NSS2		= N_SS*2;
    const int NRX2		= NumRx*2;
    const int L			= 1 << NBPSC2;
    // instantiate backend
    Wsesd sesd;
    // local vars
    Wgslmat X1(1, NRX2);
    Wgslmat G1(NSS2, NRX2);
    Wgslmat U1(1, NSS2);
    Wgslmat C1(1, NSS2);
    Wgslmat E1(1, NRX2);
    Wgslmat LLR(NSS2, NBPSC2);
    wiInt i,j,k,p;
    wiReal Kmod, kmod2;

    switch(NBPSC2) {
	case 1:	Kmod= sqrt( 2.0); break;	// QPSK
	case 2:	Kmod= sqrt(10.0); break;	// 16-QAM
	case 3:	Kmod= sqrt(42.0); break;	// 64-QAM
	default: return WI_ERROR_UNDEFINED_CASE;
    }

    kmod2 = 1/(Kmod*Kmod);

    for (p=0; p < NumSubCarriers; p++) {
	// extract channel matrix
	for (i=0; i < NumRx; i++) {
	    const int i2= i+NumRx;
	    for (j=0; j < N_SS; j++) {
		const int j2= j+N_SS;
		G1(j ,i ) =  H[p].val[i][j].Real;
		G1(j2,i2) =  H[p].val[i][j].Real;
		G1(j ,i2) =  H[p].val[i][j].Imag;
		G1(j2,i ) = -H[p].val[i][j].Imag;
	    }
	}

	// extract rx vector
	for (i=0; i < NumRx; i++) {
	    const int i2= i+NumRx;
	    X1(i ) = Y->val[i][p].Real * Kmod;
	    X1(i2) = Y->val[i][p].Imag * Kmod;
	}

	// ML solution
	try {
	    sesd(U1, NULL, NULL, X1, G1, L, NULL, 0);
	} catch(const std::exception& e) {
	    cerr << "[BF] " << e.what() << endl;
	    return WI_ERROR_RETURNED;
	}
	E1 = X1 - U1*G1;
	const double n_ml= E1.norm2();

	try {
	    // counter-hypotheses, compute log-max LLR
	    for (k=i=0; i < NSS2; i++) {
		const int hbits= tobits((int)U1(i), L);
		for (j=0; j < NBPSC2; j++, k++) {
		    const int bij= bitget(hbits, j);
		    sesd(C1, NULL, NULL, X1, G1, L, &U1, k);
		    E1 = X1 - C1*G1;
		    const double n_ch= E1.norm2();
		    const double n_ee= n_ch - n_ml;
		    if (n_ee <= 0)
			throw error(__FILE__, __LINE__, "LLR error");
		    const double llr= (2*bij-1) * n_ee * kmod2;
		    LLR(i,j) = llr;
		}
	    }
	} catch(const std::exception& e) {
	    cerr << "[BF] " << e.what() << endl;
	    return WI_ERROR_RETURNED;
	}

	// copy LLR results
	for (i=0; i < N_SS; i++) {
	    if (N_BPSC[i] != N_BPSC[0])
		return WI_ERROR_PARAMETER5;
	    const wiInt p1= p*N_BPSC[i];
	    wiReal* xllr= extr_LLR[i];
	    for (j=0; j < N_BPSC[i]; j++) {
		const int i2= (j/NBPSC2)*N_SS + i;
		const int j2= NBPSC2-1 - (j % NBPSC2);
		xllr[p1 + j] = LLR(i2,j2);
	    }
	}
    }

    return WI_SUCCESS;
}
// end of Phy11n_MLDemapper()

static void LFSDConfig(vector<int>& K1, int Nss, int Nbpsc, int K)
{
    assert(Nss > 0 && K > 1);
    assert(Nbpsc > 0 && (Nbpsc & 1)==0);
    const int L= 1 << (Nbpsc/2);
    int i,p;
    K1.resize(2*Nss,1);
    K1.back()= p= L;
    for (i= 2*Nss-1; i >= 0; i--) {
	while(p < K && K1[i] < L) {
	    K1[i] *= 2;
	    p *= 2;
	}
    }
}

extern "C"
void Phy11n_GetLFSDConfig(char* buf, int blen, int Nss, int Nbpsc, int K)
{
    if (buf==NULL || blen < 1) return;
    buf[0]=0;
    if (blen < 2) return;
    vector<int> K1;
    LFSDConfig(K1,Nss,Nbpsc,K);
    ostringstream o;
    o << Wgslmat(K1);
    string s= o.str();
    if (s.size() > unsigned(blen-1)) s="?";
#ifdef _MSC_VER
    s._Copy_s(buf,blen,s.size());
#else
    s.copy(buf,s.size());
#endif
    buf[s.size()] = 0;
}

/*============================================================================+
|  Function  : Phy11n_LFSDemapper()                                           |
|-----------------------------------------------------------------------------|
|  Purpose   : LFSD detector for                                              |
|              Y = HX + N,                                                    |
|              assume QAM constellations are normalized to have average       |
|              unit energy.                                                   |
|              assume equal modulation (MCS no larger than 32).               |
|  Parameters: Y: received signal matrix after RX FFT, N_RX-by-len.           |
|              H: array of channel matrices; each element corresponds to a    |
|                 MIMO channel.                                               |
|              prior_llr: a-priori soft bit information matrix.               |
|              extr_llr:  output extrinsic soft bit information matrix.       |
|              N_BPSC:    array of number of bits per symbol per TX.          |
|              noise_std: white noise standard deviation (complex)            |
+============================================================================*/
// XXX: this version is based on external functions
extern "C"
wiStatus Phy11n_LFSDemapper(
	wiCMat* Y, wiCMat H[],
	Phy11nSymbolLLR_t extr_LLR, Phy11nSymbolLLR_t /*prior_LLR*/,
	wiInt N_BPSC[], wiReal /*noise_std*/)
{
    // receiver params
    const Phy11n_RxState_t* pRX	= Phy11n_RxState();
    const int K			= pRX->BFDemapper.K1;
    // receiver mode
    const wiInt N_SS		= H[0].col; // # of transmit antennas
    const wiInt NumRx		= H[0].row; // # of receive antennas
    const wiInt NumSubCarriers	= Y->col;
    const int NBPSC2		= N_BPSC[0]/2;
    const int NSS2		= N_SS*2;
    const int NRX2		= NumRx*2;
    const int L			= 1 << NBPSC2;
    // instantiate backend
    Wlfsd lfsd;
    // local vars
    Wgslmat X1(1, NRX2);
    Wgslmat G1(NSS2, NRX2);
    Wgslmat U1(1, NSS2);
    Wgslmat C1;
    Wgslmat E1(1, NRX2);
    Wgslmat LLR(NSS2, NBPSC2);
    vector<int> K1;
    wiInt i,j,k,p;
    wiReal Kmod, kmod2;

    LFSDConfig(K1, N_SS, N_BPSC[0], K);

    switch(NBPSC2) {
	case 1:	Kmod= sqrt( 2.0); break;	// QPSK
	case 2:	Kmod= sqrt(10.0); break;	// 16-QAM
	case 3:	Kmod= sqrt(42.0); break;	// 64-QAM
	default: return WI_ERROR_UNDEFINED_CASE;
    }

    kmod2 = 1/(Kmod*Kmod);

    for (p=0; p < NumSubCarriers; p++) {
	// extract channel matrix
	for (i=0; i < NumRx; i++) {
	    const int i2= i+NumRx;
	    for (j=0; j < N_SS; j++) {
		const int j2= j+N_SS;
		G1(j ,i ) =  H[p].val[i][j].Real;
		G1(j2,i2) =  H[p].val[i][j].Real;
		G1(j ,i2) =  H[p].val[i][j].Imag;
		G1(j2,i ) = -H[p].val[i][j].Imag;
	    }
	}

	// extract rx vector
	for (i=0; i < NumRx; i++) {
	    const int i2= i+NumRx;
	    X1(i ) = Y->val[i][p].Real * Kmod;
	    X1(i2) = Y->val[i][p].Imag * Kmod;
	}

	// ML solution + counter-hypotheses
	try {
	    lfsd(U1, &C1, X1, G1, L, K1, true);
	} catch(const std::exception& e) {
	    cerr << "[BF] " << e.what() << endl;
	    return WI_ERROR_RETURNED;
	}
	E1 = X1 - U1*G1;
	const double n_ml= E1.norm2();

	// compute log-max LLR
	for (k=i=0; i < NSS2; i++) {
	    const int hbits= tobits((int)U1(i), L);
	    for (j=0; j < NBPSC2; j++, k++) {
		const int bij= bitget(hbits, j);
		E1 = X1 - C1.row(k)*G1;
		const double n_ch= E1.norm2();
		const double n_ee= n_ch - n_ml;
		const double llr= (2*bij-1) * n_ee * kmod2;
		LLR(i,j) = llr;
	    }
	}

	// copy LLR results
	for (i=0; i < N_SS; i++) {
	    if (N_BPSC[i] != N_BPSC[0])
		return WI_ERROR_PARAMETER5;
	    const wiInt p1= p*N_BPSC[i];
	    wiReal* xllr= extr_LLR[i];
	    for (j=0; j < N_BPSC[i]; j++) {
		const int i2= (j/NBPSC2)*N_SS + i;
		const int j2= NBPSC2-1 - (j % NBPSC2);
		xllr[p1 + j] = LLR(i2,j2);
	    }
	}
    }

    return WI_SUCCESS;
}
// end of Phy11n_LFSDemapper()

/*============================================================================+
|  Function  : Phy11n_BFDemapper_Start()                                      |
|-----------------------------------------------------------------------------|
|  Purpose   : To initialize BF demapper specific structures                  |
|                                                                             |
|  Parameters: Number of simulation threads                                   |
+============================================================================*/
extern "C"
wiStatus Phy11n_BFDemapper_Start(int NumberOfThreads)
{
    cerr << "[BF] Phy11n_BFDemapper_Start\n";
    Phy11n_RxState_t* pRX= Phy11n_RxState();
    assert(pRX->BFDemapper.bfextra == NULL);
    pRX->BFDemapper.nthreads = NumberOfThreads;

    const int N_BPSC		= pRX->N_BPSC[0];
    const int N_SS		= pRX->N_SS;
    const int K			= pRX->BFDemapper.K;
    const int bfmode		= pRX->BFDemapper.mode;
    const int bfnorm		= pRX->BFDemapper.normtype;
    const int bffixpt		= std::max(pRX->BFDemapper.fixpt, 0);
    const int bfdistlim		= std::max(pRX->BFDemapper.distlim, 0);
    const bool csesdf		= (bfmode==3 || bfmode==4);
    const char* dname		= csesdf ? "CSESDDemapper" : "KbvtDemapper";
    vector<int> K1;
    ostringstream serr;

    switch(pRX->DemapperType) {
	case 10:
	    serr << Wformat("[BF] KbestDemapper: K=%d\n",K);
	    break;
	case 11:
	    serr << "[BF] MLDemapper\n";
	    break;
	case 12:
	    LFSDConfig(K1, N_SS, N_BPSC, K);
	    serr << Wformat("[BF] LFSDemapper: K=%d/[",K);
	    serr << Wgslmat(K1) << " ]\n";
	    break;
	case 13:
	    serr << Wformat("[BF] %s: K=%d Km=%d",dname,K,bfmode);
	    if (csesdf)
		serr << Wformat(" (using norm%d)", bfnorm);
	    serr << "\n";
	    serr << Wformat("[BF] %s: fixed-point arithmetic %s",
		    dname, (bffixpt > 0 ? "ENABLED" : "DISABLED"));
	    if (bffixpt > 0)
		serr << Wformat(" (frac=%d, distlim=%d)", bffixpt, bfdistlim);
	    serr << "\n";
	    break;
    }

    cerr << serr.str() << endl;

    return WI_SUCCESS;
}

/*============================================================================+
|  Function  : Phy11n_BFDemapper_End()                                        |
|-----------------------------------------------------------------------------|
|  Purpose   : To summarize BF demapper specific stats, etc.                  |
|                                                                             |
|  Parameters: Pointer to message function                                    |
+============================================================================*/
extern "C"
wiStatus Phy11n_BFDemapper_End(wiMessageFn fnc)
{
    cerr << "\n[BF] Phy11n_BFDemapper_End\n";
    const Phy11n_RxState_t* pRX= Phy11n_RxState();
    const int bfmode= pRX->BFDemapper.mode;
    const bool csesdf= (bfmode==3 || bfmode==4);
    const char* dname= csesdf ? "CSESDDemapper" : "KbvtDemapper";
    const wiInt nth= pRX->BFDemapper.nthreads;
    if (nth > 1) pRX++;
    for (int t=0; t < nth; t++) {
	const BFextra_s* pBF= (BFextra_s*)(pRX[t].BFDemapper.bfextra);
	if (pBF==NULL) continue;
	cerr << Wformat("[BF] %s: initial SNR (%d)= %g", dname, t+1, pBF->SNR) << endl;
    }
    _PrintBFStats(fnc);
    return WI_SUCCESS;
}

//-----------------------------------------------------------------------------
//--- End of Source Code ------------------------------------------------------
