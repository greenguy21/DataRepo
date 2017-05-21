
#include <stdlib.h>
#include <stdio.h>

int fac(int a, int p){
  char f[8] = "       ";
  int b = 0;
  if (a == 1) b = 1;
  else {
    b = a * fac(a-1, p);
  }
  return b;
}


int main( int argc, char *argv[] )
{
  int n = 8;
  int r = 5;

  r = fac(n, 1);
  printf("%d\n", r);
  return(0);
}
