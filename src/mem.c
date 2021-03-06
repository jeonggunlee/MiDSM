#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "mem.h"
#include "net.h"
#include "syn.h"

extern int myhostid;
extern int hostnum;
extern milock_t locks[LOCK_NUM];

page_t pageArray[MAX_PAGE_NUM];
proc_t procArray[MAX_HOST_NUM];
interval_t *intervalNow;
int fetchPageWaitFlag, fetchDiffWaitFlag, fetchWNIWaitFlag; 
int pagenum;
long mapfd;
void *globalAddress;
int barrierTimestamps[MAX_HOST_NUM][MAX_HOST_NUM];


void segv_handler(int signo, siginfo_t *info, void *context){
	void *faultaddress = info->si_addr;
	void *addr = ((long)faultaddress % PAGESIZE == 0) ? faultaddress : (void *)((long)faultaddress / PAGESIZE * PAGESIZE);
	int pageIndex = ((long)addr - START_ADDRESS) / PAGESIZE;
	if(pageIndex < 0 || pageIndex >= MAX_PAGE_NUM){
		fprintf(stderr, "Segmentation fault\n");
		exit(1);
	}
	if(pageArray[pageIndex].state == RDONLY){
		printf("RDONLY: createWritenotice for page %d\n", pageIndex);
		createTwinPage(pageIndex);
		createWriteNotice(pageIndex);
		pageArray[pageIndex].state = WRITE;
		if(mprotect(pageArray[pageIndex].address, PAGESIZE, PROT_READ | PROT_WRITE) == -1)
			fprintf(stderr, "RDONLY mprotect error\n");
	}else if(pageArray[pageIndex].state == MISS){
		printf("MISS: fetchPage for page %d\n", pageIndex);
		fetchPage(pageIndex);	
		pageArray[pageIndex].state = RDONLY;
		if(mprotect(pageArray[pageIndex].address, PAGESIZE, PROT_READ) == -1)
			fprintf(stderr, "MISS mprotect error\n");
	}else if(pageArray[pageIndex].state == WRITE){
		;		// no action
	}else if(pageArray[pageIndex].state == INVALID){
		printf("INVALID: fetchDiff\n");
		fetchDiff(pageIndex);
		pageArray[pageIndex].state = RDONLY;
		if(mprotect(pageArray[pageIndex].address, PAGESIZE, PROT_READ) == -1)
			fprintf(stderr, "INVALID mprotect error\n");
	}else{
		fprintf(stderr, "Segmentation fault\n");
		exit(1);
	}	
}


/**
* This procedure will allocate a block of memory to user, whose size is a multiple of PAGESIZE and bigger than 'size'.
* parameters
*	size : memory size that user want to get 
* return value
*	NOT NULL --- start address of the allocated memory
*	NULL     --- no enough memory to allocate or parameters error
**/
void *mi_alloc(int size){
	if(size <= 0){
		return NULL;
	}
	int rsize = (size % PAGESIZE == 0) ? size : (size / PAGESIZE + 1) * PAGESIZE;
	int allocesize = rsize;
	if(rsize + globalAddress > (void *)MAX_MEM_SIZE){
		fprintf(stderr, "No enough space to allocate!\n");
		return NULL;
	}
	while(allocesize > 0){
		if((pagenum % hostnum) == myhostid){
			mmap(START_ADDRESS + globalAddress, 4096, PROT_READ , MAP_PRIVATE | MAP_FIXED, mapfd, 0);	
			pageArray[pagenum].state = RDONLY;
		}else{
			pageArray[pagenum].state = MISS;
			mprotect(START_ADDRESS + globalAddress, PAGESIZE, PROT_READ);
		}
		pageArray[pagenum].address = START_ADDRESS + globalAddress;
		pagenum++;
		allocesize -= PAGESIZE;
		globalAddress += PAGESIZE;
	}
	return START_ADDRESS + globalAddress - rsize;
}


/**
* initialization work of memory management before the system is normally used 
**/
void initmem(){
/*******************install segv signal handler*************/
	struct sigaction act;
	act.sa_handler = (void (*)(int))segv_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;
	sigaction(SIGSEGV, &act, NULL);
	//printf("initialize segv_handler\n");
/*******************initialize pageArray********************/
	memset(pageArray, 0, sizeof(page_t) * MAX_PAGE_NUM);
	//printf("initialize pageArray\n");
/*******************initialize procArray********************/
	memset(procArray, 0, sizeof(proc_t) * MAX_HOST_NUM);
	int i;
	for(i = 0; i < hostnum; i++){
		procArray[i].hostid = i;
	}
	//printf("initialize procArray\n");
/*******************prepare mapped file*********************/	
	mapfd = open("/dev/zero", O_RDWR, 0);
	//printf("initialize mapfd\n");
/*******************initialize other global variables*******/	
	globalAddress = NULL;
	pagenum = 0;
	fetchPageWaitFlag = 0;
	fetchDiffWaitFlag = 0;
	fetchWNIWaitFlag = 0;
	//printf("initialize other global variables\n");
/*******************initialize intervalNow******************/
	intervalNow = malloc(sizeof(interval_t));
	memset(intervalNow, 0, sizeof(interval_t));
	intervalNow->isBarrier = 1;
	procArray[myhostid].intervalList = intervalNow;
	for(i = 0; i < MAX_HOST_NUM; i++){
		int j;
		for(j = 0; j < MAX_HOST_NUM; j++){
			barrierTimestamps[i][j] = -1;
		}
	}
	//printf("initialize intervalNow\n");
}


/**
* This procedure will send a FETCH_PAGE msg to a host, whose id can be generated by 2 ways: pageIndex % hostnum(When this page does not have writenotice); hostid whose interval is latest in this page notices(When this page has writenotice). This procedure will be blocked when waiting for reply.
* parameters
*	pageIndex : index in pageArray
* return value
*	 0 --- success
*	-1 --- parameters error 
**/
int fetchPage(int pageIndex){
		if(pageIndex < 0 || pageIndex >= MAX_PAGE_NUM){
			return -1;
		}
		if(pageArray[pageIndex].state != MISS){
			return -1;
		}
		int hasWriteNotice = 0; // a flag that indicates whether this page has received writenotices
		int i;
		for(i = 0; i < hostnum; i++){
			if(i == myhostid){
				continue;
			}
			if(pageArray[pageIndex].notices[i] != NULL){
				hasWriteNotice = 1; // this page has received writenotices
			}
		}
		int hostid;
		if(hasWriteNotice == 0){ // send msg to original page holder
			hostid = pageIndex % hostnum;
		}else{			// send msg to the host which has the latest page
			interval_t *latestInterval = NULL;
			for(i = 0; i < hostnum; i++){
				if(i == myhostid){
					continue; // When page state is MISS, notices[myhostid] == NULL
				}
				writenotice_t *wn = pageArray[pageIndex].notices[i];  
				if(wn != NULL){
					if(latestInterval == NULL){
						latestInterval = wn->interval;
						hostid = i;
					}else{
						//compare interval of i to latestInterval
						if(isAfterInterval(wn->interval->timestamp, latestInterval->timestamp) == 1){
							latestInterval = wn->interval;
							hostid = i;
						}
					}
				}
			}
		}
		mimsg_t *msg = nextFreeMsgInQueue(0);
		msg->from = myhostid;
		msg->to = hostid;
		msg->command = FETCH_PAGE;
		apendMsgData(msg, (char *)&pageIndex, sizeof(int));
		fetchPageWaitFlag = 1;
		sendMsg(msg);
		while(fetchPageWaitFlag)
			;
		return 0;
}


/**
* create twin page for page in pageArray with index 'pageIndex', and store address of twin page in pageArray['pageIndex'].twinPage
* parameters
*	pageIndex : index in pageArray
* return value
*	 0 --- success
*	-1 --- parameter error
**/
int createTwinPage(int pageIndex){
	if(pageIndex < 0 || pageIndex >= MAX_PAGE_NUM){
		return -1;
	}
	if(pageArray[pageIndex].state == UNMAP || pageArray[pageIndex].state == MISS){
		return -1;
	}
	if(pageArray[pageIndex].address == NULL || pageArray[pageIndex].twinPage != NULL){
		return -1;
	}
	pageArray[pageIndex].twinPage = malloc(PAGESIZE);
	bcopy(pageArray[pageIndex].address, pageArray[pageIndex].twinPage, PAGESIZE);
	return 0;
}


/**
* free the memory area that arrayPage['pageIndex'].twinPage points to
* parameters
*	pageIndex : index in pageArray
* return value
*	 0 --- success
*	-1 --- parameters error
**/
int freeTwinPage(int pageIndex){
	if(pageIndex < 0 || pageIndex >= MAX_PAGE_NUM){
		return -1;
	}
	if(pageArray[pageIndex].state == UNMAP || pageArray[pageIndex].state == MISS){
		return -1;
	}
	if(pageArray[pageIndex].address == NULL || pageArray[pageIndex].twinPage == NULL){
		return -1;
	}
	free(pageArray[pageIndex].twinPage);
	pageArray[pageIndex].twinPage = NULL;
	return 0;
}


/**
* This procedure will fetch all the missing diff for page 'pageIndex' and it will be blocked when waiting for responses.
* parameters
*	pageIndex : index in pageArray
* return value
*	 0 --- success
*	-1 --- parameters error
**/
int fetchDiff(int pageIndex){
	if(pageIndex < 0 || pageIndex >= MAX_PAGE_NUM){
		return -1;
	}
	if(pageArray[pageIndex].state != INVALID){
		return -1;
	}
	int i, j;
	int *pageTimestamp = pageArray[pageIndex].startInterval->timestamp;
	for(i = 0; i < hostnum; i++){
		if(i == myhostid){
			continue;
		}
		writenotice_t *wn = pageArray[pageIndex].notices[i];
		while((wn != NULL) && (wn->diffAddress == NULL)){
			if(pageTimestamp == NULL || isAfterInterval(wn->interval->timestamp, pageTimestamp) == 1){
				mimsg_t *msg = nextFreeMsgInQueue(0);
				msg->from = myhostid;
				msg->to = i;
				msg->command = FETCH_DIFF;
				apendMsgData(msg, (char *)&pageIndex, sizeof(int));	
				apendMsgData(msg, (char *)wn->interval->timestamp, sizeof(int) * MAX_HOST_NUM);
				fetchDiffWaitFlag = 1;
				sendMsg(msg);
				while(fetchDiffWaitFlag)
					;
			}
			wn = wn->nextInPage;	
		}
	}
	return 0;
}


/**
* This procedure will send a FETCH_WN_I to 'hostid' and it will be blocked when waiting for response.
* parameters
*	hostid : id of the destination host 
* return value
*	 0 --- success
*	-1 --- parameters error
**/
int fetchWritenoticeAndInterval(int hostid){
	if(hostid < 0 || hostid >= hostnum){
		return -1;
	}
	if(hostid == myhostid){
		return -1;
	}
	mimsg_t *msg = nextFreeMsgInQueue(0);
	msg->from = myhostid;
	msg->to = hostid;
	msg->command = FETCH_WN_I;
	apendMsgData(msg, (char *)intervalNow->timestamp, sizeof(int) * MAX_HOST_NUM);
	fetchWNIWaitFlag = 1;
	sendMsg(msg);
	while(fetchWNIWaitFlag)
		;
	return 0;
}


/**
* This procedure will send a GRANT_WN_I msg to 'hostid'. It will also append 'intervalNow->timestamp' to msg. All intervals which after 'timestamp' and their related writenotices will also be saved to msg.
* parameters
*	hostid    : destination host
*	timestamp : a pointer to a timestamp array.
* return value
*	 0 --- success
*	-1 --- parameters error
**/
int grantWNI(int hostid, int *timestamp){
	if(hostid < 0 || hostid >= hostnum || timestamp == NULL){
		return -1;
	}
	if(hostid == myhostid){
		return -1;
	}
	
	mimsg_t *msg = nextFreeMsgInQueue(0);
	msg->from = myhostid;
	msg->to = hostid;
	msg->command = GRANT_WN_I;
	int i;
	for(i = 0; i < MAX_HOST_NUM; i++){
		msg->timestamp[i] = intervalNow->timestamp[i];
	}
	int packetNum = 0;
	apendMsgData(msg, (char *)&packetNum, sizeof(int)); //occupy the packetNum slot
	wnPacket_t packet;
	for(i = 0; i < hostnum; i++){
		proc_t proc = procArray[i];
		interval_t *intervalLast = proc.intervalList;
		if(intervalLast == NULL){
			continue;
		}
		while(intervalLast->next != NULL){
			intervalLast = intervalLast->next;
		}
		while(intervalLast != NULL){
			if(isAfterInterval(intervalLast->timestamp, timestamp) && (intervalLast->notices != NULL)){
				writenotice_t *left = addWNIIntoPacketForHost(&packet, i, intervalLast->timestamp, intervalLast->notices);
				apendMsgData(msg, (char *)&packet, sizeof(wnPacket_t));
				packetNum++;
				while(left != NULL){
					left = addWNIIntoPacketForHost(&packet, i, intervalLast->timestamp, left);
					apendMsgData(msg, (char *)&packet, sizeof(wnPacket_t));
					packetNum++;
				}
			}
			intervalLast = intervalLast->prev;
		}
	}
	//refresh the packetNum value
	memcpy(msg->data, &packetNum, sizeof(int));
	sendMsg(msg);
	return 0;
}


/**
* This procedure will put a page into a GRANT_PAGE msg, and send it to 'hostid'.
* parameters
*	hostid    : destination host
*	pageIndex : index of a page, which will be sent to 'hostid'
* return value
*	 0 --- success
*	-1 --- parameters error
**/
int grantPage(int hostid, int pageIndex){
	if(hostid < 0 || hostid >= hostnum){
		return -1;
	}
	if(hostid == myhostid){
		return -1;
	}
	if(pageIndex < 0 || pageIndex >= MAX_PAGE_NUM){
		return -1;
	}
	if(pageArray[pageIndex].state == UNMAP || pageArray[pageIndex].state == MISS){
		return -1;
	}

	mimsg_t *msg = nextFreeMsgInQueue(0);
	msg->from = myhostid;
	msg->to = hostid;
	msg->command = GRANT_PAGE;
	apendMsgData(msg, (char *)&pageIndex, sizeof(int));
	apendMsgData(msg, (char *)pageArray[pageIndex].address, PAGESIZE);
//	printf("grantPage: msg->command = %d\n", msg->command);
	sendMsg(msg);
	return 0;
}


/**
* This procedure will put a diff which comes from a page with index 'pageIndex' to a msg, if diff does not exist, this procedure will create one. Then, it will send a GRANT_DIFF msg to 'hostid'.
* parameters
*	hostid    : destination host
*	timestamp : a pointer to a timestamp array.
*	pageIndex : index of a page, whose diff will be sent to 'hostid'
* return value
*	 0 --- success
*	-1 --- parameters error
*	-2 --- no such page
*	
**/
int grantDiff(int hostid, int *timestamp, int pageIndex){
	if(hostid < 0 || hostid >= hostnum || hostid == myhostid){
		return -1;
	}
	if(timestamp == NULL){
		return -1;
	}
	if(pageIndex < 0 || pageIndex >= MAX_PAGE_NUM){
		return -1;
	}
	if(pageArray[pageIndex].state == UNMAP || pageArray[pageIndex].state == MISS){
		return -1;
	}
	writenotice_t *wn = pageArray[pageIndex].notices[myhostid];

	int i;
	int find = 1;
	while(wn != NULL){
		find = 1;
		for(i = 0; i < MAX_HOST_NUM; i++){
			if(wn->interval->timestamp[i] != timestamp[i]){
				find = 0;
				break;
			}
		}
		if(find == 1){
			break;
		}
		wn = wn->nextInPage;
	}
	if(wn != NULL && find == 1){//find writenotice
		if(wn->diffAddress == NULL){//diff has not been created
			wn->diffAddress = createLocalDiff(pageArray[pageIndex].address, pageArray[pageIndex].twinPage);
			if(pageArray[pageIndex].state == WRITE){
				freeTwinPage(pageIndex);
				pageArray[pageIndex].state = RDONLY;
				mprotect(pageArray[pageIndex].address, PAGESIZE, PROT_READ);	
			}
		}
		mimsg_t *msg = nextFreeMsgInQueue(0);
		msg->from = myhostid;
		msg->to = hostid;
		msg->command = GRANT_DIFF;
		apendMsgData(msg, (char *)timestamp, sizeof(int) * MAX_HOST_NUM);
		apendMsgData(msg, (char *)&pageIndex, sizeof(int));
		apendMsgData(msg, wn->diffAddress, PAGESIZE);
		sendMsg(msg);
		return 0;
	}else{// not find writenotice
		return -2;
	}
}


/**
* add the memory block pointed by 'diffAddress' to the memory block pointed by 'pageAddress', the sizes of whose are PAGESIZE
* parameters
*	pageAddress : pointer of targeted memory block
*	diffAddress : pointed of twinPage
* return value
*	 0 --- success
*	-1 --- parameters error 
**/
int applyDiff(void *pageAddress, void *diffAddress){
	if(pageAddress == NULL || diffAddress == NULL){
		return -1;
	}
	int i;
	for(i = 0; i < PAGESIZE; i++){
		*((char *)pageAddress+i) = *((char *)pageAddress+i) + *((char *)diffAddress+i);
	}
	return 0;
}


/**
* allocate a new memory block whose size is equal to PAGESIZE, and save a value into the 'i'th element which is the result of 'i'th element in 'pageAddress' minus 'i'the element in 'twinAddress'.  
* parameters
*	pageAddress : pointer of a memory block
*	twinAddress : pointer of a memory block
* return value
*	NOT NULL --- success
*	NULL     --- parameters error
**/
void *createLocalDiff(void *pageAddress, void *twinAddress){
	if(pageAddress == NULL || twinAddress == NULL){
		return NULL;
	}
	void *diffAddress = malloc(PAGESIZE);
	int i;
	for(i = 0; i < PAGESIZE; i++){
		*((char *)diffAddress+i) = *((char *)pageAddress+i) - *((char *)twinAddress+i);
	}
	return diffAddress;

}


/**
* This procedure incorporates an interval and its writenotices into local data structures. The page which received writenotice will become 'INVALID' and create local diff.
* parameters
	packet : a pointer to the struct that encapsulates interval and writenotices
* return value
*	 0 --- success
*	-1 --- parameters error
*	-2 --- this packet has already been incorporated
**/
int incorporateWnPacket(wnPacket_t *packet){
//	printf("entering incorporateWnPacket\n");
	if(packet == NULL){
		return -1;
	}
	int hostid = packet->hostid;
	int *timestamp = packet->timestamp;
	int wnCount = packet->wnCount;
	interval_t *interval = procArray[hostid].intervalList;
	int find = 1;
	int i;
	while(interval != NULL){
		find = 1;
		for(i = 0; i < MAX_HOST_NUM; i++){
			if(interval->timestamp[i] != timestamp[i]){
				find = 0;
				break;
			}	
		}
		if(find == 1){
			break;
		}else{
			interval = interval->next;
		}
	}
	writenotice_t *lastwn = NULL;
	if(find == 1 && interval != NULL){
		lastwn = interval->notices;
		while(lastwn != NULL && lastwn->nextInInterval != NULL){
			lastwn = lastwn->nextInInterval;
		}	
	}else{
		interval = malloc(sizeof(interval_t));
		interval->notices = NULL;
		interval->next = NULL;
		interval->prev = NULL;
		interval->isBarrier = 0;
		for(i = 0; i < MAX_HOST_NUM; i++){
			(interval->timestamp)[i] = timestamp[i];
		}
		interval->next = procArray[hostid].intervalList;
		if(procArray[hostid].intervalList != NULL){
			procArray[hostid].intervalList->prev = interval;
		}
		procArray[hostid].intervalList = interval;
	}
//	printf("wnCount = %d\n", wnCount);
	for(i = 0; i < wnCount; i++){
		int pageIndex = (packet->wnArray)[i];
		//check whether this writenotice exists
		writenotice_t *tempwn = interval->notices;
		int wnExistFlag = 0;
		while(tempwn != NULL && wnExistFlag != 1){
			if(tempwn->pageIndex == pageIndex){
				wnExistFlag = 1;
			}
			tempwn = tempwn->nextInInterval;
		}
		if(wnExistFlag == 1){
			continue; // write notice exists, go to next interation.
		}
		
//		printf("add writenotice for page %d\n", pageIndex);
		if(pageArray[pageIndex].state == RDONLY){
//			printf("page %d : receive writenotice, RDONLY TO INVALID\n", pageIndex);
			pageArray[pageIndex].state = INVALID;	
			mprotect(pageArray[pageIndex].address, PAGESIZE, PROT_NONE);
		}else if(pageArray[pageIndex].state == WRITE){
//			printf("page %d : receive writenotice, WRITE TO INVALID\n", pageIndex);
			pageArray[pageIndex].state = INVALID;	
			pageArray[pageIndex].notices[myhostid]->diffAddress = createLocalDiff(pageArray[pageIndex].address, pageArray[pageIndex].twinPage);
			freeTwinPage(pageIndex);
			mprotect(pageArray[pageIndex].address, PAGESIZE, PROT_NONE);
		}else if(pageArray[pageIndex].state == MISS){
			;
		}else if(pageArray[pageIndex].state == INVALID){
			;
		}
		writenotice_t *wn = malloc(sizeof(writenotice_t));
		wn->interval = interval;
		wn->nextInPage = pageArray[pageIndex].notices[hostid];
		pageArray[pageIndex].notices[hostid] = wn;
		wn->diffAddress = NULL;
		wn->nextInInterval = NULL;
		wn->pageIndex = pageIndex;
		if(lastwn == NULL){
			interval->notices = wn;
		}else{
			lastwn->nextInInterval = wn;
		}
		lastwn = wn;
	}
	return 0;
}


/**
* create a local writenotice for a page element in pageArray which is indexed by 'pageIndex'
* parameters
*	pageIndex : index in pageArray
* return value
*	 0 --- success
*	-1 --- parameters error 
**/
int createWriteNotice(int pageIndex){
	if(pageIndex < 0 || pageIndex >= MAX_PAGE_NUM){
		return -1;
	}
	if(pageArray[pageIndex].state != RDONLY){
		return -1;
	}
	
	writenotice_t *wn = malloc(sizeof(writenotice_t));
	wn->interval = intervalNow;
	wn->nextInPage = NULL;
	wn->nextInInterval = NULL;
	wn->diffAddress = NULL;
	wn->pageIndex = pageIndex;

	//printf("createWriteNotice : pageIndex = %d\n", pageIndex);
	wn->nextInPage = pageArray[pageIndex].notices[myhostid];
	pageArray[pageIndex].notices[myhostid] = wn;
	writenotice_t *temp = intervalNow->notices;
	if(temp == NULL){
		intervalNow->notices = wn;
	}else{
		while(temp->nextInInterval != NULL){
			temp = temp->nextInInterval;	
		}
		temp->nextInInterval = wn;
	}
	return 0;
}


/**
* This procedure will be invoked by dispatchMsg to handle FETCH_PAGE msg. It will invoke grantPage to send a page content back to the sender.
* parameters
*	msg : msg to be handled
**/
void handleFetchPageMsg(mimsg_t *msg){
	int from = msg->from;
	int pageIndex = *((int *)msg->data);
	//printf("fetchPage msg pageIndex = %d\n", pageIndex);
	//printf("msg from = %d\n", from);
	grantPage(from, pageIndex);
}


/**
* This procedure will be invoked by dispatchMsg to handle FETCH_DIFF msg. It will invoke grantDiff to send a diff content back to the sender.
* parameters
*	msg : msg to be handled 
**/
void handleFetchDiffMsg(mimsg_t *msg){
	int from = msg->from;
	int pageIndex = *((int *)msg->data);
	int *timestamp = (int *)(msg->data + sizeof(int));
	//printf("fetchDiff msg pageIndex = %d\n", pageIndex);
	//printf("fetchDiff msg timestamp = [");
	//int i;
	//for(i = 0; i < hostnum; i++){
	//	printf("%d ", timestamp[i]);
	//}
	//printf("]\n");
	grantDiff(from, timestamp, pageIndex);
}


/**
* This procedure will be invoked by dispatchMsg to handle FETCH_WN_I msg. It will invoke grantWNI.
* parameters
*	msg : msg to be handled
**/
void handleFetchWNIMsg(mimsg_t *msg){
	int from = msg->from;
	int *timestamp = (int *)msg->data;
	//printf("fetchWNI msg timestamp = [");
	//int i;
	//for(i = 0; i < hostnum; i++){
	//	printf("%d ", timestamp[i]);
	//}
	//printf("]\n");
	grantWNI(from, timestamp);
}


/**
* This  procedure will be invoked by dispatchMsg. It will copy diff to a writenotice and apply the diff to its page. After that, this procedure will set fetchDiffWaitFlag to 0.
* parameters
*	msg : msg to be handled
**/
void handleGrantDiffMsg(mimsg_t *msg){
	if(msg == NULL){
		return;
	}
	int hostid = msg->from;
	int *timestamp = (int *)msg->data; 
	int pageIndex = *((int *)(msg->data + sizeof(int) * MAX_HOST_NUM));
	void *diffAddress = msg->data + sizeof(int) * (MAX_HOST_NUM + 1);
	
	//printf("grantDiff msg pageIndex = %d\n", pageIndex);
	//printf("grantDiff msg timestamp = [");
	//int i;
	//for(i = 0; i < hostnum; i++){
	//	printf("%d ", timestamp[i]);
	//}
	//printf("]\n");




	int find = 1;
	int i;
	writenotice_t *wn = pageArray[pageIndex].notices[hostid];
	while(wn != NULL){
		find = 1;
		for(i = 0; i < MAX_HOST_NUM; i++){
			if(wn->interval->timestamp[i] != timestamp[i]){
				find = 0;
				break;
			}
		}
		if(find == 1){
			break;
		}
		wn = wn->nextInPage;
	}
	if(wn != NULL && find == 1){//find writenotice
		if(wn->diffAddress == NULL){//diff has not been created
			wn->diffAddress = malloc(PAGESIZE);
			memset(wn->diffAddress, 0, PAGESIZE);

			bcopy(diffAddress, wn->diffAddress, PAGESIZE);
		}
	//	printf("applying diff\n");
		mprotect(pageArray[pageIndex].address, PAGESIZE, PROT_READ | PROT_WRITE);
		applyDiff(pageArray[pageIndex].address, diffAddress);
		mprotect(pageArray[pageIndex].address, PAGESIZE, PROT_NONE);
	}
	fetchDiffWaitFlag = 0;
	return;
}


/**
* This procedure will be invoked by dispatchMsg. It will save page content from msg.
* parameters
*	msg : msg to be handled  
**/
void handleGrantPageMsg(mimsg_t *msg){
	if(msg == NULL){
		return;
	}
	int pageIndex = *((int *)msg->data);
	void *pageAddress = msg->data + sizeof(int);
	//printf("grantPage msg pageIndex = %d\n", pageIndex);




	mmap(pageArray[pageIndex].address, PAGESIZE, PROT_READ | PROT_WRITE , MAP_PRIVATE | MAP_FIXED, mapfd, 0);
	bcopy(pageAddress, pageArray[pageIndex].address, PAGESIZE);
	pageArray[pageIndex].startInterval = intervalNow;
	fetchPageWaitFlag = 0;
}


/**
* This procedure will be invoked by dispatchMsg. It will incoporates all wnPacket_t in 'msg' to local data structure.
* parameters
*	msg : msg to be handled  
**/
void handleGrantWNIMsg(mimsg_t *msg){
	//printf("entering handleGrantWNIMsg\n");
	if(msg == NULL){
		return;
	}	
	intervalNow = malloc(sizeof(interval_t));
	memset(intervalNow, 0, sizeof(interval_t));

//	printf("grantWNI msg timestamp = [");
//	int i;
//	for(i = 0; i < hostnum; i++){
//		printf("%d ", msg->timestamp[i]);
//	}
//	printf("]\n");





	int i;
	for(i = 0; i < MAX_HOST_NUM; i++){
		intervalNow->timestamp[i] = msg->timestamp[i];
	}
	(intervalNow->timestamp[myhostid])++;
	intervalNow->next = procArray[myhostid].intervalList;
	if(procArray[myhostid].intervalList != NULL){
		procArray[myhostid].intervalList->prev = intervalNow;
	}
	procArray[myhostid].intervalList = intervalNow;
	
	int packetNum = *((int *)msg->data);
//	printf("packetNum = %d\n",packetNum);
	wnPacket_t *packet =(wnPacket_t *)(msg->data + sizeof(int));
	for(i = 0; i < packetNum; i++){
		incorporateWnPacket(packet+i);
	}
	fetchWNIWaitFlag = 0;
}


/**
* compare two vector timestamps, if every element in 'timestamp' is bigger than or equal to (at least one bigger) the corresponding element of 'targetTimestamp', this procedure will return 1, otherwise return 0
* parameters
*	timestamp : an int array with size equal to MAX_HOST_NUM
*	targetTimestamp : an int array with size equal to MAX_HOST_NUM
* return value
*	 0 --- negative
*	 1 --- positive
*	-1 --- parameters error
**/
int isAfterInterval(int *timestamp, int *targetTimestamp){
	if((timestamp == NULL) || (targetTimestamp == NULL)){
		return -1;
	}
	int i;
	int bigger = 0;
	for(i = 0; i < hostnum; i++){
		if(timestamp[i] > targetTimestamp[i]){
			bigger = 1;
		}else if(timestamp[i] < targetTimestamp[i]){
			return 0;
		}
	}
	if(bigger == 1){
		return 1;
	}else{
		return 0;
	}
}


/**
* This procedure will be invoked when releasing a lock. It will make a new interval and point it using 'intervalNow'
**/
void addNewInterval(){
	//printf("addNewInterval disableSigio\n");
	disableSigio();
	intervalNow = malloc(sizeof(interval_t));
	memset(intervalNow, 0, sizeof(interval_t));
	int i;
	for(i = 0; i < hostnum; i++){
		(intervalNow->timestamp)[i] = (procArray[myhostid].intervalList->timestamp)[i];
	}
	(intervalNow->timestamp)[myhostid]++;
	intervalNow->notices = NULL;
	intervalNow->next = procArray[myhostid].intervalList;
	intervalNow->prev = NULL;
	if(procArray[myhostid].intervalList != NULL){
		procArray[myhostid].intervalList->prev = intervalNow;
	}
	procArray[myhostid].intervalList = intervalNow;
	intervalNow->isBarrier = 0;
	enableSigio();
}


/**
* This procedure will initialize a wnPacket with hostid, timestamp and related writenotices. If the number of notices is bigger than MAX_WN_NUM, the writenotice which is not be added to the wnPacket will be returned.
* parameters
*	packet 	   : a pointer to the wnPacket which will be initialized
*	hostid	   : id of a host
*	timestamp  : a pointer to a vector timestamp
*	notices    : writenotice linked list
* return value
*	 NOT NULL --- the pointer to the writenotice linked list that is not be added to the wnPacket
*	     NULL --- success
**/
writenotice_t *addWNIIntoPacketForHost(wnPacket_t *packet, int hostid, int *timestamp, writenotice_t *notices){
	if(packet == NULL || timestamp == NULL || notices == NULL){
		return NULL;
	}
	if(hostid < 0 || hostid >= hostnum){
		return NULL;
	}
	packet->hostid = -1;
	memset(packet->timestamp, 0, MAX_HOST_NUM * sizeof(int));
	packet->wnCount = 0;
	int i;
	for(i = 0; i < MAX_WN_NUM; i++){
		packet->wnArray[i] = -1;
	}
	packet->hostid = hostid;
	for(i = 0; i < MAX_HOST_NUM; i++){
		packet->timestamp[i] = timestamp[i];
	}
	while((notices != NULL) && (packet->wnCount < MAX_WN_NUM)){
		packet->wnArray[packet->wnCount] = notices->pageIndex;
		(packet->wnCount)++;
		notices = notices->nextInInterval;
	}
	if(notices != NULL){
		return notices;
	}else{
		return NULL;
	}
}


/**
* This procedure will be invoked by mi_barrier procedure when one client enter a barrier and it will send all relevant writenotices and intervals to host 0.
**/
void sendEnterBarrierInfo(){
	if(myhostid == 0){
		return; // host 0 is the centralized barrier manager
	}
	interval_t *interval = procArray[myhostid].intervalList;
	while(interval->isBarrier != 1){
		interval = interval->next;
	}

	mimsg_t *msg = nextFreeMsgInQueue(0);
	msg->from = myhostid;
	msg->to = 0;
	msg->command = GRANT_ENTER_BARRIER_INFO;
	int i;
	for(i = 0; i < hostnum; i++){
		msg->timestamp[i] = intervalNow->timestamp[i];
	}
	int packetNum = 0;
	int firstMsgSent = 0;
	apendMsgData(msg, (char *)&packetNum, sizeof(int)); //occupy the packetNum slot
	wnPacket_t packet;

	while(interval != NULL){
		if(interval->notices != NULL){
			writenotice_t *left = addWNIIntoPacketForHost(&packet, myhostid, interval->timestamp, interval->notices);
			apendMsgData(msg, (char *)&packet, sizeof(wnPacket_t));
			packetNum++;
			if(sizeof(wnPacket_t) > (MAX_MSG_SIZE - MSG_HEAD_SIZE - msg->size)){// there is no enough room for one more wnPacket, so this msg should be sent out and a new msg should be created.
				memcpy(msg->data, &packetNum, sizeof(int));
				sendMsg(msg);
				firstMsgSent = 1;
				msg = nextFreeMsgInQueue(0);
				msg->from = myhostid;
				msg->to = 0;
				msg->command = GRANT_ENTER_BARRIER_INFO;
				apendMsgData(msg, (char *)&packetNum, sizeof(int)); //occupy the packetNum slot
				packetNum = 0;
			}
			while(left != NULL){// the number of writenotices of this interval is larger than MAX_WN_NUM
				left = addWNIIntoPacketForHost(&packet, myhostid, interval->timestamp, left);
				apendMsgData(msg, (char *)&packet, sizeof(wnPacket_t));
				packetNum++;
				if(sizeof(wnPacket_t) > (MAX_MSG_SIZE - MSG_HEAD_SIZE - msg->size)){
					memcpy(msg->data, &packetNum, sizeof(int));
					sendMsg(msg);
					firstMsgSent = 1;
					msg = nextFreeMsgInQueue(0);
					msg->from = myhostid;
					msg->to = 0;
					msg->command = GRANT_ENTER_BARRIER_INFO;
					apendMsgData(msg, (char *)&packetNum, sizeof(int)); //occupy the packetNum slot
					packetNum = 0;
				}
			}
		}
		interval = interval->prev;
	}
	if(packetNum != 0 || firstMsgSent == 0){//one msg has not been sent out
		memcpy(msg->data, &packetNum, sizeof(int));
		sendMsg(msg);
	}
}


/**
* This procedure will be invoked by checkBarrierFlags when all clients enter barrier. It will send out all intervals and related writenotices to clients according to the interval they sent to host 0, which is the centralized barrier mananger.
**/
void returnAllBarrierInfo(){
	int timestamp[MAX_HOST_NUM];
	memset(timestamp, 0, sizeof(int) * MAX_HOST_NUM);
	int i;
	for(i = 0; i < hostnum; i++){
		timestamp[i] = barrierTimestamps[i][i] + 1;
	}
//	printf("barrier timestamp = [");
//	for(i = 0; i < hostnum; i++){
//		printf("%d ", timestamp[i]);
//	}
//	printf("]\n");

//	printf("before new intervalNow\n");
	interval_t *interval = malloc(sizeof(interval_t));
	memset(interval, 0, sizeof(interval_t));
	for(i = 0; i < hostnum; i++){
		interval->timestamp[i] = timestamp[i];
	}
	interval->isBarrier = 1;
	interval->next = procArray[0].intervalList;
	procArray[0].intervalList->prev = interval;
	procArray[0].intervalList = interval;
	intervalNow = interval;
//	printf("after new intervalNow\n");

	for(i = 1; i < hostnum; i++){
//		printf("before send for %d\n", i);
		mimsg_t *msg = nextFreeMsgInQueue(0);
		msg->from = myhostid;
		msg->to = i;
		msg->command = GRANT_EXIT_BARRIER_INFO;
		int j;
		for(j = 0; j < hostnum; j++){
			msg->timestamp[j] = timestamp[j];
		}
		int packetNum = 0;
		int firstMsgSent = 0;
		apendMsgData(msg, (char *)&packetNum, sizeof(int)); //occupy the packetNum slot
		wnPacket_t packet;
		for(j = 0; j < hostnum; j++){
			proc_t proc = procArray[j];
			interval_t *intervalLast = proc.intervalList;
			if(intervalLast == NULL){
				continue;
			}
			while(intervalLast->next != NULL){
				intervalLast = intervalLast->next;
			}
			while(intervalLast != NULL){
				if(isAfterInterval(intervalLast->timestamp, barrierTimestamps[i]) && (intervalLast->notices != NULL)){
					writenotice_t *left = addWNIIntoPacketForHost(&packet, j, intervalLast->timestamp, intervalLast->notices);
					apendMsgData(msg, (char *)&packet, sizeof(wnPacket_t));
					packetNum++;
					if(sizeof(wnPacket_t) > (MAX_MSG_SIZE - MSG_HEAD_SIZE - msg->size)){// there is no enough room for one more wnPacket, so this msg should be sent out and a new msg should be created.
						memcpy(msg->data, &packetNum, sizeof(int));
						sendMsg(msg);
						firstMsgSent = 1;
						msg = nextFreeMsgInQueue(0);
						msg->from = myhostid;
						msg->to = i;
						msg->command = GRANT_EXIT_BARRIER_INFO;
						apendMsgData(msg, (char *)&packetNum, sizeof(int)); //occupy the packetNum slot
						packetNum = 0;
					}
					while(left != NULL){// the number of writenotices of this interval is larger than MAX_WN_NUM
						left = addWNIIntoPacketForHost(&packet, j, intervalLast->timestamp, left);
						apendMsgData(msg, (char *)&packet, sizeof(wnPacket_t));
						packetNum++;
						if(sizeof(wnPacket_t) > (MAX_MSG_SIZE - MSG_HEAD_SIZE - msg->size)){
							memcpy(msg->data, &packetNum, sizeof(int));
							sendMsg(msg);
							firstMsgSent = 1;
							msg = nextFreeMsgInQueue(0);
							msg->from = myhostid;
							msg->to = 0;
							msg->command = GRANT_EXIT_BARRIER_INFO;
							apendMsgData(msg, (char *)&packetNum, sizeof(int)); //occupy the packetNum slot
							packetNum = 0;
						}
					}
				}
				intervalLast = intervalLast->prev;
			}
		}
		if(packetNum != 0 || firstMsgSent == 0){//one msg has not been sent out
			memcpy(msg->data, &packetNum, sizeof(int));
			sendMsg(msg);
		}
//		printf("after send for %d\n", i);
	}
	for(i = 0; i < hostnum; i++){
		int j;
		for(j = 0; j < hostnum; j++){
			barrierTimestamps[i][j] = -1;
		}
	}
	for(i = 0; i < LOCK_NUM; i++){
		locks[i].lasthostid = -1;
	}
}


/**
* This procedure will be invoked by dispatchMsg to handle GRANT_ENTER_BARRIER_INFO msg. It will store all writenotice to local data structures.
* parameters
*	msg : msg to be handled
**/
void handleGrantEnterBarrierMsg(mimsg_t *msg){
	if(msg == NULL){
		return;
	}
	int i;
	int *timestamp = msg->timestamp;
	int hostid = msg->from;
	if((timestamp[0] != -1) && (barrierTimestamps[hostid][0] == -1)){
		for(i = 0; i < MAX_HOST_NUM; i++){
			barrierTimestamps[hostid][i] = timestamp[i];
		}
	}
	int packetNum = *((int *)msg->data);
	wnPacket_t *packet =(wnPacket_t *)(msg->data + sizeof(int));
	for(i = 0; i < packetNum; i++){
		incorporateWnPacket(packet+i);
	}
}


/**
* This procedure will be invoked by dispatchMsg to handle GRANT_EXIT_BARRIER_INFO msg. It will store all writenotices to local data structures
* parameters
*	msg : msg to be handled
**/
void handleGrantExitBarrierMsg(mimsg_t *msg){
	if(msg == NULL){
		return;
	}
	int i;
	int *timestamp = msg->timestamp;
	int hostid = msg->from;
	if(timestamp[0] != -1){
		interval_t *interval = malloc(sizeof(interval_t));
		memset(interval, 0, sizeof(interval_t));
		for(i = 0; i < MAX_HOST_NUM; i++){
			interval->timestamp[i] = timestamp[i];
		}
		interval->isBarrier = 1;
		procArray[myhostid].intervalList->prev = interval;
		interval->next = procArray[myhostid].intervalList;
		procArray[myhostid].intervalList = interval;
		intervalNow = interval;
	}
	int packetNum = *((int *)msg->data);
	wnPacket_t *packet =(wnPacket_t *)(msg->data + sizeof(int));
	for(i = 0; i < packetNum; i++){
		incorporateWnPacket(packet+i);
	}
}


/**
* This procedure will print all local data structures, just for test.
**/
void showDataStructures(){
	int i, j;
	for(i = 0; i < pagenum; i++){
		printf("page %d:\n", i);
		printf("  address = %p\n", pageArray[i].address);
		printf("  state = %d\n", pageArray[i].state);
		printf("  notices:\n");
		for(j = 0; j < hostnum; j++){
			printf("    %d\n", j);
			writenotice_t *wn = pageArray[i].notices[j];
			while(wn != NULL){
				printf("      %p\n", wn->interval);
				printf("      %p\n", wn->diffAddress);
				wn = wn->nextInPage;
			}
		}
		printf("  twinPage = %p\n", pageArray[i].twinPage);
		printf("  startInterval = %p\n", pageArray[i].startInterval);
	}
	for(i = 0; i < hostnum; i++){
		printf("proc %d:\n", i);
		printf("  intervals:\n");
		interval_t *interval = procArray[i].intervalList;
		while(interval != NULL){
			printf("    address = %p:\n", interval);
			printf("    timestamp = [");
			for(j = 0; j < hostnum; j++){		
				printf("%d ", interval->timestamp[j]);
			}
			printf("]\n");
			interval = interval->next;
		}
	}
}
