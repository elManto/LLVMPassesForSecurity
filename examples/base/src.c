#include <stdio.h>

int foo(int a);

int main() {
    int a = 0;
    int b = 0;
    scanf("%d", &a);
    b = foo(a);
    if (b > 100)
        printf("AAAAAAAA\n");
    else 
        printf("BBBBBBB\n");
}

int foo(int a) {
    a *= 2;
    a +=1;
    return a;
}
