/*******************************************************************************
c4s_program_arguments.cpp
Contains implementation for argument and program_arguments classes for Cpp4Scripts
library.

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

#ifdef C4S_LIB_BUILD
 #include <string.h>
 #include <time.h>
 #if defined(__linux)
  #include <stdlib.h>
 #elif defined(__APPLE__)
  #include <stdlib.h>
  #include <mach-o/dyld.h>
 #endif
 #include "c4s_config.hpp"
 #include "c4s_exception.hpp"
 #include "c4s_user.hpp"
 #include "c4s_path.hpp"
 #include "c4s_program_arguments.hpp"
 using namespace c4s;
#endif
// ==================================================================================================
bool c4s::argument::operator== (const char *t) const
{
    return strcmp(text,t)==0 ? true : false;
}
// ------------------------------------------------------------------------------------------
void c4s::argument::set_value(char *val)
/*!
  \param val Pointer to the value of the second part.
*/
{
    set=true;
    two_part=true;
    if(val[0] == '\'')
    {
        val[strlen(val)-1] = 0;
        value = val+1;
    }
    else
        value = val;
}

// ==================================================================================================
void c4s::program_arguments::initialize(int argc, char *argv[], int min_args)
/*! Valid arguments should have been added into this object before calling this function.
    Exception is thrown if the argument is not found.
  \param argc Parameter to main function = number of arguments in argv
  \param argv Parameter to main function = array of argument strings.
  \param min_args Number of arguments that should be specified in the command line.
                  Excluding the automatic first argument given by the system, i.e. value is zero based.
*/
{
    // Initialize program paths
    argv0 = argv[0];
    cwd.read_cwd();
    char path_buf[512];
#if defined(__linux)
    pid_t pid = getpid();
    sprintf(path_buf,"/proc/%d/exe",pid);
    int rv = readlink(path_buf,path_buf,sizeof(path_buf));
#elif defined(__APPLE__)
    uint32_t size = sizeof(path_buf);
    int rv = _NSGetExecutablePath(path_buf, &size) == 0 ? strlen(path_buf) : 0;
#else
    DWORD rv = GetModuleFileName(0,path_buf,sizeof(path_buf));
#endif
    if(rv>0) {
        path_buf[rv]=0;
        exe = path_buf;
    }
    else
        exe = argv[0];

    // Initialize the random number generator in case we need to use it.
    srand((unsigned int)time(0));

    if(argc<min_args+1)
        throw c4s_exception("Arguments initialize - Too few arguments specified in command line.");

    // Check the arguments
    list<argument>::iterator pi;
    int argi=1;
    while(argi<argc)
    {
        // For potential arguments:
        for(pi = arguments.begin(); pi!=arguments.end(); pi++)
        {
            // If found:
            if(!strcmp(argv[argi],pi->get_text()))
            {
                // Set argument on, and copy value if it is twopart param.
                pi->set_on();
                if(pi->is_two_part())
                {
                    if(argi+1 >= argc)
                    {
                        ostringstream os;
                        os << "Arguments initialize - Missing value for argument: " << pi->get_text() << '\n';
                        throw c4s_exception(os.str());
                    }
                    pi->set_value(argv[argi+1]);
                    argi++;
                }
                break;
            }
        }
        if(pi==arguments.end())
        {
            ostringstream os;
            os << "program_arguments::initialize - Unknown argument: " << argv[argi] << '\n';
            throw c4s_exception(os.str());
        }
        argi++;
    }
}

// ==================================================================================================
void c4s::program_arguments::usage()
/*!
  Prints the help on program arguments to stdout. Initialize function should have been called
  before this function.
  \param title A title to print before the arguments.
  \param info A informational message to be printed after the arguments.
*/
{
    string paramTxt;
    list<argument>::iterator pi;
    cout << "Usage: "<<argv0.get_base();
    if(arguments.size()>0)
        cout << " [Options]\n";
    for(pi = arguments.begin(); pi!=arguments.end(); pi++)
    {
        cout << "  ";
        cout.width(20);
        paramTxt = pi->get_text();
        if(pi->is_two_part())
            paramTxt += " VALUE";
        cout <<left<< paramTxt <<right<< pi->get_info() << '\n';
    }
    cout << '\n';
}

// ==================================================================================================
bool c4s::program_arguments::is_set(const char *param)
/*!
  \param param Pointer to parameter string.
  \retval bool True if argument is set, false if not.
*/
{
    list<argument>::iterator pi;
    for(pi = arguments.begin(); pi!=arguments.end(); pi++)
    {
        if(*pi == param)
            return pi->is_on();
    }
    return false;
}

// ==================================================================================================
bool c4s::program_arguments::is_value(const char *param, const char *value)
/*!
  \param param Pointer to parameter string.
  \param value Pointer to value to check.
  \retval bool True if argument is set and correct, false if not.
*/
{
    list<argument>::iterator pi;
    for(pi = arguments.begin(); pi!=arguments.end(); pi++)
    {
        if(*pi == param)
        {
            if(pi->is_on())
            {
                // bool val = pi->get_value().compare(value) == 0 ? true : false;
                return pi->get_value().compare(value) == 0 ? true : false;
            }
            return false;
        }
    }
    return false;
}

// ==================================================================================================
const char* c4s::program_arguments::get_value_ptr(const char *param)
/*!
  \param param Pointer to parameter string.
  \retval char* Pointer to value string. Do not store the pointer!
*/
{
    static string value;
    list<argument>::iterator pi;
    for(pi = arguments.begin(); pi!=arguments.end(); pi++)
    {
        if( !strcmp(pi->get_text(),param) && pi->is_two_part() )
        {
            pi->get_value(value);
            return value.c_str();
        }
    }
    return "";
}

// ==================================================================================================
int c4s::program_arguments::get_value_index(const char* param, const char **choises)
/*!
   \param param Parameter name.
   \param choises Array of strings terminated by null pointer.
   \retval int Index of the matched choise. -1 if no match or argument is not value type.
*/
{
    const char *value = get_value_ptr(param);
    if(*value == 0)
        return -1;
    for(int ndx=0; choises[ndx]; ndx++)
    {
        if(!strcmp(value,choises[ndx]))
            return ndx;
    }
    return -1;
}

// ==================================================================================================
string c4s::program_arguments::get_value(const char *param)
/*!
  \param param Pointer to parameter string
  \retval string Argument value. If value is not specified or argument is not two part type an empty
  string is returned.
*/
{
    list<argument>::iterator pi;
    for(pi = arguments.begin(); pi!=arguments.end(); pi++)
    {
        if( *pi==param && pi->is_two_part() )
        {
            //string value;
            //pi->get_value(value);
            return pi->get_value();
        }
    }
    return string("");
}

// ==================================================================================================
bool c4s::program_arguments::set_value(const char *param, char *value)
/*!
   \param param Ptr to argument name
   \param value Ptr to value string.
   \retval bool True on succes, false if argument is not found.
*/
{
    list<argument>::iterator pi;
    for(pi = arguments.begin(); pi!=arguments.end(); pi++)
    {
        if( !strcmp(pi->get_text(),param) )
        {
            pi->set_value(value);
            return true;
        }
    }
    return false;
}

// ==================================================================================================
bool c4s::program_arguments::append_value(const char *param, char *value)
/*!
  \param param Ptr to argument name.
  \param value Value to append
  \retval bool True on succes, false if argument is not found.
*/
{
    list<argument>::iterator pi;
    for(pi = arguments.begin(); pi!=arguments.end(); pi++)
    {
        if( !strcmp(pi->get_text(),param) )
        {
            pi->append_value(value);
            return true;
        }
    }
    return false;
}

