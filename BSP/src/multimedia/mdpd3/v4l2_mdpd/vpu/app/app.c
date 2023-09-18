#include <stddef.h>

#include "app.h"

extern struct app_descriptor _app_mdp_service;
extern struct app_descriptor _app_mdp_process_service;
extern struct app_descriptor _app_mdp1_service;
extern struct app_descriptor _app_mdp1_process_service;
extern struct app_descriptor _app_mdp2_service;
extern struct app_descriptor _app_mdp2_process_service;
extern struct app_descriptor _app_mdp3_service;
extern struct app_descriptor _app_mdp3_process_service;

const struct app_descriptor *apps[] = {
	&_app_mdp_service,
	&_app_mdp_process_service,
	&_app_mdp1_service,
	&_app_mdp1_process_service,
	&_app_mdp2_service,
	&_app_mdp2_process_service,
	&_app_mdp3_service,
	&_app_mdp3_process_service,
	NULL
};

/* one time setup */
void apps_init(void)
{
	const struct app_descriptor *app;
	int i;

	/* call all the init routines */
	for (i = 0; apps[i] != NULL; i++) {
		app = apps[i];
		if (app->init)
			app->init(app);
	}
}

int app_thread_entry(void *arg)
{
	const struct app_descriptor *app = (const struct app_descriptor *)arg;

	app->entry(app, NULL);

	return 0;
}
