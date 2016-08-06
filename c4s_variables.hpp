/*******************************************************************************
c4s_variables.hpp
Defines variable class for Cpp4Scripts libarry

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
#ifndef C4S_VARIABLES_HPP
#define C4S_VARIABLES_HPP

namespace c4s {
    class path;
    // ----------------------------------------------------------------------------------------------------
    /// Variable substitution class
    /*! Variables are simple string substitutions, i.e. one string is replaced with another. Variables
     are stored in STL map container. What makes this class convenient is the ability to read variable
     definitions from text files are run-time.
     Variable files follow unix common configuration files syntax: variable name is followed by equal sign.
     Rest of the line is taken as a value to variable. '#' can be used as a comment. Blank lines are ignored.
    */
    class variables
    {
    public:
        variables() {}
        variables(const path &p) { include(p); }
        void include(const path &);
        string expand(const string&, bool se=false);
        static void exp_arch(int arch, char *var);

    protected:
        map<string,string> vmap;
    };

}

#endif
