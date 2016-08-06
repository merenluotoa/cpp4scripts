/*******************************************************************************
_publish.cpp
Program that packs and publishes this project.
Make with "makec4s"

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

#include "c4s_all.hpp"
using namespace c4s;

//#ifdef _WIN32
//#error This program should be compiled only on Linux
//#endif

program_arguments args;

int main(int argc, char **argv)
{
    path source("./"), samples("samples/"),target;
    cout << "Cpp4Script publishing\n";

    args += argument("-v",  true, "Sets VALUE as the version of the project. Required");
    args += argument("-o",  false,"Create the documentation only");
    args += argument("-s",  false,"Skip the docbook documentation");
    args += argument("-t",  true, "Sets VALUE as the target directory where the zip files are copied in.");
    args += argument("-?",  false,"Shows this help");

    try{
        args.initialize(argc,argv,1);
    }catch(c4s_exception re){
        cerr << "Error: " << re.what() << '\n';
        args.usage();
        return 1;
    }
    if(args.is_set("-?")) {
        args.usage();
        return 0;
    }
    try{
        if(!args.is_set("-s"))
        {
            cout << "--- Creating documentation ---\n";
            process ("doxygen", "c4s-doxygen.dg")(10);
            if(args.is_set("-o"))
            {
                cout << "Ready!\n";
                return 0;
            }
        }
    }catch(c4s_exception re){
        cerr << "Error: "<<re.what()<<'\n';
        return 1;
    }

    if(!args.is_set("-v"))
    {
        cerr << "Version was not specified on command line";
        args.usage();
        return 1;
    }

    if(args.is_set("-t")) {
        target.set(args.get_value("-t"));
        if(!target.exists())
        {
            cerr << "Target dir "<<target.get_dir()<<" does not exist.\n";
            return 1;
        }
    }

    path lstname("pack.lst");
    ofstream lstfile(lstname.get_path().c_str());
    if(!lstfile) {
        cerr << "Unable to create temporary list file 'pack.lst'\n";
        return 1;
    }
    try{
        // Find the source files
        const char *extra = "README:release-notes.txt";
        path_list sources(source, "*.cpp", PLF_NONE, "_*.cpp");
        sources.add(source,"*.hpp");
        sources.add(extra,':');
        sources.add(samples,"*.cpp");
        cout << "Found "<<sources.size()<<" source files.\nPacking...";
        lstfile << sources.str('\n',false);
        lstfile.close();

        string c4sname("c4s-");
        c4sname += args.get_value("-v");
        c4sname += ".zip";
        target.set_base(c4sname);
        if(target.exists())
            target.rm();

        process szip("7z", "a -tzip -i@pack.lst");
        szip.pipe_to(&cout);
        szip += target.get_path();
        if(szip())
            cout << "ERROR: packaging sources failed!\n";

        //c4sname = "c4s-doc-";
        //c4sname += args.get_value("-v");
        //c4sname += ".zip";
        //target.set_base(c4sname);
        //if(target.exists())
        //    target.rm();

        //szip.set_args("a -tzip");
        //szip += target.get_path();
        //szip += "c4s-doc";
        //if(szip())
        //    cout << "Warning: document packaging failed.\n";
    }catch(c4s_exception re){
        cout << "Publish exception - " << re.what() << '\n';
        return 1;
    }
    cout << "Completed!\n";
    return 0;
}
