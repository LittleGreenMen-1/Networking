#include <iostream>

using namespace std;

int main()
{
    char * a[10];
    int i;

    a[0] = "0123456789010101";
    a[1] = "Lo";
    a[2] = "oi";
    a[3] = "a";

    for(i = 0; i < 4; i++)
        cout << a[i] << '\n';
    
    a[5] = "OK";

    return 0;
}