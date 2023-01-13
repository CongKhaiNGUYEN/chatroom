#include<stdio.h>
#include <string.h>

typedef struct stt{
    char ooo[10] = NULL;
}stt;

int main(){
    stt cd;
    strcpy(cd.ooo, "1323");
    printf("%s", cd.ooo);
}