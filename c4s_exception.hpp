/*******************************************************************************
c4s_exception.hpp
Defines exceptions for CPP4Scritps library

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

Copyright (c) Menacon Ltd
*******************************************************************************/
#ifndef C4S_EXCEPTION_HPP
#define C4S_EXCEPTION_HPP

namespace c4s {
#ifndef C4S_STL_EXCEPTIONS
    /* STL exceptions have problems in some environments. Plain exceptions are offered as an alternative.
       Message space is limited but does not require allocation of memory at exception time */
    //! General/parent exception for Cpp4Scripts library
    class c4s_exception
    {
    public:
        c4s_exception(const char *m) {
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

        c4s_exception(const string &m) {
            if(m.empty()) {
                msg[0]=0;
                return;
            }
            size_t len = m.size()>=sizeof(msg) ? sizeof(msg) : m.size();
            strncpy(msg, m.c_str(), len);
            msg[len]=0;
        }

        char* what() { return msg; }

    protected:
        char msg[512];
    };
    //! Process exception
    class process_exception: public c4s_exception
    {
    public:
        process_exception(const char *m) : c4s_exception(m) {}
        process_exception(const string &m) : c4s_exception(m) {}
    };
    //! Path exception
    class path_exception: public c4s_exception
    {
    public:
        path_exception(const char *m) : c4s_exception(m) {}
        path_exception(const string &m) : c4s_exception(m) {}
    };
#else
    /* Cpp4Scripts standard exceptions simply inherit from STL runtime_error.*/
    //! Exception for process errors.
    class c4s_exception : public runtime_error
    {
    public:
        c4s_exception(const string &m) : runtime_error(m) {}
    };
    //! Process exception
    class process_exception : public c4s_exception
    {
    public:
        process_exception(const string &m) : c4s_exception(m) {}
    };

    //! Exceptions for path errors.
    class path_exception : public c4s_exception
    {
    public:
        path_exception(const string &m) : c4s_exception(m) {}
    };
#endif
}

#endif
