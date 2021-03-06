/* { dg-require-effective-target vect_int } */
/* { dg-require-effective-target vect_float } */

#include <stdarg.h>
#include "tree-vect.h"

#define N 256

typedef float afloat __attribute__ ((__aligned__(16)));

void bar (afloat *pa, afloat *pb, afloat *pc)
{
  int i;

  /* check results:  */
  for (i = 0; i < N/2; i++)
    {
      if (pa[i] != (pb[i+1] * pc[i+1]))
	abort ();
    }

  return;
}


int
main1 (int n , afloat *  pa, afloat *  pb, afloat *  pc)
{
  int i;

  for (i = 0; i < n/2; i++)
    {
      pa[i] = pb[i+1] * pc[i+1];
    }

  bar (pa,pb,pc);

  return 0;
}

int main (void)
{
  int i;
  int n=N;
  afloat a[N];
  afloat b[N] = {0,3,6,9,12,15,18,21,24,27,30,33,36,39,42,45,48,51,54,57};
  afloat c[N] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19};

  check_vect ();

  main1 (n,a,b,c);
  return 0;
}

/* { dg-final { scan-tree-dump-times "vectorized 1 loops" 1 "vect" { xfail *-*-* } } } */
