#include <cstdio>
#include "../FRCVLib/FRCVLib.h"

int main()
{
    TestClass test;
    test.SayHello();
    printf("hello from %s!\n", "UnitTests");
    return 0;
}