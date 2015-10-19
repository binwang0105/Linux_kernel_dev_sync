#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/light.h>
#include <asm-generic/errno-base.h>
#include <asm/uaccess.h>

static struct light_intensity kernel_li;

SYSCALL_DEFINE1(set_light_intensity, struct light_intensity __user *, user_light_intensity){

	if(current_euid() != 0){
		return -EACCES;
	}

	if(copy_from_user(&kernel_li, user_light_intensity, sizeof(struct light_intensity))){
		return -EFAULT;
	}
	
	return 0;
}

SYSCALL_DEFINE1(get_light_intensity, struct light_intensity __user *, user_light_intensity){

	if(copy_to_user(user_light_intensity, &kernel_li, sizeof(struct light_intensity))){
		return -EFAULT;
	}

	return 0;
}







