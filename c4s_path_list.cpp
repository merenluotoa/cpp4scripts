/*******************************************************************************
c4s_path_list.cpp
Class path_list implementation.

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

Copyright (c) Menacon Ltd, Finland
*******************************************************************************/
#include <regex>
#ifdef C4S_LIB_BUILD
 #include <string.h>
 #if defined(__linux) || defined(__APPLE__)
  #include <dirent.h>
  #include <sys/stat.h>
 #endif
 #include "c4s_config.hpp"
 #include "c4s_exception.hpp"
 #include "c4s_user.hpp"
 #include "c4s_path.hpp"
 #include "c4s_path_list.hpp"
 #include "c4s_util.hpp"
 using namespace c4s;
#endif

// ==================================================================================================
size_t c4s::path_list::add(const path_list &source, const string &directory, const char *ext)
/*! Only the base names are used from the source list.  All files in this list will have the
  same directory given as the given 'directory' parameter, and the same extension also given as a
  parameter. Function can be used to transform list of source files to the list of object
  files.

   \param source List of source files
   \param directory Directory for all files in the list.
   \param ext Extension for all files in the list. Optional, defaults to null. If not specified source
   file's extension is used.
   \retval size_t Number of paths added.
*/
{
    size_t count = plist.size();
    list<path>::const_iterator pi;
    for(pi=source.plist.begin(); pi!=source.plist.end(); pi++)
        plist.push_back(path(directory,pi->get_base(ext)));
    return plist.size()-count;
}

// ==================================================================================================
size_t c4s::path_list::add(const char *str, const char separator)
/*!  Default separator is C4S_PSEP. Newline \n is always taken as a separator in addition to the given one.
  \param str Pointer to the string with list of filenames
  \param separator Path separation character
  \retval size_t Number of paths added.
*/
{
    const char *point = str;
    const char *prev = str;
    size_t count = plist.size();
    while(*point)
    {
        if(*point==separator || *point=='\n')
        {
            plist.push_back(path(string(prev,point-prev)));
            point++;
            while (*point && (*point==separator || *point=='\n')) {
                point++;
            }
            if(!*point)
                return plist.size()-count;
            prev = point;
        }
        point++;
    }
    if(point-prev > 1)
        plist.push_back(path(string(prev)));
    return plist.size()-count;
}

// ==================================================================================================
size_t c4s::path_list::add(const path_list &source)
/*! \param source Source path list
    \retval size_t Number of itmes added.
*/
{
    for(list<path>::const_iterator pi=source.plist.begin(); pi!=source.plist.end(); pi++) {
        plist.push_back(*pi);
    }
    return source.plist.size();
}

// ==================================================================================================
size_t c4s::path_list::add(const path &target, const string &grep, int plf, const string &exex)
/*!
  \param target Path to the target directory. Only dir-part is considered.
  \param greap Grep regular expression to match included files. If null includes all files.
  \param plf Combination of add options. See add-constants.
  \param exex Regular expression of files to exclude.
  \returns size_t number of files that have been added.
*/
{
    regex grep_rx, exclude_rx;
    string fname;
    size_t original_size = plist.size();
    bool include;

#ifdef C4S_DEBUGTRACE
    cout << "DEBUG - path_list::add target:"<<target.get_path()<<"; grep:"<<grep;
    cout << "; options:0x"<<hex<<plf<<dec<<"; exex:"<<exex<<'\n';
#endif
#if defined(__linux) || defined(__APPLE__)
    // Open and read the directory
    string dir;
    if(target.get_dir().empty())
        dir = "./";
    else
        dir = target.get_dir();
    DIR *source_dir = opendir(dir.c_str());
    if(!source_dir)
    {
        ostringstream os;
        os << "path_list::add - Unable to access directory: "<<dir<<'\n'<<strerror(errno);
        throw runtime_error(os.str());
    }
    // Initialize regular expressions
    if(!grep.empty())
        grep_rx.assign(grep, regex_constants::grep);
    if(!exex.empty())
        exclude_rx.assign(exex, regex_constants::grep);
    // Apply regular expressions to each file in give directory
    struct stat file_stat;
    struct dirent *de = readdir(source_dir);
    while(de) {
        include = false;
        if(!grep.empty()) {
            if(regex_search(de->d_name, grep_rx)) {
                if(exex.empty() || !regex_search(de->d_name, exclude_rx))
                    include = true;
            }
        }
        else
            include = true;
        if(include) {
            // Check the file/name type.
            fname = target.get_dir();
            fname += de->d_name;
            if(!lstat(fname.c_str(), &file_stat)) {
                if( (plf & PLF_NOREG)==0 && S_ISREG(file_stat.st_mode) )
                    plist.push_back( path(target.get_dir(), string(de->d_name)) );
                else if( (plf & PLF_SYML)>0 && S_ISLNK(file_stat.st_mode) )
                    plist.push_back( path(target.get_dir(), string(de->d_name)) );
                else if( (plf & PLF_DIRS)>0 && S_ISDIR(file_stat.st_mode) && de->d_name[0]!='.' ) {
                    string dirname = target.get_dir();
                    dirname += de->d_name;
                    dirname += "/";
                    plist.push_back(path(dirname));
                }
            }
        }
        de = readdir(source_dir);
    }
#else
#error TODO: start using regular expressions as in Linux.
    WIN32_FIND_DATA data;
    HANDLE find;
    BOOL findNext = TRUE;
    string searchDir;
    if(!wild) {
        searchDir = target.get_dir_plain();
        if(searchDir[0]=='\\' && searchDir[1]=='\\')
            searchDir += "\\*";
    } else {
        searchDir = target.get_dir();
        searchDir += wild;
    }
#ifdef C4S_DEBUGTRACE
    cout << "DEBUG - path_list::add searching:"<<searchDir<<endl;
#endif
    find = FindFirstFile(searchDir.c_str(),&data);
    if(find==INVALID_HANDLE_VALUE)
    {
        DWORD gle = GetLastError();
        if(gle == ERROR_FILE_NOT_FOUND)
            return 0;
        ostringstream os;
        os << "path_list::add - file find failure from:"<<searchDir<<". Error "<<gle<<'\n'<<strerror(GetLastError());
        throw path_exception(os.str());
    }
    while(findNext)
    {
#ifdef C4S_DEBUGTRACE
        cout << "DEBUG - path_list::add found :"<<data.cFileName<<"; attribute:"<<hex<<data.dwFileAttributes<<dec<<endl;
#endif
        for(exi=exlst.begin(); exi!=exlst.end(); exi++) {
            if(match_wildcard(data.cFileName, exi->c_str()))
                break;
        }
        if(!exclude || exi==exlst.end()) {
            fname = target.get_dir();
            fname += data.cFileName;
            // |FILE_ATTRIBUTE_VIRTUAL - works only in Vista and above?
            if( (data.dwFileAttributes&(FILE_ATTRIBUTE_NORMAL|FILE_ATTRIBUTE_ARCHIVE))>0 &&
                (data.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_OFFLINE))==0 &&
                (plf&PLF_NOREG)==0
                )
                plist.push_back(path(fname));
            else if( (plf&PLF_DIRS)>0 &&
                     (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY &&
                     data.cFileName[0]!='.')
            {
                fname += '\\';
                plist.push_back(path(fname));
            }
        }
        findNext = FindNextFile(find,&data);
    }
    FindClose(find);
#endif
#ifdef C4S_DEBUGTRACE
    cout << "DEBUG - path_list::add found items:"<<plist.size()-original_size<<endl;
#endif
    return plist.size()-original_size;
}
// ==================================================================================================
size_t c4s::path_list::add_recursive(const path &p, const char *wild)
/*! Adds files recursively in recursive function.
    NOTE! Recurses only MAX_NESTING levels deep.
 */
{
    static int nesting_level=0;
    size_t count=0;
#ifdef C4S_DEBUGTRACE
    cout<<"DEBUG - path_list::add_recursive - level="<<nesting_level<<"; path="<<p.get_path()<<'\n';
#endif
    if(nesting_level>MAX_NESTING)
        return 0;
    // First add the target directory itself
    add(p, wild);
    // Search subdirs
    path_list tmp(p, 0, PLF_DIRS|PLF_NOREG);
    for(path_iterator pi=tmp.begin(); pi!=tmp.end(); pi++) {
        nesting_level++;
        // Call recursively
        count += add_recursive(*pi, wild);
        nesting_level--;
    }
    return count;
}
// ==================================================================================================
bool c4s::path_list::discard_matching(const string &tbase)
/*! Affects the list only, actual file is not removed from the disk.
   \retval bool True on succes, false on error.
*/
{
    for(list<path>::iterator pi=plist.begin(); pi!=plist.end(); pi++) {
        if( tbase == pi->base ) {
            plist.erase(pi);
            return true;
        }
    }
    return false;
}

// ==================================================================================================
int c4s::path_list::copy_to(const path &target, int flags)
/*! Targets base is ignored if it exists => path::ONAME flag is enforced. If path list
  contains directories and flags has RECURSIVE bit set then these directories are copied
  recursively. If the flag is missing then directories are disregarded.  In case of error this
  function will leave the copy partially done. Only existing files are copied.
  \param target path to target directory
  \param flags copy flags.
  \retval int Number of files copied.
*/
{
    int copy_count=0;
    list<path>::iterator pi;
    flags |= PCF_ONAME;
    for(pi = plist.begin(); pi!=plist.end(); pi++) {
        if(!pi->is_base()) {
            if( (flags & PCF_RECURSIVE)>0 ) {
                path tmp_target(target);
                tmp_target.append_last(*pi);
                copy_count += pi->copy_recursive(tmp_target, flags);
            }
        }
        else if(pi->exists())
            copy_count += pi->cp(target,flags);
    }
#ifdef C4S_DEBUGTRACE
    cout << "DEBUG - path_list::copy to "<<target.get_dir()<<"copied "<<copy_count<<" files.\n";
#endif
    return copy_count;
}

// ==================================================================================================
void c4s::path_list::chmod(int mod)
/*!  \param mod Mode to set. \see path::chmod
*/
{
    list<path>::iterator pi;
    for(pi = plist.begin(); pi!=plist.end(); pi++)
        pi->chmod(mod);
}

// ==================================================================================================
void c4s::path_list::set_dir(const string &dir)
/*! \param dir Directory to set.
*/
{
    list<path>::iterator pi;
    for(pi = plist.begin(); pi!=plist.end(); pi++)
        pi->set_dir(dir);
}
// ==================================================================================================
void c4s::path_list::set_ext(const string &ext)
/*! \param ext New extension to set. Should include the period before the extension as well.
*/
{
    list<path>::iterator pi;
    for(pi = plist.begin(); pi!=plist.end(); pi++)
        pi->set_ext(ext);
}
#if defined(__linux) || defined(__APPLE__)
// ==================================================================================================
void c4s::path_list::set_usermode(user *uptr, const int mode)
/*! \param uptr Pointer to the owner of the paths
    \param mode Mode that should be used for the paths.
*/
{
    list<path>::iterator pi;
    for(pi = plist.begin(); pi!=plist.end(); pi++)
        pi->set(uptr,mode);
}
#endif
// ==================================================================================================
void c4s::path_list::rm_all()
/*!  If there are plain directories (i.e. no base defined) then the directory is removed recursively. USE WITH CARE!!!
*/
{
    for(list<path>::iterator pi=plist.begin(); pi!=plist.end(); pi++)
    {
        if(pi->base.empty())
            pi->rmdir(true);
        else if(!pi->exists())
            continue;
        else if(!pi->rm())
        {
            ostringstream os;
            os << "path_list::remove_all - unable to delete " << pi->get_path();
            throw path_exception(os.str());
        }
    }
}

// ==================================================================================================
void c4s::path_list::create_targets(path_list &target, const string &dir, const char *ext)
/*! This function supposes that this object is a list of source files and makes corresponding
  target paths (for a compiler) by taking the given dir, base name from this list and appending a given
  extension. E.g. if this list has files with .cpp files and dir is './release' and ext is '.o' for each
  cpp file in the this list a path with given dir, base name of cpp and .o extenstion is appended
  to target list.
 */
{
    list<path>::iterator pi;
    for(pi=plist.begin(); pi!=plist.end(); pi++)
        target += path(dir,pi->get_base(ext));
}

// ==================================================================================================
string c4s::path_list::str(const char separator, bool baseonly)
/*! \param separator Separator for paths
   \param baseonly If true (default) returns the base names only.
   \retval string a list of paths.
*/
{
    string bunch;
    list<path>::iterator pi=plist.begin();
    if(pi==plist.end())
        return string("");
    bunch = baseonly ? pi->get_base_or_dir() : pi->get_path();
    pi++;
    while(pi!=plist.end())
    {
        bunch += separator;
        bunch += baseonly ? pi->get_base_or_dir() : pi->get_path();
        pi++;
    }
    return bunch;
}
// ==================================================================================================
bool compare_bases(c4s::path &fp, c4s::path &sp)
{
  return fp.compare(sp, CMP_BASE)<0 ? true : false;
}
bool compare_full(c4s::path &fp, c4s::path &sp)
{
    return fp.compare(sp, CMP_DIR|CMP_BASE)<0 ? true : false;
}
void c4s::path_list::sort(SORTTYPE st)
{
    if(st == ST_FULL) plist.sort(::compare_full);
    else plist.sort(::compare_bases);
}

// ==================================================================================================
void c4s::path_list::dump(ostream &out)
{
    list<path>::iterator pi;
    for(pi=plist.begin(); pi!=plist.end(); pi++)
        pi->dump(out);
}
