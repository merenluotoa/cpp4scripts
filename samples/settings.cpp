#include <iostream>
#include "../c4s_all.hpp"
program_arguments args;
typedef bool (*tfptr)();

bool test1()
{
    cout<<"Flat settings..";

    ifstream cf("settings_flat.conf");
    if(!cf) {
        cout<<"Failed - Unable to open settings_flat.conf\n";
        return false;
    }
    bool status = true;
    try {
        settings conf(SETTING_TYPE::FLAT, cf);
        if(conf.get_value("general", "setting1").compare("value of setting 1"))
            status = false;
        if(conf.get_value("general", "setting two").compare("2"))
            status = false;
        if(conf.get_value("general", "setting3").compare("third"))
            status = false;
    }
    catch(const c4s_exception &ce) {
        cout<<"Failed: "<<ce.what()<<'\n';
        return false;
    }
    if(!status) {
        cout<<"FAIL\n";
        return false;
    }
    cout<<"OK\n";
    return true;
}
// ==========================================================================================
int main(int argc, char **argv)
{
    tfptr tfunc[] = { &test1, 0 };

    try{
        args.initialize(argc,argv,0);
    }catch(const c4s_exception &ce){
        cout<<ce.what()<<'\n';
        args.usage();
        return 1;
    }

    cout<<"# Running unit tests:\n";
    int count=0, success=0;
    for(int ndx=0; tfunc[ndx]; ndx++) {
        if(tfunc[ndx]())
            success++;
        count++;
    }
    cout<<">> "<<count<<" tests completed. "<<success<<" tests successful.\n";
    return 0;
}
