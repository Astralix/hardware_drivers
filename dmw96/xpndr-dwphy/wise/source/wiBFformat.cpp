/*
 **************************************************************************
 * WIBFFORMAT.CPP
 * created: Wed Feb 16 10:11:37 JST 2011
 *-------------------------------------------------------------------------
 * class "Wformat" definitions.
 * provides functionality compatible with a subset of boost::format,
 * to remove dependency on boost.
 * (put more comments here)
 *-------------------------------------------------------------------------
 * Author: Leonardo Vainsencher
 *-------------------------------------------------------------------------
 * Copyright (c) 2011 DSP Group, Inc. All rights reserved.
 **************************************************************************
 */

#include "wiBFformat.h"
#include "wiBFmissing.h"
#include <exception>
#include <cstdio>
#include <cstring>
#include <sstream>

using namespace std;

namespace WiBF {

Wformat::Wformat(const char* fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    doit(_out, &va, fmt);
}

Wformat::~Wformat(void) {}

Wformat::operator string (void) const {return _out;}

string Wformat::str(void) const {return _out;}

void Wformat::doit(string& out, va_list* va, const char* fmt)
{
    char buf[1000];
    const size_t bsz= sizeof(buf);
    int n= vsnprintf(buf, bsz-1, fmt, *va);
    if (n < 0)
	out = "<<error>>";
    else {
	out = buf;
	if (n >= (int)bsz-1)
	    out += "<<truncated>>";
    }
}

ostream& operator<< (ostream& o, const Wformat& f)
{
    o << f.str();
    return o;
}

}
