#ifndef _LIGHTSYN_H
#define _LIGHTSYN_H

#include <linux/light.h>

#define NOISE 10
#define WINDOW 20

struct event_requirements {
	int req_intensity;
	int frequency;
};

int light_evt_create(struct event_requirements __user * intensity_params);

int light_evt_wait(int event_id);

int light_evt_signal(struct light_intensity __user * user_light_intensity);

int light_evt_destroy(int event_id);

#endif
