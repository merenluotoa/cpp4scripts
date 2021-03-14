/*******************************************************************************
c4s_builder_gcc.hpp

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

#ifndef C4S_BUILDER_GCC_HPP
#define C4S_BUILDER_GCC_HPP

namespace c4s {

//! Builder for g++ in GCC
class builder_gcc : public builder
{
public:
    //! g++ builder constructor
    builder_gcc(path_list *sources, const char *name, ostream *log, const BUILD &);
    builder_gcc(const char *name, ostream *log, const BUILD &);
    //! Executes the build.
    int build();
private:
    void parse_flags();
    bool late_flags;
};

}

#endif
