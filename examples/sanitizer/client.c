#include <stdio.h>
#include <stdlib.h>

void foo(int* ptr) {
    if(ptr[0] > 20)
        ptr[35] = 255;
}

void bar(char* s) {
    printf("%s\n", s);
}


int main() {
    int* res = malloc(32);

    int *res2 = malloc(32);

    res[0] = 255;
    res[2] = 255;
    char strn[10];
    //scanf("%10s", strn);
    //bar(strn);
    foo(res);
    //foo(res2);
    printf("Bomb is happening\n");
    res[36]=41;
    printf("Done...\n");
    free(res);
    //res[2] = 42;
    int boh = res[2];
    free(res2);
    res2[4] = 123;


    
    return 0;
}
