/*
 **************************************************************************
 * WIBFFORMAT.H
 * created: Wed Feb 16 10:11:37 JST 2011
 *-------------------------------------------------------------------------
 * class "Wformat" declarations.
 * provides functionality compatible with a subset of boost::format,
 * to remove dependency on boost.
 * (put more comments here)
 *-------------------------------------------------------------------------
 * Author: Leonardo Vainsencher
 *-------------------------------------------------------------------------
 * Copyright (c) 2011 DSP Group, Inc. All rights reserved.
 **************************************************************************
 */

#ifndef _WIBFFORMAT_H_
#define _WIBFFORMAT_H_

#include <string>
#include <iostream>
#include <cstdarg>

namespace WiBF {

class Wformat
{
public:
    explicit Wformat(const char* fmt, ...);
    ~Wformat(void);
    std::string str(void) const;
    operator std::string (void) const;
    void puts(std::string& s) const;
private:
    static void doit(std::string& out, va_list* va, const char* fmt);
private:
    std::string _out;
};

std::ostream& operator<< (std::ostream&, const WiBF::Wformat&);
}

#endif	//_WIBFFORMAT_H_
