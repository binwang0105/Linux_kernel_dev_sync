#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/light.h>
#include <asm-generic/errno-base.h>
#include <asm/uaccess.h>

/* open a kernel space */
static struct light_intensity kernel_li;

SYSCALL_DEFINE1(set_light_intensity, struct light_intensity __user *, user_light_intensity){
	
	/* current_euid()==0 grant permission, otherwise deny */
	if(current_euid() != 0){
		return -EACCES;
	}
	
	/* copy_from_user, &kernel_space, &user_space, sizeof */
	if(copy_from_user(&kernel_li, user_light_intensity, sizeof(struct light_intensity))){
		return -EFAULT;
	}
	
	return 0;
}

SYSCALL_DEFINE1(get_light_intensity, struct light_intensity __user *, user_light_intensity){
	/* copy_to_user, &user_space, &kernel_space, sizeof */
	if(copy_to_user(user_light_intensity, &kernel_li, sizeof(struct light_intensity))){
		return -EFAULT;
	}

	return 0;
}
