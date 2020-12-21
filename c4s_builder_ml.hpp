/*! \file c4s_builder_vc.hpp
 * \brief Microsoft macro assempbler builder header */
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

Copyright (c) Antti Merenluoto
*******************************************************************************/

#ifndef C4S_BUILDER_ML_HPP
#define C4S_BUILDER_ML_HPP

namespace c4s {

    //! Builder for the Microsoft Macro Assembler
    class builder_ml : public builder
    {
    public:
        //! Constructor for Microsoft Macro Assembler
        builder_ml(path_list *sources, const char *name, ostream *log, const BUILDF &);
        int build();
    };
}

#endif
