#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = KBUILD_MODNAME,
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
 .arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x73a3c4ca, "module_layout" },
	{ 0x50eedeb8, "printk" },
	{ 0x37a0cba, "kfree" },
	{ 0xd9eb6dce, "class_destroy" },
	{ 0x572ba1cb, "device_destroy" },
	{ 0xc29b98e7, "cdev_del" },
	{ 0x7485e15e, "unregister_chrdev_region" },
	{ 0xe4faeb5c, "cdev_add" },
	{ 0x9bd79589, "kmem_cache_alloc_trace" },
	{ 0xff64f6b3, "kmalloc_caches" },
	{ 0x16f9b492, "lockdep_init_map" },
	{ 0x1324a5ce, "cdev_init" },
	{ 0x6077825e, "device_create" },
	{ 0x6f79c6de, "__class_create" },
	{ 0x29537c9e, "alloc_chrdev_region" },
	{ 0xb4390f9a, "mcount" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "1A4FEE4F5676FFCC6682B41");
