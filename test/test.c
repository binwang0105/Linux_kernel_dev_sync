#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "light.h"
#include <errno.h>
#include <syscall.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/types.h>



#define light_evt_create	380
#define light_evt_wait  	381
#define light_evt_destroy	383


int main (int argc, char **argv)
{
	int ret;
	pid_t pid;
	int i=0, n=0;
	int low_id = 0;
	int medium_id = 0;
	int high_id = 0;

	struct event_requirements low_req;
	struct event_requirements medium_req;
	struct event_requirements high_req;

	clock_t start,end;

	start = clock();

	if (argc == 2)
	{
		n = atoi(argv[1]);
	} 
	else 
	{
		printf("incorrect argument\n");
		return -1;
	}

	low_req.req_intensity = 500;
	low_req.frequency = 10;

	medium_req.req_intensity = 8000;
	medium_req.frequency = 10;

	high_req.req_intensity = 500000;
	high_req.frequency = 10;

	low_id = syscall(light_evt_create, &low_req);
	if(low_id < 0)
	{
		printf("error in create\n");
		return -1;
	}

	medium_id = syscall(light_evt_create, &medium_req);
	if(medium_id < 0)
	{
		printf("error in create\n");
		return -1;
	}

	high_id = syscall(light_evt_create, &high_req);
	if(high_id < 0)
	{
		printf("error in create\n");
		return -1;
	}

	

	for (i = 0; i < n; i++)
	{
		pid = fork();
		if (pid < 0){
			printf("error in fork\n");
			return -1;
		}
		if(pid > 0){
			continue;
		}
		
		if(i%3 == 0){
			ret = syscall(light_evt_wait, low_id);
			if(ret == 0){
				printf("%d detected a low intensity event\n", getpid());
				return 0;
			}
		}else if(i%3 == 1){
			ret = syscall(light_evt_wait, medium_id);
			if(ret == 0){
				printf("%d detected a medium intensity event\n", getpid());
				return 0;
			}
		}else{
			ret = syscall(light_evt_wait, high_id);
			if(ret == 0){
				printf("%d detected a high intensity event\n", getpid());
				return 0;
			}
		}
		if (ret < 0){
			printf("error in wait\n");
			return -1;
		}else if(ret > 0){
			printf("%d gets destroyed during wait\n", getpid());
			return 0;
		}

	}
	while(1)
	{
		end = clock();
		if((end - start)/(double)CLOCKS_PER_SEC >= 60)
			break;
	}

	ret = syscall(light_evt_destroy, low_id);
	if(ret!=0){
		printf("error in distroy\n");
		return -1;
	}
	ret = syscall(light_evt_destroy, medium_id);
	if(ret!=0){
		printf("error in distroy\n");
		return -1;
	}
	ret = syscall(light_evt_destroy, high_id);
	if(ret!=0){
		printf("error in distroy\n");
		return -1;
	}

	while (wait(NULL) > 0)
		;

	printf("all process finish\n");

	return 0;	
}
