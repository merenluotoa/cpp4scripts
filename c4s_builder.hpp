/*! \file c4s_builder.hpp
 * \brief Defines abstract builder class. */
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
*******************************************************************************/

#ifndef C4S_BUILDER_HPP
#define C4S_BUILDER_HPP

namespace c4s {

class BUILD : public flags32_base
{
public:
    static const flag32 NONE     = 0x000; //!< No flags.
    static const flag32 DEB      = 0x001; //!< Builds a debug version
    static const flag32 REL      = 0x002; //!< Builds a release version
    static const flag32 BIN      = 0x010; //!< Builds a normal binary.
    static const flag32 SO       = 0x020; //!< Builds a dynamic library (DLL or SO)
    static const flag32 LIB      = 0x040; //!< Builds a library
    static const flag32 VERBOSE  = 0x080; //!< Verbose build, shows more output
    static const flag32 RESPFILE = 0x100; //!< Puts arguments into response file before compiling or linking.
    static const flag32 WIDECH   = 0x400; //!< Uses wide characters i.e. wchar_t
    static const flag32 NOLINK   = 0x800; //!< Only compilation is done at build-command.
    static const flag32 NODEFARGS= 0x1000; //!< Skip use of default arguments. All build arguments must be specifically added.
    static const flag32 PLAIN_C  = 0x2000; //!< Use C-compilation instead of the default C++.

    BUILD() : flags32_base(NONE) { }
    BUILD(flag32 fx) : flags32_base(fx) { }
};

/*! \brief An abstract builder class that defines an interface to CPP4Scripts builder feature.

  This is a common base class for all specific compilers. It collects options, constructs the binary
  and build directory name and has the build-function interface.
*/
class builder : public BUILD
{
public:
    // Destructor, releases source list if it was reserved during build process.
    virtual ~builder();

    // Sets compiler options, overwrites existing options.
    //void set_comp(const char *arg) { c_opts.str(""); c_opts<<arg<<' '; }
    //! Adds more compiler options to existing ones.
    void add_comp(const string &arg);
    //! Adds more linker/librarian options to existing ones.
    void add_link(const string &arg);

    //! Adds more object files to be linked.
    void add_link(const path_list &src, const char *obj) { extra_obj.add(src,build_dir+C4S_DSEP,obj); }
    //! Adds a compiled file for linking
    void add_link(const compiled_file &cf);

    //! Reads compiler variables from a file.
    void include_variables(const char *filename=0);
    //! Inserts single variable to the variable list.
    void set_variable(const string &key, const string &value) { vars.push_back(key,value); }
    //! Prints current options into given stream
    void print(ostream &out, bool list_sources=false);
    //! Returns the padded name.
    string get_name() { return name; }
    //! Returns the padded name, i.e. with system specific extension and possibly prepended 'lib'
    string get_target_name() { return target; }
    //! Returns relative target path, i.e. padded name with prepended build_dir
    path get_target_path() { return path(build_dir, target); }
    //! Returns generated build directory name. Note: name does not have dir-separator at the end.
    string get_build_dir() { return build_dir; }
    //! Sets the timeout value to other than default 15s
    void set_timeout(int to) { if(to>0 && to<300) timeout = to; }

    //! Increments the build number in the given file
    static int update_build_no(const char *filename);

    //! Template for the build command
    virtual int build()=0;

protected:
    //! Protected constructor: Initialize builder with initial list of files to compile
    builder(path_list *sources, const char *name, ostream *log, const BUILD &);
    //! Protected constructor: File list is read from git.
    builder(const char *name, ostream *log, const BUILD &);
    //! Executes compile step
    int compile(const char *out_ext, const char *out_arg, bool echo_name=true);
    //! Executes link/library step.
    int link(const char *out_ext, const char *out_arg);

    variables vars;         //!< Variables list. Compiler arguments are automatically expanded for variables before the execution.
    process compiler;       //!< Compiler process for this builder
    process linker;         //!< Linker process for this builder.
    ostringstream c_opts;   //!< List of options for the compiler.
    ostringstream l_opts;   //!< List of options for the linker.
    ostream *log;           //!< If not null, will receive compiler and linker output.
    path_list *sources;     //!< List of source files. Relative paths are possible.
    path_list extra_obj;    //!< Optional additional object files to be included at link step.
    string name;            //!< Simple target name.
    string target;          //!< Decorated and final target name.
    string build_dir;       //!< Generated build directory name. No dir-separater at the end.
    int timeout;            //!< Number of seconds to wait before build and link commands fail.
    bool my_sources;        //!< If true builder has allocated the sources list.
};

} // namespace c4s

#endif
