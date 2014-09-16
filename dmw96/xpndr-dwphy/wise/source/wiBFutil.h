/*
 **************************************************************************
 * WIBFUTIL.H
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

#ifndef _WIBFUTIL_H_
#define _WIBFUTIL_H_

#include <string>
#include <exception>

namespace WiBF {

class error : public std::exception {
    public:
	error(const std::string&);
	error(const char*, int);
	error(const char*, int, const std::string&);
	virtual ~error(void) throw();
	virtual const char* what(void) const throw();
	error& operator<< (const std::string&);
	static std::string location(const char*, int);
    private:
	void _error(const char*, int, const std::string&);
	static const char* _basename(const char*);
    private:
	std::string _what;
};

extern std::string compiler_version(const char* f=NULL);
}

#endif	//_WIBFUTIL_H_
