#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/string.h>
#include <linux/init.h>

MODULE_LICENSE("GPL");
static char *Version = "$Revision: 1.9 $";


/* LDD��"Linux Device Driver"�ļ�д������Ĵ�������϶������Ȿ�� */

/* LDD���� */
/* 
  �����ߵıȽϺ������Ƚϸ������ϵ�ĳ�������Ƿ��ĳ���豸ƥ�䣬����ֻ��ƥ��
  �����������Ƿ���豸������������ԣ����磬����:sculld, �豸:sculld0
*/
static int ldd_match(struct device *dev, struct device_driver *driver)
{
	return !strncmp(dev->kobj.name, driver->name, strlen(driver->name));
}
/* �����������ͣ����: PCI���� */
struct bus_type ldd_bus_type = {
	.name = "ldd",
	.match = ldd_match,
};

/* ����һ����������: version */
static ssize_t show_bus_version(struct bus_type *bus, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", Version);
}
static BUS_ATTR(version, S_IRUGO, show_bus_version, NULL);

/* һ��ʵ�ʵ�������һ���豸�����涨��һ��ʵ�ʵ����� */
static void ldd_bus_release(struct device *dev)
{
	printk(KERN_DEBUG "a ldd bus has been released\n");
}

/* һ������ʵ�� */
struct device ldd_bus = {
	.init_name = "ldd0",
	.release = ldd_bus_release,
};


 /* LDD���� */
 /* ����һ��LDD���������: PCI���� */
 struct ldd_driver {
	char *version;
	struct module *module;
	struct device_driver driver;
 };

 /* һ������ʵ��: sculld*/
 static struct ldd_driver sculld_driver = {
	.version = "$Reversion: 1.21 $",
	.module = THIS_MODULE,
	.driver = {
		.name = "sculld",
	},
 };

/* �����������һ������: version */
#define to_ldd_driver(drv) container_of(drv, struct ldd_driver, driver)

static ssize_t show_version(struct device_driver *driver, char *buf)
{
	struct ldd_driver *ldriver = to_ldd_driver(driver);

	snprintf(buf, PAGE_SIZE, "%s\n", ldriver->version);
	return strlen(buf);
}
static DRIVER_ATTR(version, S_IRUGO, show_version, NULL);

/* ע��һ��ldd���� */
int register_ldd_driver(struct ldd_driver *ldriver)
{
	int ret = 0;

	ldriver->driver.bus = &ldd_bus_type;

	ret = driver_register(&ldriver->driver);
	if (ret) {
		return ret;
	}

	return driver_create_file(&ldriver->driver, &driver_attr_version);
}

/* ע��һ��LDD���� */
void unregister_ldd_driver(struct ldd_driver *ldriver)
{
	driver_unregister(&ldriver->driver);
}

/* LDD�豸 */
/* ���������������һ���豸�����: һ��PCI�豸 */
struct ldd_device {
	char *name;
	struct ldd_driver *ldriver;
	struct device dev;
};

/* 
  ����������ϵ�һ�������豸�����: һ��PCI�����豸��������������
  �ṹ����ܻ�ܴ���Ϊҵ����صľ������ݶ����������Ϊ�˼򵥣�ֻ
  ��һ������
*/
struct sculld_dev {
	char dev_name[20];
	struct ldd_device ldev;
#ifdef TRUE_DEVICE	
	struct sculld_dev	*next;
	int			vmas;
	int			order;
	int			qset;
	size_t			size;
	struct semaphore	sem;
	struct cdev		cdev;
	void			**data;
#endif	
};

/* һ���豸ʵ�� */
struct sculld_dev sculld0 = 
{
	.dev_name = "sculld0",
};

/* ������豸��һ������: display */
#define to_ldd_device(device) container_of(device, struct ldd_device, dev)

static ssize_t sculld_display(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct ldd_device *ldevice = to_ldd_device(dev);

	snprintf(buf, PAGE_SIZE, "hello, this is %s\n", ldevice->name);
	return strlen(buf);
}
static DEVICE_ATTR(display, S_IRUGO, sculld_display, NULL);

static void ldd_dev_release(struct device *dev)
{}

/* ע��һ��ldd�豸 */
int register_ldd_device(struct ldd_device *ldev)
{
	ldev->dev.bus = &ldd_bus_type;
	ldev->dev.parent = &ldd_bus;
	ldev->dev.release = ldd_dev_release;
	dev_set_name(&ldev->dev, ldev->name);
	return device_register(&ldev->dev);
}

/* ע��һ��ldd�豸 */
void unregister_ldd_device(struct ldd_device *ldev)
{
	device_unregister(&ldev->dev);
}

/* ע��һ��sculld�豸 */
static int register_sculld_device(struct sculld_dev *sculld_device)
{
	int ret;

	sculld_device->ldev.name = sculld_device->dev_name;
	sculld_device->ldev.ldriver = &sculld_driver;

	ret = register_ldd_device(&sculld_device->ldev);
	if (ret) {
		return ret;
	}

	ret = device_create_file(&sculld_device->ldev.dev, &dev_attr_display);
	if (ret) {
		return ret;
	}

	return ret;
}

/* ע��һ��sculld�豸 */
static void unregister_sculld_device(struct sculld_dev *sculld_device)
{
	unregister_ldd_device(&sculld_device->ldev);
}

/* ģ���ʼ�� */
int init_module(void)
{
	int ret = 0;

	/* ע����������: ldd */
	ret = bus_register(&ldd_bus_type);
	if (ret) {
		printk(KERN_ERR "Unable to register bus type ldd\n");
		return ret;
	}

	/* �������������ߵ�����: version */
	ret = bus_create_file(&ldd_bus_type, &bus_attr_version);
	if (ret) {
		printk(KERN_ERR "Unable to create version attribute\n");
		return ret;
	}

	/* ����һ��ʵ�ʵ�����: ldd0 */
	ret = device_register(&ldd_bus);
	if (ret) {
		printk(KERN_ERR "Unable to register bus device ldd0\n");
		return ret;
	}

	/* ע��һ��ldd�豸����: sculld */
	ret = register_ldd_driver(&sculld_driver);
	if (ret) {
		printk(KERN_ERR "Unable to register sculld driver\n");
		return ret;
	}

	/* ����һ�������ldd�豸: sculld0 */
	ret = register_sculld_device(&sculld0);
	if (ret) {
		printk(KERN_ERR "Unable to register sculld0\n");
		return ret;
	}

	return ret;
}

/* ģ��ж�� */
void cleanup_module(void)
{
	/* �豸 */
	unregister_sculld_device(&sculld0);

	/* ���� */
	unregister_ldd_driver(&sculld_driver);

	/* ���� */
	device_unregister(&ldd_bus);
	bus_unregister(&ldd_bus_type);
}

