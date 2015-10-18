#include <asm-generic/errno-base.h>
#include <linux/syscalls.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/kfifo.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <asm/atomic.h>
#include <linux/light.h>
#include <linux/lightsyn.h>

struct light_intensity_node {
	struct light_intensity li;
	struct list_head list;
};

static LIST_HEAD(light_intensity_list);
static DEFINE_SPINLOCK(light_intensity_list_lock);

struct light_event_node {
	int id;
	struct event_requirements req;
	struct list_head list;
};

static LIST_HEAD(light_event_list);
static DEFINE_SPINLOCK(light_event_list_lock);


SYSCALL_DEFINE1(light_evt_create, struct event_requirements __user *, intensity_params){

	return 0;
}

SYSCALL_DEFINE1(light_evt_wait, int, event_id){

	return 0;
}


SYSCALL_DEFINE1(light_evt_signal, struct light_intensity __user *, user_light_intensity){

	return 0;
}

SYSCALL_DEFINE1(light_evt_destroy, int, event_id){


	return 0;
}









