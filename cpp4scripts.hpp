/***************************************************************************************************
cpp4scrpts.h
Main header file for Cpp4Scripts library. Includes all the rest headers.

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
***************************************************************************************************/

#ifndef CPP4SCRIPTS_H
#define CPP4SCRIPTS_H

#if !defined(CPP4SCRIPTS_ALL_HPP) && !defined(_CRT_SECURE_NO_WARNINGS) && defined(_MSC_VER)
  #define _CRT_SECURE_NO_WARNINGS
#endif

#include "c4s_config.hpp"
#include "c4s_exception.hpp"
#if defined(__linux) || defined(__APPLE__)
  #include "c4s_user.hpp"
#endif
#include "c4s_path.hpp"
#include "c4s_path_list.hpp"
#include "c4s_path_stack.hpp"
#include "c4s_variables.hpp"
#include "c4s_program_arguments.hpp"
#include "c4s_compiled_file.hpp"
#include "c4s_process.hpp"
#include "c4s_logger.hpp"
#include "c4s_util.hpp"
#include "c4s_settings.hpp"
#include "c4s_builder.hpp"
#if defined(__linux) || defined(__APPLE__)
  #include "c4s_builder_gcc.hpp"
#endif
#ifdef _MSC_VER
  #include "c4s_builder_vc.hpp"
  #include "c4s_builder_ml.hpp"
#endif
#endif
