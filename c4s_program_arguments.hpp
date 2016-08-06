/*******************************************************************************
c4s_probram_arguments.hpp
Defines two classes: argument and program_arguments for Cpp4Scripts library

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

#ifndef C4S_PROBRAM_ARGUMENTS_HPP
#define C4S_PROBRAM_ARGUMENTS_HPP

namespace c4s {

    // ----------------------------------------------------------------------------------------------------
    //! Single program argument given at the command line
    class argument
    {
    public:
        //! Default constructor initializes all values to zero.
        argument() { text=0; two_part=false; set=false; }
        //! Defines possible program argument text, type and info
        /*! \param aTxt Argument text, e.g. "--type" or "-t"
          \param aTp Two part flag, if true then argument has second part, e.g. "--path /usr/local/src".
          \param aInfo Information text that can be shown about the argument to the user when help is requested. */
        argument(const char *aTxt, bool aTp, const char *aInfo) { text=aTxt; two_part=aTp; info=aInfo; set=false; }

        //! Returns possible argument text.
        const char* get_text() const { return text; }
        //! Returns argument information
        const char* get_info() const { return info.c_str(); }
        //! Fills the possible second part into the given string.
        void get_value(string &aStr) const { aStr = value; }
        //! Returns the current value of this argument. Value is defined if this is two_part and value argument is set.
        string get_value() const { return value; }

        //! Sets argument into ON state i.e. has been specified in the command line.
        void set_on() { set=true; }
        void set_value(char *val);
        //! Appends the given string into the argument second part.
        void append_value(char *val) { value += val; }
        //! Returns true if the argument has two parts.
        bool is_two_part() const { return two_part; }
        //! Returns true if the argument is ON i.e specified in command line.
        bool is_on() const { return set; }
        //! Returns true if argument names match
        bool operator== (const char *t) const;
    private:
        const char *text;   //!< Argument name or text.
        bool two_part;      //!< If true argument has value associated.
        bool set;           //!< If true argument has been specified in command line.
        string value;       //!< Value of the argument.
        string info;        //!< Help information
    };

    // ----------------------------------------------------------------------------------------------------
    //! Manages all program arguments / arguments.
    class program_arguments
    {
    public:
        //! Constructor initializes attributes to default values. Arg list will be empty.
        program_arguments() {}
        //! Initializes the program argument list by matching user parameters to stored argument list.
        void initialize(int argc, char *argv[], int min_args=0);
        //! Appends the given argument into the list.
        void append(const argument &arg) { arguments.push_back(arg); }
        //! Appends the given argument into the list.
        void operator+=(const argument &arg) { arguments.push_back(arg); }
        //! Checks if the given argument has been given in the command line.
        bool is_set(const char *);
        //! Checks if the given argument has been given in the command line and has specified value.
        bool is_value(const char*, const char*);
        //! Sets argument's value.
        bool set_value(const char*, char*);
        //! Appends a string to the end of the argument's current value.
        bool append_value(const char*, char*);
        //! Returs argument value if the argument was of two part type.
        const char* get_value_ptr(const char*);
        //! Returns value of the argument if it was of two part type.
        string get_value(const char*);
        //! Returns the index of the choises for the argument value that matches given choises.
        int  get_value_index(const char*, const char **);
        //! Stores the second part for the argument and sets argument on. Strips single quotes from the value before storing the value.
        void usage();

        typedef list<argument>::iterator pai_t;
        //! Returns the iterator to first argument.
        pai_t begin() { return arguments.begin(); }
        //! Returns iterator to the end of the list.
        pai_t end() { return arguments.end(); }

        path argv0;     //<! Path to program as reported with argv[0] parameter
        path cwd;       //<! Current directory at the program start.
        path exe;       //<! Path to binary that was used to launch current process

    protected:
        list<argument> arguments;
    };
}
#endif
