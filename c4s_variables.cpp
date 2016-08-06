/*******************************************************************************
c4s_variables.cpp
Implementation of Cpp4Scripts libarry variables-class

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
 #include "c4s_config.hpp"
 #include "c4s_exception.hpp"
 #include "c4s_user.hpp"
 #include "c4s_path.hpp"
 #include "c4s_variables.hpp"
 #include "c4s_util.hpp"
 #include "c4s_process.hpp"
 #include "c4s_path_list.hpp"
 #include "c4s_compiled_file.hpp"
 #include "c4s_builder.hpp"
 using namespace c4s;
#endif
// ==================================================================================================
void c4s::variables::include(const path &inc_file)
/*!
  Reads the given include file and adds variable definitions from it to the given variable list.
  Variables have following syntax: "name = value". Anything before equal-sign is taken as name of the variable.
  Anything following the equal sign is taken as the value of variable. Any whitespace around the equal sign is discarded.

  In Windows variable values are searched for $$-marks. These are replaced with current build architecture's word length i.e. 32 or 64.

  \param file Path to the file to be sourced.
*/
{
    char *eq,*ptr,line[MAX_LINE];
    string key;
    int lineno = 0;

    ifstream inc(inc_file.get_path().c_str(),ios::in);
    if(!inc)
    {
        ostringstream os;
        os << "Unable to open include file '"<<inc_file.get_path();
        throw path_exception(os.str());
    }
    do
    {
        lineno++;
        inc.getline(line,sizeof(line));
        if(inc.fail() && !inc.eof())
        {
            ostringstream os;
            os << "Insufficient buffer in c4s::include_vars reading file: "<< inc_file.get_base() << " on line: " << lineno;
            throw c4s_exception(os.str());
        }

        // Ignore empty and comment lines.
        if(line[0] == '#' || line[0]=='\r' || line[0]=='\n' || line[0]==' ' || line[0]=='\t')
            continue;

        // If no equal sign: continue.
        eq = strchr(line,'=');
        if(!eq || eq==line)
            continue;
        // Trim the end of the key
        ptr = eq;
        do{
            ptr--;
        }while( ptr>line && (*ptr==' ' || *ptr==':' || *ptr=='\t') );
        if(ptr==line && (*ptr==' ' || *ptr=='\t'))
            continue;
        key.assign(line,ptr-line+1);

        // Trim beginning of value
        ptr = eq;
        do{
           ptr++;
        }while( (*ptr==' ' || *ptr=='\t') && *ptr );
        if(!ptr)
            continue;
        eq = ptr;

        // Trim the end of the value
        ptr = eq + strlen(eq);
        do{
            ptr--;
        }while( (*ptr==' ' || *ptr=='\t' || *ptr=='\r' || *ptr=='\n') && ptr!=eq );
        if(ptr==eq)
            continue;
        *(ptr+1)=0;
#ifdef _WIN32
        exp_arch(builder::get_arch(), eq);
#endif
        // insert key and value to the map
#ifdef C4S_DEBUGTRACE
        cerr << "variables::include - adding key="<<key<<"; value="<<eq<<endl;
#endif
        vmap[key] = string(eq);

    }while(!inc.eof());
    inc.close();
}
// ==================================================================================================
void c4s::variables::exp_arch(int arch, char *var)
{
    char *ba = strstr(var,"$$");
    while(ba) {
        if(arch==BUILD_X32) {
            ba[0]='3';
            ba[1]='2';
        }else{
            ba[0]='6';
            ba[1]='4';
        }
        var = ba;
        ba = strstr(var,"$$");
    }
}
// ==================================================================================================
string c4s::variables::expand(const string &source, bool se)
/*! Expands variables in the given source string. Variables have form $(name). Passed variables will
  always override the environment variables. Throws 'c4s_exception' if variable is not
  found.
   \param source Source string to replace
   \param se Search environment flag. If true then the environment variables are searched as well.
   \retval string expanded string.
*/
{
    ostringstream result;
    string name,enval;
    map<string,string>::iterator mi;
    size_t prev=0, strend;

    if(!vmap.size() && !se) {
        return string(source);
    }
    size_t offset = source.find("$(");
    while(offset != string::npos) {
        result << source.substr(prev,offset-prev);
        strend = source.find(')',offset+3);
        if(strend==string::npos)
            throw c4s_exception("Variable syntax error.");
        name = source.substr(offset+2,strend-offset-2);
        mi = vmap.find(name);
        if(mi == vmap.end()) {
            if(!se) {
                ostringstream os;
                os << "Variable "<<name<<" definition not found.";
                throw c4s_exception(os.str());
            }
            if(!get_env_var(name.c_str(),enval)) {
                ostringstream os;
                os << "Variable "<<name<<" not found from environment nor variable list.";
                throw c4s_exception(os.str());
            }
            result << enval;
        }
        else {
            result << mi->second;
        }
        prev = strend+1;
        offset = source.find("$(",prev);
    }
    result << source.substr(prev);
    return result.str();
}

