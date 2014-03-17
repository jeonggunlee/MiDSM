#include <stdio.h>
#include "../src/init.h"
#include "../src/mem.h"
#include "../src/syn.h"

#define N 512
int (*a)[N], (*b)[N], (*c)[N];
extern int myhostid;
extern int hostnum;

void seqinit()
{
	int i,j;

	if (myhostid == 0) 
	{
		for (i = 0; i < N; i++) 
		{  /* printf("i = %d\n", i); */
			for (j = 0; j < N; j++)
			{
				a[i][j] = 1;
				b[i][j] = 1;
			}
		}
	}
}

int main(int argc, char **argv)
{
	int x, y, z, p, magic;
	int error;
	int local[N][N] = { 0 };

	mi_init(argc, argv);

	mi_barrier();

	a = (int (*)[N])mi_alloc(N * N * sizeof(int));
	b = (int (*)[N])mi_alloc(N * N * sizeof(int));
	c = (int (*)[N])mi_alloc(N * N * sizeof(int));

	printf("Memory Allocation Done\n");

	printf("starting barrier\n");
	mi_barrier();
	printf("exiting barrier\n"); 

	printf("Initializing matrices ....................\n");
	seqinit();
	printf("Initializing matrices done!\n");

	printf("starting barrier 2\n");
	mi_barrier();
	printf("exiting barrier 2\n");

	magic = N / hostnum;

	for (x = myhostid * magic; x < (myhostid + 1) * magic; x++)
		for (y = 0; y < N; y++)
			for (z = 0; z < N; z++)
				local[y][z] += (a[x][y] * b[x][z]);

	mi_barrier();

	for (x = 0; x < hostnum; x++) 
	{
		p = (myhostid + x) % hostnum;
		mi_lock(p);
		for (y = p * magic; y < (p + 1) * magic; y++)
		{  
			for (z = 0; z < N; z++)
			{  if (x == 0) c[y][z] = local[y][z];
				else c[y][z] += local[y][z];
			}
		}
		mi_unlock(p);
		if (x == 0) mi_barrier();
	}

	mi_barrier();


	if (myhostid == 0)
	{
		error = 0;
		for (x = 0; x < hostnum; x++)
		{
			mi_lock(x);
			for (y = x * magic; y < (x + 1) * magic; y++)
			{
				for (z = 0; z < N; z++)
				{
					if (c[y][z] != N) 
					{  error = 1;
						printf("ERROR: y = %d, z = %d, c = %d\n", y, z, c[y][z]);
					}
				}
			}
			mi_unlock(x);
		}
		if (error == 1) 
			printf("NOTE: There is some error in the program!\n");
	}
} 
