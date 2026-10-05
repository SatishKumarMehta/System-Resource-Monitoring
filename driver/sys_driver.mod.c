#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x04784d05, "cdev_add" },
	{ 0x6bde9a2b, "class_create" },
	{ 0x3dff2093, "device_create" },
	{ 0x51379816, "class_destroy" },
	{ 0x7a26db4a, "cdev_del" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0xdc50aae2, "__ref_stack_chk_guard" },
	{ 0x6b10bee1, "_copy_to_user" },
	{ 0x13c49cc2, "_copy_from_user" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x40c7247c, "si_meminfo" },
	{ 0x15ba50a6, "jiffies" },
	{ 0x8d522714, "__rcu_read_lock" },
	{ 0x37cd5be8, "init_task" },
	{ 0x2469810f, "__rcu_read_unlock" },
	{ 0x656e4a6e, "snprintf" },
	{ 0x1c303cee, "validate_usercopy_range" },
	{ 0x88db9f48, "__check_object_size" },
	{ 0x56781631, "device_destroy" },
	{ 0x1f309c56, "default_llseek" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0x92997ed8, "_printk" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0xe37020e5, "cdev_init" },
	{ 0xfb93e521, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "6620398CE697C60DFA54AB8");
