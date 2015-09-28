#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/string.h>
#include <linux/init.h>

MODULE_LICENSE("GPL");
static char *Version = "$Revision: 1.9 $";


/* LDD是"Linux Device Driver"的简写，下面的代码基本上都来自这本书 */

/* LDD总线 */
/* 
  该总线的比较函数，比较该总线上的某个驱动是否和某个设备匹配，这里只简单匹配
  驱动的名字是否和设备的名字有相关性，例如，驱动:sculld, 设备:sculld0
*/
static int ldd_match(struct device *dev, struct device_driver *driver)
{
	return !strncmp(dev->kobj.name, driver->name, strlen(driver->name));
}
/* 定义总线类型，类比: PCI总线 */
struct bus_type ldd_bus_type = {
	.name = "ldd",
	.match = ldd_match,
};

/* 定义一个总线属性: version */
static ssize_t show_bus_version(struct bus_type *bus, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", Version);
}
static BUS_ATTR(version, S_IRUGO, show_bus_version, NULL);

/* 一个实际的总线是一个设备，下面定义一个实际的总线 */
static void ldd_bus_release(struct device *dev)
{
	printk(KERN_DEBUG "a ldd bus has been released\n");
}

/* 一个总线实例 */
struct device ldd_bus = {
	.init_name = "ldd0",
	.release = ldd_bus_release,
};


 /* LDD驱动 */
 /* 定义一个LDD驱动，类比: PCI驱动 */
 struct ldd_driver {
	char *version;
	struct module *module;
	struct device_driver driver;
 };

 /* 一个驱动实例: sculld*/
 static struct ldd_driver sculld_driver = {
	.version = "$Reversion: 1.21 $",
	.module = THIS_MODULE,
	.driver = {
		.name = "sculld",
	},
 };

/* 定义该驱动的一个属性: version */
#define to_ldd_driver(drv) container_of(drv, struct ldd_driver, driver)

static ssize_t show_version(struct device_driver *driver, char *buf)
{
	struct ldd_driver *ldriver = to_ldd_driver(driver);

	snprintf(buf, PAGE_SIZE, "%s\n", ldriver->version);
	return strlen(buf);
}
static DRIVER_ATTR(version, S_IRUGO, show_version, NULL);

/* 注册一个ldd驱动 */
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

/* 注销一个LDD驱动 */
void unregister_ldd_driver(struct ldd_driver *ldriver)
{
	driver_unregister(&ldriver->driver);
}

/* LDD设备 */
/* 定义该总线类型上一个设备，类比: 一个PCI设备 */
struct ldd_device {
	char *name;
	struct ldd_driver *ldriver;
	struct device dev;
};

/* 
  定义该总线上的一个具体设备，类比: 一个PCI网卡设备，正常情况下这个
  结构体可能会很大，因为业务相关的具体内容都在这里，现在为了简单，只
  有一个名字
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

/* 一个设备实例 */
struct sculld_dev sculld0 = 
{
	.dev_name = "sculld0",
};

/* 定义该设备的一个属性: display */
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

/* 注册一个ldd设备 */
int register_ldd_device(struct ldd_device *ldev)
{
	ldev->dev.bus = &ldd_bus_type;
	ldev->dev.parent = &ldd_bus;
	ldev->dev.release = ldd_dev_release;
	dev_set_name(&ldev->dev, ldev->name);
	return device_register(&ldev->dev);
}

/* 注销一个ldd设备 */
void unregister_ldd_device(struct ldd_device *ldev)
{
	device_unregister(&ldev->dev);
}

/* 注册一个sculld设备 */
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

/* 注销一个sculld设备 */
static void unregister_sculld_device(struct sculld_dev *sculld_device)
{
	unregister_ldd_device(&sculld_device->ldev);
}

/* 模块初始化 */
int init_module(void)
{
	int ret = 0;

	/* 注册总线类型: ldd */
	ret = bus_register(&ldd_bus_type);
	if (ret) {
		printk(KERN_ERR "Unable to register bus type ldd\n");
		return ret;
	}

	/* 创建该类型总线的属性: version */
	ret = bus_create_file(&ldd_bus_type, &bus_attr_version);
	if (ret) {
		printk(KERN_ERR "Unable to create version attribute\n");
		return ret;
	}

	/* 创建一个实际的总线: ldd0 */
	ret = device_register(&ldd_bus);
	if (ret) {
		printk(KERN_ERR "Unable to register bus device ldd0\n");
		return ret;
	}

	/* 注册一个ldd设备驱动: sculld */
	ret = register_ldd_driver(&sculld_driver);
	if (ret) {
		printk(KERN_ERR "Unable to register sculld driver\n");
		return ret;
	}

	/* 创建一个具体的ldd设备: sculld0 */
	ret = register_sculld_device(&sculld0);
	if (ret) {
		printk(KERN_ERR "Unable to register sculld0\n");
		return ret;
	}

	return ret;
}

/* 模块卸载 */
void cleanup_module(void)
{
	/* 设备 */
	unregister_sculld_device(&sculld0);

	/* 驱动 */
	unregister_ldd_driver(&sculld_driver);

	/* 总线 */
	device_unregister(&ldd_bus);
	bus_unregister(&ldd_bus_type);
}

