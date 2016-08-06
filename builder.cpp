/*! \file builder.cpp
 * \brief Library compiler program.

When compiled, builds in turn the Cpp4Scripts library. Make builder with one
of the following commands:

Linux / OSX:
  $CXX -o builder builder.cpp -Wall -pthread -fexceptions -fno-rtti -fuse-cxa-atexit -O2
  $CXX -o builder builder.cpp -Wall -pthread -fexceptions -fno-rtti -fuse-cxa-atexit -O0 -ggdb

Windows Visual Studio:
  cl /Febuilder.exe /O2 /MD /EHsc /W2 builder.cpp Advapi32.lib
  cl /Febuilder.exe /Od /MDd /Zi /EHsc /W2 builder.cpp Advapi32.lib

Runtime dependensies: 7z and Doxygen for publishing. Both must exist in path.
*/
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

#include <sstream>
#include "c4s_all.hpp"
#include "c4s-version.cpp"
using namespace c4s;

program_arguments args;
path compile_dir;

const char *cpp_list = "c4s_builder.cpp c4s_logger.cpp c4s_path.cpp c4s_path_list.cpp " \
    "c4s_process.cpp c4s_program_arguments.cpp c4s_util.cpp c4s_variables.cpp";
const char *cpp_win = "c4s_builder_vc.cpp c4s_builder_ml.cpp";
const char *cpp_linux = "c4s_user.cpp c4s_builder_gcc.cpp";

// ==========================================================================================
bool documentation(ostream *log)
{
    cout << "Creating documentation\n";
    try{
        if(process ("doxygen", "c4s-doxygen.dg",log)(10)) {
            cout << "Doxygen error.\n";
            return false;
        }
    }catch(c4s_exception re){
        cerr << "Error: "<<re.what()<<'\n';
        return false;
    }
    cout << "OK\n";
    return true;
}

// ==========================================================================================
int publish()
{
    cout << "Packaging sources and doc\n";
    path source;
    source.read_cwd();
    path target("~/mnt/mc/web/cpp4scripts/");
    if(!target.exists())
    {
        cerr << "Target dir "<<target.get_dir()<<" does not exist.\n";
        return 1;
    }

    try{
        string sources("c4s-");
        sources += args.get_value("-v");
        sources += ".zip";
        path source_zip(sources);
        if(source_zip.exists())
            source_zip.rm();
        sources += " cpp4scripts.cpp cpp4scripts.hpp";
        process("zip", sources.c_str())();

        path doc_zip("c4s-doc.zip");
        process("zip", "-r c4s-doc.zip c4s-doc")();
        doc_zip.cp(target, PCF_MOVE|PCF_FORCE);

        cout << "Copying doc and samples to html dir\n";
        source += "c4s-doc/";
        target += "c4s-doc/";
        source.cp(target, PCF_RECURSIVE|PCF_FORCE);

        source.rewind();
        target.rewind();
        source += "samples/";
        target += "samples/";
        if(!target.dirname_exists())
            target.mkdir();
        path_list cpp(source,"*.cpp");
        cpp.copy_to(target,PCF_FORCE);
    }catch(c4s_exception ce){
        cout << "Publish exception - " << ce.what() << '\n';
        return 1;
    }
    cout << "Completed!\n";
    return 0;
}

// ==========================================================================================
#if defined(__linux) || defined(__APPLE__)
int build(ostream *log)
{
    bool debug = false;
    cout << "Building library\n";
    if(args.is_set("-u") && builder::update_build_no("c4s-version.cpp"))
        cout << "Warning: Unable to update build number.\n";
    int flags = BUILD_LIB|BUILD_PAD_NAME;
    if(!args.is_set("-deb") && !args.is_set("-rel")) {
#ifdef __APPLE__
        // This piece of code is for Xcode which passes build options via environment.
        string scheme;
        if(!get_env_var("DEBUGGING_SYMBOLS",scheme)) {
            cout << "Missing target! Please specify DEBUGGING_SYMBOLS environment variable.\n";
            return 2;
        }
        if(scheme.compare("YES")==0){
            debug = true;
            flags |= BUILD_DEBUG;
        } else
            flags |= BUILD_RELEASE;
#else
        cout << "Missing target! Please specify -deb or -rel as parameter.\n";
        return 2;
#endif
    }
    else {
        if(args.is_set("-deb")) {
            debug = true;
            flags |= BUILD_DEBUG;
        }else
            flags |= BUILD_RELEASE;
    }
    if(args.is_set("-V"))
        flags |= BUILD_VERBOSE;
    if(args.is_set("-CXX"))
        flags |= BUILD_ENV_VAR;

    path_list cppFiles(cpp_list,' ');
    cppFiles.add(cpp_linux,' ');

    builder *make = new builder_gcc(&cppFiles,"c4s",log,flags);
    if(args.is_set("-t"))
        make->add_comp("-DC4S_DEBUGTRACE");
    make->add_comp("-fno-rtti -DC4S_LIB_BUILD");
    if(make->build()) {
        cout << "Build failed\n";
        delete make;
        return 2;
    }

    cout << "Building makec4s\n";
    path_list plmkc4s;
    plmkc4s += path("makec4s.cpp");
    /* Padding is not needed for library since the build directory names are always padded.*/
    flags = BUILD_BIN;
    if(debug) flags |=  BUILD_DEBUG;
    else flags |= BUILD_RELEASE;
    if(args.is_set("-V"))
        flags |= BUILD_VERBOSE;
    if(args.is_set("-CXX"))
        flags |= BUILD_ENV_VAR;

    builder *make2 = new builder_gcc(&plmkc4s,"makec4s",log,flags);
    make2->add_comp("-fno-rtti");
    string c4slib("-L./");
    c4slib += make->get_build_dir();
    c4slib += " -l";
    c4slib += make->get_name();
    make2->add_link(c4slib.c_str());
    if(make2->build()) {
        cout << "\nbuild failed\n";
        delete make;
        delete make2;
        return 2;
    }
    cout << "Compilation ready.\n";
    delete make;
    delete make2;
    return 0;
}
#endif
// ==========================================================================================
#ifdef _WIN32
int build(ostream *log)
{
    std::ostringstream liblog;
    int rv=0,flags;

    cout << "Building library\n";
    if(args.is_set("-u") && builder::update_build_no("c4s-version.cpp"))
        cout << "Warning: Unable to update build number\n";
    flags = BUILD_LIB|BUILD_PAD_NAME;
    flags |= args.is_set("-deb") ? BUILD_DEBUG:BUILD_RELEASE;
    if(args.is_set("-V"))
        flags |= BUILD_VERBOSE;
    if(args.is_set("-CXX"))
        cout << "Warning: -CXX is not supported for Visual Studio\n";

    path_list cppFiles(cpp_list,' ');
    cppFiles.add(cpp_win,' ');
    const char* libname = args.is_set("-xp") ? "c4s-xp" : "c4s";
    builder *make = new builder_vc(&cppFiles,libname,log,flags);
    make->add_comp("/DC4S_LIB_BUILD /D_CRT_SECURE_NO_WARNINGS");
    if(args.is_set("-xp"))
        make->add_comp("/D_WIN32_WINNT=0x0501");
    if(make->build()) {
        cout << "Build failed\n";
        return 2;
    }

    cout << "Building makec4s\n";
    if(log)
        *log << endl;
    path_list plmkc4s;
    plmkc4s += path("makec4s.cpp");
    flags = BUILD_BIN|BUILD_PAD_NAME;
    if(args.is_set("-deb"))
        flags |= BUILD_DEBUG;
    else
        flags |= BUILD_RELEASE;
    if(args.is_set("-V"))
        flags |= BUILD_VERBOSE;
    builder *make2 = new builder_vc(&plmkc4s,"makec4s",log,flags);
    make2->add_link("Advapi32.lib ");
    string c4slib(make->get_target_name());
    c4slib += " /LIBPATH:";
    c4slib += make->get_build_dir();
    make2->add_link(c4slib.c_str());
    if(make2->build()) {
        cout << "Build failed\n";
        return 2;
    }

    cout << "Build finished.\n";
    return 0;
}
#endif

// ==========================================================================================
int clean()
{
#if defined(__linux) || defined(__APPLE__)
    builder_gcc(0,"debug",0,BUILD_DEBUG).clean_build_dir();
    builder_gcc(0,"release",0,BUILD_RELEASE).clean_build_dir();
#else
    builder_vc(0,"debug",0,BUILD_DEBUG).clean_build_dir();
    builder_vc(0,"release",0,BUILD_RELEASE).clean_build_dir();
#endif
    path_list tmp(args.exe,"*.log");
    tmp.add(args.exe,"*~");
    tmp.add(args.exe,"*.obj");
    tmp.add(args.exe,"*.ilk");
    tmp.add(args.exe,"*.pdb");
    tmp.add(args.exe,"makec4s-*.*");
    tmp.rm_all();
    cout << "Build directories and "<<tmp.size()<<" temp-files removed\n";
    return 0;
}

// ==========================================================================================
int install()
{
    path lbin;
    path c4s_home(args.exe);
    path_iterator pi;

    cout << "Installing Cpp4Scripts\n";
    path inst_root(append_slash(args.get_value("-install")));
    if(!inst_root.dirname_exists()) {
        cout << "Installation root directory "<<inst_root.get_path()<<" must exist.\n";
        return 1;
    }
    // Set up the target directories
    if(args.is_set("-V")) cout << "Creating target directories\n";
    path inc(inst_root);
    path lib(inst_root);
    inc += "include/cpp4scripts/";
    lib += "lib/";
    if(!inc.dirname_exists())
        inc.mkdir();
    if(!lib.dirname_exists())
        lib.mkdir();

    // Copy sources
    if(args.is_set("-V")) {
        cout << "Copying headers and sources.\n";
    }
    path_list sources(cpp_list,' ');
    int arch = builder::get_arch();
    if(args.is_set("-V")) {
        const char *archtxt = arch==BUILD_X64?"64-bit":"32-bit";
        cout << "Running in "<<archtxt<<" environment\n";
    }
#if defined(__linux) || defined(__APPLE__)
    sources.add(cpp_linux,' ');
    path dlib(builder_gcc(0,"c4s",0,BUILD_LIB|BUILD_PAD_NAME|BUILD_DEBUG).get_target_path());
    path rlib(builder_gcc(0,"c4s",0,BUILD_LIB|BUILD_PAD_NAME|BUILD_RELEASE).get_target_path());
    path make_deb(builder_gcc(0,"makec4s",0,BUILD_BIN|BUILD_PAD_NAME|BUILD_DEBUG).get_target_name());
    path make_rel(builder_gcc(0,"makec4s",0,BUILD_BIN|BUILD_PAD_NAME|BUILD_RELEASE).get_target_name());
#else
    sources.add(cpp_win,' ');
    path dlib(builder_vc(0,"c4s",0,BUILD_LIB|BUILD_PAD_NAME|BUILD_DEBUG).get_target_path());
    path rlib(builder_vc(0,"c4s",0,BUILD_LIB|BUILD_PAD_NAME|BUILD_RELEASE).get_target_path());
    path make_deb(builder_vc(0,"makec4s",0,BUILD_BIN|BUILD_PAD_NAME|BUILD_DEBUG).get_target_name());
    path make_rel(builder_vc(0,"makec4s",0,BUILD_BIN|BUILD_PAD_NAME|BUILD_RELEASE).get_target_name());
#endif
    path_list libs;
    if(dlib.exists())
        libs += dlib;
    if(rlib.exists())
        libs += rlib;
    else
    if(libs.size()==0) {
        cout << "WARNING: Neither of the -deb or -rel libraries will be copied. Sure they are built?\n";
    }
    sources.set_dir(c4s_home);
    path_list headers(args.exe,"*.hpp");
    sources.copy_to(inc,PCF_FORCE);
    headers.copy_to(inc,PCF_FORCE);

    for(pi=libs.begin(); pi!=libs.end(); pi++) {
        pi->cp(lib,PCF_FORCE);
        cout << "Copying "<<pi->get_path()<<" to "<<lib.get_path()<<'\n';
    }

    // Copy makec4s utility
    if(args.is_set("-V")) cout << "Copying makec4s\n";
#if defined(__linux) || defined(__APPLE__)
    if(lib.get_dir().find("local") != string::npos)
        lbin.set("/usr/local/bin/");
    else
        lbin.set("/usr/bin/");
#else
    lbin = inst_root;
    lbin += "bin/";
    if(!lbin.dirname_exists())
        lbin.mkdir();
#endif
    int count=0;
    if(make_deb.exists()) {
        make_deb.cp(lbin,PCF_FORCE);
        count++;
        if(args.is_set("-V"))
            cout << "Copying "<<make_deb.get_path()<<" to "<<lbin.get_path()<<'\n';
    }
    if(make_rel.exists()) {
        make_rel.cp(lbin,PCF_FORCE);
        count++;
        if(args.is_set("-V"))
            cout << "Copying "<<make_rel.get_path()<<" to "<<lbin.get_path()<<'\n';
    }
    if(count == 0) {
        cout << "WARNING: makec4s-program was not copied!\n";
    }
    cout << "Completed\n";
    return 0;
}
// ==========================================================================================
int main(int argc, char **argv)
{
    ofstream *filelog=0;

    args += argument("-deb",  false,"Create debug version of library.");
    args += argument("-rel",  false,"Create release version of library.");
    args += argument("-V",    false,"Verbose mode.");
    args += argument("-t",    false,"Add TRACE define into target build that enables lots of debug output.");
    args += argument("-u",    false, "Updates the build number (last part of version number).");
#ifdef _WIN32
    args += argument("-xp",   false,"Make a build suitable for WindowsXP.");
    args += argument("-wda",  false,"Wait for debugger to attach, i.e. wait for a keypress.");
#endif
    args += argument("-CXX",  false, "Reads the compiler name from CXX environment variable.");
    args += argument("-publish", true, "Publish project as version VALUE.");
    args += argument("-doc",  false,"Create docbook documentation only.");
    args += argument("-clean",false,"Clean up temporary files.");
    args += argument("-install",true,"Installs the library to given root directory. include- and lib-directories are created if necessary.");
    args += argument("-v",   false,"Shows the version nubmer and exists.");
    args += argument("-?",   false,"Shows this help");

    cout << "CPP4Scripts Builder Program. "<<CPP4SCRIPTS_VERSION<<' '<<get_build_type()<<'\n';

    try{
        args.initialize(argc,argv);
    }catch(c4s_exception ce){
        cerr << "Error: " << ce.what() << '\n';
        args.usage();
        return 1;
    }
    if(args.is_set("-v"))
        return 0;
#ifdef _WIN32
    if(args.is_set("-wda")) {
        char ch;
        cin >> ch;
    }
#endif
    if(args.is_set("-?")) {
        args.usage();
        return 0;
    }
    try {
        if(args.is_set("-clean"))
            return clean();
        if(args.is_set("-install"))
            return install();
    }catch(c4s_exception ce) {
        cout << "Function failed: "<<ce.what()<<'\n';
        return 1;
    }
    ostream *log = &cout;
    if(args.is_set("-publish") || args.is_set("-doc")) {
        if(!documentation(log) || args.is_set("-doc"))
            return 0;
        return publish();
    }

    int rv = 0;
    try {
        rv = build(log);
    }catch(c4s_exception ce) {
        cout << "Build failed: "<<ce.what()<<'\n';
        if(filelog)
            filelog->close();
        return 1;
    }
    if(filelog)
        filelog->close();
    return rv;
}
