/*******************************************************************************
c4s_exception.cpp
Exception class implementation for CPP4Scripts library

--------------------------------------------------------------------------------
This file is part of Cpp4Scripts library.

  Cpp4Scripts is free software: you can redistribute it and/or modify it under
  the terms of the GNU Lesser General Public License as published by the Free
  Software Foundation, either version 3 of the License, or (at your option) any
  later version.

  Cpp4Scripts is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details:
  http://www.gnu.org/licenses/lgpl.html

Copyright (c) Menacon Ltd.
*******************************************************************************/
#ifdef C4S_LIB_BUILD
 #include <string.h>
 #include "c4s_config.hpp"
 #include "c4s_exception.hpp"
#endif
using namespace c4s;

#ifndef C4S_STL_EXCEPTIONS
c4s_exception::c4s_exception(const char *m)
{
    if(!m) {
        msg[0]=0;
        return;
    }
    size_t len = strlen(m);
    if(len>=sizeof(msg))
        len = sizeof(msg)-1;
    strncpy(msg,m, len);
    msg[len]=0;
}

c4s_exception::c4s_exception(const string &m)
{
    if(m.empty()) {
        msg[0]=0;
        return;
    }
    size_t len = m.size()>=sizeof(msg) ? sizeof(msg) : m.size();
    strncpy(msg, m.c_str(), len);
    msg[len]=0;
}
#endif
