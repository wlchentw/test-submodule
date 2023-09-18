#pragma once

#define MAGIC 0x5A5A5A5A

/* app support api */
int app_thread_entry(void *arg);

/* app entry point */
struct app_descriptor;
typedef void (*app_init)(const struct app_descriptor *);
typedef void (*app_entry)(const struct app_descriptor *, void *args);

/* each app needs to define one of these to define its startup conditions */
struct app_descriptor {
    unsigned int magic;
	const char *name;
	app_init  init;
	app_entry entry;
	void *priv; /* app private data */
	unsigned int flags;
};

#define APP_START(appname) volatile  __section(".apps") struct app_descriptor _app_##appname  = { .magic = MAGIC, /*.name = #appname,*/
#define APP_END };
