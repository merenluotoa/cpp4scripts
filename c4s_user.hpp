/*******************************************************************************
c4s_user.hpp
C4S library user-class definition

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

Copyright (c) Menacon Ltd.
*******************************************************************************/
#ifndef C4S_USER_HPP
#define C4S_USER_HPP

#if defined(__linux) || defined(__APPLE__)
namespace c4s {
    // ----------------------------------------------------------------------------------------------------
    //! Class that encapsulates a user in the target system.
    /*! User stores the system's user and group id as well as the user and group names.
      User and group ids corresponding to given names are read at the constructor. Please note that user
      does not necessarily belong to the named group. In this case the valid() function
      returns false. User can be added with create() -funtion.
    */
    class user
    {
    public:
        //! Default constructor initializes empty user object.
        user() { uid=-1; gid=-1; system=false; }
        //! Creates a new user object from user name. Group name will be empty.
        user(const char *name);
        //! Creates a new user object with name and group only.
        user(const char *name, const char *group, bool system=false);
        //! Creates a new user object with all given information
        user(const char *name, const char *group, bool system, const char *home, const char *shell=0, const char *GROUPS=0);
        //! Copy constructor
        user(const user& orig);

        //! Sets user information by reading it from the system.
        void set(int id_u, int id_g);
        //! Copies user to another.
        void operator=(const user &org) {
            name=org.name; group=org.group; home=org.home;
            uid=org.uid; gid=org.gid; system=org.system;
        }

        //! Clears this user object.
        void clear() { name.clear(); group.clear(); uid=-1; gid=-1; }
        //! Makes sure the user exists in the system as currently presented in memory.
        void create(bool append_groups=false);
        //! Returns zero if user exists in system as it is now in memory.
        int status(bool refresh=false);
        //! Returns true if the user and group names have not been specified.
        bool empty() { return name.empty() && group.empty() ? true:false; }
        //! Returns true if given ids match this user.
        bool match(int uid, int gid);

        //! Returns user's name.
        string get_name() { return name; }
        //! Returns user's primary group.
        string get_group() { return group; }
        //! Returns user's ID. Value is -1 if user did not exist in the system.
        int get_uid() { return uid; }
        //! Returns user's group ID. Value is -1 if group did not exist in the system.
        int get_gid() { return gid; }

        //! Returns true if this user is designated as root/admin
        bool is_admin() { return (status()==0 && uid==0) ? true:false; }
        //! Returns true if user's name and group can be found from the system.
        bool is_ok() { return (uid>=0 || gid>=0) ? true:false; }

        static user get_current();
        //! Writes user's attributes to given stream. Use for debugging.
        void dump(ostream &);

        string home;        //!< User's home directory
        string GROUPS;      //!< Comma separated list that the user should also belong to.
        string shell;       //!< User's login shell.

    protected:
        //! Reads the user's user and group ids from the system.
        void read();

        bool system;
        string name;        //!< User's account name.
        string group;       //!< User's primary group.
        int uid, gid;
    };
}
#endif

#endif
