#ifndef _WIBFMISSING_H_
#define _WIBFMISSING_H_

#ifdef _MSC_VER

#include <cmath>

namespace WiBF {
    static inline double log2(double x) {
	return log(x)/log(2.0);
    }

    static inline double round(const double& x) {
	return x < 0 ? ceil(x-0.5) : floor(x+0.5);
    }
}

//#define	vsnprintf(b,bsz,fm,va) _vsnprintf_s(b,bsz,(bsz-1),fm,va)

#endif	// _MSC_VER

#ifdef	max
#undef	max
#endif
#ifdef	min
#undef	min
#endif

#endif	// _WIBFMISSING_H_
