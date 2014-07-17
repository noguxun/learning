/*
 * Sample disk driver, from the beginning.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>


#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/blkdev.h>
#include <linux/bio.h>

MODULE_LICENSE("GPL");

static int sbull_major = 0;
module_param(sbull_major, int, 0);
static int hardsect_size = 512;
module_param(hardsect_size, int, 0);
static int nsectors = 0x1000;	/* How big the drive is */
module_param(nsectors, int, 0);

/*
 * We can tweak our hardware sector size, but the kernel talks to us
 * in terms of small sectors, always.
 */
#define KERNEL_SECTOR_SIZE	512

#define PK(fmt, args...)  printk(KERN_INFO "sbull: " fmt, ## args )
#define PKL(fmt, args...) printk(KERN_INFO "sbull: " fmt "\n", ## args )

/*
 * The internal representation of our device.
 */
struct sbull_dev {
        int size;                       /* Device size in sectors */
        u8 *data;                       /* The data array */
        short users;                    /* How many users */
        short media_change;             /* Flag a media change? */
        spinlock_t lock;                /* For mutual exclusion */
        struct request_queue *queue;    /* The device request queue */
        struct gendisk *gd;             /* The gendisk structure */
        struct timer_list timer;        /* For simulated media changes */
};

#if 1
static struct sbull_dev *Device = NULL;

/*
 * Handle an I/O request.
 */
static void sbull_transfer(struct sbull_dev *dev, unsigned long sector,
		unsigned long nsect, char *buffer, int write)
{
#if 0
	unsigned long offset = sector*KERNEL_SECTOR_SIZE;
	unsigned long nbytes = nsect*KERNEL_SECTOR_SIZE;

	if ((offset + nbytes) > dev->size) {
		printk (KERN_NOTICE "Beyond-end write (%ld %ld)\n", offset, nbytes);
		return;
	}
	if (write)
		memcpy(dev->data + offset, buffer, nbytes);
	else
		memcpy(buffer, dev->data + offset, nbytes);
#endif
	PKL("  xfer sector %lx nr %lx", sector, nsect);
}

/*
 * Transfer a single BIO.
 */
static int sbull_xfer_bio(struct sbull_dev *dev, struct bio *bio)
{
	int i;
	struct bio_vec *bvec;
	sector_t sector = bio->bi_sector;

	/* Do each segment independently. */
	bio_for_each_segment(bvec, bio, i) {
		char *buffer = __bio_kmap_atomic(bio, i);
		sbull_transfer(dev, sector, bio_cur_bytes(bio) >> 9,
				buffer, bio_data_dir(bio) == WRITE);
		sector += bio_cur_bytes(bio) >> 9;
		__bio_kunmap_atomic(bio);
	}
	return 0; /* Always "succeed" */
}

/*
 * Transfer a full request.
 */
static int sbull_xfer_request(struct sbull_dev *dev, struct request *req)
{
	struct bio *bio;
	int nsect = 0;
#if 0
	__rq_for_each_bio(bio, req) {
		sbull_xfer_bio(dev, bio);
		nsect += bio->bi_size/KERNEL_SECTOR_SIZE;
	}
	return nsect;
#else
	return 	(int)blk_rq_sectors(req);
#endif
}

/*
 * Smarter request function that "handles clustering".
 */
static void sbull_full_request(struct request_queue *q)
{
	struct request *req;
	int sectors_xferred;
	struct sbull_dev *dev = q->queuedata;

	while ((req = blk_fetch_request(q)) != NULL) {
		if (req->cmd_type != REQ_TYPE_FS) {
			printk (KERN_NOTICE "Skip non-fs request\n");
			__blk_end_request(req, -EIO, blk_rq_cur_bytes(req));
			continue;
		}
		sectors_xferred = sbull_xfer_request(dev, req);
		PKL("request sector %lx nr %x xfered %x",
			(unsigned long)blk_rq_pos(req),
			(int)blk_rq_sectors(req),
			sectors_xferred);
		blk_end_request(req, 0, sectors_xferred);
	}
}


/*
 * Open and close.
 */
static int sbull_open(struct block_device *bdev, fmode_t mode)
{
	struct sbull_dev *dev = bdev->bd_disk->private_data;

	spin_lock(&dev->lock);
	dev->users++;
	spin_unlock(&dev->lock);
        PKL("open %d", dev->users);
	return 0;
}

static void sbull_release(struct gendisk *disk, fmode_t mode)
{
	struct sbull_dev *dev = disk->private_data;

	spin_lock(&dev->lock);
	dev->users--;
	spin_unlock(&dev->lock);
        PKL("release %d", dev->users);

	return;
}



/*
 * The ioctl() implementation
 */

int sbull_ioctl (struct block_device *bdev, fmode_t mode,
                 unsigned int cmd, unsigned long arg)
{
	PKL("ioctl");
	return -ENOTTY; /* unknown command */
}



/*
 * The device operations structure.
 */
static struct block_device_operations sbull_ops = {
	.owner           = THIS_MODULE,
	.open 	         = sbull_open,
	.release 	 = sbull_release,
	.ioctl	         = sbull_ioctl
};


/*
 * Set up our internal device.
 */
static void setup_device(struct sbull_dev *dev)
{
	/*
	 * Get some memory.
	 */
	memset (dev, 0, sizeof (struct sbull_dev));
	dev->size = nsectors*hardsect_size;
	dev->data = vmalloc(dev->size);
	if (dev->data == NULL) {
		printk (KERN_NOTICE "vmalloc failure.\n");
		return;
	}
	spin_lock_init(&dev->lock);

	dev->queue = blk_init_queue(sbull_full_request, &dev->lock);
	if (dev->queue == NULL)
		goto out_vfree;

	blk_queue_logical_block_size(dev->queue, hardsect_size);
	dev->queue->queuedata = dev;

	/*
	 * And the gendisk structure.
	 */
	dev->gd = alloc_disk(16);
	if (! dev->gd) {
		printk (KERN_NOTICE "alloc_disk failure\n");
		goto out_vfree;
	}
	dev->gd->major = sbull_major;
	dev->gd->first_minor = 0;
	dev->gd->fops = &sbull_ops;
	dev->gd->queue = dev->queue;
	dev->gd->private_data = dev;
	dev->gd->flags |= GENHD_FL_SUPPRESS_PARTITION_INFO;
	snprintf (dev->gd->disk_name, 32, "sbull0");
	set_capacity(dev->gd, nsectors*(hardsect_size/KERNEL_SECTOR_SIZE));
	PKL("capacity %x", nsectors);
	add_disk(dev->gd);
	return;

  out_vfree:
	if (dev->data)
		vfree(dev->data);
}

#endif


static int __init sbull_init(void)
{
	/*
	 * Get registered.
	 */
#if 1
	sbull_major = register_blkdev(sbull_major, "sbull");
	if (sbull_major <= 0) {
		PKL("blk device registration failed\n");
		return -EBUSY;
	}
	/*
	 * Allocate the device array, and initialize each one.
	 */
	Device = kmalloc(sizeof (struct sbull_dev), GFP_KERNEL);
	if (Device == NULL)
		goto out_unregister;

	setup_device(Device);
#endif
        PKL("init");

	return 0;

#if 1
  out_unregister:
	unregister_blkdev(sbull_major, "sbull");
	return -ENOMEM;
#endif
}

static void sbull_exit(void)
{
#if 1
	struct sbull_dev *dev = Device;

	if (dev->gd) {
		del_gendisk(dev->gd);
		put_disk(dev->gd);
	}
	if (dev->queue) {
		blk_cleanup_queue(dev->queue);
	}
	if (dev->data)
		vfree(dev->data);

	unregister_blkdev(sbull_major, "sbull");
	kfree(Device);
#endif
	PKL("exit");
}

module_init(sbull_init);
module_exit(sbull_exit);
