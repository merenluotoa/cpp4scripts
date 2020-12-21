/*! \file c4s_builder_vc.hpp
 * \brief Visual studio builder header */
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

#ifndef C4S_BUILDER_VC_HPP
#define C4S_BUILDER_VC_HPP

namespace c4s {

    //! Builder for Microsoft Visual Studio
    class builder_vc : public builder
    {
    public:
        //! Constructor for Microsoft Visual Studio builder.
        builder_vc(path_list *sources, const char *name, ostream *log, const BUILDF &);
        int build();
        //! Helper function for MS VC to create precompiled headers.
        int precompile(const char *name, const char *content, const char *stopname);
        //! Sets static VisualStudio runtime (MT). Default is the dynamic (MD)
        void setStaticRuntime() { static_runtime = true; }
    private:
        bool precompiled;
        bool static_runtime;

    };
}

#endif
