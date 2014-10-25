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
	{ 0x7485e15e, "unregister_chrdev_region" },
	{ 0xd9eb6dce, "class_destroy" },
	{ 0x572ba1cb, "device_destroy" },
	{ 0xc29b98e7, "cdev_del" },
	{ 0x37a0cba, "kfree" },
	{ 0xe4faeb5c, "cdev_add" },
	{ 0x9bd79589, "kmem_cache_alloc_trace" },
	{ 0xff64f6b3, "kmalloc_caches" },
	{ 0x1324a5ce, "cdev_init" },
	{ 0x89fb5210, "__init_waitqueue_head" },
	{ 0x6077825e, "device_create" },
	{ 0x6f79c6de, "__class_create" },
	{ 0x29537c9e, "alloc_chrdev_region" },
	{ 0x16f9b492, "lockdep_init_map" },
	{ 0x2f287f0d, "copy_to_user" },
	{ 0x2559c990, "finish_wait" },
	{ 0x4292364c, "schedule" },
	{ 0x28370321, "prepare_to_wait" },
	{ 0xc8b57c27, "autoremove_wake_function" },
	{ 0x29c2e190, "__wake_up" },
	{ 0x362ef408, "_copy_from_user" },
	{ 0x87e02037, "current_task" },
	{ 0x8d1fc7c9, "down_interruptible" },
	{ 0x8a124bae, "up" },
	{ 0x50eedeb8, "printk" },
	{ 0xb4390f9a, "mcount" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "824EB0D5B8E087AD4AB046E");
