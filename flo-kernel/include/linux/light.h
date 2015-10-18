#ifndef _LIGHT_H
#define _LIGHT_H

struct light_intensity{
	int cur_intensity;
};

int set_light_intensity(struct light_intensity __user * user_light_intensity);

int get_light_intensity(struct light_intensity __user * user_light_intensity);

#endif
