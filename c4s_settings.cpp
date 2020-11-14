/*******************************************************************************
c4s_settings.cpp

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
 #include "c4s_config.hpp"
 #include "c4s_exception.hpp"
 #include "c4s_logger.hpp"
 #include "c4s_settings.hpp"
 using namespace c4s;
 using namespace std;
#endif

c4s::setting_item::setting_item()
{
    //
}
c4s::setting_item::setting_item(const char *_value)
        :value(_value)
{
    //
}

// ------------------------------------------------------------------------------------------
c4s::setting_section::setting_section()
{
    // Intentionally empty
}

c4s::setting_section::setting_section(const std::string &_name)
        :name(_name)
{
    // Intentionally empty;
}

c4s::setting_section::setting_section(const std::string &_name, SETTING_TYPE type, std::istream &input)
        :name(_name)
{
    if(!read(type, input))
        throw c4s_exception("setting_section::setting_section - syntax or read failure.");
}

std::string c4s::setting_section::get_name()
{
    return name;
}

bool c4s::setting_section::is_name(const std::string &_name)
{
    return name.compare(_name)==0 ? true : false;
}

bool c4s::setting_section::read(SETTING_TYPE type, std::istream &input)
{
    if(type == SETTING_TYPE::FLAT) {
        char line[256];
        do{
            input.getline(line, sizeof(line));
            char *es = strchr(line, '=');
            if(es) {
                *es = 0;
                // Trim name
                char *end = es-1;
                while(*end==' ' || *end=='\t') {
                    *end = 0;
                    end--;
                }
                // Trim value
                es++;
                while(*es==' ' || *es=='\t') {
                    es++;
                }
                items[string(line)] = setting_item(es);
            }
        } while(!input.eof());
    }
    else {
        CS_PRINT_ERRO("setting_section::read - only flat-type settings supported");
        return false;
    }
    return true;
}
// ------------------------------------------------------------------------------------------
c4s::settings::settings()
{
    // Intentionally empty
}

c4s::settings::settings(SETTING_TYPE type, std::istream &input)
{
    if(!read(type, input))
        throw c4s_exception("settings::settings - syntax or read failure.");
}

bool c4s::settings::read(SETTING_TYPE type, std::istream &input)
{
    if(type == SETTING_TYPE::FLAT) {
        setting_section general("general");
        if(!general.read(type, input))
            return false;
        sections.push_back(general);
    }
    else {
        CS_PRINT_ERRO("settings::read - only flat-type settings supported.");
        return false;
    }
    return true;
}

setting_section& c4s::settings::get_section(const std::string &name)
{
    list<setting_section>::iterator ss;
    for(ss=sections.begin(); ss != sections.end(); ss++) {
        if(ss->is_name(name))
            return *ss;
    }
    throw runtime_error("section not found");
}

string c4s::settings::get_value(const std::string &section, const std::string &name)
{
    // find section
    list<setting_section>::iterator ss;
    for(ss=sections.begin(); ss != sections.end(); ss++) {
        if(ss->is_name(section))
            break;
    }
    if(ss==sections.end())
        return string();
    // Find item
    auto si = ss->items.find(name);
    if(si==ss->items.end())
        return string();
    return si->second.value;
}
