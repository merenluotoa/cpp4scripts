/*! \file c4s_util.hpp
 * \brief Misc. utility functions for Cpp4Scripts library */
/*--------------------------------------------------------------------------------
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

Copyright (c) Menacon Ltd, Finland
*******************************************************************************/
#ifndef C4S_UTIL_HPP
#define C4S_UTIL_HPP
namespace c4s {

//inline void cp(const char *from, const char *to) { path source(from); source.cp(to); }
//inline void cp_to_dir(const char *from, path &to, path::cpf flags=path::NONE) { path source(from); source.cp(to,flags); }
enum DATETYPE { DATE_ONLY, WITH_TIME };
bool exists_in_path(const char *file);
#ifdef _WIN32
//! Provides error string output from Windows system with the same name as the linux 'strerror' fuction.
const char* strerror(int);
#endif
//! Returns a string designating the build type. Eg '64bit-Release'
const char* get_build_type();
//! Returns todays timestamp in ISO format
const char* get_ISO_date(DATETYPE);

//! Makes sure that the path separators int the given target string are native type.
string force_native_dsep(const string &);
//! Gets an environment value.
bool   get_env_var(const char *, string &);
//! Matches the wildcard to the string and returns true if it succeeds.
bool   match_wildcard(const char * pWild, const char * pString);
//! Returns users login or real name as configured to the system.
string get_user_name(bool loginname=true);
//! Returns current hostname
const char* get_host_name();
//! Searches needle in the target file.
bool search_file(fstream &target, const string &needle);
//! Efficient string search function.
bool search_bmh(const unsigned char* haystack, SSIZE_T hlen, const unsigned char* needle, SSIZE_T nlen, SIZE_T *offset_out);
//! Waits for input on stdin
bool wait_stdin(int timeout);
//! Appends a slash tot he end of the string if it does not have one
string append_slash(const string &);
//! Creates a 'next' available filename into the base part based on given wild card.
bool generate_next_base(path &target, const char *wild);
#if defined(__linux) || defined(__APPLE__)
//! Maps the mode from numeric hex to linux symbolic
mode_t hex2mode(int);
//! Maps the mode from linux symbolic into numeric hex
int mode2hex(mode_t);
//! Reads the current file mode from named file.
int get_path_mode(const char *pname);
//! Sets owner and mode recursively to entire subtree. Use with care. (Linux&Apple only)
void set_owner_mode(const char *dirname, int userid, int groupid, int dirmode, int filemode);
#endif

bool has_anybits(uint32_t target, uint32_t bits);
bool has_allbits(uint32_t target, uint32_t bits);

typedef unsigned int flag32;

class flags32_base
{
public:
    flags32_base(flag32 _value) : value(_value) { }
    flags32_base() : value(0) { }

    flag32 get() { return value; }
    bool has_any(flag32 bits) { return (value&bits)>0 ? true : false; }
    bool has_all(flag32 bits) { return (value&bits)==bits ? true : false; }
    void set(flag32 bits) { value = bits; }
    void clear(flag32 bits) { value &= ~bits; }

    void add(flag32 bits) { value |= bits; }
    flag32& operator|=(flag32 bits) { value|=bits; return value; }

protected:
    flag32 value;
};

} // namespace c4s
#endif
