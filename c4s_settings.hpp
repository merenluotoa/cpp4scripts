/*******************************************************************************
c4s_settings.hpp

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

#ifndef C4S_SETTINGS_HPP
#define C4S_SETTINGS_HPP

namespace c4s {

class settings;
class setting_section;

enum class SETTING_TYPE { FLAT, SECTIONS, JSON };

// ...............................................
class setting_item
{
    friend class setting_section;
public:
    setting_item();
    std::string value;
private:
    setting_item(const char *_value);
};

// ...............................................
class setting_section
{
    friend class settings;
public:
    setting_item& create_item(const std::string &name, const std::string &value);
    bool read(SETTING_TYPE type, std::istream &input);

    std::string get_name();
    bool is_name(const std::string &_name);

    std::unordered_map<std::string, setting_item> items;

protected:
    setting_section();
    setting_section(const std::string &name);
    setting_section(const std::string &name, SETTING_TYPE type, std::istream &input);
    std::string name;
};

// ...............................................
class settings
{
public:
    settings();
    settings(SETTING_TYPE type, std::istream &input);
    bool read(SETTING_TYPE type, std::istream &input);
    setting_section& create_section(const std::string name);
    setting_section& get_section(const std::string &name);
    std::string get_value(const std::string &section, const std::string &name);

protected:
    std::list<setting_section> sections;
};

};
#endif
