#include <stdio.h>
#include <stdlib.h>
#include "syn.h"

extern int myhostid;
extern int hostnum;

milock_t locks[LOCK_NUM];
int waitFlag;
int myLocks[LOCK_NUM];
int barrierFlags[MAX_HOST_NUM];


/**
* initialization of syn module, set initial values for all global variables
**/
void initsyn(){
	int i, j;
	for(i = 0; i < LOCK_NUM; i++){
		locks[i].state = FREE;
		locks[i].owner = -1;
		locks[i].lasthostid = -1;
		for(j = 0; j < MAX_HOST_NUM; j++){
			locks[i].waitingList[j] = -1;
		}
		locks[i].waitingListCount = 0;
	}
	waitFlag = 0;
	for(i = 0; i < LOCK_NUM; i++){
		myLocks[i] = 0;
	}
	for(i = 0; i < MAX_HOST_NUM; i++){
		barrierFlags[i] = 0;
	}
}


/** 
* user will use this procedure to acuquire a lock
* parameters
*	lockno : index of lock in locks array
* return value
*	 0 --- success
*	-1 --- parameter error
*	-2 --- user has already owned this lock
**/
int mi_lock(int lockno){
	if((lockno < 0) || (lockno >= LOCK_NUM)){
		return -1;
	}
	if(myLocks[lockno] == 1){
		return -2;
	}
	if((lockno % hostnum) == myhostid){
		int result = graspLock(lockno, myhostid);
		if(result == -2){
			waitFlag = 1;
			while(waitFlag)
				;
			return 0;
		}else{
			myLocks[lockno] = 1;
			return 0;
		}
	}else{
		mimsg_t *msg = nextFreeMsgInQueue(0);
		msg->from = myhostid;
		msg->to = lockno % hostnum;
		msg->command = ACQ;
		char buffer[20];
		sprintf(buffer, "%d", lockno);
		apendMsgData(msg, buffer, sizeof(int));
		sendMsg(msg);
		waitFlag = 1;
		while(waitFlag)
			;
		return 0;
	}
}


/**
* user will use this procedure to release a lock
* parameters
*	lockno : index of lcok in locks array
* return value
*	 0 --- success
*	-1 --- parameter error
*	-2 --- user does not own this lock 
**/
int mi_unlock(int lockno){
	if((lockno < 0) || (lockno >= LOCK_NUM)){
		return -1;
	}
	if(myLocks[lockno] == 0){
		return -2;
	}
	if((lockno % hostnum) == myhostid){
		myLocks[lockno] = 0;
		int result = freeLock(lockno, myhostid);	
		if(result == -5){
			return 0;
		}else if((result >= 0) && (result != myhostid)){
			grantLock(lockno, result);
			return 0;
		}else{
			fprintf(stderr, "error occured in mi_lock with result = %d\n",result);
			return 0;
		}
	}else{
		mimsg_t *msg = nextFreeMsgInQueue(0);
		msg->from = myhostid;
		msg->to = lockno % hostnum;
		msg->command = RLS;
		char buffer[20];
		sprintf(buffer, "%d", lockno);
		apendMsgData(msg, buffer, sizeof(int));
		sendMsg(msg);
		return 0;
	}
}


/** 
* user will use this procedure to enter barrier
**/
void mi_barrier(){
	if(myhostid == 0){
		barrierFlags[0] = 1;
		int result = checkBarrierFlags();
		if(result == 0){
			return;
		}
	}else{
		mimsg_t *msg = nextFreeMsgInQueue(0);
		msg->from = myhostid;
		msg->to = 0;
		msg->command = ENTER_BARRIER;
		sendMsg(msg);
	}
	waitFlag = 1;
	while(waitFlag)
		;
	return;
}


/**
* dispatchMsg will use this procedure to handle ACQ msg
* parameters
*	msg : msg to be handled
**/
void handleAcquireMsg(mimsg_t *msg){
	int lockno = strtol(msg->data, NULL, 10);
	int hostid = msg->from;
	if((lockno >= 0) && (lockno <= LOCK_NUM)){
		int result = graspLock(lockno, hostid);
		if(result == 0){
			grantLock(lockno, hostid);	
		}
	}	
}


/**
* dispatchMsg will use this procedure to handle RLS msg
* parameters
*	msg : msg to be handled
**/
void handleReleaseMsg(mimsg_t *msg){
	int lockno = strtol(msg->data, NULL, 10);
	int hostid = msg->from;
	if((lockno >= 0) && (lockno <= LOCK_NUM)){
		int result = freeLock(lockno, hostid);
		if(result >= 0){
			if(result == myhostid){
				waitFlag = 0;
				myLocks[lockno] = 1;
			}else{
				grantLock(lockno, result);	
			}
		}
	}	
}


/**
* dispatchMsg will use this procedure to handle ENTER_BARRIER msg
* parameters
*	msg : msg to be handled
**/
void handleEnterBarrierMsg(mimsg_t *msg){
	int hostid = msg->from;
	barrierFlags[hostid] = 1;
	int result = checkBarrierFlags();
	if(result == 0){
		waitFlag = 0;
	}
}


/**
* dispatchMsg will use this procedure to handle EXIT_BARRIER msg
* parameters
*	msg : msg to be handled
**/
void handleExitBarrierMsg(mimsg_t *msg){
	if(msg == NULL){
		return;
	}
	int from = msg->from;
	if(from == 0){
		printf("waitFlag == 0 in exitBarrierHandler\n");
		waitFlag = 0;
	}
}


/**
* dispatchMsg will use this procedure to handle GRANT msg
* parameters
*	msg : msg to be handled
**/
void handleGrantMsg(mimsg_t *msg){
	if(msg == NULL){
		return;
	}
	int lockno = strtol(msg->data, NULL, 10);
	if((lockno >= 0) && (lockno <= LOCK_NUM)){
		myLocks[lockno] = 1;
		waitFlag = 0;
	}
}


/**
* lock manager will use this procedure to check lock state. If lock is free, return 0, otherwise return -3, and add hostid to the lock waiting list.
* parameters
*	lockno : index of lock in locks array
*	hostid : host which want to acquire this lock
* return value
*	 0 --- success
*	-1 --- parameters error
*	-2 --- this node has no right to manipulate this lock
*	-3 --- lock is busy
*	-4 --- this host has already owned this lock
**/
int graspLock(int lockno, int hostid){
	if(lockno < 0 || lockno >= LOCK_NUM || hostid < 0 || hostid >= hostnum){
		return -1;
	}
	if((lockno % hostnum) != myhostid){
		return -2;
	}
	if(locks[lockno].state == FREE){
		locks[lockno].state = LOCKED;
		locks[lockno].owner = hostid;
		return 0;
	}else{
		if(locks[lockno].owner == hostid){
			return -4;
		}
		locks[lockno].waitingList[locks[lockno].waitingListCount] = hostid;
		(locks[lockno].waitingListCount)++;
		return -3;
	}
}


/** 
* send GRANT msg to hostid, telling it that lockno belongs to it
* parameters
*	lockno : index of lock in locks array
*	hostid : the id of host which will be received GRANT msg
**/
void grantLock(int lockno, int hostid){
	if((lockno < -1) || (lockno >= LOCK_NUM) || (hostid < 0) || (hostid >= hostnum)){
		return;
	}
	if(hostid == myhostid){
		return;
	}
	if((lockno % hostnum) != myhostid){
		return;
	}
	mimsg_t *msg = nextFreeMsgInQueue(0);
	msg->from = myhostid;
	msg->to = hostid;
	msg->command = GRANT;
	char buffer[20];
	sprintf(buffer, "%d", lockno);
	apendMsgData(msg, buffer, sizeof(int));
	sendMsg(msg);
}


/**
* lock manager will use this procedure to change lock state to free. If waiting list of this lock is not NULL, the top hostid of waiting list will be returned.
* parameters
*	lockno : index of lock in locks array
* return value
*	>= 0 --- id of a waiting host
*	  -1 --- parameters error
*	  -2 --- this node has no right to manipulate this lock
*	  -3 --- the state of this lock is free
*	  -4 --- this lock does not belong to the host
*	  -5 --- no waiting host
**/
int freeLock(int lockno, int hostid){
	if(lockno < 0 || lockno >= LOCK_NUM || hostid < 0 || hostid >= hostnum){
		return -1;
	}
	if((lockno % hostnum) != myhostid){
		return -2;
	}
	if(locks[lockno].state == FREE){
		return -3;
	}else{
		if(locks[lockno].owner != hostid){
			return -4;
		}
		if(locks[lockno].waitingListCount == 0){
			locks[lockno].state = FREE;
			locks[lockno].owner = -1;
			locks[lockno].lasthostid = hostid;
			return -5;
		}else{
			int waitinghostid = locks[lockno].waitingList[0];
			int i;
			for(i = 0; i < (locks[lockno].waitingListCount)-1; i++){
				locks[lockno].waitingList[i] = locks[lockno].waitingList[i+1];
			}
			locks[lockno].waitingList[(locks[lockno].waitingListCount)-1] = -1;
			(locks[lockno].waitingListCount)--;
			locks[lockno].owner = waitinghostid;
			locks[lockno].lasthostid = hostid;
			return waitinghostid;
		}
	}
}


/**
* if all hosts enter barrier, send msg to them which tells them exit barrier.
* return value
*	 0 --- success
*	-1 --- not all elements of barrierFlags are 1    
*	-2 --- this node has no right to manipulate barriers
**/
int checkBarrierFlags(){
	if(myhostid != 0){
		return -2;
	}
	int i;
	for(i = 0; i < hostnum; i++){
		if(barrierFlags[i] != 1){
			return -1;
		}
	}
	mimsg_t *msg;
	for(i = 1; i < hostnum; i++){
		msg = nextFreeMsgInQueue(0);
		msg->from = 0;
		msg->to = i;
		msg->command = EXIT_BARRIER;
		sendMsg(msg);
	}
	for(i = 0; i < hostnum; i++){
		barrierFlags[i] = 0;
	}
	return 0;
}

