#define C4S_OWN_EXCEPTIONS
#include "cpp4scripts.hpp"
#include "cpp4scripts.cpp"
using namespace c4s;

int main(int argc, char **argv)
{
    cout << "Testing cpp4scripts exceptions\n";
    try {
        // Trying to execute process that does not exist.
        process("none")();
    }catch(c4s_exception ce) {
        cerr << "Caught:"<<ce.what()<<endl;
    }
    return 0;
}
