/*
 **************************************************************************
 * WIBFUTIL.CPP
 * created: Sun Nov 21 18:35:54 JST 2010
 *-------------------------------------------------------------------------
 * Misc utilities for BF dempappers
 * (put more comments here)
 *-------------------------------------------------------------------------
 * Author: Leonardo Vainsencher
 *-------------------------------------------------------------------------
 * Copyright (c) 2011 DSP Group, Inc. All rights reserved.
 **************************************************************************
 */

#include "wiBFutil.h"
#include "wiBFformat.h"
#include <sstream>
#include <cassert>

using namespace std;

#ifdef __GNUC__
#define _compnam_ "GCC";
#define _compver_ ((__GNUC__)+(__GNUC_MINOR__)/10.0) 
#elif defined(_MSC_VER)
#define _compnam_ "MSC";
#define _compver_ (_MSC_VER/100.0)
#else
#define _compnam_ "???";
#define _compver_ (0.0)
#endif

namespace WiBF {

string compiler_version(const char* f)
{
    const char* cn= _compnam_;
    const double cv= _compver_;
    ostringstream o;
    if (f!=NULL) {
	string fn= f;
	size_t k= fn.find_last_of("/\\");
	if (k < fn.size()) fn.erase(0,k+1);
	o << Wformat("\"%s\" compiled with %s-%0.2f", fn.c_str(), cn, cv);
    } else
	o << Wformat("%s-%0.2f", cn, cv);
    return o.str();
}

//-------------------------------------------------------------------------
error::error(const string& tx) : _what(tx) {}
error::error(const char* fn, int ln) {_error(fn,ln,"");}
error::error(const char* f, int l, const string& s) {_error(f,l,s);}

error::~error(void) throw() {}

const char* error::what(void) const throw() {return _what.c_str();}

error& error::operator<< (const std::string& tx)
{
    _what += tx;
    return *this;
}

void error::_error(const char* fn, int ln, const string& tx)
{
    ostringstream o;
    if (fn==NULL || ln <= 0) _what=tx;
    else {
	o << _basename(fn) << ":" << ln << " : " << tx;
	_what = o.str();
    }
}

const char* error::_basename(const char* p)
{
    static const char _empty[]="";
    if (p==NULL) return "<NULL>";
    string s(p);
    size_t k= s.find_last_of("/\\");
    if (k+1==s.size()) {
	assert(p[k+1]==0);
	return _empty;
    }
    if (k+1 < s.size()) k++;
    else k=0;
    return p+k;
}

string error::location(const char* P, int L)
{
    ostringstream o;
    o << _basename(P) << ":" << L << " : ";
    return o.str();
}

}
