#include <linux/device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sysfs.h>
#include <linux/stat.h>
#include <linux/kobject.h>

MODULE_LICENSE("GPL");

#define PK(fmt, args...)  printk(KERN_INFO "hx: " fmt, ## args )
#define PKL(fmt, args...) printk(KERN_INFO "hx: " fmt "\n", ## args )

#define HX_SIZE PAGE_SIZE


static char hxbuf[HX_SIZE];
static struct kobject kobj;


void hx_kobj_release(struct kobject *kobject)
{
	PKL("hx_release");
}

ssize_t hx_show(struct kobject *object, struct attribute *attr, char *buf)
{
	PKL("hx_show");
	scnprintf(buf, HX_SIZE, "%s", hxbuf);

	return strnlen(buf, HX_SIZE) + 1;
}

ssize_t hx_store(struct kobject *object, struct attribute *attr, const char *buf, size_t count)
{
	PKL("hx_store");
	scnprintf(hxbuf, HX_SIZE, "%s", buf);

	return count;
}

static struct attribute hx_attr = {
	.name = "attr1",
	.mode = S_IRWXUGO,
};

static struct attribute *hx_attrs[] = {
	&hx_attr,
	NULL,
};

static struct sysfs_ops hx_sysops = {
	.show = hx_show,
	.store = hx_store,
};

static struct kobj_type ktype = {
	.release = hx_kobj_release,
	.sysfs_ops = &hx_sysops,
	.default_attrs = hx_attrs,
};

static int __init hx_init(void)
{
	PKL("hx_init");

	scnprintf(hxbuf, HX_SIZE, "There is nothing here\n");

	kobject_init_and_add(&kobj, &ktype, NULL, "hellx");

	return 0;
}

static void __exit hx_exit(void)
{
	PKL("hx_exit");
	kobject_del(&kobj);
}

module_init(hx_init);
module_exit(hx_exit);
