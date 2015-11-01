#include <asm-generic/errno-base.h>
#include <linux/syscalls.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <asm/atomic.h>
#include <linux/light.h>
#include <linux/lightsyn.h>

struct light_event_node {
	int id;
	struct event_requirements req;
	wait_queue_head_t waitq;
	spinlock_t light_event_lock;
	int waitq_size;
	int in_destroy;
	struct list_head list;
};
LIST_HEAD(light_event_list);

struct light_intensity_node {
	struct light_intensity li;
	struct list_head list;
};

LIST_HEAD(light_intensity_list);

DEFINE_SPINLOCK(light_event_list_lock);
DEFINE_SPINLOCK(light_intensity_list_lock);

static int cur_id;
static int num_evts;

struct light_event_node* get_evt_by_id(int id){
	struct light_event_node* result;
	
	list_for_each_entry(result,&light_event_list,list){
		if(result->id == id){
			return result;
		}
	}
	return NULL;
}

bool get_condition(struct light_event_node * light_evt){
	int trigger;
	int cnt;
	struct light_intensity_node * lin;
	cnt = 0;

	trigger = light_evt->req.req_intensity - NOISE;
	list_for_each_entry(lin,&light_intensity_list,list){
		if(lin->li.cur_intensity >= trigger){
			++cnt;
		}
	}
	if(cnt >= light_evt->req.frequency){
		return 1;
	}
	return 0;
}

SYSCALL_DEFINE1(light_evt_create, struct event_requirements __user *, intensity_params){
	int ret;
	int result;
	struct light_event_node * new_evt;

	if(cur_id > 10000){
		return -ENOMEM;
	}

	new_evt = kmalloc(sizeof(struct light_event_node),GFP_KERNEL);
	if(new_evt == NULL){
		return -ENOMEM;
	}
	
	ret = copy_from_user(&(new_evt->req), intensity_params, sizeof(struct event_requirements));
	if(ret < 0){
		return -EFAULT;
	}

	if(new_evt->req.frequency > WINDOW){
		new_evt->req.frequency = WINDOW;
	}

	init_waitqueue_head(&new_evt->waitq);
	spin_lock_init(&(new_evt->light_event_lock));
	new_evt->waitq_size = 0;
	new_evt->in_destroy = 0;
	spin_lock(&light_event_list_lock);
	result = cur_id++;

	new_evt->id = result;
	list_add(&(new_evt->list), &light_event_list);
	spin_unlock(&light_event_list_lock);

	return result;
}

SYSCALL_DEFINE1(light_evt_wait, int, event_id){

	DEFINE_WAIT(wait);
	struct light_event_node * the_evt;

	spin_lock(&light_event_list_lock);
	the_evt = get_evt_by_id(event_id);

	if(the_evt == NULL){
		spin_unlock(&light_event_list_lock);
		return -ENODATA;
	}
	spin_lock(&(the_evt->light_event_lock));
	++the_evt->waitq_size;
	spin_unlock(&(the_evt->light_event_lock));
	spin_unlock(&light_event_list_lock);


	while(1){
		spin_lock(&light_intensity_list_lock);
		prepare_to_wait(&the_evt->waitq,&wait,TASK_INTERRUPTIBLE);
		if(the_evt->in_destroy){
			finish_wait(&the_evt->waitq,&wait);
			spin_unlock(&light_intensity_list_lock);
			spin_lock(&the_evt->light_event_lock);
			--the_evt->waitq_size;
			spin_unlock(&the_evt->light_event_lock);
			return 1;
		}
		if(get_condition(the_evt)){
			break;
		}
		if(signal_pending(current)){
			finish_wait(&the_evt->waitq,&wait);
			spin_unlock(&light_intensity_list_lock);
			spin_lock(&the_evt->light_event_lock);
			--the_evt->waitq_size;
			spin_unlock(&the_evt->light_event_lock);
			return -ERESTARTSYS;
		}
		spin_unlock(&light_intensity_list_lock);
		schedule();
	}
	
	finish_wait(&the_evt->waitq,&wait);
	spin_unlock(&light_intensity_list_lock);
	spin_lock(&the_evt->light_event_lock);
	--the_evt->waitq_size;
	spin_unlock(&the_evt->light_event_lock);

	return 0;
}


SYSCALL_DEFINE1(light_evt_signal, struct light_intensity __user *, user_light_intensity){
	int ret;
	struct light_intensity_node *new_lin;
	struct light_intensity_node *tmp_lin;
	struct light_event_node *light_evt;
	new_lin = kmalloc(sizeof(struct light_intensity_node),GFP_KERNEL);
	if(new_lin == NULL){
		return -ENOMEM;
	}

	ret = copy_from_user(&new_lin->li,user_light_intensity,sizeof(struct light_intensity));
	if(ret < 0){
		return -EFAULT;
	}
	spin_lock(&light_intensity_list_lock);
	list_add_tail(&(new_lin->list),&light_intensity_list);
	if(num_evts < WINDOW){
		++num_evts;
	}else{
		tmp_lin = list_first_entry(&light_intensity_list,struct light_intensity_node, list);
		list_del(&tmp_lin->list);
		kfree(tmp_lin);
	}

	spin_lock(&(light_event_list_lock));

	list_for_each_entry(light_evt,&light_event_list,list){

		if(get_condition(light_evt)){
			wake_up(&light_evt->waitq);
		}
	}
	spin_unlock(&(light_event_list_lock));
	spin_unlock(&light_intensity_list_lock);

	return 0;
}

SYSCALL_DEFINE1(light_evt_destroy, int, event_id){

	struct light_event_node *light_event;

	spin_lock(&light_event_list_lock);
	light_event = get_evt_by_id(event_id);
	if(light_event == NULL){
		spin_unlock(&light_event_list_lock);
		return -ENODATA;
	}
	list_del(&(light_event->list));
	spin_unlock(&light_event_list_lock);
	spin_lock(&light_event->light_event_lock);
	light_event->in_destroy = true;
	wake_up(&light_event->waitq);
	while(light_event->waitq_size > 0){
		spin_unlock(&light_event->light_event_lock);
		spin_lock(&light_event->light_event_lock);
	}
	spin_unlock(&light_event->light_event_lock);
	kfree(light_event);
	return 0;
}









