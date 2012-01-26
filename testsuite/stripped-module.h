#ifndef _TESTSUITE_STRIPPED_MODULE_H
#define _TESTSUITE_STRIPPED_MODULE_H

enum module_state
{
	MODULE_STATE_LIVE,
	MODULE_STATE_COMING,
	MODULE_STATE_GOING,
};

struct list_head {
	struct list_head *next, *prev;
};

#define MODULE_NAME_LEN (64 - sizeof(unsigned long))
struct module
{
	enum module_state state;

	/* Member of list of modules */
	struct list_head list;

	/* Unique handle for this module */
	char name[MODULE_NAME_LEN];
};

#endif
