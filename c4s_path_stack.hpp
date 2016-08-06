/*******************************************************************************
c4s_path_stack.hpp
Class path_stack definition for CPP4Script library

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
#ifndef C4S_PATH_STACK
#define C4S_PATH_STACK
namespace c4s {
    // ----------------------------------------------------------------------------------------------------
    //! Stack of current directories.
    /* Designed to be used when current directory needs to be changed during program execution. Pushing
       new path causes the current directory to be stored into the stack and then current directory is
       changed to pushed directory. Popping a path reverses the action. When stack is deleted original
       path is restored.
    */
    class path_stack {
    public:
        //! Initializes an empty stack.
        path_stack() {}
        //! Destroys the stack and restores the original directory.
        ~path_stack() { if(pstack.size() > 0) pstack.front().cd(); }
        //! Pushes current dir to stack and changes to given directory
        void push(const char *cdto) { path p; p.read_cwd(); pstack.push_back(p); path::cd(cdto); }
        //! Pushes current dir to stack and changes to given directory
        void push(const path &cdto) { path p; p.read_cwd(); pstack.push_back(p); cdto.cd(); }
        //! Pushes the 'from' directory into stack and changes to 'to'
        void push(const path &to, const path &from) { pstack.push_back(from); to.cd(); }
        //! Changes into the topmost path and pops it out of stack.
        void pop() { if(pstack.size()>0) { pstack.back().cd(); pstack.pop_back(); } }
        //! Changes into first (bottom) path and removes all stack items.
        void pop_all() { pstack.front().cd(); pstack.clear(); }
        //! Retuns the first path from the 'bottom' of the stack. Stack is not altered.
        path start() { return pstack.front(); }
        //! Returns number of items in the stack.
        size_t size() { return pstack.size(); }

    protected:
        list<path> pstack;
    };

}
#endif
