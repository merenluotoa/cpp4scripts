/*******************************************************************************
c4s_path.hpp
Defines path and directory management class for CPP4Scripts library.

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

Copyright (c) Menacon Ltd
*******************************************************************************/
#ifndef C4S_PATH_HPP
#define C4S_PATH_HPP

// #ifdef _MSC_VER
//   // Visual Studio headers do not define the POSIX SSIZE.
//   #ifdef _WIN64
//     typedef __int64 SSIZE_T;
//   #else
//     typedef int SSIZE_T;
//   #endif
// #endif

namespace c4s {
    class path_list;
    class user;

    //! Flags and options for path's copy function.
    const int PCF_NONE=0;         //!< None.
    const int PCF_FORCE=0x1;      //!< Force / overwrite if exists.
    const int PCF_MOVE=0x2;       //!< Move the file i.e. delete original.
    const int PCF_APPEND=0x4;     //!< Append to possibly existing file.
    const int PCF_ONAME=0x8;      //!< Use original base name for the target.
    const int PCF_DEFPERM=0x10;   //!< Use default file permissions given by operating system. Note, this also overrides possible user and permission settings for the target.
    const int PCF_BACKUP=0x20;    //!< Backup original if it exists.
    const int PCF_RECURSIVE=0x40; //!< Copy recursively. Valid only if source is a directory (i.e. base is empty).

    //! Flags for path compare function.
    const unsigned char CMP_DIR =  1;     //!< Compare Dir parts together
    const unsigned char CMP_BASE = 2;     //!< Compare Base parts together

    // ----------------------------------------------------------------------------------------------------
    //! Class that encapsulates a path to a file or directory.
    /*! Path has directory part (dir) and file name part (base). File name includes the extension if there is one.
      Dir and base can be set and queried together or separately. If set together the library separates the dir
      part from base. Path can be relative.<br>
      Defines:<br>
      C4S_FORCE_NATIVE_PATH = If this define is set during compile time, it enforces native path separators
      to directory. It is recommended if the same code is to be used in different environments.
    */
    class path
    {
    public:
        //! Default constructor that initializes empty path.
        path();
        //! Copy constructor.
        path(const path &p);
        //! Constructs path from dir part and given base.
        path(const path &dir, const char *base);
        //! Constructs path from single string.
        path(const string &p);
        //! Path constructor. Combines path from directory, base and extension.
        path(const string &d, const string &b, const char *e);
        //! Constructs a path from directory and base
        path(const string &d, const string &b);
        //! Constructs a path from directory and base
        path(const string &d, const char *b);
        //! Constructs a path from directory and base
        path(const char *d, const char *b);
#if defined(__linux) || defined(__APPLE__)
        //! Constructs path with user data
        path(const string &d, const string &b, user *o, int m=-1);
        //! Constructs path with user data
        path(const string &p, user *o, int m=-1);
#endif

        //! Sets path so that it equals another path.
        void operator=(const path &p) { dir=p.dir; base=p.base; change_time=p.change_time; }
        //! Sets the path from pointer to const char.
        void operator=(const char *p);
        //! Sets the path from constant string.
        void operator=(const string &p) { set(p); }
        //! Synonym for merge() function
        void operator+=(const path &p) { merge(p); }
        //! Synonym for merge() function
        void operator+=(const char *cp) { merge(path(cp)); }

        //! Clears the path.
        void clear() { change_time=0; dir.clear(); base.clear(); }
        //! Checks whether the path is clear (or empty). \retval bool True if empty.
        bool empty() { return dir.empty() && base.empty(); }

        //! Returns the directory path
        string get_dir() const { return dir; }
        //! Returns the directory path without the trailin slash.
        string get_dir_plain() const { return dir.substr(0,dir.size()-1); }
        //! Returns the base part with extension.
        string get_base() const { return base; }
        //! Returns the base and swaps its extension to the one given as parameter.
        string get_base(const char *ext) const;
        //! Returns the base without the extension
        string get_base_plain() const;
        //! Returns the base or if it is empty the last directory entry.
        string get_base_or_dir();
        //! Returns the extension from base if there is any.
        string get_ext() const;
        //! Returns the complete path.
        string get_path() const { return dir+base; }
        //! Returns the full path with quotes if the file name contains any spaces.
        string get_path_quot() const;

        //! Sets the directory part of the path.
        void set_dir(const string &d);
        //! Sets the directory part to user's home directory.
        void set_dir2home();
        //! Changes the base (=file name) part of the path.
        void set_base(const string &b);
        //! Sets the extension for the file name.
        void set_ext(const string &e);
        //! Sets the path components by parsing the given string
        void set(const string &p);
        //! Sets path attributes from given directory name, base name and optional extension
        void set(const string &d, const string &b, const char *e);
        //! Sets path attributes from given directory name and base name.
        void set(const string &d, const string &b);
        //! Changes current working directory to given path.
        static void cd(const char *);
        //! Changes current working directory to the directory stored in this object.
        void cd() const { cd(dir.c_str()); }
        //! Reads the current workd directory and sets it to dir-part. Base is not affected.
        void read_cwd();

#if defined(__linux) || defined(__APPLE__)
        //! Verifies that owner exists and the is owner of this path. (Linux only)
        int  owner_status();
        //! Reads the owner information from the path. (Linux only)
        void owner_read();
        //! Writes the current owner to disk, i.e. possibly changes ownership.  (Linux only)
        void owner_write();
        //! Sets the user and mode for this path. Does not commit change to disk. (Linux only)
        void set(user *u, int m) { owner = u; mode=m; }
        //! Sets the user for the path. Does not commit change to disk. (Linux only)
        void set_owner(user *u) { owner = u; }
        //! Returns true if the owner has bee defined for this path.
        bool has_owner() { return owner ? true:false; }
        //! Returns the user for this path. (Linux only)
        user* get_owner() { return owner; }
#endif
        //! Returns true if path has directory part.
        bool is_dir() const { return dir.empty()?false:true; }
        //! Returns true if path has a base i.e. filename
        bool is_base() const { return base.empty()?false:true; }
        //! Returns true if the path is absolute, false if not.
        bool is_absolute() const;
        //! Makes the path absolute if it is relative. Otherwice it does nothing.
        void make_absolute();
        //! Makes this path absolute based on a given root directory.
        void make_absolute(const string &root);
        //! Makes the path relative to the current working directory.
        void make_relative();
        //! Makes the path relative to the given parent directory.
        void make_relative(const path&);
        //! Rewinds the directory down to its parent as many times as given in parameter.
        void rewind(int count=1);
        //! Merges two paths.
        void merge(const path &);
        //! Appends the last directory from source to this directory.
        void append_last(const path &);
        //! Appends given directory to the directory part.
        void append_dir(const char *);

        //! Checks if the base exists in any of the directories specified in the given environment variable.
        bool exists_in_env_path(const char *envar, bool set_dir=false);
        //! Returns true if the directory specified by the path exists in the file system. False if not.
        bool dirname_exists() const;
        //! Checks if the directory and base exists.
        bool exists() const;

        //! Sets a selection flag.
        void flag_set() { flag = true; }
        //! Toggles the selection flag.
        void flag_toggle() { flag = !flag; }
        //! Returns the current selection.
        bool flag_get() { return flag; }

        //! compares full path or just the base depending on flag
        int compare(const path &target, unsigned char flag) const;

        //! Creates the directory specified by the directory part of the path.
        void mkdir() const;
        //! Removes the directory from the disk this path points to.
        void rmdir(bool recursive=false) const;

        //! Copy file pointed by path to a new location
        int cp(const char *to, int flags=PCF_NONE) { path target(to); return cp(target,flags); }
        //! Copy file pointed by path to a new location
        int cp(const path &, int flags=PCF_NONE);
        //! Concatenate file
        void cat(const path &) const;
        //! Rename the base part
        void ren(const string &, bool force=false);
        //! Remove / delete file.
        bool rm();
        //! Make symbolic link
        void symlink(const path&) const;
        //! Changes file / directory permissions.
        void chmod(int mod=-1);

        //! Returns true if this file is newer than the given file.
        bool outdated(path &p, bool checkInside=false);
        //! Checks the outdated status against a list of files.
        bool outdated(path_list &lst);
        //! Compares this files timestamp to given targets timestamp.
        int  compare_times(path &) const;
        //! Changes all directory separators from unix '/' to dos '\'.
        void unix2dos();
        //! Changes all directory separators from dos '\' to unix '/'.
        void dos2unix();
        //! Performs a search-replace for a file pointed by this path
        int search_replace(const string &search, const string &replace, bool bu=false);
        //! Performs a single block replacement in a file pointed by this path.
        bool replace_block(const string &, const string &, const string &, bool bu=false);
        // SIZE_T search_text(const string &needle);
        void dump(ostream &);
    private:
        //! Reads the last change time from the disk.
        void update_time();
        //! Copies attributes and permissions from this file to target.
        void copy_mode(const path &target);
        //! Recursive copy from this to target.
        int copy_recursive(const path &, int);

#if defined(__linux) || defined(__APPLE__)
        user *owner;        //!< Pointer to User and group for this file's permissions
        int  mode;          //!< Path/file access mode.
        time_t change_time; //!< Time that the file was last changed. Zero until internal function update_time has been called.
#else
        __int64 change_time;
#endif
        string dir;         //!< directory part of the path. Directory needs to end at the directory separator.
        string base;        //!< Base name (file name) part of the path.
        bool flag;          //!< General purpose flag for application use.
        friend class path_list;
        friend bool compare_paths(c4s::path fp, c4s::path sp);
    };

}
#endif
