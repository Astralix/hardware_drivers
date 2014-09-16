/*
 **	IError.h
 **
 **	Published / author: 2004-06-11 / grymse@alhem.net
 **/

/*
Copyright (C) 2004,2005,2006  Anders Hedstrom

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "sqlite3.h"

#ifndef _IERROR_H_SQLITE
#define _IERROR_H_SQLITE

#include <string>


#ifdef SQLITEW_NAMESPACE
namespace SQLITEW_NAMESPACE {
#endif


class Database;
class Query;


/** Log class interface. */
class IError
{
public:
	virtual void error(Database&,const std::string&) = 0;
	virtual void error(Database&,Query&,const std::string&) = 0;
#ifdef ANDROID
    virtual ~IError(){};
#endif
};


#ifdef SQLITEW_NAMESPACE
} // namespace SQLITEW_NAMESPACE {
#endif

#endif // _IERROR_H
