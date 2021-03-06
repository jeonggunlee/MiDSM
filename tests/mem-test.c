#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/mem.h"
#include "../src/init.h"
#include "../src/net.h"
#include "minunit.h"
int tests_run = 0;

int myhostid;
int hostnum;
extern page_t pageArray[MAX_PAGE_NUM];

static char *test_isAfterInterval(){
	int a[MAX_HOST_NUM];
	int b[MAX_HOST_NUM];
	memset(a, 0, MAX_HOST_NUM);
	memset(b, 0, MAX_HOST_NUM);

	hostnum = 4;
	
	a[0] = 1;
	b[0] = 2;
	mu_assert("mem1", isAfterInterval(a, b) == 0);

	a[0] = 1;
	b[0] = 1;
	mu_assert("mem2", isAfterInterval(a, b) == 0);

	a[1] = 2;
	b[1] = 1;
	mu_assert("mem3", isAfterInterval(a, b) == 1);

	a[0] = 1;
	b[0] = 2;
	a[2] = 2;
	b[2] = 0;
	mu_assert("mem4", isAfterInterval(a, b) == 0);

	a[0] = 1;
	b[0] = 1;
	a[1] = 4;
	b[1] = 4;
	a[2] = 2;
	b[2] = 2;
	a[4] = 3;
	b[4] = 3;
	mu_assert("mem5", isAfterInterval(a, b) == 0);

	a[0] = 1;
	b[0] = 1;
	a[2] = 3;
	b[2] = 2;
	a[4] = 3;
	b[4] = 3;
	mu_assert("mem5.1", isAfterInterval(a, b) == 1);

	mu_assert("mem6", isAfterInterval(NULL, b) == -1);
	mu_assert("mem7", isAfterInterval(a, NULL) == -1);
	mu_assert("mem8", isAfterInterval(NULL, NULL) == -1);
	
	return 0;
}


static char *test_createTwinPage(){
	memset(pageArray, 0, MAX_PAGE_NUM * sizeof(page_t));
	pageArray[0].address = malloc(PAGESIZE);
	pageArray[0].state = RDONLY;		
	pageArray[0].twinPage = NULL;
	memset(pageArray[0].address, 0, PAGESIZE);
	((int *)(pageArray[0].address))[2] = 3;
	((int *)(pageArray[0].address))[1023] = 1023;
	mu_assert("mem9", createTwinPage(0) == 0);
	mu_assert("mem10", ((int *)pageArray[0].twinPage)[2] == 3);
	mu_assert("mem11", ((int *)pageArray[0].twinPage)[1023] == 1023);
	mu_assert("mem12", createTwinPage(0) == -1);
	mu_assert("mem13", createTwinPage(1) == -1);
	mu_assert("mem14", createTwinPage(-1) == -1);
	mu_assert("mem15", createTwinPage(MAX_PAGE_NUM) == -1);
	pageArray[1].address = malloc(PAGESIZE);
	pageArray[1].state = MISS;		
	pageArray[1].twinPage = NULL;
	mu_assert("mem16", createTwinPage(1) == -1);
	free(pageArray[0].address);
	free(pageArray[0].twinPage);
	free(pageArray[1].address);

	return 0;
}


static char *test_freeTwinPage(){
	memset(pageArray, 0, MAX_PAGE_NUM * sizeof(page_t));
	pageArray[0].address = malloc(PAGESIZE);
	pageArray[0].state = RDONLY;		
	pageArray[0].twinPage = NULL;
	memset(pageArray[0].address, 0, PAGESIZE);
	((int *)(pageArray[0].address))[2] = 3;
	((int *)(pageArray[0].address))[1023] = 1023;
	mu_assert("mem17", createTwinPage(0) == 0);
	mu_assert("mem18", ((int *)pageArray[0].twinPage)[2] == 3);
	mu_assert("mem19", ((int *)pageArray[0].twinPage)[1023] == 1023);
	mu_assert("mem20", freeTwinPage(0) == 0);
	mu_assert("mem21", pageArray[0].twinPage == NULL);
	
	mu_assert("mem22", freeTwinPage(-1) == -1);
	mu_assert("mem23", freeTwinPage(MAX_PAGE_NUM) == -1);
	mu_assert("mem24", freeTwinPage(0) == -1);
	mu_assert("mem25", freeTwinPage(3) == -1);

	free(pageArray[0].address);
	return 0;
}


static char *test_applyDiff(){
	void *memory1 = malloc(PAGESIZE);
	void *memory2 = malloc(PAGESIZE);
	memset(memory1, 0, PAGESIZE);
	memset(memory2, 0, PAGESIZE);
	((char *)memory1)[3] = 30;
	((char *)memory1)[PAGESIZE -1] = 11;
	((char *)memory2)[3] = -11;
	((char *)memory2)[PAGESIZE -1] = 5;
	mu_assert("mem26", applyDiff(memory1, memory2) == 0);
	mu_assert("mem27", ((char *)memory1)[0] == 0);
	mu_assert("mem28", ((char *)memory1)[3] == 19);
	mu_assert("mem29", ((char *)memory1)[PAGESIZE - 1] == 16);

	mu_assert("mem30", applyDiff(memory1, NULL) == -1);
	mu_assert("mem31", applyDiff(NULL, memory2) == -1);
	mu_assert("mem32", applyDiff(NULL, NULL) == -1);

	free(memory1);
	free(memory2);
	return 0;
}


static char *test_createLocalDiff(){
	void *memory1 = malloc(PAGESIZE);
	void *memory2 = malloc(PAGESIZE);
	memset(memory1, 0, PAGESIZE);
	memset(memory2, 0, PAGESIZE);
	((char *)memory1)[3] = 30;
	((char *)memory1)[PAGESIZE -1] = 11;
	((char *)memory2)[3] = -11;
	((char *)memory2)[PAGESIZE -1] = 5;
	void *memory3 = createLocalDiff(memory1, memory2);
	mu_assert("mem33", memory3 != NULL);
	mu_assert("mem34", ((char *)memory3)[0] == 0);
	mu_assert("mem35", ((char *)memory3)[3] == 41);
	mu_assert("mem36", ((char *)memory3)[PAGESIZE - 1] == 6);

	mu_assert("mem37", createLocalDiff(memory1, NULL) == NULL);
	mu_assert("mem38", createLocalDiff(NULL, memory2) == NULL);
	mu_assert("mem39", createLocalDiff(NULL, NULL) == NULL);

	free(memory1);
	free(memory2);
	free(memory3);
	return 0;
}


static char *test_createWriteNotice(){
	extern interval_t *intervalNow;

	interval_t tempInterval;
	intervalNow = &tempInterval;
	myhostid = 0;
	
	memset(intervalNow, 0, sizeof(interval_t));
	intervalNow->notices = NULL;
	intervalNow->next = NULL;
	intervalNow->isBarrier = 0;

	memset(pageArray, 0, MAX_PAGE_NUM * sizeof(page_t));
	pageArray[0].address = malloc(PAGESIZE);
	pageArray[0].state = RDONLY;	
	
	mu_assert("mem40", createWriteNotice(0) == 0);
	writenotice_t *wn = pageArray[0].notices[myhostid];
	mu_assert("mem41",  wn != NULL);
	mu_assert("mem42",  wn->interval == intervalNow);
	mu_assert("mem43",  wn->nextInPage == NULL);
	mu_assert("mem44",  wn->nextInInterval == NULL);
	mu_assert("mem45",  wn->diffAddress == NULL);
	mu_assert("mem45.1", wn->pageIndex == 0);
	mu_assert("mem46",  intervalNow->notices == wn);

	mu_assert("mem47", createWriteNotice(0) == 0);
	writenotice_t *wn2 = pageArray[0].notices[myhostid];
	mu_assert("mem48",  wn2 != NULL);
	mu_assert("mem49",  wn2->interval == intervalNow);
	mu_assert("mem50",  wn2->nextInPage == wn);
	mu_assert("mem51",  wn2->nextInInterval == NULL);
	mu_assert("mem52",  wn2->diffAddress == NULL);
	mu_assert("mem52.1",  wn2->pageIndex == 0);
	mu_assert("mem53",  intervalNow->notices == wn);
	mu_assert("mem54",  intervalNow->notices->nextInInterval == wn2);

	mu_assert("mem55", createWriteNotice(-1) == -1);
	mu_assert("mem56", createWriteNotice(MAX_PAGE_NUM) == -1);
	pageArray[1].address = malloc(PAGESIZE);
	pageArray[1].state = WRITE;	
	mu_assert("mem57", createWriteNotice(1) == -1);
	pageArray[1].state = UNMAP;	
	mu_assert("mem58", createWriteNotice(1) == -1);
	pageArray[1].state = MISS;	
	mu_assert("mem59", createWriteNotice(1) == -1);
	pageArray[1].state = INVALID;	
	mu_assert("mem60", createWriteNotice(1) == -1);

	return 0;
}


static char *test_addNewInterval(){
	extern interval_t *intervalNow;
	extern proc_t procArray[MAX_HOST_NUM];

	interval_t tempInterval;
	intervalNow = &tempInterval;
	myhostid = 0;
	hostnum = 4;
	
	memset(intervalNow, 0, sizeof(interval_t));
	intervalNow->notices = NULL;
	intervalNow->next = NULL;
	intervalNow->isBarrier = 0;

	memset(procArray, 0, MAX_HOST_NUM * sizeof(proc_t));
	procArray[0].hostid = 0;
	procArray[0].intervalList = intervalNow;	
	addNewInterval();
	mu_assert("mem61", (intervalNow->timestamp)[0] == 1);
	mu_assert("mem62", (intervalNow->timestamp)[1] == 0);
	mu_assert("mem63", (intervalNow->timestamp)[2] == 0);
	mu_assert("mem64", (intervalNow->timestamp)[3] == 0);
	mu_assert("mem65", procArray[0].intervalList == intervalNow);
	mu_assert("mem66", procArray[0].intervalList->next == &tempInterval);
	mu_assert("mem66.1", procArray[0].intervalList->next->prev == intervalNow);
	mu_assert("mem66.2", intervalNow->prev == NULL);

	return 0;
}


static char *test_incorporateWnPacket(){
	extern proc_t procArray[MAX_HOST_NUM];
	extern page_t pageArray[MAX_PAGE_NUM];
	extern interval_t *intervalNow;

	interval_t tempInterval;
	intervalNow = &tempInterval;
	myhostid = 0;
	hostnum = 4;
	
	memset(intervalNow, 0, sizeof(interval_t));
	intervalNow->notices = NULL;
	intervalNow->next = NULL;
	intervalNow->isBarrier = 0;

	memset(procArray, 0, MAX_HOST_NUM * sizeof(proc_t));
	procArray[0].hostid = 0;
	procArray[0].intervalList = intervalNow;	

	memset(pageArray, 0, MAX_PAGE_NUM * sizeof(page_t));
	pageArray[0].state = WRITE;
	pageArray[1].state = RDONLY;
	pageArray[2].state = MISS;
	pageArray[3].state = INVALID;

	writenotice_t *wn1 = malloc(sizeof(writenotice_t));
	wn1->interval = intervalNow;
	wn1->nextInPage = NULL;
	wn1->nextInInterval = NULL;
	wn1->diffAddress = NULL;
	wn1->pageIndex = 0;
	pageArray[0].notices[myhostid] = wn1;
	pageArray[0].address = malloc(sizeof(PAGESIZE));
	pageArray[0].twinPage = malloc(sizeof(PAGESIZE));
	memset(pageArray[0].address, 0, sizeof(PAGESIZE));
	memset(pageArray[0].twinPage, 0, sizeof(PAGESIZE));
	((char *)pageArray[0].address)[0] = 0;	
	((char *)pageArray[0].address)[1] = 1;	
	((char *)pageArray[0].address)[2] = 2;	
	((char *)pageArray[0].address)[3] = 3;	
	((char *)pageArray[0].address)[4] = 4;	
	((char *)pageArray[0].address)[5] = 5;	
	((char *)pageArray[0].address)[6] = 6;	
	((char *)pageArray[0].address)[7] = 7;	

	
	pageArray[1].address = malloc(sizeof(PAGESIZE));
	memset(pageArray[1].address, 0, sizeof(PAGESIZE));
	
	mu_assert("mem67", incorporateWnPacket(NULL) == -1);
	
	wnPacket_t *packet = malloc(sizeof(wnPacket_t));

	memset(packet, 0, sizeof(wnPacket_t));
	packet->hostid = 1;
	(packet->timestamp)[1] = 1;
	packet->wnCount = 2;
	(packet->wnArray)[0] = 0;
	(packet->wnArray)[1] = 1;
	
	mu_assert("mem68", incorporateWnPacket(packet) == 0);
	mu_assert("mem69", (procArray[1].intervalList->timestamp)[1] == 1);
	mu_assert("mem70", (procArray[1].intervalList->timestamp)[0] == 0);
	mu_assert("mem71", (procArray[1].intervalList->timestamp)[2] == 0);
	mu_assert("mem72", (procArray[1].intervalList->timestamp)[3] == 0);
	mu_assert("mem73", (procArray[1].intervalList)->notices == pageArray[0].notices[1]);
	mu_assert("mem74", (procArray[1].intervalList)->notices->nextInInterval == pageArray[1].notices[1]);
	mu_assert("mem75", (procArray[1].intervalList)->notices->nextInInterval->nextInInterval == NULL);
	mu_assert("mem76", (procArray[1].intervalList)->notices->nextInPage == NULL);
	mu_assert("mem77", (procArray[1].intervalList)->notices->nextInInterval->nextInPage == NULL);
	mu_assert("mem78", (procArray[1].intervalList)->notices->interval == procArray[1].intervalList);
	mu_assert("mem79", (procArray[1].intervalList)->notices->nextInInterval->interval == procArray[1].intervalList);
	mu_assert("mem80", (procArray[1].intervalList)->notices->diffAddress == NULL);
	mu_assert("mem81", (procArray[1].intervalList)->notices->nextInInterval->diffAddress == NULL);
	mu_assert("mem81.1", (procArray[1].intervalList)->notices->nextInInterval->pageIndex == 1);
	mu_assert("mem81.2", (procArray[1].intervalList)->notices->pageIndex == 0);
	mu_assert("mem81.3", pageArray[0].state == INVALID);
	mu_assert("mem81.4", pageArray[0].twinPage == NULL);
	mu_assert("mem81.5", pageArray[0].notices[myhostid]->diffAddress != NULL);
	char *diffAddress = (char *)pageArray[0].notices[myhostid]->diffAddress;
	mu_assert("mem81.6", diffAddress[0] == 0);
	mu_assert("mem81.7", diffAddress[1] == 1);
	mu_assert("mem81.8", diffAddress[2] == 2);
	mu_assert("mem81.9", diffAddress[3] == 3);
	mu_assert("mem81.10", diffAddress[4] == 4);
	mu_assert("mem81.11", diffAddress[5] == 5);
	mu_assert("mem81.12", diffAddress[6] == 6);
	mu_assert("mem81.13", diffAddress[7] == 7);

	mu_assert("mem81.14", pageArray[1].state == INVALID);
	


	memset(packet, 0, sizeof(wnPacket_t));
	packet->hostid = 1;
	(packet->timestamp)[1] = 1;
	packet->wnCount = 2;
	(packet->wnArray)[0] = 1;
	(packet->wnArray)[1] = 2;
	
	mu_assert("mem81.15", incorporateWnPacket(packet) == 0);
	mu_assert("mem81.16", (procArray[1].intervalList->timestamp)[1] == 1);
	mu_assert("mem81.17", (procArray[1].intervalList->timestamp)[0] == 0);
	mu_assert("mem81.18", (procArray[1].intervalList->timestamp)[2] == 0);
	mu_assert("mem81.19", (procArray[1].intervalList->timestamp)[3] == 0);
	mu_assert("mem81.20", procArray[1].intervalList->next == NULL);

	mu_assert("mem81.21", (procArray[1].intervalList)->notices->nextInInterval->nextInInterval == pageArray[1].notices[1]);
	mu_assert("mem81.22", (procArray[1].intervalList)->notices->nextInInterval->nextInInterval->nextInInterval == pageArray[2].notices[1]);
	mu_assert("mem81.23", (procArray[1].intervalList)->notices->nextInInterval->nextInInterval->nextInInterval->nextInInterval == NULL);
	mu_assert("mem81.24", (procArray[1].intervalList)->notices->nextInInterval->nextInInterval->nextInPage != NULL);
	mu_assert("mem81.25", (procArray[1].intervalList)->notices->nextInInterval->nextInInterval->nextInInterval->nextInPage == NULL);
	mu_assert("mem81.26", (procArray[1].intervalList)->notices->nextInInterval->nextInInterval->interval == procArray[1].intervalList);
	mu_assert("mem81.27", (procArray[1].intervalList)->notices->nextInInterval->nextInInterval->nextInInterval->interval == procArray[1].intervalList);
	mu_assert("mem81.28", (procArray[1].intervalList)->notices->nextInInterval->nextInInterval->diffAddress == NULL);
	mu_assert("mem81.29", (procArray[1].intervalList)->notices->nextInInterval->nextInInterval->nextInInterval->diffAddress == NULL);
	mu_assert("mem81.30", (procArray[1].intervalList)->notices->nextInInterval->nextInInterval->nextInInterval->pageIndex == 2);
	mu_assert("mem81.31", (procArray[1].intervalList)->notices->nextInInterval->nextInInterval->pageIndex == 1);

	mu_assert("mem81.32", pageArray[1].state == INVALID);
	mu_assert("mem81.33", pageArray[2].state == MISS);

	memset(packet, 0, sizeof(wnPacket_t));
	packet->hostid = 1;
	(packet->timestamp)[1] = 2;
	packet->wnCount = 1;
	(packet->wnArray)[0] = 2;
	
	mu_assert("mem82", incorporateWnPacket(packet) == 0);
	mu_assert("mem83", (procArray[1].intervalList->timestamp)[1] == 2);
	mu_assert("mem84", (procArray[1].intervalList->timestamp)[0] == 0);
	mu_assert("mem85", (procArray[1].intervalList->timestamp)[2] == 0);
	mu_assert("mem86", (procArray[1].intervalList->timestamp)[3] == 0);
	mu_assert("mem87", (procArray[1].intervalList->next->timestamp)[0] == 0);
	mu_assert("mem88", (procArray[1].intervalList->next->timestamp)[1] == 1);
	mu_assert("mem89", (procArray[1].intervalList->next->timestamp)[2] == 0);
	mu_assert("mem90", (procArray[1].intervalList->next->timestamp)[3] == 0);


	mu_assert("mem91", (procArray[1].intervalList)->notices == pageArray[2].notices[1]);
	mu_assert("mem92", (procArray[1].intervalList)->notices->nextInInterval == NULL);
	mu_assert("mem93", (procArray[1].intervalList)->notices->nextInPage->interval == procArray[1].intervalList->next);
	mu_assert("mem94", (procArray[1].intervalList)->notices->interval == procArray[1].intervalList);
	mu_assert("mem95", (procArray[1].intervalList)->notices->diffAddress == NULL);
	mu_assert("mem96", (procArray[1].intervalList)->notices->pageIndex == 2);

	return 0;
}


static char *test_addWNIIntoPacketForHost(){
	myhostid = 0;
	hostnum = 4;

	wnPacket_t *packet = malloc(sizeof(wnPacket_t));
	memset(packet, 0, sizeof(wnPacket_t));
	int timestamp[MAX_HOST_NUM];
	memset(timestamp, 0, MAX_HOST_NUM * sizeof(int));
	timestamp[0] = 1;
	timestamp[3] = 3;
	
	int wnCount = 3;
	int i;
	writenotice_t *notices = NULL;
	writenotice_t *lastwn = NULL;
	writenotice_t *wn = NULL;
	for(i = 0; i < wnCount; i++){
		wn = malloc(sizeof(writenotice_t));
		wn->nextInInterval = NULL;
		wn->pageIndex = i;
		if(lastwn == NULL){
			lastwn = notices = wn;
		}else{
			lastwn->nextInInterval = wn;
		}
		lastwn = wn;
	}
	
	//parameter error check
	mu_assert("mem97", addWNIIntoPacketForHost(NULL, 2, timestamp, notices) == NULL);
	mu_assert("mem98", addWNIIntoPacketForHost(packet, 5, timestamp, notices) == NULL);
	mu_assert("mem99", addWNIIntoPacketForHost(packet, 2, NULL, notices) == NULL);
	mu_assert("mem100", addWNIIntoPacketForHost(packet, 5, timestamp, NULL) == NULL);
	//normal add
	mu_assert("mem101", addWNIIntoPacketForHost(packet, 2, timestamp, notices) == NULL);
	mu_assert("mem102", packet->hostid == 2);
	mu_assert("mem103", packet->timestamp[0] == 1);
	mu_assert("mem104", packet->timestamp[3] == 3);
	mu_assert("mem105", packet->wnCount == 3);
	mu_assert("mem106", packet->wnArray[0] == 0);
	mu_assert("mem107", packet->wnArray[1] == 1);
	mu_assert("mem108", packet->wnArray[2] == 2);
	//return value not null	
	wnCount = MAX_WN_NUM + 1;
	notices = NULL;
	lastwn = NULL;
	wn = NULL;
	for(i = 0; i < wnCount; i++){
		wn = malloc(sizeof(writenotice_t));
		wn->nextInInterval = NULL;
		wn->pageIndex = i;
		if(lastwn == NULL){
			lastwn = notices = wn;
		}else{
			lastwn->nextInInterval = wn;
		}
		lastwn = wn;
	}
	mu_assert("mem109", addWNIIntoPacketForHost(packet, 2, timestamp, notices) == lastwn);
	mu_assert("mem110", packet->hostid == 2);
	mu_assert("mem111", packet->timestamp[0] == 1);
	mu_assert("mem112", packet->timestamp[3] == 3);
	mu_assert("mem113", packet->wnCount == MAX_WN_NUM);
	mu_assert("mem114", packet->wnArray[0] == 0);
	mu_assert("mem115", packet->wnArray[3] == 3);
	mu_assert("mem116", packet->wnArray[MAX_WN_NUM-1] == MAX_WN_NUM-1);

	return 0;
}


static char *test_grantWNI(){
	extern interval_t *intervalNow;
	extern page_t pageArray[MAX_PAGE_NUM];
	extern proc_t procArray[MAX_HOST_NUM];
	extern int parametertype;
	extern int sendMsgCalled; 
	extern int nextFreeMsgInQueueCalled;
	extern mimsg_t msg;
	extern int alreadyCalled;
	

	myhostid = 0;
	hostnum = 4;

	memset(pageArray, 0, sizeof(page_t) * MAX_PAGE_NUM);
	memset(procArray, 0, sizeof(proc_t) * MAX_HOST_NUM);

	interval_t *interval1 = malloc(sizeof(interval_t));
	memset(interval1, 0, sizeof(interval_t));
	int i;
	interval1->timestamp[0] = 1;
	procArray[0].hostid = 0;	
	procArray[0].intervalList = interval1;	
	writenotice_t *lastWnInInterval = NULL;
	int wnCount = 3;
	for(i = 0; i < wnCount; i++){
		writenotice_t *wn = malloc(sizeof(writenotice_t));
		wn->interval = interval1;
		wn->nextInPage = pageArray[i].notices[0];
		wn->pageIndex = i;
		pageArray[i].notices[0] = wn;
		wn->nextInInterval = NULL;
		if(lastWnInInterval == NULL){
			interval1->notices = lastWnInInterval = wn;
		}else{
			lastWnInInterval->nextInInterval = wn;
			lastWnInInterval = wn;
		}
	}

	interval_t *interval2 = malloc(sizeof(interval_t));
	memset(interval2, 0, sizeof(interval_t));
	interval2->timestamp[0] = 2;
	interval2->timestamp[3] = 1;
	procArray[3].hostid = 3;	
	procArray[3].intervalList = interval2;	
	lastWnInInterval = NULL;

	for(i = 0; i < wnCount; i++){
		writenotice_t *wn = malloc(sizeof(writenotice_t));
		wn->interval = interval2;
		wn->nextInPage = pageArray[i].notices[3];
		wn->pageIndex = i;
		pageArray[i].notices[3] = wn;
		wn->nextInInterval = NULL;
		if(lastWnInInterval == NULL){
			interval2->notices = lastWnInInterval = wn;
		}else{
			lastWnInInterval->nextInInterval = wn;
			lastWnInInterval = wn;
		}
	}

	interval_t *interval3 = malloc(sizeof(interval_t));
	memset(interval3, 0, sizeof(interval_t));
	interval3->timestamp[0] = 2;
	interval3->timestamp[3] = 0;
	interval3->next = procArray[0].intervalList;
	procArray[0].intervalList->prev = interval3;
	procArray[0].intervalList = interval3;

	interval_t *interval4 = malloc(sizeof(interval_t));
	memset(interval4, 0, sizeof(interval_t));
	interval4->timestamp[0] = 3;
	interval4->timestamp[3] = 2;
	interval4->next = procArray[0].intervalList;
	procArray[0].intervalList->prev = interval4;
	procArray[0].intervalList = interval4;

	lastWnInInterval = NULL;
	for(i = 0; i < wnCount; i++){
		writenotice_t *wn = malloc(sizeof(writenotice_t));
		wn->interval = interval4;
		wn->nextInPage = pageArray[i].notices[3];
		wn->pageIndex = i;
		pageArray[i].notices[0] = wn;
		wn->nextInInterval = NULL;
		if(lastWnInInterval == NULL){
			interval4->notices = lastWnInInterval = wn;
		}else{
			lastWnInInterval->nextInInterval = wn;
			lastWnInInterval = wn;
		}
	}


	intervalNow = malloc(sizeof(interval_t));
	memset(intervalNow, 0, sizeof(interval_t));
	intervalNow->timestamp[0] = 4;
	intervalNow->timestamp[3] = 2;
	intervalNow->next = procArray[0].intervalList;
	procArray[0].intervalList->prev = intervalNow;
	procArray[0].intervalList = intervalNow;

	int timestamp[MAX_HOST_NUM];
	for(i = 0; i < MAX_HOST_NUM; i++){
		timestamp[i] = 0;
	}

	alreadyCalled = 0;
	mu_assert("mem117", grantWNI(2, timestamp) == 0);
	mu_assert("mem118", nextFreeMsgInQueueCalled == 1);
	mu_assert("mem119", sendMsgCalled == 1);
	mu_assert("mem120", parametertype == 0);
	mu_assert("mem121", msg.from == myhostid);
	mu_assert("mem122", msg.to == 2);
	mu_assert("mem123", msg.command == GRANT_WN_I);
	mu_assert("mem124", msg.timestamp[0] == 4);
	mu_assert("mem125", msg.timestamp[3] == 2);
	mu_assert("mem126", msg.size == (sizeof(wnPacket_t) * 3)+sizeof(int));

	int *packetNum = (int *)msg.data;
	mu_assert("mem127", *packetNum == 3);
	wnPacket_t *packet = (wnPacket_t *)(msg.data+sizeof(int));

	mu_assert("mem128", packet != NULL);
	mu_assert("mem129", packet->hostid == 0);
	mu_assert("mem130", packet->timestamp[0] == 1);
	mu_assert("mem131", packet->timestamp[3] == 0);
	mu_assert("mem132", packet->wnCount == 3);
	mu_assert("mem133", packet->wnArray[0] == 0);
	mu_assert("mem134", packet->wnArray[1] == 1);
	mu_assert("mem135", packet->wnArray[2] == 2);

	packet = (wnPacket_t *)packet+1;
	mu_assert("mem136", packet != NULL);
	mu_assert("mem137", packet->hostid == 0);
	mu_assert("mem138", packet->timestamp[0] == 3);
	mu_assert("mem139", packet->timestamp[3] == 2);
	mu_assert("mem140", packet->wnCount == 3);
	mu_assert("mem141", packet->wnArray[0] == 0);
	mu_assert("mem142", packet->wnArray[1] == 1);
	mu_assert("mem143", packet->wnArray[2] == 2);


	packet = (wnPacket_t *)packet+1;
	mu_assert("mem143.1", packet != NULL);
	mu_assert("mem143.2", packet->hostid == 3);
	mu_assert("mem143.3", packet->timestamp[0] == 2);
	mu_assert("mem143.4", packet->timestamp[3] == 1);
	mu_assert("mem143.5", packet->wnCount == 3);
	mu_assert("mem144.6", packet->wnArray[0] == 0);
	mu_assert("mem143.7", packet->wnArray[1] == 1);
	mu_assert("mem143.8", packet->wnArray[2] == 2);

	lastWnInInterval = NULL;
	wnCount = MAX_WN_NUM + 1;
	for(i = 0; i < wnCount; i++){
		writenotice_t *wn = malloc(sizeof(writenotice_t));
		wn->interval = intervalNow;
		wn->nextInPage = pageArray[i].notices[0];
		wn->pageIndex = i;
		pageArray[i].notices[0] = wn;
		wn->nextInInterval = NULL;
		if(lastWnInInterval == NULL){
			intervalNow->notices = lastWnInInterval = wn;
		}else{
			lastWnInInterval->nextInInterval = wn;
			lastWnInInterval = wn;
		}
	}
	
	parametertype = 1;
	sendMsgCalled = 0;
	nextFreeMsgInQueueCalled = 0;
	msg.from = -1;
	msg.to = -1;
	msg.command = -1;
	memset(msg.timestamp, 0, sizeof(int)*MAX_HOST_NUM);
	msg.size = 0;
	timestamp[0] = 3;
	timestamp[3] = 2;
	
	alreadyCalled = 0;
	mu_assert("mem144", grantWNI(2, timestamp) == 0);
	mu_assert("mem145", nextFreeMsgInQueueCalled == 1);
	mu_assert("mem146", sendMsgCalled == 1);
	mu_assert("mem147", parametertype == 0);
	mu_assert("mem148", msg.from == myhostid);
	mu_assert("mem149", msg.to == 2);
	mu_assert("mem150", msg.command == GRANT_WN_I);
	mu_assert("mem151", msg.size == (sizeof(wnPacket_t) * 2)+sizeof(int));
	mu_assert("mem152", msg.timestamp[0] == 4);
	mu_assert("mem153", msg.timestamp[3] == 2);
	
	packetNum = (int *)msg.data;
	mu_assert("mem154", *packetNum == 2);
	packet = (wnPacket_t *)(msg.data+sizeof(int));
	mu_assert("mem155", packet != NULL);
	mu_assert("mem156", packet->hostid == 0);
	mu_assert("mem157", packet->timestamp[0] == 4);
	mu_assert("mem158", packet->timestamp[3] == 2);
	mu_assert("mem159", packet->wnCount == MAX_WN_NUM);
	mu_assert("mem160", packet->wnArray[0] == 0);
	mu_assert("mem161", packet->wnArray[1] == 1);
	mu_assert("mem162", packet->wnArray[MAX_WN_NUM-1] == (MAX_WN_NUM-1));
	packet = (wnPacket_t *)packet+1;
	mu_assert("mem163", packet != NULL);
	mu_assert("mem164", packet->hostid == 0);
	mu_assert("mem165", packet->timestamp[0] == 4);
	mu_assert("mem166", packet->timestamp[3] == 2);
	mu_assert("mem167", packet->wnCount == 1);
	mu_assert("mem168", packet->wnArray[0] == MAX_WN_NUM);


	alreadyCalled = 0;
	mu_assert("mem169", grantWNI(-1, timestamp) == -1);
	mu_assert("mem170", grantWNI(hostnum, timestamp) == -1);
	mu_assert("mem171", grantWNI(0, timestamp) == -1);
	mu_assert("mem172", grantWNI(2, NULL) == -1);
	return 0;
}


static char *test_handleGrantWNIMsg(){
	extern page_t pageArray[MAX_PAGE_NUM];
	extern proc_t procArray[MAX_HOST_NUM];
	extern interval_t *intervalNow;

	memset(pageArray, 0, sizeof(page_t) * MAX_PAGE_NUM);
	memset(procArray, 0, sizeof(proc_t) * MAX_HOST_NUM);

	myhostid = 0;
	hostnum = 4;

	intervalNow = malloc(sizeof(interval_t));
	memset(intervalNow, 0, sizeof(interval_t));
	procArray[0].hostid = 0;
	procArray[0].intervalList = intervalNow;
	procArray[1].hostid = 1;
	procArray[2].hostid = 2;
	procArray[3].hostid = 3;
	
	mimsg_t *msg = (mimsg_t *)malloc(sizeof(mimsg_t));	
	memset(msg, 0, sizeof(mimsg_t));
	msg->from = 2;
	msg->to = myhostid; 
	msg->command = GRANT_WN_I;
	int i;
	msg->timestamp[2] = 2;	
	msg->timestamp[3] = 2;	

	wnPacket_t packet;
	memset(&packet, 0, sizeof(wnPacket_t));

	int packetNum = 2;
	apendMsgData(msg, (char *)&packetNum, sizeof(int));
	packet.hostid = 2;
	packet.timestamp[2] = 1;
	packet.timestamp[3] = 2;
	packet.wnCount = 3;
	packet.wnArray[0] = 2;
	packet.wnArray[1] = 3;
	packet.wnArray[2] = 4;
	apendMsgData(msg, (char *)&packet, sizeof(wnPacket_t));
	
	memset(&packet, 0, sizeof(wnPacket_t));
	packet.hostid = 3;
	packet.timestamp[2] = 0;
	packet.timestamp[3] = 1;
	packet.wnCount = 4;
	packet.wnArray[0] = 2;
	packet.wnArray[1] = 3;
	packet.wnArray[2] = 4;
	packet.wnArray[3] = 5;
	apendMsgData(msg, (char *)&packet, sizeof(wnPacket_t));

	handleGrantWNIMsg(msg);
	mu_assert("mem173", intervalNow->timestamp[0] == 1);
	mu_assert("mem174", intervalNow->timestamp[1] == 0);
	mu_assert("mem175", intervalNow->timestamp[2] == 2);
	mu_assert("mem176", intervalNow->timestamp[3] == 2);
	mu_assert("mem177", procArray[0].intervalList->next->timestamp[0] == 0);
	mu_assert("mem178", intervalNow->notices == NULL);
	mu_assert("mem179", procArray[2].intervalList->timestamp[0] == 0);
	mu_assert("mem180", procArray[2].intervalList->timestamp[1] == 0);
	mu_assert("mem181", procArray[2].intervalList->timestamp[2] == 1);
	mu_assert("mem182", procArray[2].intervalList->timestamp[3] == 2);
	mu_assert("mem183", procArray[2].intervalList->next == NULL);
	mu_assert("mem184", procArray[2].intervalList->notices->pageIndex == 2);
	mu_assert("mem185", procArray[2].intervalList->notices->nextInInterval->pageIndex == 3);
	mu_assert("mem186", procArray[2].intervalList->notices->nextInInterval->nextInInterval->pageIndex == 4);

	mu_assert("mem187", procArray[3].intervalList->timestamp[0] == 0);
	mu_assert("mem188", procArray[3].intervalList->timestamp[1] == 0);
	mu_assert("mem189", procArray[3].intervalList->timestamp[2] == 0);
	mu_assert("mem190", procArray[3].intervalList->timestamp[3] == 1);
	mu_assert("mem191", procArray[3].intervalList->next == NULL);
	mu_assert("mem192", procArray[3].intervalList->notices->pageIndex == 2);
	mu_assert("mem193", procArray[3].intervalList->notices->nextInInterval->pageIndex == 3);
	mu_assert("mem194", procArray[3].intervalList->notices->nextInInterval->nextInInterval->pageIndex == 4);
	mu_assert("mem195", procArray[3].intervalList->notices->nextInInterval->nextInInterval->nextInInterval->pageIndex == 5);

	memset(&packet, 0, sizeof(wnPacket_t));

	memset(msg, 0, sizeof(mimsg_t));
	msg->from = 2;
	msg->to = myhostid; 
	msg->command = GRANT_WN_I;
	msg->timestamp[2] = 4;	
	msg->timestamp[3] = 2;	

	packetNum = 2;
	apendMsgData(msg, (char *)&packetNum, sizeof(int));
	packet.hostid = 2;
	packet.timestamp[2] = 3;
	packet.timestamp[3] = 2;
	packet.wnCount = MAX_WN_NUM;
	for(i = 0; i < MAX_WN_NUM; i++){
		packet.wnArray[i] = i;
	}
	apendMsgData(msg, (char *)&packet, sizeof(wnPacket_t));
	
	memset(&packet, 0, sizeof(wnPacket_t));
	packet.hostid = 2;
	packet.timestamp[2] = 3;
	packet.timestamp[3] = 2;
	packet.wnCount = 1;
	packet.wnArray[0] = MAX_WN_NUM;
	apendMsgData(msg, (char *)&packet, sizeof(wnPacket_t));

	handleGrantWNIMsg(msg);
	mu_assert("mem196", intervalNow->timestamp[0] == 1);
	mu_assert("mem197", intervalNow->timestamp[1] == 0);
	mu_assert("mem198", intervalNow->timestamp[2] == 4);
	mu_assert("mem199", intervalNow->timestamp[3] == 2);
	mu_assert("mem200", procArray[0].intervalList->next->timestamp[0] == 1);
	mu_assert("mem201", intervalNow->notices == NULL);
	mu_assert("mem202", procArray[2].intervalList->timestamp[0] == 0);
	mu_assert("mem203", procArray[2].intervalList->timestamp[1] == 0);
	mu_assert("mem204", procArray[2].intervalList->timestamp[2] == 3);
	mu_assert("mem205", procArray[2].intervalList->timestamp[3] == 2);
	mu_assert("mem206", procArray[2].intervalList->next->timestamp[2] == 1);
	mu_assert("mem207", procArray[2].intervalList->notices->pageIndex == 0);
	writenotice_t *tempwn = procArray[2].intervalList->notices;
	for(i = 0; i < MAX_WN_NUM; i++){
		tempwn = tempwn->nextInInterval;
	}
	mu_assert("mem208", tempwn->pageIndex == MAX_WN_NUM);
	return 0;
}


static char *test_grantDiff(){
	extern page_t pageArray[MAX_PAGE_NUM];
	extern proc_t procArray[MAX_HOST_NUM];
	extern interval_t *intervalNow;
	extern int parametertype;
	extern int sendMsgCalled; 
	extern int nextFreeMsgInQueueCalled;
	extern mimsg_t msg;
	extern int alreadyCalled;

	memset(pageArray, 0, sizeof(page_t) * MAX_PAGE_NUM);
	memset(procArray, 0, sizeof(proc_t) * MAX_HOST_NUM);

	pageArray[0].state = WRITE;
	pageArray[1].state = RDONLY;
	pageArray[2].state = UNMAP;
	pageArray[3].state = MISS;

	myhostid = 0;
	hostnum = 4;
	interval_t *interval1 = malloc(sizeof(interval_t));
	memset(interval1, 0, sizeof(interval_t));
	procArray[myhostid].intervalList = interval1;

	interval_t *interval2 = malloc(sizeof(interval_t));
	memset(interval2, 0, sizeof(interval_t));
	interval2->timestamp[0] = 1;
	interval2->next = procArray[myhostid].intervalList;
	procArray[myhostid].intervalList->prev = interval2;
	procArray[myhostid].intervalList = interval2;

	interval_t *interval3 = malloc(sizeof(interval_t));
	memset(interval3, 0, sizeof(interval_t));
	interval3->timestamp[0] = 2;
	interval3->next = procArray[myhostid].intervalList;
	procArray[myhostid].intervalList->prev = interval3;
	procArray[myhostid].intervalList = interval3;

	writenotice_t *wn1 = malloc(sizeof(writenotice_t));
	memset(wn1, 0, sizeof(writenotice_t));
	wn1->interval = interval3;
	wn1->pageIndex = 0;
	interval3->notices = wn1;
	pageArray[0].notices[myhostid] = wn1;

	writenotice_t *wn2 = malloc(sizeof(writenotice_t));
	memset(wn2, 0, sizeof(writenotice_t));
	wn2->interval = interval3;
	wn2->pageIndex = 1;
	interval3->notices->nextInInterval = wn2;
	pageArray[1].notices[myhostid] = wn2;

	writenotice_t *wn3 = malloc(sizeof(writenotice_t));
	memset(wn3, 0, sizeof(writenotice_t));
	wn3->interval = interval2;
	wn2->pageIndex = 0;
	interval2->notices = wn3;
	pageArray[0].notices[myhostid]->nextInPage = wn3;


	pageArray[0].address = malloc(PAGESIZE);
	memset(pageArray[0].address, 0, PAGESIZE);
	((char *)pageArray[0].address)[0] = 0;
	((char *)pageArray[0].address)[1] = 1;
	((char *)pageArray[0].address)[2] = 2;
	((char *)pageArray[0].address)[3] = 3;
	((char *)pageArray[0].address)[4] = 4;
	pageArray[0].twinPage = malloc(PAGESIZE);
	memset(pageArray[0].twinPage, 0, PAGESIZE);

	pageArray[1].address = malloc(PAGESIZE);
	memset(pageArray[1].address, 0, PAGESIZE);
	pageArray[1].notices[myhostid]->diffAddress = malloc(PAGESIZE);
	memset(pageArray[1].notices[myhostid]->diffAddress, 0, PAGESIZE);
	((char *)wn2->diffAddress)[0] = 1;
	((char *)wn2->diffAddress)[1] = 2;
	((char *)wn2->diffAddress)[2] = 3;
	((char *)wn2->diffAddress)[3] = 4;
	((char *)wn2->diffAddress)[4] = 5;
	((char *)wn2->diffAddress)[5] = 6;
	((char *)wn2->diffAddress)[6] = 7;
	((char *)wn2->diffAddress)[7] = 8;

	wn3->diffAddress = malloc(PAGESIZE);
	memset(wn3->diffAddress, 0, PAGESIZE);
	((char *)wn3->diffAddress)[0] = 0;
	((char *)wn3->diffAddress)[1] = 1;
	((char *)wn3->diffAddress)[2] = 2;
	((char *)wn3->diffAddress)[3] = 3;
	((char *)wn3->diffAddress)[4] = 4;
	((char *)wn3->diffAddress)[5] = 5;
	((char *)wn3->diffAddress)[6] = 6;
	((char *)wn3->diffAddress)[7] = 7;


	parametertype = 1;
	sendMsgCalled = 0;
	nextFreeMsgInQueueCalled = 0;
	memset(&msg, 0, sizeof(mimsg_t));
	
	int timestamp[MAX_HOST_NUM];
	int i;
	for(i = 0; i < MAX_HOST_NUM; i++){
		timestamp[i] = 0;
	}
	timestamp[0] = 1;

	alreadyCalled = 0;
	mu_assert("mem209", grantDiff(2, timestamp, 0) == 0);
	mu_assert("mem210", parametertype == 0);
	mu_assert("mem211", sendMsgCalled == 1);
	mu_assert("mem212", nextFreeMsgInQueueCalled == 1);
	mu_assert("mem213", msg.from == myhostid);
	mu_assert("mem214", msg.to == 2);
	mu_assert("mem215", msg.command == GRANT_DIFF);
	mu_assert("mem216", ((int *)msg.data)[0] == 1);
	mu_assert("mem217", ((int *)msg.data)[1] == 0);
	mu_assert("mem218", ((int *)msg.data)[2] == 0);
	mu_assert("mem219", ((int *)msg.data)[3] == 0);
	mu_assert("mem220", ((int *)msg.data)[20] == 0);
	char *diff = msg.data + sizeof(int)*21;
	mu_assert("mem221", diff[0] == 0);
	mu_assert("mem222", diff[1] == 1);
	mu_assert("mem223", diff[2] == 2);
	mu_assert("mem224", diff[3] == 3);
	mu_assert("mem225", diff[4] == 4);
	mu_assert("mem226", diff[5] == 5);
	mu_assert("mem227", diff[6] == 6);
	mu_assert("mem228", diff[7] == 7);

	parametertype = 1;
	sendMsgCalled = 0;
	nextFreeMsgInQueueCalled = 0;
	memset(&msg, 0, sizeof(mimsg_t));
	
	for(i = 0; i < MAX_HOST_NUM; i++){
		timestamp[i] = 0;
	}
	timestamp[0] = 2;

	alreadyCalled = 0;
	mu_assert("mem229", grantDiff(2, timestamp, 0) == 0);
	mu_assert("mem230", parametertype == 0);
	mu_assert("mem231", sendMsgCalled == 1);
	mu_assert("mem232", nextFreeMsgInQueueCalled == 1);
	mu_assert("mem233", msg.from == myhostid);
	mu_assert("mem234", msg.to == 2);
	mu_assert("mem235", msg.command == GRANT_DIFF);
	mu_assert("mem236", ((int *)msg.data)[0] == 2);
	mu_assert("mem237", ((int *)msg.data)[1] == 0);
	mu_assert("mem238", ((int *)msg.data)[2] == 0);
	mu_assert("mem239", ((int *)msg.data)[3] == 0);
	mu_assert("mem240", ((int *)msg.data)[20] == 0);
	diff = msg.data + sizeof(int)*21;
	mu_assert("mem241", diff[0] == 0);
	mu_assert("mem242", diff[1] == 1);
	mu_assert("mem243", diff[2] == 2);
	mu_assert("mem244", diff[3] == 3);
	mu_assert("mem245", diff[4] == 4);

	mu_assert("mem246", pageArray[0].twinPage == NULL);

	parametertype = 1;
	sendMsgCalled = 0;
	nextFreeMsgInQueueCalled = 0;
	memset(&msg, 0, sizeof(mimsg_t));
	
	for(i = 0; i < MAX_HOST_NUM; i++){
		timestamp[i] = 0;
	}
	timestamp[0] = 2;

	alreadyCalled = 0;
	mu_assert("mem247", grantDiff(2, timestamp, 1) == 0);
	mu_assert("mem248", parametertype == 0);
	mu_assert("mem249", sendMsgCalled == 1);
	mu_assert("mem250", nextFreeMsgInQueueCalled == 1);
	mu_assert("mem251", msg.from == myhostid);
	mu_assert("mem252", msg.to == 2);
	mu_assert("mem253", msg.command == GRANT_DIFF);
	mu_assert("mem254", ((int *)msg.data)[0] == 2);
	mu_assert("mem255", ((int *)msg.data)[1] == 0);
	mu_assert("mem256", ((int *)msg.data)[2] == 0);
	mu_assert("mem257", ((int *)msg.data)[3] == 0);
	mu_assert("mem258", ((int *)msg.data)[20] == 1);
	diff = msg.data + sizeof(int)*21;
	mu_assert("mem259", diff[0] == 1);
	mu_assert("mem260", diff[1] == 2);
	mu_assert("mem261", diff[2] == 3);
	mu_assert("mem262", diff[3] == 4);
	mu_assert("mem263", diff[4] == 5);
	mu_assert("mem264", diff[5] == 6);
	mu_assert("mem265", diff[6] == 7);
	mu_assert("mem266", diff[7] == 8);

	alreadyCalled = 0;
	mu_assert("mem267", grantDiff(-1, timestamp, 1) == -1);
	mu_assert("mem268", grantDiff(4, timestamp, 1) == -1);
	mu_assert("mem269", grantDiff(0, timestamp, 1) == -1);
	mu_assert("mem270", grantDiff(2, NULL, 1) == -1);
	mu_assert("mem271", grantDiff(2, timestamp, -1) == -1);
	mu_assert("mem272", grantDiff(2, timestamp, MAX_PAGE_NUM) == -1);
	mu_assert("mem273", grantDiff(2, timestamp, 3) == -1);
	mu_assert("mem274", grantDiff(2, timestamp, 2) == -1);
	timestamp[2] = 3;
	mu_assert("mem275", grantDiff(2, timestamp, 1) == -2);
	
	return 0;
}


static char *test_handleGrantDiffMsg(){
	extern page_t pageArray[MAX_PAGE_NUM];
	extern proc_t procArray[MAX_HOST_NUM];
	extern int fetchDiffWaitFlag;

	memset(pageArray, 0, sizeof(page_t) * MAX_PAGE_NUM);
	memset(procArray, 0, sizeof(proc_t) * MAX_HOST_NUM);

	pageArray[0].state = WRITE;
	pageArray[1].state = RDONLY;
	pageArray[2].state = UNMAP;
	pageArray[3].state = MISS;

	myhostid = 0;
	hostnum = 4;
	interval_t *interval1 = malloc(sizeof(interval_t));
	memset(interval1, 0, sizeof(interval_t));
	procArray[myhostid].intervalList = interval1;

	interval_t *interval2 = malloc(sizeof(interval_t));
	memset(interval2, 0, sizeof(interval_t));
	interval2->timestamp[0] = 1;
	interval2->next = procArray[myhostid].intervalList;
	procArray[myhostid].intervalList->prev = interval2;
	procArray[myhostid].intervalList = interval2;

	interval_t *interval3 = malloc(sizeof(interval_t));
	memset(interval3, 0, sizeof(interval_t));
	interval3->timestamp[0] = 2;
	interval3->next = procArray[myhostid].intervalList;
	procArray[myhostid].intervalList->prev = interval3;
	procArray[myhostid].intervalList = interval3;

	writenotice_t *wn1 = malloc(sizeof(writenotice_t));
	memset(wn1, 0, sizeof(writenotice_t));
	wn1->interval = interval3;
	wn1->pageIndex = 0;
	interval3->notices = wn1;
	pageArray[0].notices[2] = wn1;

	writenotice_t *wn2 = malloc(sizeof(writenotice_t));
	memset(wn2, 0, sizeof(writenotice_t));
	wn2->interval = interval3;
	wn2->pageIndex = 1;
	interval3->notices->nextInInterval = wn2;
	pageArray[1].notices[2] = wn2;

	writenotice_t *wn3 = malloc(sizeof(writenotice_t));
	memset(wn3, 0, sizeof(writenotice_t));
	wn3->interval = interval2;
	wn2->pageIndex = 0;
	interval2->notices = wn3;
	pageArray[0].notices[2]->nextInPage = wn3;


	pageArray[0].address = malloc(PAGESIZE);
	memset(pageArray[0].address, 0, PAGESIZE);
	((char *)pageArray[0].address)[0] = 0;
	((char *)pageArray[0].address)[1] = 1;
	((char *)pageArray[0].address)[2] = 2;
	((char *)pageArray[0].address)[3] = 3;
	((char *)pageArray[0].address)[4] = 4;

	pageArray[1].address = malloc(PAGESIZE);
	memset(pageArray[1].address, 0, PAGESIZE);

	int timestamp[MAX_HOST_NUM];
	int i;
	for(i = 0; i < MAX_HOST_NUM; i++){
		timestamp[i] = 0;
	}
	timestamp[0] = 1;

	mimsg_t msg;

	msg.from = 2;
	msg.to = myhostid;
	msg.command = GRANT_DIFF;
	
	int pageIndex = 0;
	char *diff = (char *)malloc(PAGESIZE);
	memset(diff, 0, PAGESIZE);
	diff[0] = 0;
	diff[1] = 1;
	diff[2] = 2;
	diff[3] = 3;
	diff[4] = 4;
	diff[5] = 5;
	diff[6] = 6;
	diff[7] = 7;

	apendMsgData(&msg, (char *)timestamp, sizeof(int) * MAX_HOST_NUM);
	apendMsgData(&msg, (char *)&pageIndex, sizeof(int));
	apendMsgData(&msg, diff, PAGESIZE);
	
	fetchDiffWaitFlag = 1;
	handleGrantDiffMsg(&msg);

	mu_assert("mem276", wn3->diffAddress != NULL);
	mu_assert("mem276.1", ((char *)wn3->diffAddress)[0] == 0);
	mu_assert("mem276.2", ((char *)wn3->diffAddress)[1] == 1);
	mu_assert("mem276.3", ((char *)wn3->diffAddress)[2] == 2);
	mu_assert("mem276.4", ((char *)wn3->diffAddress)[3] == 3);
	mu_assert("mem276.5", ((char *)wn3->diffAddress)[4] == 4);
	mu_assert("mem276.6", ((char *)wn3->diffAddress)[5] == 5);
	mu_assert("mem276.7", ((char *)wn3->diffAddress)[6] == 6);
	mu_assert("mem276.8", ((char *)wn3->diffAddress)[7] == 7);
	mu_assert("mem277", ((char *)pageArray[0].address)[0] == 0);
	mu_assert("mem278", ((char *)pageArray[0].address)[1] == 2);
	mu_assert("mem279", ((char *)pageArray[0].address)[2] == 4);
	mu_assert("mem280", ((char *)pageArray[0].address)[3] == 6);
	mu_assert("mem281", ((char *)pageArray[0].address)[4] == 8);
	mu_assert("mem282", ((char *)pageArray[0].address)[5] == 5);
	mu_assert("mem283", ((char *)pageArray[0].address)[6] == 6);
	mu_assert("mem284", ((char *)pageArray[0].address)[7] == 7);
	mu_assert("mem285", ((char *)pageArray[0].address)[8] == 0);
	mu_assert("mem286", fetchDiffWaitFlag == 0);

	for(i = 0; i < MAX_HOST_NUM; i++){
		timestamp[i] = 0;
	}
	timestamp[0] = 2;

	memset(&msg, 0, sizeof(mimsg_t));
	msg.from = 2;
	msg.to = myhostid;
	msg.command = GRANT_DIFF;
	
	pageIndex = 0;
	diff = (char *)malloc(PAGESIZE);
	memset(diff, 0, PAGESIZE);
	diff[0] = 0;
	diff[1] = -1;
	diff[2] = -2;
	diff[3] = -3;
	diff[4] = -4;
	diff[5] = -5;
	diff[6] = -6;
	diff[7] = -7;

	apendMsgData(&msg, (char *)timestamp, sizeof(int) * MAX_HOST_NUM);
	apendMsgData(&msg, (char *)&pageIndex, sizeof(int));
	apendMsgData(&msg, diff, PAGESIZE);
	
	fetchDiffWaitFlag = 1;
	handleGrantDiffMsg(&msg);

	mu_assert("mem287", wn1->diffAddress != NULL);
	mu_assert("mem287.1", ((char *)wn1->diffAddress)[0] == 0);
	mu_assert("mem287.2", ((char *)wn1->diffAddress)[1] == -1);
	mu_assert("mem287.3", ((char *)wn1->diffAddress)[2] == -2);
	mu_assert("mem287.4", ((char *)wn1->diffAddress)[3] == -3);
	mu_assert("mem287.5", ((char *)wn1->diffAddress)[4] == -4);
	mu_assert("mem287.6", ((char *)wn1->diffAddress)[5] == -5);
	mu_assert("mem287.7", ((char *)wn1->diffAddress)[6] == -6);
	mu_assert("mem287.8", ((char *)wn1->diffAddress)[7] == -7);
	mu_assert("mem288", ((char *)pageArray[0].address)[0] == 0);
	mu_assert("mem289", ((char *)pageArray[0].address)[1] == 1);
	mu_assert("mem290", ((char *)pageArray[0].address)[2] == 2);
	mu_assert("mem291", ((char *)pageArray[0].address)[3] == 3);
	mu_assert("mem292", ((char *)pageArray[0].address)[4] == 4);
	mu_assert("mem293", ((char *)pageArray[0].address)[5] == 0);
	mu_assert("mem294", ((char *)pageArray[0].address)[6] == 0);
	mu_assert("mem295", ((char *)pageArray[0].address)[7] == 0);
	mu_assert("mem296", ((char *)pageArray[0].address)[8] == 0);
	mu_assert("mem297", fetchDiffWaitFlag == 0);

	for(i = 0; i < MAX_HOST_NUM; i++){
		timestamp[i] = 0;
	}
	timestamp[0] = 2;

	memset(&msg, 0, sizeof(mimsg_t));
	msg.from = 2;
	msg.to = myhostid;
	msg.command = GRANT_DIFF;
	
	pageIndex = 1;
	diff = (char *)malloc(PAGESIZE);
	memset(diff, 0, PAGESIZE);
	diff[0] = 1;
	diff[1] = 3;
	diff[2] = 5;
	diff[3] = 7;
	diff[4] = 9;
	diff[5] = 11;
	diff[6] = 13;
	diff[7] = 15;

	apendMsgData(&msg, (char *)timestamp, sizeof(int) * MAX_HOST_NUM);
	apendMsgData(&msg, (char *)&pageIndex, sizeof(int));
	apendMsgData(&msg, diff, PAGESIZE);
	
	fetchDiffWaitFlag = 1;
	handleGrantDiffMsg(&msg);

	mu_assert("mem298", wn2->diffAddress != NULL);
	mu_assert("mem298.1", ((char *)wn2->diffAddress)[0] == 1);
	mu_assert("mem298.2", ((char *)wn2->diffAddress)[1] == 3);
	mu_assert("mem298.3", ((char *)wn2->diffAddress)[2] == 5);
	mu_assert("mem298.4", ((char *)wn2->diffAddress)[3] == 7);
	mu_assert("mem298.5", ((char *)wn2->diffAddress)[4] == 9);
	mu_assert("mem298.6", ((char *)wn2->diffAddress)[5] == 11);
	mu_assert("mem298.7", ((char *)wn2->diffAddress)[6] == 13);
	mu_assert("mem298.8", ((char *)wn2->diffAddress)[7] == 15);
	mu_assert("mem299", ((char *)pageArray[1].address)[0] == 1);
	mu_assert("mem300", ((char *)pageArray[1].address)[1] == 3);
	mu_assert("mem301", ((char *)pageArray[1].address)[2] == 5);
	mu_assert("mem302", ((char *)pageArray[1].address)[3] == 7);
	mu_assert("mem303", ((char *)pageArray[1].address)[4] == 9);
	mu_assert("mem304", ((char *)pageArray[1].address)[5] == 11);
	mu_assert("mem305", ((char *)pageArray[1].address)[6] == 13);
	mu_assert("mem306", ((char *)pageArray[1].address)[7] == 15);
	mu_assert("mem307", ((char *)pageArray[1].address)[8] == 0);
	mu_assert("mem308", fetchDiffWaitFlag == 0);

	fetchDiffWaitFlag = 1;
	handleGrantDiffMsg(NULL);
	mu_assert("mem309", fetchDiffWaitFlag == 1);


	return 0;
}


static char *test_grantPage(){
	extern page_t pageArray[MAX_PAGE_NUM];
	extern proc_t procArray[MAX_HOST_NUM];
	extern int parametertype;
	extern int sendMsgCalled; 
	extern int nextFreeMsgInQueueCalled;
	extern mimsg_t msg;
	extern int alreadyCalled;

	memset(pageArray, 0, sizeof(page_t) * MAX_PAGE_NUM);
	memset(procArray, 0, sizeof(proc_t) * MAX_HOST_NUM);

	pageArray[0].state = WRITE;
	pageArray[1].state = RDONLY;
	pageArray[2].state = UNMAP;
	pageArray[3].state = MISS;

	pageArray[0].address = malloc(PAGESIZE);
	memset(pageArray[0].address, 0, PAGESIZE);

	((char *)pageArray[0].address)[0] = 0;
	((char *)pageArray[0].address)[1] = 1;
	((char *)pageArray[0].address)[2] = 2;
	((char *)pageArray[0].address)[3] = 3;
	((char *)pageArray[0].address)[4] = 4;

	pageArray[1].address = malloc(PAGESIZE);
	memset(pageArray[1].address, 0, PAGESIZE);
	((char *)pageArray[1].address)[0] = -1;
	((char *)pageArray[1].address)[1] = -2;
	((char *)pageArray[1].address)[2] = -3;
	((char *)pageArray[1].address)[3] = -4;
	((char *)pageArray[1].address)[4] = -5;
	((char *)pageArray[1].address)[5] = -6;

	parametertype = 1;	
	sendMsgCalled = 0;
	nextFreeMsgInQueueCalled = 0;
	memset(&msg, 0, sizeof(mimsg_t));

	alreadyCalled = 0;
	mu_assert("mem310", grantPage(2, 0) == 0);
	mu_assert("mem311", parametertype == 0);
	mu_assert("mem312", sendMsgCalled == 1);
	mu_assert("mem313", nextFreeMsgInQueueCalled == 1);
	mu_assert("mem314", msg.from == myhostid);
	mu_assert("mem315", msg.to == 2);
	mu_assert("mem316", msg.command == GRANT_PAGE);
	mu_assert("mem317", msg.size == (PAGESIZE + sizeof(int)));
	mu_assert("mem318", *((int *)msg.data) == 0);
	void *pageAddress = msg.data + sizeof(int);
	mu_assert("mem319", ((char *)pageAddress)[0] == 0);
	mu_assert("mem320", ((char *)pageAddress)[1] == 1);
	mu_assert("mem321", ((char *)pageAddress)[2] == 2);
	mu_assert("mem322", ((char *)pageAddress)[3] == 3);
	mu_assert("mem323", ((char *)pageAddress)[4] == 4);
	mu_assert("mem324", ((char *)pageAddress)[5] == 0);
	
	parametertype = 1;	
	sendMsgCalled = 0;
	nextFreeMsgInQueueCalled = 0;
	memset(&msg, 0, sizeof(mimsg_t));

	alreadyCalled = 0;
	mu_assert("mem325", grantPage(2, 1) == 0);
	mu_assert("mem326", parametertype == 0);
	mu_assert("mem327", sendMsgCalled == 1);
	mu_assert("mem328", nextFreeMsgInQueueCalled == 1);
	mu_assert("mem329", msg.from == myhostid);
	mu_assert("mem330", msg.to == 2);
	mu_assert("mem331", msg.command == GRANT_PAGE);
	mu_assert("mem332", msg.size == (PAGESIZE + sizeof(int)));
	mu_assert("mem333", *((int *)msg.data) == 1);
	pageAddress = msg.data + sizeof(int);
	mu_assert("mem334", ((char *)pageAddress)[0] == -1);
	mu_assert("mem335", ((char *)pageAddress)[1] == -2);
	mu_assert("mem336", ((char *)pageAddress)[2] == -3);
	mu_assert("mem337", ((char *)pageAddress)[3] == -4);
	mu_assert("mem338", ((char *)pageAddress)[4] == -5);
	mu_assert("mem339", ((char *)pageAddress)[5] == -6);
	mu_assert("mem340", ((char *)pageAddress)[6] == 0);

	alreadyCalled = 0;
	mu_assert("mem341", grantPage(-1, 1) == -1);
	mu_assert("mem342", grantPage(hostnum, 1) == -1);
	mu_assert("mem343", grantPage(myhostid, 1) == -1);
	mu_assert("mem344", grantPage(2, -1) == -1);
	mu_assert("mem345", grantPage(2, MAX_PAGE_NUM) == -1);
	mu_assert("mem346", grantPage(2, 3) == -1);
	
	return 0;
}


static char *test_handleGrantPageMsg(){
	extern page_t pageArray[MAX_PAGE_NUM];
	extern long mapfd;
	extern int fetchPageWaitFlag;
	extern interval_t *intervalNow;

	memset(pageArray, 0, sizeof(page_t) * MAX_PAGE_NUM);
	mapfd = open("/dev/zero", O_RDWR, 0);
	intervalNow = malloc(sizeof(interval_t));
	memset(intervalNow, 0, sizeof(interval_t));
	intervalNow->timestamp[0] = 1;
	intervalNow->timestamp[1] = 1;
	intervalNow->timestamp[2] = 2;
	intervalNow->timestamp[3] = 1;

	pageArray[0].state = MISS;
	pageArray[0].address = (void *)START_ADDRESS;
	pageArray[1].state = MISS;
	pageArray[1].address = (void *)START_ADDRESS + PAGESIZE;
	pageArray[2].state = UNMAP;
	pageArray[3].state = MISS;

	mimsg_t msg;
	memset(&msg, 0, sizeof(mimsg_t));

	msg.from = 2;
	msg.to = myhostid;
	msg.command = GRANT_PAGE;
	int pageIndex = 0;
	apendMsgData(&msg, (char *)&pageIndex, sizeof(int));
	
	char *pageAddress = (char *)malloc(PAGESIZE);
	memset(pageAddress, 0, PAGESIZE);
	pageAddress[0] = 0;
	pageAddress[1] = 1;
	pageAddress[2] = 2;
	pageAddress[3] = 3;
	pageAddress[4] = 4;
	apendMsgData(&msg, pageAddress, PAGESIZE);
	
	fetchPageWaitFlag = 1;
	handleGrantPageMsg(&msg);
	mu_assert("mem347", ((char *)pageArray[0].address)[0] == 0);
	mu_assert("mem348", ((char *)pageArray[0].address)[1] == 1);
	mu_assert("mem349", ((char *)pageArray[0].address)[2] == 2);
	mu_assert("mem350", ((char *)pageArray[0].address)[3] == 3);
	mu_assert("mem351", ((char *)pageArray[0].address)[4] == 4);
	mu_assert("mem352", ((char *)pageArray[0].address)[5] == 0);
	mu_assert("mem352.1", pageArray[0].startInterval->timestamp[0] == 1);
	mu_assert("mem352.2", pageArray[0].startInterval->timestamp[1] == 1);
	mu_assert("mem352.3", pageArray[0].startInterval->timestamp[2] == 2);
	mu_assert("mem352.4", pageArray[0].startInterval->timestamp[3] == 1);
	mu_assert("mem353", fetchPageWaitFlag == 0);
	
	fetchPageWaitFlag = 1;
	handleGrantPageMsg(NULL);
	mu_assert("mem353", fetchPageWaitFlag == 1);

	return 0;
}


static char *test_initmem(){
	extern page_t pageArray[MAX_PAGE_NUM];
	extern proc_t procArray[MAX_HOST_NUM];
	extern long mapfd;
	extern int fetchPageWaitFlag;
	extern int fetchDiffWaitFlag;
	extern int fetchWNIWaitFlag;
	extern interval_t *intervalNow;
	extern int pagenum;
	extern void *globalAddress;
	extern int barrierTimestamps[MAX_HOST_NUM][MAX_HOST_NUM];
	
	myhostid = 0;
	hostnum = 4;
	
	initmem();
	mu_assert("mem354", pageArray[0].address == NULL);
	mu_assert("mem355", pageArray[0].notices[0] == NULL);
	mu_assert("mem356", pageArray[0].state == UNMAP);
	mu_assert("mem357", pageArray[0].twinPage == NULL);
	mu_assert("mem358", pageArray[0].startInterval == NULL);
	mu_assert("mem359", pageArray[MAX_PAGE_NUM-1].address == NULL);
	mu_assert("mem360", pageArray[MAX_PAGE_NUM-1].notices[0] == NULL);
	mu_assert("mem361", pageArray[MAX_PAGE_NUM-1].state == UNMAP);
	mu_assert("mem362", pageArray[MAX_PAGE_NUM-1].twinPage == NULL);
	mu_assert("mem363", pageArray[MAX_PAGE_NUM-1].startInterval == NULL);
	
	mu_assert("mem364", procArray[myhostid].hostid == myhostid);
	mu_assert("mem365", procArray[myhostid].intervalList != NULL);
	mu_assert("mem366", procArray[myhostid].intervalList == intervalNow);
	mu_assert("mem367", procArray[1].hostid == 1);
	mu_assert("mem368", procArray[1].intervalList == NULL);
	mu_assert("mem369", intervalNow->timestamp[0] == 0);
	mu_assert("mem369.1", intervalNow->isBarrier == 1);
	mu_assert("mem370", fetchPageWaitFlag == 0);
	mu_assert("mem371", fetchDiffWaitFlag == 0);
	mu_assert("mem372", fetchWNIWaitFlag == 0);
	mu_assert("mem373", pagenum == 0);
	mu_assert("mem374", mapfd != 0);
	mu_assert("mem375", globalAddress == NULL);

	mu_assert("mem375.1", barrierTimestamps[0][0] == -1);
	mu_assert("mem375.2", barrierTimestamps[0][MAX_HOST_NUM-1] == -1);
	mu_assert("mem375.3", barrierTimestamps[1][0] == -1);
	mu_assert("mem375.4", barrierTimestamps[1][MAX_HOST_NUM-1] == -1);
	mu_assert("mem375.5", barrierTimestamps[2][0] == -1);
	mu_assert("mem375.6", barrierTimestamps[2][MAX_HOST_NUM-1] == -1);
	mu_assert("mem375.7", barrierTimestamps[3][0] == -1);
	mu_assert("mem375.8", barrierTimestamps[3][MAX_HOST_NUM-1] == -1);
	return 0;
}


static char *test_mi_alloc(){
	extern page_t pageArray[MAX_PAGE_NUM];
	extern proc_t procArray[MAX_HOST_NUM];
	extern int pagenum;
	extern void *globalAddress;
	
	myhostid = 0;
	hostnum = 4;
	
	initmem();
//normal condition 1 page
	void *address = mi_alloc(PAGESIZE);
	mu_assert("mem376", address != NULL);
	mu_assert("mem378", pageArray[0].state == RDONLY);
	mu_assert("mem379", pageArray[0].address == address);
	mu_assert("mem380", pageArray[0].notices[0] == NULL);
	mu_assert("mem381", pageArray[0].twinPage == NULL);
	mu_assert("mem382", pageArray[0].startInterval == NULL);
	mu_assert("mem383", pagenum == 1);
	mu_assert("mem384", globalAddress == (void *)PAGESIZE);
//normal condition small than 1 page
	address = mi_alloc(1);
	mu_assert("mem385", address != NULL);
	mu_assert("mem386", pageArray[1].state == MISS);
	mu_assert("mem387", pageArray[1].address == address);
	mu_assert("mem388", pageArray[1].notices[0] == NULL);
	mu_assert("mem389", pageArray[1].twinPage == NULL);
	mu_assert("mem390", pageArray[1].startInterval == NULL);
	mu_assert("mem391", pagenum == 2);
	mu_assert("mem392", globalAddress == (void *)(2*PAGESIZE));
//normal condition slightly bigger than 1 page
	address = mi_alloc(PAGESIZE+1);
	mu_assert("mem393", address != NULL);
	mu_assert("mem394", pageArray[2].state == MISS);
	mu_assert("mem395", pageArray[2].address == address);
	mu_assert("mem396", pageArray[2].notices[0] == NULL);
	mu_assert("mem397", pageArray[2].twinPage == NULL);
	mu_assert("mem398", pageArray[2].startInterval == NULL);
	mu_assert("mem399", pageArray[3].state == MISS);
	mu_assert("mem400", pageArray[3].address == (address + PAGESIZE));
	mu_assert("mem401", pageArray[3].notices[0] == NULL);
	mu_assert("mem402", pageArray[3].twinPage == NULL);
	mu_assert("mem403", pageArray[3].startInterval == NULL);
	mu_assert("mem404", pagenum == 4);
	mu_assert("mem405", globalAddress == (void *)(PAGESIZE*4));
//normal condition 5 page
	address = mi_alloc(PAGESIZE*5);
	mu_assert("mem406", address != NULL);
	mu_assert("mem407", pageArray[4].state == RDONLY);
	mu_assert("mem408", pageArray[4].address == address);
	mu_assert("mem409", pageArray[4].notices[0] == NULL);
	mu_assert("mem410", pageArray[4].twinPage == NULL);
	mu_assert("mem411", pageArray[4].startInterval == NULL);
	mu_assert("mem412", pageArray[5].state == MISS);
	mu_assert("mem413", pageArray[5].address == (address + PAGESIZE));
	mu_assert("mem414", pageArray[5].notices[0] == NULL);
	mu_assert("mem415", pageArray[5].twinPage == NULL);
	mu_assert("mem416", pageArray[5].startInterval == NULL);
	mu_assert("mem417", pageArray[6].state == MISS);
	mu_assert("mem418", pageArray[6].address == (address + PAGESIZE*2));
	mu_assert("mem419", pageArray[6].notices[0] == NULL);
	mu_assert("mem420", pageArray[6].twinPage == NULL);
	mu_assert("mem421", pageArray[6].startInterval == NULL);
	mu_assert("mem422", pageArray[7].state == MISS);
	mu_assert("mem423", pageArray[7].address == (address + PAGESIZE*3));
	mu_assert("mem424", pageArray[7].notices[0] == NULL);
	mu_assert("mem425", pageArray[7].twinPage == NULL);
	mu_assert("mem426", pageArray[7].startInterval == NULL);
	mu_assert("mem427", pageArray[8].state == RDONLY);
	mu_assert("mem428", pageArray[8].address == (address+ PAGESIZE*4));
	mu_assert("mem429", pageArray[8].notices[0] == NULL);
	mu_assert("mem430", pageArray[8].twinPage == NULL);
	mu_assert("mem431", pageArray[8].startInterval == NULL);
	mu_assert("mem432", pagenum == 9);
	mu_assert("mem433", globalAddress == (void *)(PAGESIZE*9));
//normal condition slightly smaller than 5 page
	address = mi_alloc(PAGESIZE*5-1);
	mu_assert("mem434", address != NULL);
	mu_assert("mem435", pageArray[9].state == MISS);
	mu_assert("mem436", pageArray[9].address == address);
	mu_assert("mem437", pageArray[9].notices[0] == NULL);
	mu_assert("mem438", pageArray[9].twinPage == NULL);
	mu_assert("mem439", pageArray[9].startInterval == NULL);
	mu_assert("mem440", pageArray[10].state == MISS);
	mu_assert("mem441", pageArray[10].address == (address + PAGESIZE));
	mu_assert("mem442", pageArray[10].notices[0] == NULL);
	mu_assert("mem443", pageArray[10].twinPage == NULL);
	mu_assert("mem444", pageArray[10].startInterval == NULL);
	mu_assert("mem445", pageArray[11].state == MISS);
	mu_assert("mem446", pageArray[11].address == (address + PAGESIZE*2));
	mu_assert("mem447", pageArray[11].notices[0] == NULL);
	mu_assert("mem448", pageArray[11].twinPage == NULL);
	mu_assert("mem449", pageArray[11].startInterval == NULL);
	mu_assert("mem450", pageArray[12].state == RDONLY);
	mu_assert("mem451", pageArray[12].address == (address + PAGESIZE*3));
	mu_assert("mem452", pageArray[12].notices[0] == NULL);
	mu_assert("mem453", pageArray[12].twinPage == NULL);
	mu_assert("mem454", pageArray[12].startInterval == NULL);
	mu_assert("mem454.1", pageArray[13].state == MISS);
	mu_assert("mem454.2", pageArray[13].address == (address + PAGESIZE*4));
	mu_assert("mem454.3", pageArray[13].notices[0] == NULL);
	mu_assert("mem454.4", pageArray[13].twinPage == NULL);
	mu_assert("mem454.5", pageArray[13].startInterval == NULL);

	mu_assert("mem455", pagenum == 14);
	mu_assert("mem456", globalAddress == (void *)(PAGESIZE*14));
//normal condition slightly bigger than 5 page
	address = mi_alloc(PAGESIZE*5+1);
	mu_assert("mem456", address != NULL);
	mu_assert("mem457", pageArray[14].state == MISS);
	mu_assert("mem458", pageArray[14].address == address);
	mu_assert("mem459", pageArray[14].notices[0] == NULL);
	mu_assert("mem460", pageArray[14].twinPage == NULL);
	mu_assert("mem461", pageArray[14].startInterval == NULL);
	mu_assert("mem462", pageArray[15].state == MISS);
	mu_assert("mem463", pageArray[15].address == (address + PAGESIZE));
	mu_assert("mem464", pageArray[15].notices[0] == NULL);
	mu_assert("mem465", pageArray[15].twinPage == NULL);
	mu_assert("mem466", pageArray[15].startInterval == NULL);
	mu_assert("mem467", pageArray[16].state == RDONLY);
	mu_assert("mem468", pageArray[16].address == (address + PAGESIZE*2));
	mu_assert("mem469", pageArray[16].notices[0] == NULL);
	mu_assert("mem470", pageArray[16].twinPage == NULL);
	mu_assert("mem471", pageArray[16].startInterval == NULL);
	mu_assert("mem472", pageArray[17].state == MISS);
	mu_assert("mem473", pageArray[17].address == (address + PAGESIZE*3));
	mu_assert("mem474", pageArray[17].notices[0] == NULL);
	mu_assert("mem475", pageArray[17].twinPage == NULL);
	mu_assert("mem476", pageArray[17].startInterval == NULL);
	mu_assert("mem477", pageArray[18].state == MISS);
	mu_assert("mem478", pageArray[18].address == (address + PAGESIZE*4));
	mu_assert("mem479", pageArray[18].notices[0] == NULL);
	mu_assert("mem480", pageArray[18].twinPage == NULL);
	mu_assert("mem481", pageArray[18].startInterval == NULL);
	mu_assert("mem481.1", pageArray[19].state == MISS);
	mu_assert("mem481.2", pageArray[19].address == (address + PAGESIZE*5));
	mu_assert("mem481.3", pageArray[19].notices[0] == NULL);
	mu_assert("mem481.4", pageArray[19].twinPage == NULL);
	mu_assert("mem481.5", pageArray[19].startInterval == NULL);
	mu_assert("mem482", pagenum == 20);
	mu_assert("mem483", globalAddress == (void *)(PAGESIZE*20));
//no enough memory	
	address = mi_alloc(MAX_PAGE_NUM * PAGESIZE);
	mu_assert("mem484", address == NULL);
	mu_assert("mem485", pageArray[20].state == UNMAP);
	mu_assert("mem486", pageArray[20].address == NULL);
//parameters error	
	address = mi_alloc(0);
	mu_assert("mem487", address == NULL);
	mu_assert("mem488", pageArray[20].state == UNMAP);
	mu_assert("mem489", pageArray[20].address == NULL);

	address = mi_alloc(-1);
	mu_assert("mem490", address == NULL);
	mu_assert("mem491", pageArray[20].state == UNMAP);
	mu_assert("mem492", pageArray[20].address == NULL);

	return 0;
}


static char *test_sendEnterBarrierInfo(){
	extern page_t pageArray[MAX_PAGE_NUM];
	extern proc_t procArray[MAX_HOST_NUM];
	extern int parametertype;
	extern int sendMsgCalled; 
	extern int nextFreeMsgInQueueCalled;
	extern mimsg_t msg;
	extern mimsg_t msg2;
	extern int alreadyCalled;
	extern interval_t *intervalNow;

	myhostid = 1;
	hostnum = 4;
	
	initmem();	
//myhostid == 0
	parametertype = 1;
	sendMsgCalled = 0;
	nextFreeMsgInQueueCalled = 0;
	memset(&msg, 0, sizeof(mimsg_t));

	myhostid = 0;
	sendEnterBarrierInfo();
	mu_assert("mem493", parametertype == 1);
	mu_assert("mem494", sendMsgCalled == 0);
	mu_assert("mem495", nextFreeMsgInQueueCalled == 0);
//send one msg 0 wnPacket
	parametertype = 1;
	sendMsgCalled = 0;
	nextFreeMsgInQueueCalled = 0;
	alreadyCalled = 0;
	memset(&msg, 0, sizeof(mimsg_t));
	int i;
	for(i = 0; i < MAX_HOST_NUM; i++){
		msg.timestamp[i] = -1;
	}
	
	myhostid = 1;
	sendEnterBarrierInfo();
	mu_assert("mem495.1", parametertype == 0);
	mu_assert("mem495.2", sendMsgCalled == 1);
	mu_assert("mem495.3", nextFreeMsgInQueueCalled == 1);
	mu_assert("mem496", msg.from == 1);
	mu_assert("mem497", msg.to == 0);
	mu_assert("mem498", msg.command == GRANT_ENTER_BARRIER_INFO);
	mu_assert("mem499", msg.timestamp[0] == 0);
	mu_assert("mem500", msg.timestamp[1] == 0);
	mu_assert("mem501", msg.timestamp[2] == 0);
	mu_assert("mem502", msg.timestamp[3] == 0);
	mu_assert("mem503", msg.size == sizeof(int));
	int packetCount = *((int *)msg.data);
	mu_assert("mem504", packetCount == 0);
//send 1 wnPacket

	initmem();
	interval_t *interval1 = malloc(sizeof(interval_t));
	memset(interval1, 0, sizeof(interval_t));
	interval1->timestamp[1] = 1;
	procArray[1].hostid = 1;	
	interval1->next = procArray[1].intervalList;
	procArray[1].intervalList->prev = interval1;
	procArray[1].intervalList = interval1;
	intervalNow = interval1;
	writenotice_t *lastWnInInterval = NULL;
	int wnCount = 3;
	for(i = 0; i < wnCount; i++){
		writenotice_t *wn = malloc(sizeof(writenotice_t));
		wn->interval = interval1;
		wn->nextInPage = pageArray[i].notices[1];
		wn->pageIndex = i;
		pageArray[i].notices[0] = wn;
		wn->nextInInterval = NULL;
		if(lastWnInInterval == NULL){
			interval1->notices = lastWnInInterval = wn;
		}else{
			lastWnInInterval->nextInInterval = wn;
			lastWnInInterval = wn;
		}
	}

	parametertype = 1;
	sendMsgCalled = 0;
	nextFreeMsgInQueueCalled = 0;
	alreadyCalled = 0;
	memset(&msg, 0, sizeof(mimsg_t));
	for(i = 0; i < MAX_HOST_NUM; i++){
		msg.timestamp[i] = -1;
	}
	
	myhostid = 1;
	sendEnterBarrierInfo();	
	mu_assert("mem505", parametertype == 0);
	mu_assert("mem506", sendMsgCalled == 1);
	mu_assert("mem507", nextFreeMsgInQueueCalled == 1);
	mu_assert("mem508", alreadyCalled == 1);
	mu_assert("mem511", msg.from == 1);
	mu_assert("mem512", msg.to == 0);
	mu_assert("mem513", msg.command == GRANT_ENTER_BARRIER_INFO);
	mu_assert("mem514", msg.timestamp[0] == 0);
	mu_assert("mem515", msg.timestamp[1] == 1);
	mu_assert("mem516", msg.timestamp[2] == 0);
	mu_assert("mem517", msg.timestamp[3] == 0);
	mu_assert("mem518", msg.size == (sizeof(int)+sizeof(wnPacket_t)*1));
	packetCount = *((int *)msg.data);
	mu_assert("mem519", packetCount == 1);
	wnPacket_t *packet = (wnPacket_t *)(msg.data+sizeof(int));
	mu_assert("mem520", packet->hostid == 1);
	mu_assert("mem521", packet->timestamp[0] == 0);
	mu_assert("mem522", packet->timestamp[1] == 1);
	mu_assert("mem523", packet->timestamp[2] == 0);
	mu_assert("mem524", packet->timestamp[3] == 0);
	mu_assert("mem525", packet->wnCount == 3);
	mu_assert("mem526", packet->wnArray[0] == 0);
	mu_assert("mem527", packet->wnArray[1] == 1);
	mu_assert("mem528", packet->wnArray[2] == 2);

//send two wnPacket for one interval
	interval_t *interval2 = malloc(sizeof(interval_t));
	memset(interval2, 0, sizeof(interval_t));
	interval2->timestamp[1] = 2;
	interval2->isBarrier = 1;
	procArray[1].intervalList->prev = interval2;
	interval2->next = procArray[1].intervalList;
	procArray[1].intervalList = interval2;	
	
	interval_t *interval3 = malloc(sizeof(interval_t));
	memset(interval3, 0, sizeof(interval_t));
	interval3->timestamp[1] = 3;
	interval3->next = procArray[1].intervalList;
	procArray[1].intervalList->prev = interval3;
	procArray[1].intervalList = interval3;	
	intervalNow = interval3;
	lastWnInInterval = NULL;
	wnCount = MAX_WN_NUM+1;
	for(i = 0; i < wnCount; i++){
		writenotice_t *wn = malloc(sizeof(writenotice_t));
		wn->interval = interval3;
		wn->nextInPage = pageArray[i].notices[1];
		wn->pageIndex = i;
		pageArray[i].notices[0] = wn;
		wn->nextInInterval = NULL;
		if(lastWnInInterval == NULL){
			interval3->notices = lastWnInInterval = wn;
		}else{
			lastWnInInterval->nextInInterval = wn;
			lastWnInInterval = wn;
		}
	}

	parametertype = 1;
	sendMsgCalled = 0;
	nextFreeMsgInQueueCalled = 0;
	alreadyCalled = 0;
	memset(&msg, 0, sizeof(mimsg_t));
	for(i = 0; i < MAX_HOST_NUM; i++){
		msg.timestamp[i] = -1;
	}
	
	myhostid = 1;
	sendEnterBarrierInfo();	
	mu_assert("mem528.1", parametertype == 0);
	mu_assert("mem528.2", sendMsgCalled == 1);
	mu_assert("mem528.3", nextFreeMsgInQueueCalled == 1);
	mu_assert("mem529", msg.from == 1);
	mu_assert("mem530", msg.to == 0);
	mu_assert("mem531", msg.command == GRANT_ENTER_BARRIER_INFO);
	mu_assert("mem532", msg.timestamp[0] == 0);
	mu_assert("mem533", msg.timestamp[1] == 3);
	mu_assert("mem534", msg.timestamp[2] == 0);
	mu_assert("mem535", msg.timestamp[3] == 0);
	mu_assert("mem536", msg.size == (sizeof(int)+sizeof(wnPacket_t)*2));
	packetCount = *((int *)msg.data);
	mu_assert("mem537", packetCount == 2);
	packet = (wnPacket_t *)(msg.data+sizeof(int));
	mu_assert("mem544", packet->hostid == 1);
	mu_assert("mem545", packet->timestamp[0] == 0);
	mu_assert("mem546", packet->timestamp[1] == 3);
	mu_assert("mem547", packet->timestamp[2] == 0);
	mu_assert("mem548", packet->timestamp[3] == 0);
	mu_assert("mem549", packet->wnCount == MAX_WN_NUM);
	mu_assert("mem550", packet->wnArray[0] == 0);
	mu_assert("mem551", packet->wnArray[1] == 1);
	mu_assert("mem552", packet->wnArray[2] == 2);
	mu_assert("mem553", packet->wnArray[MAX_WN_NUM-1] == (MAX_WN_NUM-1));
	packet = packet+1;
	mu_assert("mem554", packet->hostid == 1);
	mu_assert("mem555", packet->timestamp[0] == 0);
	mu_assert("mem556", packet->timestamp[1] == 3);
	mu_assert("mem557", packet->timestamp[2] == 0);
	mu_assert("mem558", packet->timestamp[3] == 0);
	mu_assert("mem559", packet->wnCount == 1);
	mu_assert("mem560", packet->wnArray[0] == MAX_WN_NUM);
//send two msgs
	interval_t *interval4 = malloc(sizeof(interval_t));
	memset(interval4, 0, sizeof(interval_t));
	interval4->timestamp[1] = 4;
	interval4->isBarrier = 1;
	procArray[1].intervalList->prev = interval4;
	interval4->next = procArray[1].intervalList;
	procArray[1].intervalList = interval4;	
	
	interval_t *interval5 = NULL;
	int temp = 5;
	int intervalCount = 11;
	for(i = 0; i < intervalCount; i++){
		interval5 = malloc(sizeof(interval_t));
		memset(interval5, 0, sizeof(interval_t));
		interval5->timestamp[1] = temp;
		procArray[1].intervalList->prev = interval5;
		interval5->next = procArray[1].intervalList;
		procArray[1].intervalList = interval5;	
		temp++;
		lastWnInInterval = NULL;
		wnCount = 1;
		int j;
		for(j = 0; j < wnCount; j++){
			writenotice_t *wn = malloc(sizeof(writenotice_t));
			wn->interval = interval5;
			wn->nextInPage = pageArray[i].notices[1];
			wn->pageIndex = j;
			pageArray[j].notices[0] = wn;
			wn->nextInInterval = NULL;
			if(lastWnInInterval == NULL){
				interval5->notices = lastWnInInterval = wn;
			}else{
				lastWnInInterval->nextInInterval = wn;
				lastWnInInterval = wn;
			}
		}
	}
	intervalNow = interval5;

	parametertype = 1;
	sendMsgCalled = 0;
	nextFreeMsgInQueueCalled = 0;
	alreadyCalled = 0;
	memset(&msg, 0, sizeof(mimsg_t));
	for(i = 0; i < MAX_HOST_NUM; i++){
		msg.timestamp[i] = -1;
	}
	memset(&msg2, 0, sizeof(mimsg_t));
	for(i = 0; i < MAX_HOST_NUM; i++){
		msg2.timestamp[i] = -1;
	}
	
	myhostid = 1;
	sendEnterBarrierInfo();	

	mu_assert("mem561", parametertype == 0);
	mu_assert("mem562", sendMsgCalled == 1);
	mu_assert("mem563", nextFreeMsgInQueueCalled == 1);
	mu_assert("mem564", alreadyCalled == 1);
	mu_assert("mem565", msg.from == 1);
	mu_assert("mem566", msg.to == 0);
	mu_assert("mem567", msg.command == GRANT_ENTER_BARRIER_INFO);
	mu_assert("mem568", msg.timestamp[0] == 0);
	mu_assert("mem569", msg.timestamp[1] == 15);
	mu_assert("mem570", msg.timestamp[2] == 0);
	mu_assert("mem571", msg.timestamp[3] == 0);
	mu_assert("mem572", msg.size == (sizeof(int)+sizeof(wnPacket_t)*9));
	packetCount = *((int *)msg.data);
	mu_assert("mem573", packetCount == 9);
	packet = (wnPacket_t *)(msg.data+sizeof(int));
	mu_assert("mem580", packet->hostid == 1);
	mu_assert("mem581", packet->timestamp[0] == 0);
	mu_assert("mem582", packet->timestamp[1] == 5);
	mu_assert("mem583", packet->timestamp[2] == 0);
	mu_assert("mem584", packet->timestamp[3] == 0);
	mu_assert("mem585", packet->wnCount == 1);
	mu_assert("mem586", packet->wnArray[0] == 0);
	packet = packet+8;
	mu_assert("mem587", packet->hostid == 1);
	mu_assert("mem588", packet->timestamp[0] == 0);
	mu_assert("mem589", packet->timestamp[1] == 13);
	mu_assert("mem590", packet->timestamp[2] == 0);
	mu_assert("mem591", packet->timestamp[3] == 0);
	mu_assert("mem592", packet->wnCount == 1);
	mu_assert("mem593", packet->wnArray[0] == 0);

	mu_assert("mem594", msg2.from == 1);
	mu_assert("mem595", msg2.to == 0);
	mu_assert("mem596", msg2.command == GRANT_ENTER_BARRIER_INFO);
	mu_assert("mem597", msg2.timestamp[0] == -1);
	mu_assert("mem598", msg2.timestamp[1] == -1);
	mu_assert("mem599", msg2.timestamp[2] == -1);
	mu_assert("mem600", msg2.timestamp[3] == -1);
	mu_assert("mem601", msg2.size == (sizeof(int)+sizeof(wnPacket_t)*2));
	packetCount = *((int *)msg2.data);
	mu_assert("mem602", packetCount == 2);
	packet = (wnPacket_t *)(msg2.data+sizeof(int));
	mu_assert("mem603", packet->hostid == 1);
	mu_assert("mem604", packet->timestamp[0] == 0);
	mu_assert("mem605", packet->timestamp[1] == 14);
	mu_assert("mem607", packet->timestamp[2] == 0);
	mu_assert("mem608", packet->timestamp[3] == 0);
	mu_assert("mem609", packet->wnCount == 1);
	mu_assert("mem610", packet->wnArray[0] == 0);
	packet = packet+1;
	mu_assert("mem611", packet->hostid == 1);
	mu_assert("mem612", packet->timestamp[0] == 0);
	mu_assert("mem613", packet->timestamp[1] == 15);
	mu_assert("mem614", packet->timestamp[2] == 0);
	mu_assert("mem615", packet->timestamp[3] == 0);
	mu_assert("mem616", packet->wnCount == 1);
	mu_assert("mem617", packet->wnArray[0] == 0);
	return 0;
}


static char *test_handleGrantEnterBarrierMsg(){
	extern page_t pageArray[MAX_PAGE_NUM];
	extern proc_t procArray[MAX_HOST_NUM];
	extern int barrierTimestamps[MAX_HOST_NUM][MAX_HOST_NUM];

	myhostid = 0;
	hostnum = 4;
//no writenotice since last barrier
	memset(pageArray, 0, sizeof(page_t) * MAX_PAGE_NUM);
	memset(procArray, 0, sizeof(proc_t) * MAX_HOST_NUM);
	int i, j;
	for(i = 0; i < MAX_HOST_NUM; i++){
		for(j = 0; j < MAX_HOST_NUM; j++){
			barrierTimestamps[i][j] = -1;
		}
	}
	mimsg_t *msg = (mimsg_t *)malloc(sizeof(mimsg_t));	
	memset(msg, 0, sizeof(mimsg_t));
	msg->from = 1;
	msg->to = myhostid; 
	msg->command = GRANT_ENTER_BARRIER_INFO;
	msg->timestamp[1] = 0;	

	wnPacket_t packet;
	memset(&packet, 0, sizeof(wnPacket_t));

	int packetNum = 0;
	apendMsgData(msg, (char *)&packetNum, sizeof(int));
	handleGrantEnterBarrierMsg(msg);
	mu_assert("mem618", barrierTimestamps[1][0] == 0);
	mu_assert("mem619", barrierTimestamps[1][1] == 0);
	mu_assert("mem620", barrierTimestamps[1][2] == 0);
	mu_assert("mem621", barrierTimestamps[1][3] == 0);
	mu_assert("mem622", procArray[1].intervalList == NULL);
//one interval
	memset(pageArray, 0, sizeof(page_t) * MAX_PAGE_NUM);
	memset(procArray, 0, sizeof(proc_t) * MAX_HOST_NUM);
	for(i = 0; i < MAX_HOST_NUM; i++){
		for(j = 0; j < MAX_HOST_NUM; j++){
			barrierTimestamps[i][j] = -1;
		}
	}
	msg = (mimsg_t *)malloc(sizeof(mimsg_t));	
	memset(msg, 0, sizeof(mimsg_t));
	msg->from = 1;
	msg->to = myhostid; 
	msg->command = GRANT_ENTER_BARRIER_INFO;
	msg->timestamp[1] = 1;	

	memset(&packet, 0, sizeof(wnPacket_t));

	packetNum = 1;
	apendMsgData(msg, (char *)&packetNum, sizeof(int));
	packet.hostid = 1;
	packet.timestamp[1] = 1;
	packet.wnCount = 3;
	packet.wnArray[0] = 0;
	packet.wnArray[1] = 1;
	packet.wnArray[2] = 2;
	apendMsgData(msg, (char *)&packet, sizeof(wnPacket_t));
	handleGrantEnterBarrierMsg(msg);
	mu_assert("mem623", barrierTimestamps[1][0] == 0);
	mu_assert("mem624", barrierTimestamps[1][1] == 1);
	mu_assert("mem625", barrierTimestamps[1][2] == 0);
	mu_assert("mem626", barrierTimestamps[1][3] == 0);
	mu_assert("mem627", procArray[1].intervalList->timestamp[0] == 0);
	mu_assert("mem628", procArray[1].intervalList->timestamp[1] == 1);
	mu_assert("mem629", procArray[1].intervalList->timestamp[2] == 0);
	mu_assert("mem630", procArray[1].intervalList->timestamp[3] == 0);
	mu_assert("mem630.1", procArray[1].intervalList->next == NULL);
//full msg
	memset(pageArray, 0, sizeof(page_t) * MAX_PAGE_NUM);
	memset(procArray, 0, sizeof(proc_t) * MAX_HOST_NUM);
	for(i = 0; i < MAX_HOST_NUM; i++){
		for(j = 0; j < MAX_HOST_NUM; j++){
			barrierTimestamps[i][j] = -1;
		}
	}
	msg = (mimsg_t *)malloc(sizeof(mimsg_t));	
	memset(msg, 0, sizeof(mimsg_t));
	msg->from = 1;
	msg->to = myhostid; 
	msg->command = GRANT_ENTER_BARRIER_INFO;
	msg->timestamp[1] = 10;	


	packetNum = 9;
	apendMsgData(msg, (char *)&packetNum, sizeof(int));
	for(i = 0; i < packetNum; i++){
		memset(&packet, 0, sizeof(wnPacket_t));
		packet.hostid = 1;
		packet.timestamp[1] = i+1;
		packet.wnCount = 1;
		packet.wnArray[0] = i+1;
		apendMsgData(msg, (char *)&packet, sizeof(wnPacket_t));
	}
	handleGrantEnterBarrierMsg(msg);
	mu_assert("mem631", barrierTimestamps[1][0] == 0);
	mu_assert("mem632", barrierTimestamps[1][1] == 10);
	mu_assert("mem633", barrierTimestamps[1][2] == 0);
	mu_assert("mem634", barrierTimestamps[1][3] == 0);
	mu_assert("mem635", procArray[1].intervalList->timestamp[0] == 0);
	mu_assert("mem636", procArray[1].intervalList->timestamp[1] == 9);
	mu_assert("mem637", procArray[1].intervalList->timestamp[2] == 0);
	mu_assert("mem638", procArray[1].intervalList->timestamp[3] == 0);
	mu_assert("mem639", procArray[1].intervalList->next->timestamp[0] == 0);
	mu_assert("mem640", procArray[1].intervalList->next->timestamp[1] == 8);
	mu_assert("mem641", procArray[1].intervalList->next->timestamp[2] == 0);
	mu_assert("mem642", procArray[1].intervalList->next->timestamp[3] == 0);
	interval_t *tempi = procArray[1].intervalList;
	for(i = 0; i < (packetNum-1); i++){
		tempi = tempi->next;
	}
	mu_assert("mem643", tempi->timestamp[0] == 0);
	mu_assert("mem644", tempi->timestamp[1] == 1);
	mu_assert("mem645", tempi->timestamp[2] == 0);
	mu_assert("mem646", tempi->timestamp[3] == 0);
	mu_assert("mem647", procArray[1].intervalList->notices->pageIndex == 9);
	mu_assert("mem648", procArray[1].intervalList->next->notices->pageIndex == 8);
	mu_assert("mem649", tempi->notices->pageIndex == 1);
//second msg
	msg = (mimsg_t *)malloc(sizeof(mimsg_t));	
	memset(msg, 0, sizeof(mimsg_t));
	msg->from = 1;
	msg->to = myhostid; 
	msg->command = GRANT_ENTER_BARRIER_INFO;
	for(i = 0; i < MAX_HOST_NUM; i++){
		msg->timestamp[i] = -1;	
	}

	memset(&packet, 0, sizeof(wnPacket_t));

	packetNum = 1;
	apendMsgData(msg, (char *)&packetNum, sizeof(int));
	packet.hostid = 1;
	packet.timestamp[1] = 10;
	packet.wnCount = 1;
	packet.wnArray[0] = 10;
	apendMsgData(msg, (char *)&packet, sizeof(wnPacket_t));
	handleGrantEnterBarrierMsg(msg);
	mu_assert("mem650", barrierTimestamps[1][0] == 0);
	mu_assert("mem651", barrierTimestamps[1][1] == 10);
	mu_assert("mem652", barrierTimestamps[1][2] == 0);
	mu_assert("mem653", barrierTimestamps[1][3] == 0);
	mu_assert("mem654", procArray[1].intervalList->timestamp[0] == 0);
	mu_assert("mem655", procArray[1].intervalList->timestamp[1] == 10);
	mu_assert("mem656", procArray[1].intervalList->timestamp[2] == 0);
	mu_assert("mem657", procArray[1].intervalList->timestamp[3] == 0);
	mu_assert("mem658", procArray[1].intervalList->next->timestamp[0] == 0);
	mu_assert("mem659", procArray[1].intervalList->next->timestamp[1] == 9);
	mu_assert("mem660", procArray[1].intervalList->next->timestamp[2] == 0);
	mu_assert("mem661", procArray[1].intervalList->next->timestamp[3] == 0);
	mu_assert("mem662", tempi->timestamp[0] == 0);
	mu_assert("mem663", tempi->timestamp[1] == 1);
	mu_assert("mem664", tempi->timestamp[2] == 0);
	mu_assert("mem665", tempi->timestamp[3] == 0);
	mu_assert("mem666", procArray[1].intervalList->notices->pageIndex == 10);
	mu_assert("mem667", procArray[1].intervalList->next->notices->pageIndex == 9);
	mu_assert("mem668", tempi->notices->pageIndex == 1);
//second msg and first packet is a second packet
	memset(pageArray, 0, sizeof(page_t) * MAX_PAGE_NUM);
	memset(procArray, 0, sizeof(proc_t) * MAX_HOST_NUM);
	for(i = 0; i < MAX_HOST_NUM; i++){
		for(j = 0; j < MAX_HOST_NUM; j++){
			barrierTimestamps[i][j] = -1;
		}
	}
	msg = (mimsg_t *)malloc(sizeof(mimsg_t));	
	memset(msg, 0, sizeof(mimsg_t));
	msg->from = 1;
	msg->to = myhostid; 
	msg->command = GRANT_ENTER_BARRIER_INFO;
	msg->timestamp[1] = 9;	


	packetNum = 9;
	apendMsgData(msg, (char *)&packetNum, sizeof(int));
	for(i = 0; i < (packetNum-1); i++){
		memset(&packet, 0, sizeof(wnPacket_t));
		packet.hostid = 1;
		packet.timestamp[1] = i+1;
		packet.wnCount = 1;
		packet.wnArray[0] = i+1;
		apendMsgData(msg, (char *)&packet, sizeof(wnPacket_t));
	}
	packet.hostid = 1;
	packet.timestamp[1] = 9;
	packet.wnCount = MAX_WN_NUM;
	for(i = 0; i < MAX_WN_NUM; i++){
		packet.wnArray[i] = i;
	}
	apendMsgData(msg, (char *)&packet, sizeof(wnPacket_t));

	handleGrantEnterBarrierMsg(msg);
	mu_assert("mem669", barrierTimestamps[1][0] == 0);
	mu_assert("mem670", barrierTimestamps[1][1] == 9);
	mu_assert("mem671", barrierTimestamps[1][2] == 0);
	mu_assert("mem672", barrierTimestamps[1][3] == 0);
	mu_assert("mem673", procArray[1].intervalList->timestamp[0] == 0);
	mu_assert("mem674", procArray[1].intervalList->timestamp[1] == 9);
	mu_assert("mem675", procArray[1].intervalList->timestamp[2] == 0);
	mu_assert("mem676", procArray[1].intervalList->timestamp[3] == 0);
	mu_assert("mem677", procArray[1].intervalList->next->timestamp[0] == 0);
	mu_assert("mem678", procArray[1].intervalList->next->timestamp[1] == 8);
	mu_assert("mem679", procArray[1].intervalList->next->timestamp[2] == 0);
	mu_assert("mem680", procArray[1].intervalList->next->timestamp[3] == 0);
	tempi = procArray[1].intervalList;
	for(i = 0; i < (packetNum-1); i++){
		tempi = tempi->next;
	}
	mu_assert("mem681", tempi->timestamp[0] == 0);
	mu_assert("mem682", tempi->timestamp[1] == 1);
	mu_assert("mem683", tempi->timestamp[2] == 0);
	mu_assert("mem684", tempi->timestamp[3] == 0);
	
	writenotice_t *tempwn = procArray[1].intervalList->notices;
	for(i = 0; i < (MAX_WN_NUM-1); i++){
		tempwn = tempwn->nextInInterval;
	}
	
	mu_assert("mem685", procArray[1].intervalList->notices->pageIndex == 0);
	mu_assert("mem686", tempwn->pageIndex == (MAX_WN_NUM-1));
	mu_assert("mem686.1", tempwn->nextInInterval == NULL);
	mu_assert("mem687", procArray[1].intervalList->next->notices->pageIndex == 8);
	mu_assert("mem688", tempi->notices->pageIndex == 1);


	msg = (mimsg_t *)malloc(sizeof(mimsg_t));	
	memset(msg, 0, sizeof(mimsg_t));
	msg->from = 1;
	msg->to = myhostid; 
	msg->command = GRANT_WN_I;
	for(i = 0; i < MAX_HOST_NUM; i++){
		msg->timestamp[i] = -1;	
	}

	memset(&packet, 0, sizeof(wnPacket_t));

	packetNum = 1;
	apendMsgData(msg, (char *)&packetNum, sizeof(int));
	packet.hostid = 1;
	packet.timestamp[1] = 9;
	packet.wnCount = 1;
	packet.wnArray[0] = MAX_WN_NUM;
	apendMsgData(msg, (char *)&packet, sizeof(wnPacket_t));
	handleGrantEnterBarrierMsg(msg);
	mu_assert("mem689", barrierTimestamps[1][0] == 0);
	mu_assert("mem690", barrierTimestamps[1][1] == 9);
	mu_assert("mem691", barrierTimestamps[1][2] == 0);
	mu_assert("mem692", barrierTimestamps[1][3] == 0);
	mu_assert("mem693", procArray[1].intervalList->timestamp[0] == 0);
	mu_assert("mem694", procArray[1].intervalList->timestamp[1] == 9);
	mu_assert("mem695", procArray[1].intervalList->timestamp[2] == 0);
	mu_assert("mem696", procArray[1].intervalList->timestamp[3] == 0);
	mu_assert("mem697", procArray[1].intervalList->next->timestamp[0] == 0);
	mu_assert("mem698", procArray[1].intervalList->next->timestamp[1] == 8);
	mu_assert("mem699", procArray[1].intervalList->next->timestamp[2] == 0);
	mu_assert("mem700", procArray[1].intervalList->next->timestamp[3] == 0);
	mu_assert("mem701", procArray[1].intervalList->notices->pageIndex == 0);
	mu_assert("mem702", tempwn->pageIndex == (MAX_WN_NUM-1));
	mu_assert("mem703", tempwn->nextInInterval->pageIndex == MAX_WN_NUM);
	mu_assert("mem704", tempwn->nextInInterval->nextInInterval == NULL);
	mu_assert("mem705", procArray[1].intervalList->next->notices->pageIndex == 8);
	mu_assert("mem706", tempi->notices->pageIndex == 1);


	return 0;
}


static char *test_returnAllBarrierInfo(){
	extern page_t pageArray[MAX_PAGE_NUM];
	extern proc_t procArray[MAX_HOST_NUM];
	extern int parametertype;
	extern int sendMsgCalled; 
	extern int nextFreeMsgInQueueCalled;
	extern mimsg_t msg;
	extern mimsg_t msg2;
	extern int alreadyCalled;
	extern interval_t *intervalNow;
	extern int barrierTimestamps[MAX_HOST_NUM][MAX_HOST_NUM];
	
	myhostid = 0;
	hostnum = 3;
//no writenotice since last barrier
	initmem();
	int i, j;
	for(i = 0; i < hostnum; i++){
		for(j = 0; j < hostnum; j++){
			barrierTimestamps[i][j] = 0;
		}
	}

	parametertype = 1;
	sendMsgCalled = 0;
	nextFreeMsgInQueueCalled = 0;
	alreadyCalled = 0;
	memset(&msg, 0, sizeof(mimsg_t));
	memset(&msg2, 0, sizeof(mimsg_t));
	for(i = 0; i < hostnum; i++){
		msg.timestamp[i] = -1;
	}
	for(i = 0; i < hostnum; i++){
		msg2.timestamp[i] = -1;
	}

	returnAllBarrierInfo();

	mu_assert("mem707", parametertype == 0);
	mu_assert("mem708", sendMsgCalled == 1);
	mu_assert("mem709", nextFreeMsgInQueueCalled == 1);
	mu_assert("mem710", alreadyCalled == 1);
	mu_assert("mem710.1", intervalNow->timestamp[0] = 1);
	mu_assert("mem710.2", intervalNow->timestamp[1] = 1);
	mu_assert("mem710.3", intervalNow->timestamp[2] = 1);
	mu_assert("mem710.4", intervalNow == procArray[0].intervalList);
	mu_assert("mem710.5", intervalNow->next->timestamp[0] == 0);
	mu_assert("mem710.6", intervalNow->next->timestamp[1] == 0);
	mu_assert("mem710.7", intervalNow->next->timestamp[2] == 0);
	mu_assert("mem710.8", intervalNow->isBarrier == 1);
	mu_assert("mem711", msg.from == myhostid);
	mu_assert("mem712", msg.to == 1);
	mu_assert("mem713", msg.command == GRANT_EXIT_BARRIER_INFO);
	mu_assert("mem714", msg.timestamp[0] == 1);
	mu_assert("mem715", msg.timestamp[1] == 1);
	mu_assert("mem716", msg.timestamp[2] == 1);
	int packetNum = *((int *)msg.data);
	mu_assert("mem717", packetNum == 0);
	mu_assert("mem718", msg2.from == myhostid);
	mu_assert("mem719", msg2.to == 2);
	mu_assert("mem720", msg2.command == GRANT_EXIT_BARRIER_INFO);
	mu_assert("mem721", msg2.timestamp[0] == 1);
	mu_assert("mem722", msg2.timestamp[1] == 1);
	mu_assert("mem723", msg2.timestamp[2] == 1);
	packetNum = *((int *)msg2.data);
	mu_assert("mem724", packetNum == 0);

//no related writenotice
	initmem();
	barrierTimestamps[0][0] = 2;
	barrierTimestamps[0][1] = 0;
	barrierTimestamps[0][2] = 0;
	barrierTimestamps[1][0] = 0;
	barrierTimestamps[1][1] = 2;
	barrierTimestamps[1][2] = 0;
	barrierTimestamps[2][0] = 0;
	barrierTimestamps[2][1] = 0;
	barrierTimestamps[2][2] = 2;

	interval_t *interval2 = malloc(sizeof(interval_t));
	memset(interval2, 0, sizeof(interval_t));
	interval2->timestamp[0] = 1;
	interval2->next = procArray[0].intervalList;
	procArray[0].intervalList->prev = interval2;
	procArray[0].intervalList = interval2;
	interval_t *interval3 = malloc(sizeof(interval_t));
	memset(interval3, 0, sizeof(interval_t));
	interval3->timestamp[0] = 2;
	interval3->next = procArray[0].intervalList;
	procArray[0].intervalList->prev = interval3;
	procArray[0].intervalList = interval3;
	intervalNow = interval3;
	
	writenotice_t *lastWnInInterval = NULL;
	int wnCount = 3;
	for(i = 0; i < wnCount; i++){
		writenotice_t *wn = malloc(sizeof(writenotice_t));
		wn->interval = interval2;
		wn->nextInPage = pageArray[i].notices[1];
		wn->pageIndex = i;
		pageArray[i].notices[0] = wn;
		wn->nextInInterval = NULL;
		if(lastWnInInterval == NULL){
			interval2->notices = lastWnInInterval = wn;
		}else{
			lastWnInInterval->nextInInterval = wn;
			lastWnInInterval = wn;
		}
	}

	parametertype = 1;
	sendMsgCalled = 0;
	nextFreeMsgInQueueCalled = 0;
	alreadyCalled = 0;
	memset(&msg, 0, sizeof(mimsg_t));
	memset(&msg2, 0, sizeof(mimsg_t));
	for(i = 0; i < hostnum; i++){
		msg.timestamp[i] = -1;
	}
	for(i = 0; i < hostnum; i++){
		msg2.timestamp[i] = -1;
	}

	returnAllBarrierInfo();

	mu_assert("mem725", parametertype == 0);
	mu_assert("mem726", sendMsgCalled == 1);
	mu_assert("mem727", nextFreeMsgInQueueCalled == 1);
	mu_assert("mem728", alreadyCalled == 1);
	mu_assert("mem728.1", intervalNow->timestamp[0] = 3);
	mu_assert("mem728.2", intervalNow->timestamp[1] = 3);
	mu_assert("mem728.3", intervalNow->timestamp[2] = 3);
	mu_assert("mem728.4", intervalNow == procArray[0].intervalList);
	mu_assert("mem728.5", intervalNow->next->timestamp[0] == 2);
	mu_assert("mem728.6", intervalNow->next->timestamp[1] == 0);
	mu_assert("mem728.7", intervalNow->next->timestamp[2] == 0);
	mu_assert("mem728.8", intervalNow->isBarrier == 1);
	mu_assert("mem729", msg.from == myhostid);
	mu_assert("mem730", msg.to == 1);
	mu_assert("mem731", msg.command == GRANT_EXIT_BARRIER_INFO);
	mu_assert("mem732", msg.timestamp[0] == 3);
	mu_assert("mem733", msg.timestamp[1] == 3);
	mu_assert("mem734", msg.timestamp[2] == 3);
	packetNum = *((int *)msg.data);
	mu_assert("mem735", packetNum == 0);
	mu_assert("mem736", msg2.from == myhostid);
	mu_assert("mem737", msg2.to == 2);
	mu_assert("mem738", msg2.command == GRANT_EXIT_BARRIER_INFO);
	mu_assert("mem739", msg2.timestamp[0] == 3);
	mu_assert("mem740", msg2.timestamp[1] == 3);
	mu_assert("mem741", msg2.timestamp[2] == 3);
	packetNum = *((int *)msg2.data);
	mu_assert("mem742", packetNum == 0);
//one wnPacket
	initmem();
	barrierTimestamps[0][0] = 2;
	barrierTimestamps[0][1] = 0;
	barrierTimestamps[0][2] = 0;
	barrierTimestamps[1][0] = 0;
	barrierTimestamps[1][1] = 0;
	barrierTimestamps[1][2] = 0;
	barrierTimestamps[2][0] = 0;
	barrierTimestamps[2][1] = 0;
	barrierTimestamps[2][2] = 0;

	interval_t *interval4 = malloc(sizeof(interval_t));
	memset(interval4, 0, sizeof(interval_t));
	interval4->timestamp[0] = 1;
	interval4->next = procArray[0].intervalList;
	procArray[0].intervalList->prev = interval4;
	procArray[0].intervalList = interval4;
	interval_t *interval5 = malloc(sizeof(interval_t));
	memset(interval5, 0, sizeof(interval_t));
	interval5->timestamp[0] = 2;
	interval5->next = procArray[0].intervalList;
	procArray[0].intervalList->prev = interval5;
	procArray[0].intervalList = interval5;
	intervalNow = interval5;
	
	lastWnInInterval = NULL;
	wnCount = 3;
	for(i = 0; i < wnCount; i++){
		writenotice_t *wn = malloc(sizeof(writenotice_t));
		wn->interval = interval4;
		wn->nextInPage = pageArray[i].notices[1];
		wn->pageIndex = i;
		pageArray[i].notices[0] = wn;
		wn->nextInInterval = NULL;
		if(lastWnInInterval == NULL){
			interval4->notices = lastWnInInterval = wn;
		}else{
			lastWnInInterval->nextInInterval = wn;
			lastWnInInterval = wn;
		}
	}

	parametertype = 1;
	sendMsgCalled = 0;
	nextFreeMsgInQueueCalled = 0;
	alreadyCalled = 0;
	memset(&msg, 0, sizeof(mimsg_t));
	memset(&msg2, 0, sizeof(mimsg_t));
	for(i = 0; i < hostnum; i++){
		msg.timestamp[i] = -1;
	}
	for(i = 0; i < hostnum; i++){
		msg2.timestamp[i] = -1;
	}

	returnAllBarrierInfo();

	mu_assert("mem743", parametertype == 0);
	mu_assert("mem744", sendMsgCalled == 1);
	mu_assert("mem745", nextFreeMsgInQueueCalled == 1);
	mu_assert("mem746", alreadyCalled == 1);
	mu_assert("mem746.1", intervalNow->timestamp[0] = 3);
	mu_assert("mem746.2", intervalNow->timestamp[1] = 1);
	mu_assert("mem746.3", intervalNow->timestamp[2] = 1);
	mu_assert("mem746.4", intervalNow == procArray[0].intervalList);
	mu_assert("mem746.5", intervalNow->next->timestamp[0] == 2);
	mu_assert("mem746.6", intervalNow->next->timestamp[1] == 0);
	mu_assert("mem746.7", intervalNow->next->timestamp[2] == 0);
	mu_assert("mem746.8", intervalNow->isBarrier == 1);

	mu_assert("mem747", msg.from == myhostid);
	mu_assert("mem748", msg.to == 1);
	mu_assert("mem749", msg.command == GRANT_EXIT_BARRIER_INFO);
	mu_assert("mem750", msg.timestamp[0] == 3);
	mu_assert("mem751", msg.timestamp[1] == 1);
	mu_assert("mem752", msg.timestamp[2] == 1);
	packetNum = *((int *)msg.data);
	mu_assert("mem753", packetNum == 1);
	wnPacket_t *packet = (wnPacket_t *)(msg.data + sizeof(int));
	mu_assert("mem754", packet->hostid == 0);
	mu_assert("mem755", packet->timestamp[0] == 1);
	mu_assert("mem756", packet->timestamp[1] == 0);
	mu_assert("mem757", packet->timestamp[2] == 0);
	mu_assert("mem758", packet->wnCount == 3);
	mu_assert("mem759", packet->wnArray[0] == 0);
	mu_assert("mem760", packet->wnArray[1] == 1);
	mu_assert("mem761", packet->wnArray[2] == 2);
	mu_assert("mem762", msg2.from == myhostid);
	mu_assert("mem763", msg2.to == 2);
	mu_assert("mem764", msg2.command == GRANT_EXIT_BARRIER_INFO);
	mu_assert("mem765", msg2.timestamp[0] == 3);
	mu_assert("mem766", msg2.timestamp[1] == 1);
	mu_assert("mem767", msg2.timestamp[2] == 1);
	packetNum = *((int *)msg2.data);
	mu_assert("mem768", packetNum == 1);
	packet = (wnPacket_t *)(msg2.data + sizeof(int));
	mu_assert("mem769", packet->hostid == 0);
	mu_assert("mem770", packet->timestamp[0] == 1);
	mu_assert("mem771", packet->timestamp[1] == 0);
	mu_assert("mem772", packet->timestamp[2] == 0);
	mu_assert("mem773", packet->wnCount == 3);
	mu_assert("mem774", packet->wnArray[0] == 0);
	mu_assert("mem775", packet->wnArray[1] == 1);
	mu_assert("mem776", packet->wnArray[2] == 2);


//two wnPackets for one interval
	myhostid = 0;
	hostnum = 3;
	initmem();
	barrierTimestamps[0][0] = 2;
	barrierTimestamps[0][1] = 0;
	barrierTimestamps[0][2] = 0;
	barrierTimestamps[1][0] = 0;
	barrierTimestamps[1][1] = 0;
	barrierTimestamps[1][2] = 0;
	barrierTimestamps[2][0] = 0;
	barrierTimestamps[2][1] = 0;
	barrierTimestamps[2][2] = 0;


	interval_t *interval6 = malloc(sizeof(interval_t));
	memset(interval6, 0, sizeof(interval_t));
	interval6->timestamp[0] = 1;
	interval6->next = procArray[0].intervalList;
	procArray[0].intervalList->prev = interval6;
	procArray[0].intervalList = interval6;
	interval_t *interval7 = malloc(sizeof(interval_t));
	memset(interval7, 0, sizeof(interval_t));
	interval7->timestamp[0] = 2;
	interval7->next = procArray[0].intervalList;
	procArray[0].intervalList->prev = interval7;
	procArray[0].intervalList = interval7;
	intervalNow = interval7;
	
	lastWnInInterval = NULL;
	wnCount = MAX_WN_NUM + 1;
	for(i = 0; i < wnCount; i++){
		writenotice_t *wn = malloc(sizeof(writenotice_t));
		wn->interval = interval6;
		wn->nextInPage = pageArray[i].notices[1];
		wn->pageIndex = i;
		pageArray[i].notices[0] = wn;
		wn->nextInInterval = NULL;
		if(lastWnInInterval == NULL){
			interval6->notices = lastWnInInterval = wn;
		}else{
			lastWnInInterval->nextInInterval = wn;
			lastWnInInterval = wn;
		}
	}

	parametertype = 1;
	sendMsgCalled = 0;
	nextFreeMsgInQueueCalled = 0;
	alreadyCalled = 0;
	memset(&msg, 0, sizeof(mimsg_t));
	memset(&msg2, 0, sizeof(mimsg_t));
	for(i = 0; i < hostnum; i++){
		msg.timestamp[i] = -1;
	}
	for(i = 0; i < hostnum; i++){
		msg2.timestamp[i] = -1;
	}

	returnAllBarrierInfo();

	mu_assert("mem777", parametertype == 0);
	mu_assert("mem778", sendMsgCalled == 1);
	mu_assert("mem779", nextFreeMsgInQueueCalled == 1);
	mu_assert("mem780", alreadyCalled == 1);
	mu_assert("mem780.1", intervalNow->timestamp[0] = 3);
	mu_assert("mem780.2", intervalNow->timestamp[1] = 1);
	mu_assert("mem780.3", intervalNow->timestamp[2] = 1);
	mu_assert("mem780.4", intervalNow == procArray[0].intervalList);
	mu_assert("mem780.5", intervalNow->next->timestamp[0] == 2);
	mu_assert("mem780.6", intervalNow->next->timestamp[1] == 0);
	mu_assert("mem780.7", intervalNow->next->timestamp[2] == 0);
	mu_assert("mem780.8", intervalNow->isBarrier == 1);
	mu_assert("mem781", msg.from == myhostid);
	mu_assert("mem782", msg.to == 1);
	mu_assert("mem783", msg.command == GRANT_EXIT_BARRIER_INFO);
	mu_assert("mem784", msg.timestamp[0] == 3);
	mu_assert("mem785", msg.timestamp[1] == 1);
	mu_assert("mem786", msg.timestamp[2] == 1);
	packetNum = *((int *)msg.data);
	mu_assert("mem787", packetNum == 2);
	packet = (wnPacket_t *)(msg.data + sizeof(int));
	mu_assert("mem788", packet->hostid == 0);
	mu_assert("mem789", packet->timestamp[0] == 1);
	mu_assert("mem790", packet->timestamp[1] == 0);
	mu_assert("mem791", packet->timestamp[2] == 0);
	mu_assert("mem792", packet->wnCount == MAX_WN_NUM);
	mu_assert("mem793", packet->wnArray[0] == 0);
	mu_assert("mem794", packet->wnArray[1] == 1);
	mu_assert("mem795", packet->wnArray[MAX_WN_NUM-1] == MAX_WN_NUM-1);
	packet = packet + 1;
	mu_assert("mem796", packet->hostid == 0);
	mu_assert("mem797", packet->timestamp[0] == 1);
	mu_assert("mem798", packet->timestamp[1] == 0);
	mu_assert("mem799", packet->timestamp[2] == 0);
	mu_assert("mem800", packet->wnCount == 1);
	mu_assert("mem801", packet->wnArray[0] == MAX_WN_NUM);
	
	mu_assert("mem802", msg2.from == myhostid);
	mu_assert("mem803", msg2.to == 2);
	mu_assert("mem804", msg2.command == GRANT_EXIT_BARRIER_INFO);
	mu_assert("mem805", msg2.timestamp[0] == 3);
	mu_assert("mem806", msg2.timestamp[1] == 1);
	mu_assert("mem807", msg2.timestamp[2] == 1);
	packetNum = *((int *)msg2.data);
	mu_assert("mem808", packetNum == 2);
	packet = (wnPacket_t *)(msg2.data + sizeof(int));
	mu_assert("mem809", packet->hostid == 0);
	mu_assert("mem810", packet->timestamp[0] == 1);
	mu_assert("mem811", packet->timestamp[1] == 0);
	mu_assert("mem812", packet->timestamp[2] == 0);
	mu_assert("mem813", packet->wnCount == MAX_WN_NUM);
	mu_assert("mem814", packet->wnArray[0] == 0);
	mu_assert("mem815", packet->wnArray[1] == 1);
	mu_assert("mem816", packet->wnArray[MAX_WN_NUM-1] == MAX_WN_NUM-1);
	packet = packet + 1;
	mu_assert("mem817", packet->hostid == 0);
	mu_assert("mem818", packet->timestamp[0] == 1);
	mu_assert("mem819", packet->timestamp[1] == 0);
	mu_assert("mem820", packet->timestamp[2] == 0);
	mu_assert("mem821", packet->wnCount == 1);
	mu_assert("mem822", packet->wnArray[0] == MAX_WN_NUM);
//full msg
	myhostid = 0;
	hostnum = 2;
	initmem();
	barrierTimestamps[0][0] = 10;
	barrierTimestamps[0][1] = 0;
	barrierTimestamps[0][2] = 0;
	barrierTimestamps[1][0] = 0;
	barrierTimestamps[1][1] = 0;
	barrierTimestamps[1][2] = 0;


	interval_t *interval8 = NULL;
	int temp = 1;
	int intervalCount = 10;
	for(i = 0; i < intervalCount; i++){
		interval8 = malloc(sizeof(interval_t));
		memset(interval8, 0, sizeof(interval_t));
		interval8->timestamp[0] = temp;
		procArray[0].intervalList->prev = interval8;
		interval8->next = procArray[0].intervalList;
		procArray[0].intervalList = interval8;	
		temp++;
		lastWnInInterval = NULL;
		wnCount = 1;
		for(j = 0; j < wnCount; j++){
			writenotice_t *wn = malloc(sizeof(writenotice_t));
			wn->interval = interval8;
			wn->nextInPage = pageArray[i].notices[0];
			wn->pageIndex = j;
			pageArray[j].notices[0] = wn;
			wn->nextInInterval = NULL;
			if(lastWnInInterval == NULL){
				interval8->notices = lastWnInInterval = wn;
			}else{
				lastWnInInterval->nextInInterval = wn;
				lastWnInInterval = wn;
			}
		}
	}
	intervalNow = interval8;

	parametertype = 1;
	sendMsgCalled = 0;
	nextFreeMsgInQueueCalled = 0;
	alreadyCalled = 0;
	memset(&msg, 0, sizeof(mimsg_t));
	memset(&msg2, 0, sizeof(mimsg_t));
	for(i = 0; i < hostnum; i++){
		msg.timestamp[i] = -1;
	}
	for(i = 0; i < hostnum; i++){
		msg2.timestamp[i] = -1;
	}

	returnAllBarrierInfo();

	mu_assert("mem823", parametertype == 0);
	mu_assert("mem824", sendMsgCalled == 1);
	mu_assert("mem825", nextFreeMsgInQueueCalled == 1);
	mu_assert("mem826", alreadyCalled == 1);
	mu_assert("mem826.1", intervalNow->timestamp[0] = 11);
	mu_assert("mem826.2", intervalNow->timestamp[1] = 1);
	mu_assert("mem826.4", intervalNow == procArray[0].intervalList);
	mu_assert("mem826.5", intervalNow->next->timestamp[0] == 10);
	mu_assert("mem826.6", intervalNow->next->timestamp[1] == 0);
	mu_assert("mem826.7", intervalNow->next->timestamp[2] == 0);
	mu_assert("mem826.8", intervalNow->isBarrier == 1);
	mu_assert("mem827", msg.from == myhostid);
	mu_assert("mem828", msg.to == 1);
	mu_assert("mem829", msg.command == GRANT_EXIT_BARRIER_INFO);
	mu_assert("mem830", msg.timestamp[0] == 11);
	mu_assert("mem831", msg.timestamp[1] == 1);
	packetNum = *((int *)msg.data);
	mu_assert("mem833", packetNum == 9);
	packet = (wnPacket_t *)(msg.data + sizeof(int));
	mu_assert("mem834", packet->hostid == 0);
	mu_assert("mem835", packet->timestamp[0] == 1);
	mu_assert("mem836", packet->timestamp[1] == 0);
	mu_assert("mem838", packet->wnCount == 1);
	mu_assert("mem839", packet->wnArray[0] == 0);
	packet = packet + 8;
	mu_assert("mem840", packet->hostid == 0);
	mu_assert("mem841", packet->timestamp[0] == 9);
	mu_assert("mem842", packet->timestamp[1] == 0);
	mu_assert("mem844", packet->wnCount == 1);
	mu_assert("mem845", packet->wnArray[0] == 0);
	
	mu_assert("mem846", msg2.from == myhostid);
	mu_assert("mem847", msg2.to == 1);
	mu_assert("mem848", msg2.command == GRANT_EXIT_BARRIER_INFO);
	mu_assert("mem849", msg2.timestamp[0] == -1);
	mu_assert("mem850", msg2.timestamp[1] == -1);
	packetNum = *((int *)msg2.data);
	mu_assert("mem852", packetNum == 1);
	packet = (wnPacket_t *)(msg2.data + sizeof(int));
	mu_assert("mem853", packet->hostid == 0);
	mu_assert("mem854", packet->timestamp[0] == 10);
	mu_assert("mem855", packet->timestamp[1] == 0);
	mu_assert("mem857", packet->wnCount == 1);
	mu_assert("mem858", packet->wnArray[0] == 0);
//second msg and second packet
	myhostid = 0;
	hostnum = 2;
	initmem();
	barrierTimestamps[0][0] = 9;
	barrierTimestamps[0][1] = 0;
	barrierTimestamps[0][2] = 0;
	barrierTimestamps[1][0] = 0;
	barrierTimestamps[1][1] = 0;
	barrierTimestamps[1][2] = 0;


	interval_t *interval9 = NULL;
	temp = 1;
	intervalCount = 9;
	for(i = 0; i < (intervalCount-1); i++){
		interval9 = malloc(sizeof(interval_t));
		memset(interval9, 0, sizeof(interval_t));
		interval9->timestamp[0] = temp;
		procArray[0].intervalList->prev = interval9;
		interval9->next = procArray[0].intervalList;
		procArray[0].intervalList = interval9;	
		temp++;
		lastWnInInterval = NULL;
		wnCount = 1;
		for(j = 0; j < wnCount; j++){
			writenotice_t *wn = malloc(sizeof(writenotice_t));
			wn->interval = interval9;
			wn->nextInPage = pageArray[i].notices[0];
			wn->pageIndex = j;
			pageArray[j].notices[0] = wn;
			wn->nextInInterval = NULL;
			if(lastWnInInterval == NULL){
				interval9->notices = lastWnInInterval = wn;
			}else{
				lastWnInInterval->nextInInterval = wn;
				lastWnInInterval = wn;
			}
		}
	}
	interval9 = malloc(sizeof(interval_t));
	memset(interval9, 0, sizeof(interval_t));
	interval9->timestamp[0] = 9;
	procArray[0].intervalList->prev = interval9;
	interval9->next = procArray[0].intervalList;
	procArray[0].intervalList = interval9;	
	lastWnInInterval = NULL;
	wnCount = MAX_WN_NUM + 1;
	for(j = 0; j < wnCount; j++){
		writenotice_t *wn = malloc(sizeof(writenotice_t));
		wn->interval = interval9;
		wn->nextInPage = pageArray[i].notices[0];
		wn->pageIndex = j;
		pageArray[j].notices[0] = wn;
		wn->nextInInterval = NULL;
		if(lastWnInInterval == NULL){
			interval9->notices = lastWnInInterval = wn;
		}else{
			lastWnInInterval->nextInInterval = wn;
			lastWnInInterval = wn;
		}
	}
	intervalNow = interval9;

	parametertype = 1;
	sendMsgCalled = 0;
	nextFreeMsgInQueueCalled = 0;
	alreadyCalled = 0;
	memset(&msg, 0, sizeof(mimsg_t));
	memset(&msg2, 0, sizeof(mimsg_t));
	for(i = 0; i < hostnum; i++){
		msg.timestamp[i] = -1;
	}
	for(i = 0; i < hostnum; i++){
		msg2.timestamp[i] = -1;
	}

	returnAllBarrierInfo();

	mu_assert("mem859", parametertype == 0);
	mu_assert("mem860", sendMsgCalled == 1);
	mu_assert("mem861", nextFreeMsgInQueueCalled == 1);
	mu_assert("mem862", alreadyCalled == 1);
	mu_assert("mem862.1", intervalNow->timestamp[0] = 10);
	mu_assert("mem862.2", intervalNow->timestamp[1] = 1);
	mu_assert("mem862.4", intervalNow == procArray[0].intervalList);
	mu_assert("mem862.5", intervalNow->next->timestamp[0] == 9);
	mu_assert("mem862.6", intervalNow->next->timestamp[1] == 0);
	mu_assert("mem862.7", intervalNow->next->timestamp[2] == 0);
	mu_assert("mem863", msg.from == myhostid);
	mu_assert("mem864", msg.to == 1);
	mu_assert("mem865", msg.command == GRANT_EXIT_BARRIER_INFO);
	mu_assert("mem866", msg.timestamp[0] == 10);
	mu_assert("mem867", msg.timestamp[1] == 1);
	packetNum = *((int *)msg.data);
	mu_assert("mem868", packetNum == 9);
	packet = (wnPacket_t *)(msg.data + sizeof(int));
	mu_assert("mem869", packet->hostid == 0);
	mu_assert("mem870", packet->timestamp[0] == 1);
	mu_assert("mem871", packet->timestamp[1] == 0);
	mu_assert("mem872", packet->wnCount == 1);
	mu_assert("mem873", packet->wnArray[0] == 0);
	packet = packet + 8;
	mu_assert("mem874", packet->hostid == 0);
	mu_assert("mem875", packet->timestamp[0] == 9);
	mu_assert("mem876", packet->timestamp[1] == 0);
	mu_assert("mem877", packet->wnCount == MAX_WN_NUM);
	mu_assert("mem878", packet->wnArray[0] == 0);
	mu_assert("mem878", packet->wnArray[MAX_WN_NUM-1] == (MAX_WN_NUM-1));

	mu_assert("mem879", msg2.from == myhostid);
	mu_assert("mem880", msg2.to == 1);
	mu_assert("mem881", msg2.command == GRANT_EXIT_BARRIER_INFO);
	mu_assert("mem882", msg2.timestamp[0] == -1);
	mu_assert("mem883", msg2.timestamp[1] == -1);
	packetNum = *((int *)msg2.data);
	mu_assert("mem885", packetNum == 1);
	packet = (wnPacket_t *)(msg2.data + sizeof(int));
	mu_assert("mem886", packet->hostid == 0);
	mu_assert("mem887", packet->timestamp[0] == 9);
	mu_assert("mem888", packet->timestamp[1] == 0);
	mu_assert("mem889", packet->wnCount == 1);
	mu_assert("mem890", packet->wnArray[0] == MAX_WN_NUM);

	return 0;
}


static char *test_handleGrantExitBarrierMsg(){
	extern page_t pageArray[MAX_PAGE_NUM];
	extern proc_t procArray[MAX_HOST_NUM];
	extern interval_t *intervalNow;
	
	myhostid = 1;
	hostnum = 4;
	initmem();

	mimsg_t *msg = (mimsg_t *)malloc(sizeof(mimsg_t));	
	memset(msg, 0, sizeof(mimsg_t));
	msg->from = 0;
	msg->to = myhostid; 
	msg->command = GRANT_EXIT_BARRIER_INFO;
	msg->timestamp[0] = 1;	
	msg->timestamp[1] = 1;	
	msg->timestamp[2] = 1;	
	msg->timestamp[3] = 1;	

	wnPacket_t packet;
	memset(&packet, 0, sizeof(wnPacket_t));

	int packetNum = 0;
	apendMsgData(msg, (char *)&packetNum, sizeof(int));
	handleGrantExitBarrierMsg(msg);
	mu_assert("mem891", intervalNow->timestamp[0]== 1);
	mu_assert("mem892", intervalNow->timestamp[1]== 1);
	mu_assert("mem893", intervalNow->timestamp[2]== 1);
	mu_assert("mem894", intervalNow->timestamp[3]== 1);
	mu_assert("mem894", intervalNow->isBarrier == 1);
	mu_assert("mem894", intervalNow->prev == NULL);
	mu_assert("mem894", intervalNow->next->timestamp[0] == 0);
	mu_assert("mem894", intervalNow->next->timestamp[1] == 0);
	mu_assert("mem894", intervalNow->next->timestamp[2] == 0);
	mu_assert("mem894", intervalNow->next->timestamp[3] == 0);

	return 0;
}


static char *all_tests(){
	mu_run_test(test_isAfterInterval);
	mu_run_test(test_createTwinPage);
	mu_run_test(test_freeTwinPage);
	mu_run_test(test_applyDiff);
	mu_run_test(test_createLocalDiff);
	mu_run_test(test_createWriteNotice);
	mu_run_test(test_addNewInterval);
	mu_run_test(test_incorporateWnPacket);
	mu_run_test(test_addWNIIntoPacketForHost);
	mu_run_test(test_grantWNI);
	mu_run_test(test_handleGrantWNIMsg);
	mu_run_test(test_grantDiff);
	mu_run_test(test_handleGrantDiffMsg);
	mu_run_test(test_grantPage);
	mu_run_test(test_handleGrantPageMsg);
	mu_run_test(test_initmem);
	mu_run_test(test_mi_alloc);
	mu_run_test(test_sendEnterBarrierInfo);
	mu_run_test(test_handleGrantEnterBarrierMsg);
	mu_run_test(test_returnAllBarrierInfo);
	mu_run_test(test_handleGrantWNIMsg);
	return 0;
}


int main(){
	char *result = all_tests();
	if(result != 0){
		printf("%s\n", result);
	}else{
		printf("ALL TESTS PASSED\n");
	}
	printf("Tests run : %d\n", tests_run);
	return result != 0;
}

