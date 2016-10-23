/*! \file makec4s.cpp
 * \brief Cpp4Scripts make utility

This is a handly utility that simplifies the compilation of other Cpp4Script programs.
This is built by Cpp4Script builder. Please build parameters from builder.cpp.

With -s parameter you may specify other single source Cpp4Script files for compilation.
These executables will be placed into your current directory.

Version 0.14
- Allow script files to be named with c4s-extension.

*/
// Copyright (c) Menacon Oy
/*****************************************************************************************
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
******************************************************************************************/
#include "cpp4scripts.hpp"
#include "c4s-version.cpp"
using namespace c4s;

program_arguments args;

int make_template()
{
    path tname(args.get_value("-new"));
    if(tname.exists()) {
        cout << "Template "<< args.get_value("-new") <<" already exists. Please use another name.\n";
        return 2;
    }
    ofstream ts(tname.get_path().c_str());
    if(!ts) {
        cout << "Unable to open template "<<args.get_value("-new")<<" for writing.\n";
        return 2;
    }
    ts<<"#include <cpp4scripts.hpp>\n"\
        "using namespace c4s;\n\n"\
        "program_arguments args;\n";
    ts<<"int main(int argc, char **argv)\n{\n\n";
    ts<<"    args += argument(\"-?\",  false, \"Outputs this help / parameter list.\");\n" \
        "    try {\n"                                                   \
        "        args.initialize(argc,argv);\n"                         \
        "    }catch(c4s_exception ce){\n"                               \
        "        cout << \"Incorrect parameters.\\n\"<<ce.what()<<'\\n';\n" \
        "        return 1;\n"                                           \
        "    }\n";
    ts<<"    if( args.is_set(\"-?\") ) {\n"         \
        "        args.usage();\n"                   \
        "        return 0;\n"                       \
        "    }\n";
    ts<<"    return 0;\n";
    ts<<"}\n";

    ts.close();
    cout<<"Template created.\n";
    return 0;
}

int main(int argc, char **argv)
{
    bool debug=false;
    bool verbose=false;
    int timeout=15;

    // Set arguments and initialize them.
    args += argument("-s",  true,  "Sets VALUE as a source file to be compiled. If not defined looks for the c4s-build.cpp from current dir.");
    args += argument("-deb", false, "Create debug version.");
    args += argument("-rel", false, "Create release version (default).");
    args += argument("-def", false, "Print the default compiler argumens to stdout.");
    args += argument("-new", true , "Make a new c4s-template file with VALUE as the name.");
#ifdef _WIN32
    args += argument("-sr", false, "Makes VC to build against Microsoft static runtime (MT).");
#endif
    args += argument("-CXX",false, "Requests the makec4s to read the C++ compiler name from CXX environment variable.");
    args += argument("-a",  false, "Use the 'all inclusive' sources i.e. without the CPP4Scripts library.");
    args += argument("-l",  true,  "Environment label to apend to executable name.");
    args += argument("-m",  true,  "Set the VALUE as timeout (seconds) for the compile.");
    //args += argument("-p",  false, "Pad the final executable name with environment and build type.");
    args += argument("-t",  false, "Enable C4S_DEBUGTRACE define for tracing the cpp4scripts code.");
    args += argument("-h",  false, "Outputs this help / parameter list.");
    args += argument("-?",  false, "Outputs this help / parameter list.");
    args += argument("-v",  false, "Prints the version number.");
    args += argument("-V",  false, "Verbose mode. Prints more messages, including build command.");
    try {
        args.initialize(argc,argv);
    }catch(c4s_exception ce){
        cout << "Incorrect parameters.\n"<<ce.what()<<'\n';
        return 1;
    }
    // ............................................................
    // Handle simple arguments.
    if( args.is_set("-h") || args.is_set("-?") ) {
        cout << "Cpp4Scripts make program. "<< CPP4SCRIPTS_VERSION;
        cout << "\n\nWith no parameters makec4s searches for 'c4s-build.cpp' program in current directory.\n" \
            "If found it will build it, supposing it is a Cpp4Scripts program and contains only\n"\
            "this single source file.\n"\
            "Use the '-s' parameter to specify a Cpp4Scripts source file to compile.\n\n"\
            "Rest of the parameters:\n";
        args.usage();
        return 0;
    }
    cout << "Cpp4Scripts make program. "<< CPP4SCRIPTS_VERSION <<' '<<get_build_type()<<'\n';
    if(args.is_set("-v")) {
        return 0;
    }
    if(args.is_set("-new")) {
        return make_template();
    }
    if(args.is_set("-V"))
        verbose = true;
#ifdef __APPLE__
    else {
        string verbose_env;
        if(get_env_var("MAKEC4S_VERBOSITY",verbose_env) && verbose_env[0]=='1')
            verbose=true;
    }
    if(!args.is_set("-deb") && !args.is_set("-rel")) {
        string scheme;
        if(get_env_var("DEBUGGING_SYMBOLS",scheme)) {
            debug = scheme.compare("YES")==0 ? true : false;
            if(verbose)
                cout << "Debugging mode selected via environment variable.\n";
        }
    }
    else
#endif
        debug = args.is_set("-deb") ? true:false;

    if(args.is_set("-def")) {
        int flags = BUILD_BIN;
        flags |= debug?BUILD_DEBUG:BUILD_RELEASE;
        if(args.is_set("-CXX"))
            flags |= BUILD_ENV_VAR;
        try {
#if defined(__linux) || defined(__APPLE__)
            builder_gcc gcc(0, "dummy", 0, flags);
            cout << "Binary build default options:\n";
            gcc.print(cout);
            cout << "Library build default options:\n";
            flags &= ~BUILD_BIN;
            flags |= BUILD_LIB;
            builder_gcc link(0, "dummy", 0, flags);
            link.print(cout);
            string name("[binary name]");
            builder::pad_name(name,0,flags);
            cout << "Name padding: "<<name<<'\n';
#else
            builder_vc vc(0, "dummy", 0, flags);
            cout << "Binary build default options:\n";
            vc.print(cout);
            cout << "Library build default options:\n";
            flags &= ~BUILD_BIN;
            flags |= BUILD_LIB;
            builder_vc link(0, "dummy", 0, flags);
            link.print(cout);
#endif
        }catch(c4s_exception ce){
            cout << "Parameter output failed:"<<ce.what()<<'\n';
            return 1;
        }
        return 0;
    }
    // ............................................................
    // Set the sources and the target
    path_list sources;
    path src;
    string target;
    if(args.is_set("-s"))
        src = args.get_value("-s");
    else {
        // try the environment
        string source_file;
        if(get_env_var("MAKEC4S_DEF_SOURCE", source_file))
            src.set(source_file);
        else {
            cout << "Nothing to do. Use either -s [source] or env.var. 'MAKEC4S_DEF_SOURCE'\n";
            return 1;
        }
    }
    if(!src.exists()) {
        cout << "Source file '"<<src.get_path()<<"' does not exist.\n";
        return 1;
    }
    if(verbose)
        cout << "Using "<<src.get_path()<<" as a source file.\n";
    sources += src;
    if(args.is_set("-m")) {
        timeout = strtol(args.get_value("-m").c_str(),0,10);
        if(timeout == 0) {
            cout << "Warning: unable to recognize the compile process timeout. Using the default.\n";
            timeout = 15;
        }
    }
    target = src.get_base_plain();
    if(args.is_set("-l")) {
        target += '-';
        target += args.get_value("-l");
    }
    try {
        builder *make=0;
        int flags = BUILD_BIN;
        flags |= debug ? BUILD_DEBUG : BUILD_RELEASE;
        if(verbose)
            flags |= BUILD_VERBOSE;
        if(args.is_set("-CXX"))
            flags |= BUILD_ENV_VAR;

        int arch = builder::get_arch();
        flags |= arch;
#ifdef _WIN32
        // ............................................................
        // Windows with Visual Studio. TODO: add parameter so that gcc could be used as well.
        string libname("c4s.lib");
        make = new builder_vc(&sources,target.c_str(),&cout,flags);
        if(args.is_set("-sr"))
            ((builder_vc*)make)->setStaticRuntime();
        make->include_variables();
        make->add_comp("/I$(C4S)\\include\\cpp4scripts");
        make->add_link("Advapi32.lib");
        if(args.is_set("-t"))
            make->add_comp("/DC4S_DEBUGTRACE");
        if(!args.is_set("-a") && !args.is_set("-t")) {
            make->add_link(libname.c_str());
            if(debug)
                make->add_link("/LIBPATH:$(C4S)\\lib-d");
            else
                make->add_link("/LIBPATH:$(C4S)\\lib");
        }
#endif
#if defined(__linux) || defined(__APPLE__)
        // ............................................................
        // Gcc options for Linux
        // Build options
        string libname("-lc4s");
        make = new builder_gcc(&sources,target.c_str(),&cout,flags);
        // Get C4S location
        string c4svar;
        if(!get_env_var("C4S",c4svar))
            make->set_variable("C4S","/usr/local");
        make->add_comp("-x c++ -fno-rtti -I$(C4S)/include/cpp4scripts");
        if(args.is_set("-t"))
            make->add_comp("-DC4S_DEBUGTRACE");
        if(!args.is_set("-a") && !args.is_set("-t")) {
            make->add_comp("-I$(C4S)/include/cpp4scripts");
            make->add_link(libname.c_str());
            if(debug)
                make->add_link("-L$(C4S)/lib-d");
            else
                make->add_link("-L$(C4S)/lib");
        }
#endif
        if(!make) {
            cout << "Only Windows, Linux and OSX environments are supported.\n";
            return 1;
        }
        make->set_timeout(timeout);
        if(make->build()) {
            cout << "Build failed.\n";
            delete make;
            return 2;
        }
        else
            cout << make->get_target_name() << " ready.\n";
        delete make;
    }catch(c4s_exception ce){
        cout << "Error: " << ce.what() <<'\n';
        return 1;
    }
#ifdef _WIN32
    // Remove the annoying and undeeded object files
    sources.set_ext(string(".obj"));
    sources.rm_all();
#endif
    return 0;
}
