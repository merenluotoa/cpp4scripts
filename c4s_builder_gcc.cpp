/*! \file c4s_builder_gcc.cpp
 *  \brief GCC builder source */
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

Copyright (c) Menacon Oy, Finland
*******************************************************************************/

#ifdef C4S_LIB_BUILD
 #include "c4s_config.hpp"
 #include "c4s_exception.hpp"
 #include "c4s_variables.hpp"
 #include "c4s_path.hpp"
 #include "c4s_path_list.hpp"
 #include "c4s_process.hpp"
 #include "c4s_compiled_file.hpp"
 #include "c4s_util.hpp"
 #include "c4s_builder.hpp"
 #include "c4s_builder_gcc.hpp"
 using namespace c4s;
#endif

c4s::builder_gcc::builder_gcc(path_list *_sources, const char *_name, ostream *_log, const BUILD &build_flags)
    : builder(_sources,_name,_log, build_flags)
{
    parse_flags();
    late_flags = false;
}
// ------------------------------------------------------------------------------------------
c4s::builder_gcc::builder_gcc(const char *_name, ostream *_log)
    : builder(_name, _log)
{
    late_flags = true;
}
// ------------------------------------------------------------------------------------------
void c4s::builder_gcc::parse_flags()
{
    // Determine the compiler name
    string gpp,link;
    gpp = has_any(BUILD::PLAIN_C)?"gcc":"g++";
    link = has_any(BUILD::PLAIN_C)?"gcc":"g++";
    compiler.set_command(gpp.c_str());

    // Determine the real target name.
    if(has_any(BUILD::LIB)) {
        linker.set_command("ar");
        target = "lib";
        target += name;
        target += ".a";
        l_opts << "-rcs ";
    }
    else {
        target = name;
        linker.set_command(link.c_str());
        if(has_any(BUILD::SO)) {
            c_opts << "-fpic ";
            l_opts << "-shared -fpic ";
            target += ".so";
        }
        l_opts << "-fexceptions -pthread ";
    }

    if(build_dir.size()==0) {
        build_dir = has_any(BUILD::DEBUG)?"debug":"release";
        if(log && has_any(BUILD::VERBOSE))
            *log << "builder::builder - output dir set to: "<<build_dir<<'\n';
    }

    if(!has_any(BUILD::NODEFARGS)) {
#if __GNUC__ >= 5
        c_opts << "-Wall -fexceptions -pthread -fuse-cxa-atexit -Wundef -Wno-unused-result -std=c++14 ";
#elif defined(__APPLE__) && __clang_major__>=5
        c_opts << "-Wall -fexceptions -pthread -fuse-cxa-atexit -Wundef -Wno-unused-result -std=c++17 ";
#else
        c_opts << "-Wall -fexceptions -pthread -fuse-cxa-atexit -Wundef -Wno-unused-result ";
#endif
    }

    if(has_any(BUILD::DEBUG)) {
        c_opts << "-ggdb -O0 -D_DEBUG ";
        if(!has_any(BUILD::LIB)) {
            l_opts << "-ggdb -O0 ";
        }
    }
    else {
        c_opts << "-O2 ";
        if(!has_any(BUILD::LIB)) {
            l_opts << "-O2 ";
        }
    }
    if(has_any(BUILD::WIDECH))
        c_opts << "-D_UNICODE -DUNICODE ";
    if(sources && sources->size()>1)
        c_opts << "-c ";
}
// ==================================================================================================
int c4s::builder_gcc::build()
{
    if(!sources)
        throw c4s_exception("builder_gcc::build - no sources to build.");
    if(late_flags)
        parse_flags();

    // Only one file?
    if(sources->size()==1) {
        // Combine compile and link.
        ostringstream single;
        path src = sources->front();
        single << c_opts.str();
        single << "-o " << target << ' ' << src.get_base()<<' ';
        single << l_opts.str();
        try {
            if(log && has_any(BUILD::VERBOSE)) {
                *log << "Compiling "<<src.get_base()<<'\n';
                *log << "Compile parameters: "<<single.str()<<'\n';
            }
            return compiler.exec(20,vars.expand(single.str()).c_str());
        }catch (const c4s_exception &ce) {
            if(log)
                *log << "builder_gcc::build - Failed:"<<ce.what()<<'\n';
        }
        return 3;
    }
    // Make sure build dir exists
    path buildp(build_dir+C4S_DSEP);
    if(!buildp.dirname_exists()) {
        if(log && has_any(BUILD::VERBOSE) )
            *log << "builder_gcc - created build directory:"<<buildp.get_path()<<'\n';
        buildp.mkdir();
    }
    // Call parent to do the job
    if(log && has_any(BUILD::VERBOSE) )
        builder::print(*log);
    int rv = builder::compile(".o","-o ",true);
    if(!rv) {
        if(has_any(BUILD::LIB))
            rv = builder::link(".o",0);
        else
            rv = builder::link(".o","-o ");
        if(!rv) return 0;
    }
    if(log && has_any(BUILD::VERBOSE) )
        *log << "builder_gcc::build - Mapping return value "<<rv<<" to 4\n";
    return 4;
}

