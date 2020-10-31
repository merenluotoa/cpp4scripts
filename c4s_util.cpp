/*! \file c4s_util.cpp
 * \brief Implementation of misc. utility functions for Cpp4Scripts library.*/
// Copyright (c) Menacon Oy
/****************************************************************************************************
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
****************************************************************************************************/
#ifdef C4S_LIB_BUILD
 #include <time.h>
 #include <stdlib.h>
 #include <stdio.h>
 #include <string.h>
 #if defined(__linux) || defined(__APPLE__)
  #include <poll.h>
  #include <pwd.h>
  #include <grp.h>
  #include <dirent.h>
  #include <sys/stat.h>
 #endif
 #ifdef _WIN32
  #include <direct.h>
  #include <windows.h>
 #endif
 #include "c4s_config.hpp"
 #include "c4s_user.hpp"
 #include "c4s_path.hpp"
 #include "c4s_exception.hpp"
 #include "c4s_util.hpp"
 #include "c4s_path_list.hpp"
 using namespace c4s;
#endif

// ==================================================================================================
bool c4s::exists_in_path(const char *fname)
/*!
  Checks if the given base name exists in the PATH.
  \param fname Name of the file.
  \retval bool True on succes, false on error.
*/
{
#if defined(__linux) || defined(__APPLE__)
    path target(fname);
    return target.exists_in_env_path("PATH");
#else
    char foundPath[255],**fnamePtr=0;
    DWORD rv = SearchPath(0,fname,0,sizeof(foundPath),foundPath,fnamePtr);
    return rv==0 ? false : true;
#endif
}

// ==================================================================================================
string c4s::force_native_dsep(const string &source)
/*! Windows \ and Linux /. Please note that the native type is evaluated at the compile time
  by using preprocessor macros defined by the compiler.
*/

{
    string target(source);
    size_t pos=0;
    //cout << "DEBUG - force native:"<<C4S_DSEP<<'\n';
    if(C4S_DSEP == '/') // linux native mode
    {
        while( (pos = target.find('\\',pos)) != string::npos)
            target[pos++] = C4S_DSEP;
    }else{
        while( (pos = target.find('/',pos)) != string::npos)
            target[pos++] = C4S_DSEP;
    }
    return target;
}

// ==================================================================================================
bool c4s::get_env_var(const char *name, string &value)
/*! An utility function that hides OS specific implementations. Given string buffer will be
  emptied if the value is not found.

  \param name Name of the value to get.
  \param value String buffer for the value.
  \retval bool True if value was found, false if not.
*/
{
#if defined(__linux) || defined(__APPLE__)
    char *envstr = getenv(name);
    if(!envstr)
#else
    char envstr[8096];
    if(!GetEnvironmentVariable(name,envstr,sizeof(envstr)))
#endif
    {
        value.clear();
        return false;
    }
    value = envstr;
    return true;
}

// ==================================================================================================
bool c4s::match_wildcard(const char *target, const char *wild)
/*! Function is copied from Dr.Dobb's journal.

  \param target Target string (filename) to test for the wild card match.
  \param wild A String with wildchars (*,?)
  \retval bool True if string matches the wild card, false if it does not.
*/
{
    int ndx;
    const char *star;
 new_segment:
    star = 0;
    while (*wild == '*') {
        star = wild;
        wild++;
    }
 test_match:
    for (ndx = 0; wild[ndx] && (wild[ndx] != '*'); ndx++) {
        if (wild[ndx] != target[ndx]) {
            if (! target[ndx])
                return false;
            if (wild[ndx] == '?')
                continue;
            if (! star)
                return false;
            target++;
            goto test_match;
        }
    }
    if (wild[ndx] == '*') {
        target += ndx;
        wild += ndx;
        goto new_segment;
    }
    if (! target[ndx])
        return true;
    if (ndx && wild[ndx-1] == '*')
        return true;
    if (! star)
        return false;
    target++;
    goto test_match;
}

// ==================================================================================================
string c4s::get_user_name(bool loginname)
/*!
  \param loginname if true then login name is returned, otherwice the real name.
  \returns string Requested name. Empty on error.
*/
{
    string name;
#if defined(__linux) || defined(__APPLE__)
    struct passwd *pwd = getpwuid (getuid());
    if(!pwd)
        return string("");
    name = loginname ? pwd->pw_name : pwd->pw_gecos;
#else
    char nameBuf[128];
    DWORD nsize = 128;
    if(!GetUserName(nameBuf,&nsize))
        return string("");
    name = nameBuf;
#endif
    return name;
}

#ifdef _WIN32
// ==================================================================================================
const char* c4s::strerror(int error)
/*!
  \param error ...
  \retval const char * Pointer to error description. Empty string if not found.
*/
{
    static char buffer[2048];
    static const char *none = "[No error description]";
    if(FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,0,error,0,buffer,sizeof(buffer),0))
        return buffer;
    return none;
}
#endif

// ==================================================================================================
bool c4s::search_bmh(const unsigned char* haystack, SSIZE_T hlen,
                     const unsigned char* needle,   SSIZE_T nlen,
                     SIZE_T *offset_out)
/*! See: http://en.wikipedia.org/wiki/Boyer-Moore-Horspool_algorithm
  \param haystack Pointer to buffer to be searched
  \param hlen Lenght of the haystack buffer.
  \param needle Pointer to the sring to be searched.
  \param nlen Lenth of the search string.
  \param offset_out Variable is filled with offset to the beginning of the needle in haystack.
  \retval bool True if search is successful
 */
{
    const unsigned int MAX_BCS = 256;
    SIZE_T scan = 0;
    SIZE_T bad_char_skip[MAX_BCS]; /* Officially called: bad character shift */
    const unsigned char *original = haystack;

    /* Sanity checks on the parameters */
    if (nlen <= 0 || !haystack || !needle || hlen<nlen || !offset_out)
        return false;

    /* Initialize the table to default value */
    for (scan = 0; scan < MAX_BCS; scan++) {
        bad_char_skip[scan] = nlen;
    }
    SIZE_T last = nlen - 1;

    /* Then populate it with the analysis of the needle */
    for (scan = 0; scan < last; scan = scan + 1)
        bad_char_skip[needle[scan]] = last - scan;

    /* Search the haystack, while the needle can still be within it. */
    while (hlen >= nlen)
    {
        /* scan from the end of the needle */
        for (scan = last; haystack[scan] == needle[scan]; scan = scan - 1)
            if (scan == 0) { /* If the first byte matches, we've found it. */
                *offset_out = SIZE_T(haystack-original);
                return true;
            }
        /* otherwise, we need to skip some bytes and start again. */
        hlen     -= bad_char_skip[haystack[last]];
        haystack += bad_char_skip[haystack[last]];
    }
    return false;
}

// ==================================================================================================
bool c4s::search_file(fstream &target, const string &needle)
/*!  Uses Boyer-Moore algorithm to search for a text in a given stream. Stream needs to be
  opened before this function is called. Search begins from the current position. If match is found
  the file pointer is positioned to the start of the next needle.
  On error an exception is thrown.

  \param target Opened file stream to search for.
  \param needle String that should be found
  \retval bool True if needle was found, false if not.
*/
{
    const SIZE_T BMAX = 0x1000;
    char buffer[BMAX];
    streamsize tg;
    SIZE_T br, boffset, total_offset, overlap=0;
    SIZE_T nsize = needle.size();
    if(!target.good())
        throw c4s_exception("search_file: given stream does not have 'good' status.");
    if(nsize >= BMAX)
        throw c4s_exception("search_file: size of search text exceeds internal read buffer size.");
    tg = target.tellg();
    if(tg<0)
        throw c4s_exception("search_file: unable to get file position information.");
    total_offset = SIZE_T(tg);
    do {
        target.read(buffer+overlap,BMAX-overlap);
        br = SIZE_T(target.gcount());
        if(search_bmh((unsigned char*)buffer, br+overlap, (unsigned char*)needle.c_str(), nsize, &boffset)) {
            target.clear();
            target.seekg(total_offset+boffset,ios_base::beg);
            return true;
        }
        total_offset += br;
        memcpy(buffer, buffer+BMAX-nsize, nsize);
        if(!overlap) {
            overlap = nsize;
            total_offset -= nsize;
        }
    }while(!target.eof());
    return false;
}
// ------------------------------------------------------------------------------------------
const char* c4s::get_ISO_date(DATETYPE dt)
{
    static char datestr[20];
    time_t now=time(0);
    if(dt==DATE_ONLY)
        strftime(datestr,sizeof(datestr),"%Y-%m-%d",localtime(&now));
    else
        strftime(datestr,sizeof(datestr),"%Y-%m-%d %H:%M:%S",localtime(&now));
    return datestr;
}
// ------------------------------------------------------------------------------------------
const char* c4s::get_build_type()
{
#ifdef _DEBUG

#if defined(_WIN64) || defined(__x86_64)
    return "64bit-Debug";
#elif defined(_WIN32) || defined(__x86_32)
    return "32bit-Debug";
#else
    return "[Undetermined]";
#endif

    // ..................................................
#else

#if defined(_WIN64) || defined(__x86_64)
    return "64bit-Release";
#elif defined(_WIN32) || defined(__x86_32)
    return "32bit-Release";
#else
    return "[Undetermined]";
#endif

#endif
}

// ------------------------------------------------------------------------------------------
const char* c4s::get_host_name()
{
    static char nbuf[128];
#if defined(__linux) || defined(__APPLE__)
    gethostname(nbuf,sizeof(nbuf));
#else
    DWORD bs = sizeof(nbuf);
    if(!GetComputerNameEx(ComputerNameDnsHostname,nbuf,&bs))
        return "[Undetermined]";
#endif
    return nbuf;
}

// ------------------------------------------------------------------------------------------
bool c4s::wait_stdin(int timeout)
/*! Performs non-blocking wait for the input on stdin.
   \param timeout Number of milliseconds to wait for.
   \retval bool True on input within timeout period. False if no input.
*/
{
#if defined(__linux) || defined(__APPLE__)
    struct pollfd pf;
    pf.fd = 0;
    pf.events = POLLIN|POLLRDNORM;
    if(poll(&pf,1,timeout)>0)
        return true;
    return false;
#else
    if(WaitForSingleObject(GetStdHandle(STD_INPUT_HANDLE),timeout) == WAIT_OBJECT_0)
        return true;
    return false;
#endif
}

// ==================================================================================================
string c4s::append_slash(const string &src)
/*!
   \param src Source string
   \retval string New string with / appended.
*/
{
    string work(src);
    string::reverse_iterator rit = work.rbegin();
    if(*rit != C4S_DSEP)
        work += C4S_DSEP;
    return work;
}
// ==================================================================================================
bool c4s::generate_next_base(path &target, const char *wild)
/*! Searches the directory (or cwd if dir part is empty) with given wild card. Then determines the next
  possible unique filename and stores it to the base part. Please note that only one wild card '*' is
  accepted.
  \param target Target directory to search
  \param wild A search string.
  \retval bool True on succes, false on error or too many wild cards.
*/
{
    const char *pos;
    char next[128], *tail, *ptr, *tail_copy;

    if(!strchr(wild,'*'))
        return false;
    // Search files based on wild
    path_list bases(target,wild);
    if(bases.size()==0) {
        string base = target.get_base();
        if(base.empty())
            target.set_base("fil_001");
        base=wild;
        size_t ap = base.find('*');
        if(ap==string::npos)
            return false;
        base.replace(ap,1,"001");
        target.set_base(base);
        return true;
    }
    // Get the last one
    bases.sort(path_list::ST_PARTIAL);
    path last = bases.back();
    strcpy(next, last.get_base().c_str());
    ptr = next;
    // Find the 'wild' part
    for(pos=wild; *pos && *pos!='*'; pos++)
        ptr++;
    tail = strstr(ptr,pos+1);
    if(!tail)
        tail = next + strlen(next) ;
    tail_copy = tail;
    tail--;
    // Start calculating from the back.
    while(tail>=ptr) {
        (*tail)++;
        if(*tail==0x3A) {
            *tail=0x30;
            tail--;
        }
        else if(*tail==0x5B) {
            *tail=0x41;
            tail--;
        }
        else if(*tail==0x7B) {
            *tail=0x61;
            tail--;
        }
        else
            break;
    }
    if(tail<ptr) {
        memcpy(tail_copy+1, tail_copy, tail_copy-next-1);
        *tail_copy = 0x30;
    }
    target.set_base(next);
    return true;
}

#if defined(__linux) || defined(__APPLE__)
// ==================================================================================================
mode_t c4s::hex2mode(int hex_in)
{
    mode_t final=0;
    if( (hex_in&0x400)>0 )
        final|= S_IRUSR;
    if( (hex_in&0x200)>0 )
        final|= S_IWUSR;
    if( (hex_in&0x100)>0 )
        final|= S_IXUSR;
    if( (hex_in&0x40)>0 )
        final|= S_IRGRP;
    if( (hex_in&0x20)>0 )
        final|= S_IWGRP;
    if( (hex_in&0x10)>0 )
        final|= S_IXGRP;
    if( (hex_in&0x4)>0 )
        final|= S_IROTH;
    if( (hex_in&0x2)>0 )
        final|= S_IWOTH;
    if( (hex_in&0x1)>0 )
        final|= S_IXOTH;
    return final;
}
// ------------------------------------------------------------------------------------------
int c4s::mode2hex(mode_t mode_in)
{
    int final=0;
    if( (mode_in & S_IRUSR)>0 )
        final|= 0x400;
    if( (mode_in & S_IWUSR)>0 )
        final|= 0x200;
    if( (mode_in & S_IXUSR)>0 )
        final|= 0x100;
    if( (mode_in & S_IRGRP)>0 )
        final|= 0x40;
    if( (mode_in & S_IWGRP)>0 )
        final|= 0x20;
    if( (mode_in & S_IXGRP)>0 )
        final|= 0x10;
    if( (mode_in & S_IROTH)>0 )
        final|= 0x4;
    if( (mode_in & S_IWOTH)>0 )
        final|= 0x2;
    if( (mode_in & S_IXOTH)>0 )
        final|= 0x1;
    return final;
}
// ==================================================================================================
int c4s::get_path_mode(const char *pname)
/// Reads current path mode from file system.
/**
   \param pname Full or relative path to directory or file.
   \retval int Negative on error, otherwise mode in hex.
 */
{
    struct stat file_stat;
    int rv = ::stat(pname, &file_stat);
    if(rv == 0)
        return mode2hex(file_stat.st_mode);
    return -1;
}

// ==================================================================================================
void c4s::set_owner_mode(const char *dirname, int userid, int groupid, int dirmode, int filemode)
{
    static int nesting=0;
    char fpath[255];

    DIR *source_dir = opendir(dirname);
    if(!source_dir) {
        nesting = 0;
        ostringstream os;
        os << "c4s::set_owner_mode - Unable to access directory: "<<dirname<<'\n'<<strerror(errno);
        throw c4s_exception(os.str().c_str());
    }
    size_t dn_len = strlen(dirname);
    if(dn_len >= sizeof(fpath)) {
        nesting = 0;
        throw c4s_exception("c4s::set_owner_mode - Max filepath buffer size exceeded.");
    }
    if(nesting >= 20) {
        nesting = 0;
        throw c4s_exception("c4s::set_owner_mode - Too high nesting of directories. ");
    }

    mode_t final_dmode = hex2mode(dirmode);
    mode_t final_fmode = hex2mode(filemode);
    struct stat file_stat;
    struct dirent *de = readdir(source_dir);
    //cout << "c4s::set_owner_mode - userid:"<<userid<<"; groupid:"<<groupid<<"; nesting:"<<nesting<<"; reading: "<<dirname<<'\n';
    while(de) {
        if(dn_len + strlen(de->d_name) >= sizeof(fpath)) {
            nesting = 0;
            throw c4s_exception("c4s::set_owner_mode - Max filepath buffer size exceeded.");
        }
        strcpy(fpath, dirname);
        strcat(fpath, de->d_name);
        if(!lstat(fpath, &file_stat)) {
            if(S_ISREG(file_stat.st_mode)) {
                if(userid && groupid) {
                    if(chown(fpath, userid, groupid)==-1)
                        goto SET_OWNER_MODE_FAIL;
                }
                if(chmod(fpath, final_fmode)==-1 )
                    goto SET_OWNER_MODE_FAIL;
            }
            else if(S_ISDIR(file_stat.st_mode) && de->d_name[0]!='.') {
                if(userid && groupid) {
                    if(chown(fpath, userid, groupid)==-1)
                        goto SET_OWNER_MODE_FAIL;
                }
                if(chmod(fpath, final_dmode)==-1 )
                    goto SET_OWNER_MODE_FAIL;
                nesting++;
                strcat(fpath,"/");
                set_owner_mode((const char*)fpath, userid, groupid, dirmode, filemode);
            }
        }
        de = readdir(source_dir);
    }
    if(nesting)
        nesting--;
    closedir(source_dir);
    return;

 SET_OWNER_MODE_FAIL:
    {
        nesting = 0;
        ostringstream os;
        os << "c4s::set_owner_mode - chown or chmode fails on path: "<<fpath<<';'<<strerror(errno);
        throw c4s_exception(os.str().c_str());
    }
}
#endif
