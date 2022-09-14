#include <stdio.h>
#include <stdlib.h>

// int plus(int op1, int op2) { return op1 + op2; }
// int minus(int op1, int op2) { return op1 - op2; }

// typedef int(Binary)(int, int);
int** f(int op, int a, int b) {
  int c;
  int *p=&a, *q=&b;
  int **w;
  int* boh = (int*) malloc(10);
  if(op < 2) {
    w = &p;
  } else if (op > 5) {
    w = &q;
  }
  else {
    *w = boh;
  }
  return w;
}
//
//void* bar(int p) {
//    void* res = malloc(p);
//    if (res)
//        return res;
//    return NULL;
//}

 int main() {
   int **ptr = f(1, 2, 3);

   printf("%p\n",ptr);
   return 0;
 }
