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

    const int BUILD_NONE     = 0x000;   //!< No flags.
    const int BUILD_DEBUG    = 0x001;   //!< Builds a debug version
    const int BUILD_RELEASE  = 0x002;   //!< Builds a release version
    const int BUILD_X32      = 0x004;   //!< 32bit environment detected. (Automatically determined. Cannot be set)
    const int BUILD_X64      = 0x008;   //!< 64bit environment detected. (Automatically determined. Cannot be set)
    const int BUILD_BIN      = 0x010;   //!< Builds a normal binary.
    const int BUILD_SO       = 0x020;   //!< Builds a dynamic library (DLL or SO)
    const int BUILD_LIB      = 0x040;   //!< Builds a library
    const int BUILD_VERBOSE  = 0x080;   //!< Verbose build, shows more output
    const int BUILD_ENV_VAR  = 0x100;  //!< Read compiler name from environment variable CXX
    const int BUILD_RESPFILE = 0x200;  //!< Puts arguments into response file before compiling or linking.
    const int BUILD_GUI      = 0x400;  //!< Builds with GUI flags as opposed to console.
    const int BUILD_PAD_NAME = 0x800;  //!< Pad target name with build type and architecture designators.
    // const int BUILD_STATIC   = 0x1000; //!< Uses static linking and runtime libraries.
    const int BUILD_WIDECH   = 0x2000; //!< Uses wide characters i.e. wchar_t
    const int BUILD_NOLINK   = 0x4000; //!< Only compilation is done at build-command.
    const int BUILD_NODEFARGS= 0x8000; //!< Skip use of default arguments. All build arguments must be specifically added.
    const int BUILD_PLAIN_C  = 0x10000; //!< Use C-compilation instead of the default C++.

    /*! \brief An abstract builder class that defines an interface to CPP4Scripts builder feature.

      This is a common base class for all specific compilers. It collects options, constructs the binary
      and build directory name and has the build-function interface.

      # Forming the build directory name.
      Directory starts with [deb|rel] depending on the debug flag and is followed by [32|64] depending
      on the architecture. If user has specified subsystem name then it is appended last.

      # Forming the binary padding.
      Given name gets appended with [d|r] depending on the debug flag and [32|64] depending
      on the architecture. If user has specified subsystem name then it is appended last.
     */
    class builder
    {
    public:
        //! Empty virtual destructor
        virtual ~builder() { }

        // Sets compiler options, overwrites existing options.
        //void set_comp(const char *arg) { c_opts.str(""); c_opts<<arg<<' '; }
        //! Adds more compiler options to existing ones. In Win32 replaces $$ automatically with 32/64.
        void add_comp(const char *arg);
        //! Adds more linker/librarian options to existing ones. In Win32 replaces $$ automatically with 32/64.
        void add_link(const char *arg);
        //! Adds more object files to be linked.
        void add_link(const path_list &src, const char *obj) { extra_obj.add(src,build_dir+C4S_DSEP,obj); }
        //! Adds a compiled file for linking
        void add_link(const compiled_file &cf);

        //! Returns true if the given flag has been set.
        bool is_set(const int f) { return (flags&f)==f ? true:false; }
        //! Reads compiler variables from a file.
        void include_variables(const char *filename=0);
        //! Prints current options into given stream
        void print(ostream &);
        //! Returns the padded name.
        string get_name() { return name; }
        //! Returns the padded name, i.e. with system specific extension and possibly prepended 'lib'
        string get_target_name() { return target; }
        //! Returns relative target path, i.e. padded name with prepended build_dir
        path get_target_path() { return path(build_dir, target); }
        //! Returns the current flags. Most notably the detected architecure.
        int get_flags() { return flags; }
        //! Returns generated build directory name. Note: name does not have dir-separator at the end.
        string get_build_dir() { return build_dir; }
        //! Cleans build directory designated by the flags.
        void clean_build_dir();
        //! Sets the timeout value to other than default 15s
        void set_timeout(int to) { if(to>0 && to<300) timeout = to; }

        //! Returns architecture of the current environment.
        static int get_arch();
        //! Pads the name in BUILD_PAD_NAME style
        static void pad_name(string &name, const char *subsys, int flags);
        //! Increments the build number in the given file
        static int update_build_no(const char *filename);

        //! Template for the build command
        virtual int build()=0;

    protected:
        //! Protected constructor. Called by child classes only.
        builder(path_list *sources, const char *name, ostream *log, const int flags, const char *target);
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
        int flags;              //!< Build flags.
        int timeout;            //!< Number of seconds to wait before build and link commands fail.
    };
}

#endif
