#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(){
    char *str = "/nickname Maxime\n";
    char m = '/';
    char cpstr[30];
    strcpy(cpstr, str + 10);
    printf("%s", cpstr);
    printf("%c", m);
}