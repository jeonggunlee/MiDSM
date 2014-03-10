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

	a[2] = 2;
	b[2] = 0;
	mu_assert("mem4", isAfterInterval(a, b) == 1);

	a[4] = 2;
	b[4] = 3;
	mu_assert("mem5", isAfterInterval(a, b) == 1);


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
	
	mu_assert("mem67", incorporateWnPacket(NULL) == -1);
	
	wnPacket_t *packet = malloc(sizeof(wnPacket_t));

	memset(packet, 0, sizeof(wnPacket_t));
	packet->hostid = 1;
	(packet->timestamp)[1] = 1;
	packet->wnCount = 2;
	(packet->wnArray)[0] = 2;
	(packet->wnArray)[1] = 1023;
	
	mu_assert("mem68", incorporateWnPacket(packet) == 0);
	mu_assert("mem69", (procArray[1].intervalList->timestamp)[1] == 1);
	mu_assert("mem70", (procArray[1].intervalList->timestamp)[0] == 0);
	mu_assert("mem71", (procArray[1].intervalList->timestamp)[2] == 0);
	mu_assert("mem72", (procArray[1].intervalList->timestamp)[3] == 0);
	mu_assert("mem73", (procArray[1].intervalList)->notices == pageArray[2].notices[1]);
	mu_assert("mem74", (procArray[1].intervalList)->notices->nextInInterval == pageArray[1023].notices[1]);
	mu_assert("mem75", (procArray[1].intervalList)->notices->nextInInterval->nextInInterval == NULL);
	mu_assert("mem76", (procArray[1].intervalList)->notices->nextInPage == NULL);
	mu_assert("mem77", (procArray[1].intervalList)->notices->nextInInterval->nextInPage == NULL);
	mu_assert("mem78", (procArray[1].intervalList)->notices->interval == procArray[1].intervalList);
	mu_assert("mem79", (procArray[1].intervalList)->notices->nextInInterval->interval == procArray[1].intervalList);
	mu_assert("mem80", (procArray[1].intervalList)->notices->diffAddress == NULL);
	mu_assert("mem81", (procArray[1].intervalList)->notices->nextInInterval->diffAddress == NULL);
	mu_assert("mem81.1", (procArray[1].intervalList)->notices->nextInInterval->pageIndex == 1023);
	mu_assert("mem81.2", (procArray[1].intervalList)->notices->pageIndex == 2);


	memset(packet, 0, sizeof(wnPacket_t));
	packet->hostid = 1;
	(packet->timestamp)[1] = 1;
	packet->wnCount = 2;
	(packet->wnArray)[0] = 3;
	(packet->wnArray)[1] = 1024;
	
	mu_assert("mem81.3", incorporateWnPacket(packet) == 0);
	mu_assert("mem81.4", (procArray[1].intervalList->timestamp)[1] == 1);
	mu_assert("mem81.5", (procArray[1].intervalList->timestamp)[0] == 0);
	mu_assert("mem81.6", (procArray[1].intervalList->timestamp)[2] == 0);
	mu_assert("mem81.7", (procArray[1].intervalList->timestamp)[3] == 0);
	mu_assert("mem81.8", procArray[1].intervalList->next == NULL);

	mu_assert("mem81.9", (procArray[1].intervalList)->notices->nextInInterval->nextInInterval == pageArray[3].notices[1]);
	mu_assert("mem81.10", (procArray[1].intervalList)->notices->nextInInterval->nextInInterval->nextInInterval == pageArray[1024].notices[1]);
	mu_assert("mem81.11", (procArray[1].intervalList)->notices->nextInInterval->nextInInterval->nextInInterval->nextInInterval == NULL);
	mu_assert("mem81.12", (procArray[1].intervalList)->notices->nextInInterval->nextInInterval->nextInPage == NULL);
	mu_assert("mem81.13", (procArray[1].intervalList)->notices->nextInInterval->nextInInterval->nextInInterval->nextInPage == NULL);
	mu_assert("mem81.14", (procArray[1].intervalList)->notices->nextInInterval->nextInInterval->interval == procArray[1].intervalList);
	mu_assert("mem81.15", (procArray[1].intervalList)->notices->nextInInterval->nextInInterval->nextInInterval->interval == procArray[1].intervalList);
	mu_assert("mem81.16", (procArray[1].intervalList)->notices->nextInInterval->nextInInterval->diffAddress == NULL);
	mu_assert("mem81.17", (procArray[1].intervalList)->notices->nextInInterval->nextInInterval->nextInInterval->diffAddress == NULL);
	mu_assert("mem81.18", (procArray[1].intervalList)->notices->nextInInterval->nextInInterval->nextInInterval->pageIndex == 1024);
	mu_assert("mem81.19", (procArray[1].intervalList)->notices->nextInInterval->nextInInterval->pageIndex == 3);


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

