#ifndef _LAX_COMMON_H
#define _LAX_COMMON_H
/*
 * compile-time options: to be removed as soon as all the drivers are
 * converted to the new debugging mechanism
 */
#undef LAX_DEBUG		/* debugging output */
#undef LAX_VERBOSE_DEBUG	/* yet more debugging output */


/* note: prints function name for you */
#ifdef LAX_DEBUG
#define VPK(fmt, args...) printk(KERN_ERR "%s: " fmt, __func__, ## args)
#ifdef LAX_VERBOSE_DEBUG
#define VPK(fmt, args...) printk(KERN_ERR "%s: " fmt, __func__, ## args)
#else
#define VPK(fmt, args...)
#endif
#else
#define VPK(fmt, args...)
#endif

#define PK(fmt, args...) printk(KERN_INFO fmt, ## args )

#endif
