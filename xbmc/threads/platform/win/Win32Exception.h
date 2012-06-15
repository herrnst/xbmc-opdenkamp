#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <windows.h>
#include <exception>
#include "commons/Exception.h"

class win32_exception: public XbmcCommons::Exception
{
public:
    typedef const void* Address; // OK on Win32 platform

    static void install_handler();
    virtual const char* what() const { return mWhat; };
    Address where() const { return mWhere; };
    unsigned code() const { return mCode; };
    virtual void LogThrowMessage(const char *prefix) const;
protected:
    win32_exception(const EXCEPTION_RECORD& info, const char* classname = NULL);
    static void translate(unsigned code, EXCEPTION_POINTERS* info);
private:
    const char* mWhat;
    Address mWhere;
    unsigned mCode;
};

class access_violation: public win32_exception
{
  enum access_type
  {
    Invalid,
    Read,
    Write,
    DEP
  };

public:
    Address address() const { return mBadAddress; };
    virtual void LogThrowMessage(const char *prefix) const;
protected:
    friend void win32_exception::translate(unsigned code, EXCEPTION_POINTERS* info);
private:
    access_type mAccessType;
    Address mBadAddress;
    access_violation(const EXCEPTION_RECORD& info);
};
