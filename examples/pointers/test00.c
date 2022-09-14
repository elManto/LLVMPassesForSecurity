#include <stdio.h>


// int plus(int op1, int op2) { return op1 + op2; }
// int minus(int op1, int op2) { return op1 - op2; }

// typedef int(Binary)(int, int);
int** f(int op, int a, int b) {
  int c;
  int *p=&a, *q=&b;
  int **w;
  if(op < 2) {
    w = &p;
  } else {
    w = &q;
  }
  *w = &c;
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
