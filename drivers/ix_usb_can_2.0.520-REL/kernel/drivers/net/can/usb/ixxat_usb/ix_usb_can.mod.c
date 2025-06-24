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
	{ 0x3495aea1, "usb_alloc_urb" },
	{ 0xca4c86e6, "usb_anchor_urb" },
	{ 0xe38dc5d, "usb_free_urb" },
	{ 0x60954ec1, "can_free_echo_skb" },
	{ 0x52c5c991, "__kmalloc_noprof" },
	{ 0x78eb8908, "can_put_echo_skb" },
	{ 0x8133b342, "consume_skb" },
	{ 0xb542635f, "can_get_echo_skb" },
	{ 0xd6ddecf3, "alloc_canfd_skb" },
	{ 0xecebd217, "rt_mutex_base_init" },
	{ 0x3c9eea37, "usb_register_driver" },
	{ 0x37a0cba, "kfree" },
	{ 0x838abbd4, "open_candev" },
	{ 0xfb4d8f7b, "netdev_err" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0x7510ee16, "dev_addr_mod" },
	{ 0x65487097, "__x86_indirect_thunk_rax" },
	{ 0x122c3a7e, "_printk" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x61a8bcf9, "alloc_can_skb" },
	{ 0xc690a605, "usb_kill_anchored_urbs" },
	{ 0x20ce51e7, "netif_device_detach" },
	{ 0x7f298afe, "unregister_candev" },
	{ 0x472b942f, "usb_submit_urb" },
	{ 0x29d5172d, "_dev_info" },
	{ 0xbd406005, "can_change_mtu" },
	{ 0x2064999, "can_dropped_invalid_skb" },
	{ 0x8122b572, "_dev_err" },
	{ 0xe923430, "free_candev" },
	{ 0x6bee6f5f, "sk_skb_reason_drop" },
	{ 0x983b66f, "alloc_candev_mqs" },
	{ 0x6047ede6, "can_fd_len2dlc" },
	{ 0x4c03a563, "random_kmalloc_seed" },
	{ 0xd4b5dbda, "netdev_printk" },
	{ 0xc74e311a, "usb_control_msg" },
	{ 0xf12d9387, "can_fd_dlc2len" },
	{ 0x5612c21b, "sysfs_create_group" },
	{ 0xd81e32d2, "usb_deregister" },
	{ 0x75ca79b5, "__fortify_panic" },
	{ 0x191b8713, "netif_tx_wake_queue" },
	{ 0xcdb04e45, "close_candev" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0xf73ec46f, "__init_waitqueue_head" },
	{ 0xe8c35af5, "netif_rx" },
	{ 0xcefc199b, "can_bus_off" },
	{ 0x59e74729, "usb_unanchor_urb" },
	{ 0x37af18f3, "rt_spin_unlock" },
	{ 0xc4a0ca38, "rt_spin_lock" },
	{ 0x15ba50a6, "jiffies" },
	{ 0xaa14de9, "sysfs_remove_group" },
	{ 0x3c3ff9fd, "sprintf" },
	{ 0x95603802, "__kmalloc_cache_noprof" },
	{ 0x56470118, "__warn_printk" },
	{ 0xe1cd60ee, "alloc_can_err_skb" },
	{ 0x3c5deaf4, "register_candev" },
	{ 0xf9a482f9, "msleep" },
	{ 0xc4f0da12, "ktime_get_with_offset" },
	{ 0xbc2fdd78, "kmalloc_caches" },
	{ 0x607cf71d, "netdev_info" },
	{ 0xe8786a6a, "module_layout" },
};

MODULE_INFO(depends, "can-dev");

MODULE_ALIAS("usb:v08D8p0008d*dc*dsc*dp*ic*isc*ip*in*");
MODULE_ALIAS("usb:v08D8p0009d*dc*dsc*dp*ic*isc*ip*in*");
MODULE_ALIAS("usb:v08D8p000Ad*dc*dsc*dp*ic*isc*ip*in*");
MODULE_ALIAS("usb:v08D8p000Bd*dc*dsc*dp*ic*isc*ip*in*");
MODULE_ALIAS("usb:v08D8p001Fd*dc*dsc*dp*ic*isc*ip*in*");
MODULE_ALIAS("usb:v08D8p0014d*dc*dsc*dp*ic*isc*ip*in*");
MODULE_ALIAS("usb:v08D8p0016d*dc*dsc*dp*ic*isc*ip*in*");
MODULE_ALIAS("usb:v08D8p0017d*dc*dsc*dp*ic*isc*ip*in*");
MODULE_ALIAS("usb:v08D8p001Bd*dc*dsc*dp*ic*isc*ip*in*");
MODULE_ALIAS("usb:v08D8p001Cd*dc*dsc*dp*ic*isc*ip*in*");
MODULE_ALIAS("usb:v08D8pFF12d*dc*dsc*dp*ic*isc*ip*in*");
MODULE_ALIAS("usb:v08D8pFF13d*dc*dsc*dp*ic*isc*ip*in*");

MODULE_INFO(srcversion, "17D724792B529134D49739E");
