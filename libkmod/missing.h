#pragma once

#ifdef HAVE_LINUX_MODULE_H
#include <linux/module.h>
#endif

#ifndef MODULE_INIT_IGNORE_MODVERSIONS
# define MODULE_INIT_IGNORE_MODVERSIONS 1
#endif

#ifndef MODULE_INIT_IGNORE_VERMAGIC
# define MODULE_INIT_IGNORE_VERMAGIC 2
#endif
