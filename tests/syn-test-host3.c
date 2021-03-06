#include "../src/net.h"
#include "../src/syn.h"
#include "../src/init.h"
#include <string.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/socket.h>


//int myhostid;
//int hostnum;
//host_t hosts[MAX_HOST_NUM];

int main(int argc, char **argv){
	//myhostid = 0;
	//strcpy(hosts[0].address, "192.168.48.42");
	//strcpy(hosts[0].username, "yating");
	//strcpy(hosts[1].address, "192.168.48.40");
	//strcpy(hosts[1].username, "zeyu");
	//hostnum = 2;


	//initnet();
	//initsyn();
	mi_init(argc, argv);
	
	printf("enter barrier\n");
	mi_barrier();
	printf("exit barrier\n");

	int i, j, result;
	result = 0;
	printf("grasp lock 0\n");
	mi_lock(0);
	printf("grasp lock 0 successfully\n");
	for(i = 0; i < 10000; i++){
		for(j = 0; j < 100; j++){
			result++;
		}
	}
	printf("free lock 0\n");
	mi_lock(0);
	printf("free lock 0 successfully\n");
	
	printf("grasp lock 1\n");
	mi_lock(1);
	printf("grasp lock 1 successfully\n");

	printf("enter barrier\n");
	mi_barrier();
	printf("exit barrier\n");
	printf("result = %d\n",result);
}
