/*! \file c4s_builder.cpp
 * \brief Implementation of the builder-class. */
// Copyright (c) Menacon Oy
/*******************************************************************************
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
#include <string.h>
#include <stdlib.h>
#ifdef C4S_LIB_BUILD
 #include "c4s_config.hpp"
 #include "c4s_exception.hpp"
 #include "c4s_variables.hpp"
 #include "c4s_path.hpp"
 #include "c4s_path_list.hpp"
 #include "c4s_process.hpp"
 #include "c4s_util.hpp"
 #include "c4s_compiled_file.hpp"
 #include "c4s_builder.hpp"
 using namespace c4s;
#endif

// ==================================================================================================
c4s::builder::builder(path_list *_sources, const char *_name, ostream *_log, const int _flags, const char *subsys)
/** Base builder constructor. Handles the debug/release and 32/64 architecture selection.
   \param _sources List of source files to compile.
   \param _name Name of the target binary
   \param _log If specified, will receive compiler output.
   \param _flags Combination of build flags
   \param subsys Optional subsystem name.
*/
    : log(_log), name(_name), flags(_flags)
{
    sources = _sources;
    flags |= get_arch();
    if(is_set(BUILD_PAD_NAME)) {
        build_dir = "build";
        pad_name(build_dir, subsys, flags);
    }
    else
        build_dir = is_set(BUILD_DEBUG)?"debug":"release";

    if(log) {
        compiler.pipe_to(log);
        linker.pipe_to(log);
    }
    if(log && is_set(BUILD_VERBOSE))
        *log << "builder::builder - output dir set to: "<<build_dir<<'\n';
    timeout = 15;
}
// ==================================================================================================
void c4s::builder::include_variables(const char *filename)
/*! Build variables are read from a given file. If the file is not specified environment variable
    C4S_VARIABLES will be read and its value used as a path to variable file.
    \param filename Optional name of the variable file to be included.
 */
{
    string c4s_var_name;
    if(!filename) {
        if(!get_env_var("C4S_VARIABLES",c4s_var_name)) {
            throw c4s_exception("builder::include_variables - Unable to find 'C4S_VARIABLES' environment variable.");
        }
    }
    else {
        c4s_var_name = filename;
    }
    path inc_path(c4s_var_name);
    if(!inc_path.exists()) {
        ostringstream error;
        error << "builder::include_variables - C4S_VARIABLES file "<<c4s_var_name<<" does not exist.";
        throw c4s_exception(error.str());
    }
    if(log && is_set(BUILD_VERBOSE))
        *log << "builder - including variables from: "<<inc_path.get_path()<<'\n';
    vars.include(inc_path);
}
// ==================================================================================================
void c4s::builder::add_comp(const char *arg)
{
#ifdef _WIN32
    char var[MAX_LINE];
    strcpy(var,arg);
    variables::exp_arch(builder::get_arch(), var);
    c_opts<<var<<' ';
#else
    c_opts<<vars.expand(arg, true);
    c_opts<<' ';
#endif
}
// ------------------------------------------------------------------------------------------
void c4s::builder::add_link(const char *arg)
{
#ifdef _WIN32
    char var[MAX_LINE];
    strcpy(var,arg);
    variables::exp_arch(builder::get_arch(),var);
    l_opts<<var<<' ';
#else
    l_opts<<vars.expand(arg, true);
    l_opts<<' ';
#endif
}
// ------------------------------------------------------------------------------------------
void c4s::builder::add_link(const compiled_file &cf)
{
    extra_obj.add(cf.target);
}
// ==================================================================================================
int c4s::builder::compile(const char *out_ext, const char *out_arg, bool echo_name)
{
    list<path>::iterator src;
    ostringstream options;
    bool exec=false;

    if(!sources)
        throw c4s_exception("builder::compile - sources not defined!");

    string prepared(vars.expand(c_opts.str()));
    try{
        if(log && is_set(BUILD_VERBOSE))
            *log << "Considering "<<sources->size()<<" source files for build.\n";
        for(src=sources->begin(); src!=sources->end(); src++)
        {
            path objfile(build_dir+C4S_DSEP, src->get_base_plain(), out_ext);
            if(src->outdated(objfile,true)) {
                if(compiler.is_running() && compiler.wait_for_exit(timeout)) {
                    return compiler.last_return_value();
                }
                if(log && echo_name)
                    *log << src->get_base() << " >>\n";
                options.str("");
                options << prepared;
                options << ' ' << out_arg<<objfile.get_path();
                options << ' ' << src->get_path();
                if(log && is_set(BUILD_VERBOSE))
                    *log << "  "<<options.str() <<'\n';
                compiler.start(options.str().c_str());
                exec=true;
            }
        }
        if(compiler.is_running()) {
            return compiler.wait_for_exit(timeout);
        }
        if(!exec && log && is_set(BUILD_VERBOSE))
            *log << "No outdated source files found.\n";
    }
    catch(c4s_exception ex){
        if(log)
            *log << "builder::compile - "<<ex.what()<<'\n';
        return 2;
    }
    return compiler.last_return_value();
}
// ==================================================================================================
int c4s::builder::link(const char *out_ext, const char *out_arg)
{
    int rv;
    ostringstream options;
    if(!sources)
        throw c4s_exception("builder::link - sources not defined!");
    if(!out_ext)
        throw c4s_exception("builder::link - link file extenstion missing. Unable to link.");
    path_list linkFiles(*sources, build_dir+C4S_DSEP, out_ext);

    try {
        if(log && is_set(BUILD_VERBOSE))
            *log << "Linking "<<target<<'\n';
        if(is_set(BUILD_LIB))
            options<<' '<<vars.expand(l_opts.str())<<' ';
        if(out_arg)
            options << out_arg;
        options <<build_dir<<C4S_DSEP<<target<<' ';
        if(is_set(BUILD_RESPFILE)) {
            string respname(name+".resp");
            if(log && is_set(BUILD_VERBOSE))
                *log << "builder::link - using response file: "<<respname<<'\n';;
            ofstream respf(respname.c_str());
            if(!respf)
                throw c4s_exception("builder::link - Unable to open linker response file.\n");
            respf << linkFiles.str('\n',false);
            if(extra_obj.size())
                respf << extra_obj.str('\n',false);
            respf.close();
            options << '@'<<respname;
        }else{
            options << linkFiles.str(' ',false);
            if(extra_obj.size())
                options <<' '<<extra_obj.str(' ',false);
        }
        if(!is_set(BUILD_LIB))
            options<<' '<<vars.expand(l_opts.str());
        if(log && is_set(BUILD_VERBOSE))
            *log << "Link options: "<<options.str() <<'\n';
        rv = linker.exec(3*timeout,options.str().c_str());
    }
    catch(c4s_exception ce) {
        if(log)
            *log << "builder::link - Error: "<<ce.what()<<'\n';
        return 2;
    }
    //if(is_set(BUILD_RESPFILE))
    //    path("cpp4scripts_link.resp").rm();
    return rv;
}
// ------------------------------------------------------------------------------------------
void c4s::builder::print(ostream &os)
{
    os << "  COMPILER options: "<<c_opts.str()<<'\n';
    if(is_set(BUILD_LIB))
        os << "  LIB options: "<<l_opts.str()<<'\n';
    else
        os << "  LINK options: "<<l_opts.str()<<'\n';
}
// ------------------------------------------------------------------------------------------
int c4s::builder::get_arch()
{
#if defined(_WIN32)
    string arch;
    if(get_env_var("Platform",arch) && !arch.compare("X64"))
        return BUILD_X64;
    return BUILD_X32;
#elif defined(__APPLE__)
    return BUILD_X64;
#else
    string arch;
    try {
        process::catch_output("uname","-m",arch);
    }catch(c4s_exception){
        return BUILD_X32;
    }
    return arch.compare(0,6,"x86_64")==0 ? BUILD_X64:BUILD_X32;
#endif
}
// ------------------------------------------------------------------------------------------
void c4s::builder::pad_name(string &name, const char *subsys, int fval)
{
#if defined(__APPLE__)
    string pad("-osx");
#elif defined(__linux)
    string pad("-lnx");
#else
    string pad("-win");
#endif
    if( (fval&BUILD_DEBUG)==BUILD_DEBUG ) pad+="d";
    else pad+="r";
#if defined(__linux) || defined(_WIN32)
    if( (fval&BUILD_X32)==BUILD_X32 ) pad+="32";
    else pad+="64";
#endif
    if(subsys)
        pad += subsys;
    size_t pos = name.rfind('.');
    if(pos == string::npos)
        name.append(pad);
    else
        name.insert(pos,pad);
}
// ------------------------------------------------------------------------------------------
void c4s::builder::clean_build_dir()
/*! removes the build directory designated by the given flags. Call repeatedly to delete
  more than one directory.
  \param flags Combination of flags that was used to create binary.
 */
{
    path bd;
    bd.set_dir(build_dir);
    if(bd.dirname_exists()) {
        cout << "builder - removing: "<<bd.get_dir()<<'\n';
        bd.rmdir(true);
    }
}
// ------------------------------------------------------------------------------------------
int c4s::builder::update_build_no(const char *filename)
{
    char *vbuffer, *tail, *head, *dummy, bno_str[10];
    ifstream fbn(filename);
    if(!fbn) {
        // cout << "builder::update_build_no - Unable to open given file for reading.\n";
        return -1;
    }
    fbn.seekg(0, ios_base::end);
    size_t max = (size_t) fbn.tellg();
    if(max>255) {
        // cout << "builder::update_build_no - Build number file is too large to handle.\n";
        fbn.close();
        return -2;
    }
    vbuffer = new char[max];
    fbn.seekg(0, ios_base::beg);
    fbn.read(vbuffer,max);
    tail = vbuffer + max - 1;
    fbn.close();

    // Search the last number.
    while(tail>vbuffer) {
        if(*tail>='0' && *tail<='9')
            break;
        tail--;
    }
    if(tail == vbuffer) {
        // cout << "builder::update_build_no - Number not found from build number file.\n";
        delete[] vbuffer;
        fbn.close();
        return -3;
    }
    head = tail++;
    while(head>vbuffer) {
        if(*head<'0' || *head>'9')
            break;
        head--;
    }
    if(head>vbuffer)
        head++;
    // Get and update number.
    long bno = strtol(head,&dummy,10);
    int bno_len = sprintf(bno_str,"%ld",bno+1);

    // Rewrite the number file.
    ofstream obn(filename, ios_base::out|ios_base::trunc);
    if(head > vbuffer)
        obn.write(vbuffer, head-vbuffer);
    obn.write(bno_str,bno_len);
    obn.write(tail, max-(tail-vbuffer));
    obn.close();
    delete[] vbuffer;
    return 0;
}
