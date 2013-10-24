#ifndef _LAX_AHCI_H
#define _LAX_AHCI_H

void ata_ahci_set_para(int port_i);
long ata_ahci_file_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
int ata_ahci_file_open(struct inode *inode, struct file *filp);
int ata_ahci_file_release(struct inode *inode, struct file *filp);

#endif
