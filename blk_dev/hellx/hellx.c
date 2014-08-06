/*
 * Sample disk driver, from the beginning.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/bio.h>
#include <linux/hdreg.h>
#include <linux/workqueue.h>

MODULE_LICENSE("GPL");

static int hx_major = 0;
static int hardsect_size = 512;
static int nsectors = 0x100000;	/* How big the drive is */

/*
 * We can tweak our hardware sector size, but the kernel talks to us
 * in terms of small sectors, always.
 */
#define KERNEL_SECTOR_SIZE	512

#define PK(fmt, args...)  printk(KERN_INFO "hx: " fmt, ## args )
#define PKL(fmt, args...) printk(KERN_INFO "hx: " fmt "\n", ## args )

/*
 * The internal representation of our device.
 */
/*
 * TODO: need to add a mutex protection in process context
 */
struct hx_dev {
        int size;                       /* Device size in sectors */
        u8 *data;                       /* The data array */
        spinlock_t rq_lock;             /* For block request queue */
	struct mutex lock;              /* For driver data */
        struct request_queue *queue;    /* The device request queue */
        struct gendisk *gd;             /* The gendisk structure */
	struct workqueue_struct *wq;    /* The work queue */
	struct work_struct work;        /* The work object */
};

static struct hx_dev *Device = NULL;

/*
 * Handle an I/O request.
 */
static void hx_transfer(struct hx_dev *dev, unsigned long sector,
		unsigned long nsect, char *buffer, int write)
{
	unsigned long offset = sector*KERNEL_SECTOR_SIZE;
	unsigned long nbytes = nsect*KERNEL_SECTOR_SIZE;

	if ((offset + nbytes) > dev->size) {
		PKL("Beyond-end write (%ld %ld)\n", offset, nbytes);
		return;
	}
	if (write)
		memcpy(dev->data + offset, buffer, nbytes);
	else
		memcpy(buffer, dev->data + offset, nbytes);
	PKL("  xfer sector %lx nr %lx wr %d offset %lx", sector, nsect, write, offset);
}

/*
 * Transfer a single BIO.
 */
static int hx_xfer_bio(struct hx_dev *dev, struct bio *bio)
{
	int i;
	struct bio_vec *bvec;
	sector_t sector = bio->bi_sector;

	/* Do each segment independently. */
	bio_for_each_segment(bvec, bio, i) {
		char *buffer = __bio_kmap_atomic(bio, i);
		hx_transfer(dev, sector, bio_cur_bytes(bio) >> 9,
				buffer, bio_data_dir(bio) == WRITE);
		sector += bio_cur_bytes(bio) >> 9;
		__bio_kunmap_atomic(bio);
	}
	return 0; /* Always "succeed" */
}

/*
 * Transfer a full request.
 */
static int hx_xfer_request(struct hx_dev *dev, struct request *req)
{
	struct bio *bio;
	int nsect = 0;

	__rq_for_each_bio(bio, req) {
		hx_xfer_bio(dev, bio);
		nsect += bio->bi_size/KERNEL_SECTOR_SIZE;
	}
	return nsect;
}

/*
 * Smarter request function that "handles clustering".
 */
static void hx_request(struct request_queue *q)
{
	struct hx_dev *dev = q->queuedata;

	queue_work(dev->wq, &dev->work);
}


/*
 * Open and close.
 */
static int hx_open(struct block_device *bdev, fmode_t mode)
{
	struct hx_dev *dev = bdev->bd_disk->private_data;

	mutex_lock(&dev->lock);

	/*
	 * shall we doing something here?
	 */

	mutex_unlock(&dev->lock);
        PKL("open");

	return 0;
}

static void hx_release(struct gendisk *disk, fmode_t mode)
{
	struct hx_dev *dev = disk->private_data;

	mutex_lock(&dev->lock);

	/*
	 * shall we doing something here?
	 */

	mutex_unlock(&dev->lock);

 	PKL("release");

	return;
}

static void hx_work(struct work_struct *work)
{
	struct hx_dev *dev =
		container_of(work, struct hx_dev, work);
	struct request_queue *rq = dev->queue;
	struct request *req;
	int sectors_xferred;


	while (1) {
		spin_lock_irq(rq->queue_lock);
		req = blk_fetch_request(rq);
		spin_unlock_irq(rq->queue_lock);

		if (req == NULL) {
			break;
		}

		if (req->cmd_type != REQ_TYPE_FS) {
			PKL("Skip non-fs request\n");

			spin_lock_irq(rq->queue_lock);
			__blk_end_request(req, -EIO, blk_rq_cur_bytes(req));
			spin_unlock_irq(rq->queue_lock);

			continue;
		}
		sectors_xferred = hx_xfer_request(dev, req);
		PKL("request sector %lx nr %x xfered %x",
			(unsigned long)blk_rq_pos(req),
			(int)blk_rq_sectors(req),
			sectors_xferred * hardsect_size);

		spin_lock_irq(rq->queue_lock);
		__blk_end_request(req, 0, sectors_xferred*hardsect_size);
		spin_unlock_irq(rq->queue_lock);
	}
}



/*
 * The ioctl() implementation
 */
int hx_ioctl (struct block_device *bdev, fmode_t mode,
                 unsigned int cmd, unsigned long arg)
{
	struct hx_dev *dev = bdev->bd_disk->private_data;
	mutex_lock(&dev->lock);

	/*
	 * shall we doing something here?
	 */

	mutex_unlock(&dev->lock);

	PKL("ioctl cmd %x", cmd);

	return -ENOTTY; /* unknown command */
}


/*
 * The device operations structure.
 */
static struct block_device_operations hx_ops = {
	.owner           = THIS_MODULE,
	.open 	         = hx_open,
	.release 	 = hx_release,
	.ioctl	         = hx_ioctl,
};

static void clean_up(struct hx_dev *dev)
{
	if (dev->gd) {
		del_gendisk(dev->gd);
		put_disk(dev->gd);
	}

	if (dev->queue) {
		blk_cleanup_queue(dev->queue);
	}

	if (dev->data){
		vfree(dev->data);
	}

	if (dev->wq){
		destroy_workqueue(dev->wq);
	}
}


/*
 * Set up our internal device.
 */
static int setup_device(struct hx_dev *dev)
{
	int ret = 0;

	/*
	 * Alloc memory for storage
	 */
	memset (dev, 0, sizeof (struct hx_dev));
	dev->size = nsectors*hardsect_size;
	dev->data = vmalloc(dev->size);
	if (dev->data == NULL) {
		PKL("vmalloc failure.");
		ret = -ENOMEM;
		goto END;
	}

	/*
	 * Initialize the work queue
	 */
	dev->wq = create_workqueue("hellx_wq");
	if (dev->wq == NULL) {
		PKL("Work queue creation failure.");
		ret = -ENOMEM;
		goto END;
	}
	INIT_WORK(&dev->work, hx_work);

	/*
	 * Init the lock used by request queue
	 */
	mutex_init(&dev->lock);
	spin_lock_init(&dev->rq_lock);

	dev->queue = blk_init_queue(hx_request, &dev->rq_lock);
	if (dev->queue == NULL) {
		ret = -ENOMEM;
		goto END;
	}

	blk_queue_logical_block_size(dev->queue, hardsect_size);
	dev->queue->queuedata = dev;

	/*
	 * And the gendisk structure.
	 */
	dev->gd = alloc_disk(1);
	if (!dev->gd) {
		PKL("alloc_disk failure");
		ret = -ENOMEM;
		goto END;
	}
	dev->gd->major = hx_major;
	dev->gd->first_minor = 0;
	dev->gd->fops = &hx_ops;
	dev->gd->queue = dev->queue;
	dev->gd->private_data = dev;
	snprintf (dev->gd->disk_name, 32, "hellx");
	set_capacity(dev->gd, nsectors*(hardsect_size/KERNEL_SECTOR_SIZE));
	PKL("capacity %x", nsectors);
	add_disk(dev->gd);

	ret = 0;

  END:
  	if (ret != 0 ) {
		clean_up(dev);
	}

	return ret;
}


static int __init hx_init(void)
{
	/*
	 * Get registered.
	 */
	hx_major = register_blkdev(hx_major, "hx");
	if (hx_major <= 0) {
		PKL("blk device registration failed\n");
		return -EBUSY;
	}
	/*
	 * Allocate the device array, and initialize each one.
	 */
	Device = kmalloc(sizeof (struct hx_dev), GFP_KERNEL);
	if (Device == NULL)
		goto out_unregister;

	setup_device(Device);
        PKL("init");

	return 0;

  out_unregister:
	unregister_blkdev(hx_major, "hx");
	return -ENOMEM;
}

static void hx_exit(void)
{
	struct hx_dev *dev = Device;

	clean_up(dev);
	unregister_blkdev(hx_major, "hx");
	kfree(Device);
	PKL("exit");
}

module_init(hx_init);
module_exit(hx_exit);
