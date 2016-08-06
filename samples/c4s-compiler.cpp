#include "../c4s_all.hpp"
using namespace c4s;

int parse_includes(const string fname)
{
    char block[512];
    ifstream source(fname);
    if(!source)
    {
        cout << "Unable to open!\n";
        return 0;
    }
    do {
        source.read(block);
    }while(1);
}

int main(int argc, char **argv)
{
    if(argc<2) {
        cout << "Missing source path\n";
        return 1;
    }
    try {
        path source(argv[1]);
        path_list cpps(source,"*.cpp");
        cout << "Found "<<cpps.size()<<" cpp files\n";

        path_iterator pi = cpps.begin();
        while(pi!=cpps.end()) {
            cout << pi->get_base() << ':';
            parse_includes(pi->get_path().c_str());
            pi++;
        }
    }
}
