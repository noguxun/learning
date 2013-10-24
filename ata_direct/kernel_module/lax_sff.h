#ifndef _LAX_SFF_H
#define _LAX_SFF_H

#include <linux/fs.h>
#include <linux/compiler.h>

long ata_sff_file_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
int ata_sff_file_open(struct inode *inode, struct file *filp);
int ata_sff_file_release(struct inode *inode, struct file *filp);
void ata_sff_set_para(int ctl_i, int port_i);

#endif
