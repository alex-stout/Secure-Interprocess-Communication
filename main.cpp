#include <iostream>
#include "encryption.hpp"
#include "settings.hpp"

using namespace std;

int main()
{
    int results = encrypt(5);
    cout << results << endl;
    getType();
}