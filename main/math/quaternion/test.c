#include "stdio.h"
#include "string.h"
#define s2 "+sdodo\r\n"
int main()
{
    char s1[100] = "+sssss\r\n";
    strcat(s1, s2);
    printf("%s", s1);
    return 0;
}