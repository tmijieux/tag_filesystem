/*  
 *  tfs.c - Entry point and setup for the tag filesystem
 */
#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */

int __init tfs_init(void)
{
        printk(KERN_INFO "tag filesystem 'tfs' starting.\n");

        /* 
         * A non 0 return means init_module failed; module can't be loaded. 
         */
        return 0;
}

void __exit tfs_exit(void)
{
        printk(KERN_INFO "tag filesystem 'tfs' exiting.\n");
}


module_init(tfs_init);
module_exit(tfs_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Thomas Mijieux");
MODULE_DESCRIPTION("a basic tag-driven vfs");

MODULE_SUPPORTED_DEVICE("n/a");
