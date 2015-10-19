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

struct light_event_node {
	int id;
	struct event_requirements req;
	wait_queue_head_t waitq;
	spinlock_t light_event_lock;
	int waitq_size;
	struct list_head list;
	int in_destroy;
};

LIST_HEAD(light_event_list);
/* add static? */

struct kfifo light_intensity_fifo;

DEFINE_SPINLOCK(light_event_list_lock);
DEFINE_SPINLOCK(light_intensity_fifo_lock);
DEFINE_SPINLOCK(trivial_lock);

int cur_id;
bool initialized;

int initialize_fifo(void){
	int err;
	spin_lock(&trivial_lock);
	if(!initialized){
		initialized = 1;
		err = kfifo_alloc(&light_intensity_fifo,sizeof(struct light_intensity)*32,GFP_KERNEL);
		if(err < 0){
			spin_unlock(&trivial_lock);
			return err;
		}
	}
	spin_unlock(&trivial_lock);
	return 0;
}

struct light_event_node* get_evt_by_id(int id){
	struct light_event_node* result;
	
	list_for_each_entry(result,&light_event_list,list){
		if(result->id == id){
			return result;
		}
	}
	return NULL;
}

int get_condition(struct light_event_node * light_evt){
	int trigger;
	int cnt;
	int len;
	int i;
	int ret;
	struct light_intensity li;

	cnt = 0;	
	trigger = light_evt->req.req_intensity - NOISE;
	
	spin_lock(&light_intensity_fifo_lock);
	len = kfifo_len(&light_intensity_fifo);
	for(i=0;i<len/sizeof(struct light_intensity);i++){
		ret = kfifo_out_peek(&light_intensity_fifo,&li,sizeof(struct light_intensity));
		if(ret != sizeof(struct light_intensity)){
			spin_unlock(&light_intensity_fifo_lock);
			return -EFAULT;
		}
		if(li.cur_intensity >= trigger){
			++cnt;
		}
		ret = kfifo_out(&light_intensity_fifo,&li,sizeof(struct light_intensity));
		if(ret != sizeof(struct light_intensity)){
			spin_unlock(&light_intensity_fifo_lock);
			return -EFAULT;
		}
		ret = kfifo_in(&light_intensity_fifo,&li,sizeof(struct light_intensity));
		if(ret != sizeof(struct light_intensity)){
			spin_unlock(&light_intensity_fifo_lock);
			return -EFAULT;
		}
	}
	spin_unlock(&light_intensity_fifo_lock);

	if(cnt >= light_evt->req.frequency){
		return 1;
	}
	return 0;
}

SYSCALL_DEFINE1(light_evt_create, struct event_requirements __user *, intensity_params){
	int ret;
	struct light_event_node * new_evt;
	
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

	init_waitqueue_head(&(new_evt->waitq));
	spin_lock_init(&(new_evt->light_event_lock));
	new_evt->waitq_size = 0;
	new_evt->in_destroy = 0;
	spin_lock(&light_event_list_lock);
	new_evt->id = cur_id++;
	list_add(&(new_evt->list), &light_event_list);
	spin_unlock(&light_event_list_lock);

	return 0;
}

SYSCALL_DEFINE1(light_evt_wait, int, event_id){

	int ret;
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


	
	spin_lock(&the_evt->light_event_lock);
	ret = get_condition(the_evt);
	while(ret == 0){
		spin_unlock(&the_evt->light_event_lock);
		prepare_to_wait(&the_evt->waitq,&wait,TASK_INTERRUPTIBLE);
		schedule();
		spin_lock(&the_evt->light_event_lock);
		if(the_evt->in_destroy){
			--the_evt->waitq_size;
			finish_wait(&the_evt->waitq,&wait);
			spin_unlock(&(the_evt->light_event_lock));
			return 0;
		}
		ret = get_condition(the_evt);
	}
	--the_evt->waitq_size;
	if(ret < 0){
		spin_unlock(&the_evt->light_event_lock);
		return ret;
	}
	finish_wait(&the_evt->waitq,&wait);
	spin_unlock(&(the_evt->light_event_lock));

	return 0;
}


SYSCALL_DEFINE1(light_evt_signal, struct light_intensity __user *, user_light_intensity){
	int ret;
	int len;
	struct light_intensity *new_li;
	struct light_intensity tmp_li;
	struct light_event_node *light_evt;

	ret = initialize_fifo();
	if(ret < 0){
		return ret;
	}

	new_li = kmalloc(sizeof(struct light_intensity),GFP_KERNEL);
	if(new_li == NULL){
		return -ENOMEM;
	}

	ret = copy_from_user(new_li,user_light_intensity,sizeof(struct light_intensity));
	if(ret < 0){
		return -EFAULT;
	}

	spin_lock(&light_intensity_fifo_lock);
	len = kfifo_len(&light_intensity_fifo);
	if(len < sizeof(struct light_intensity)*WINDOW){
		ret = kfifo_in(&light_intensity_fifo,new_li,sizeof(struct light_intensity));
		if(ret < 0){
			spin_unlock(&light_intensity_fifo_lock);
			return ret;
		}
	}else{
		ret = kfifo_out(&light_intensity_fifo,&tmp_li,sizeof(struct light_intensity));
		if(ret < 0){
			spin_unlock(&light_intensity_fifo_lock);
			return ret;
		}
		ret = kfifo_in(&light_intensity_fifo,new_li,sizeof(struct light_intensity));
		if(ret < 0){
			spin_unlock(&light_intensity_fifo_lock);
			return ret;
		}
	}
	spin_lock(&(light_event_list_lock));
	list_for_each_entry(light_evt,&light_event_list,list){
		ret = get_condition(light_evt);
		if(ret < 0){
			spin_unlock(&(light_event_list_lock));
			spin_unlock(&light_intensity_fifo_lock);
			return ret;
		}
		if(ret){
			wake_up(&(light_evt->waitq));
		}
	}
	spin_unlock(&(light_event_list_lock));
	spin_unlock(&light_intensity_fifo_lock);
	
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
	while(light_event->waitq_size > 0){
		spin_unlock(&light_event->light_event_lock);

		wake_up(&(light_event->waitq));

		spin_lock(&light_event->light_event_lock);
	}
	spin_unlock(&light_event->light_event_lock);
	kfree(light_event);
	return 0;
}

