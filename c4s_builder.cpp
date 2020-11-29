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
#include <cstring>
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
c4s::builder::builder(path_list *_sources, const char *_name, ostream *_log, const BUILD &flags)
/** Base builder constructor. Handles the debug/release and 32/64 architecture selection.
   \param _sources List of source files to compile.
   \param _name Name of the target binary
   \param _log If specified, will receive compiler output.
   \param _flags Combination of build flags
*/
        : BUILD(flags), log(_log), name(_name)
{
    sources = _sources;
    my_sources = false;
    build_dir = has_any(BUILD::DEBUG)?"debug":"release";
    if(log) {
        compiler.pipe_to(log);
        linker.pipe_to(log);
    }
    if(log && has_any(BUILD::VERBOSE))
        *log << "builder::builder - output dir set to: "<<build_dir<<'\n';
    timeout = 15;
}
// ==================================================================================================
c4s::builder::builder(const char *_name, ostream *_log)
/** Base builder constructor.
    Files are automatically read from git filelist. Files with cpp-extension are considered part of
    of the project.
   \param _name Name of the target binary
   \param _log If specified, will receive compiler output.
*/
        : log(_log), name(_name)
{
    if(log) {
        compiler.pipe_to(log);
        linker.pipe_to(log);
    }
    timeout = 20;
    try {
        sources = new path_list();
        my_sources = true;
        char gitline[128];
        stringstream gitfiles;
        process("git","ls-files", &gitfiles)(20);
        do {
            gitfiles.getline(gitline, sizeof(gitline));
            // cout<<gitline<<'\n';
            if(strstr(gitline, ".cpp"))
                sources->add(path(gitline));
        } while(!gitfiles.eof() && gitfiles.gcount()>0);
    }
    catch(const c4s_exception &ce) {
        if(_log)
            *_log<<"Unable to read source list from git: "<<ce.what()<<'\n';
    }
}
// ------------------------------------------------------------------------------------------
builder::~builder()
{
    if(my_sources && sources)
        delete sources;
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
    if(log && has_any(BUILD::VERBOSE))
        *log << "builder - including variables from: "<<inc_path.get_path()<<'\n';
    vars.include(inc_path);
}
// ==================================================================================================
void c4s::builder::add_comp(const string &arg)
{
    if(arg.size())
        c_opts<<vars.expand(arg, true)<<' ';
}
// ------------------------------------------------------------------------------------------
void c4s::builder::add_link(const string &arg)
{
    if(arg.size())
        l_opts<<vars.expand(arg, true)<<' ';
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
        if(log && has_any(BUILD::VERBOSE))
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
                if(log && has_any(BUILD::VERBOSE))
                    *log << "  "<<options.str() <<'\n';
                compiler.start(options.str().c_str());
                exec=true;
            }
        }
        if(compiler.is_running()) {
            return compiler.wait_for_exit(timeout);
        }
        if(!exec && log && has_any(BUILD::VERBOSE))
            *log << "No outdated source files found.\n";
    }
    catch(const c4s_exception &ex){
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
        if(log && has_any(BUILD::VERBOSE))
            *log << "Linking "<<target<<'\n';
        if(has_any(BUILD::LIB))
            options<<' '<<vars.expand(l_opts.str())<<' ';
        if(out_arg)
            options << out_arg;
        options <<build_dir<<C4S_DSEP<<target<<' ';
        if(has_any(BUILD::RESPFILE)) {
            string respname(name+".resp");
            if(log && has_any(BUILD::VERBOSE))
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
        if(!has_any(BUILD::LIB))
            options<<' '<<vars.expand(l_opts.str());
        if(log && has_any(BUILD::VERBOSE))
            *log << "Link options: "<<options.str() <<'\n';
        rv = linker.exec(3*timeout,options.str().c_str());
    }
    catch(const c4s_exception &ce) {
        if(log)
            *log << "builder::link - Error: "<<ce.what()<<'\n';
        return 2;
    }
#ifndef _DEBUG
    if(has_any(BUILD::RESPFILE))
        path("cpp4scripts_link.resp").rm();
#endif
    return rv;
}
// ------------------------------------------------------------------------------------------
void c4s::builder::print(ostream &os, bool list_sources)
/**
   \param os Reference to output stream.
   \param list_sources If true, lists source files as well.
 */
{
    using namespace std;
    os << "  COMPILER options: "<<c_opts.str()<<'\n';
    if(has_any(BUILD::LIB))
        os << "  LIB options: "<<l_opts.str()<<'\n';
    else
        os << "  LINK options: "<<l_opts.str()<<'\n';

    if(list_sources) {
        os << "  Source files:\n";
        list<path>::iterator src;
        for(src=sources->begin(); src!=sources->end(); src++) {
            os << "    "<<src->get_base() <<'\n';
        }
    }
}
// ------------------------------------------------------------------------------------------
int c4s::builder::update_build_no(const char *filename)
/** Function opens the named file and increments the last number seen in the file. No special
    tags are needed. File is expected to be very short, just a variable declaration with version
    string.
    \param filename Relative path to the version file.
 */
{
    char *vbuffer, *tail, *head, *dummy, bno_str[24];
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
