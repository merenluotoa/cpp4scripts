/*******************************************************************************
client.cpp
Program to test the Cpp4Scripts process class
Copyright (c) Menacon Ltd
*******************************************************************************/
#include <iostream>
#include <stdio.h>
#include <string.h>
using namespace std;
#ifdef __linux
 #include <sys/types.h>
 #include <unistd.h>
#else
 #include <windows.h>
#endif
#include "../c4s_all.hpp"

int main(int argc, char **argv)
{
    cout << "C4S test client\n";
    ofstream log("client.log");
    if(!log) {
        cout << "Failed to open output log. Aborted.\n";
        return 1;
    }
#ifdef __linux
    log << "Current real UID:"<<getuid()<<'\n';
    log << "Current real GID:"<<getgid()<<'\n';
    log << "Current effective UID:"<<geteuid()<<'\n';
    log << "Current effective GID:"<<getegid()<<'\n';
    log << endl;
#endif

    if(c4s::wait_stdin(500)) {
        log << "Detected stdin input. Reading untill EOF."<<endl;
        char ch;
        while(!feof(stdin)) {
            fread(&ch,1,1,stdin);
            if(ch == '\n') log << endl;
            else log << ch;
        }
        log << "\n<< Done with input.\n"<<endl;
    }
    // Print the arguments
    for(int ndx=0; ndx<argc; ndx++) {
        log << "  Arg "<<ndx<<" : ["<<argv[ndx]<<"]\n";
    }
    cout << "Client done.\n";
    return 0;
}
