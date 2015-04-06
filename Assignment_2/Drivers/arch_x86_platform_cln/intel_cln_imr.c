/*
 * Copyright(c) 2013 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Contact Information:
 * Intel Corporation
 */
/*
 * Intel Clanton IMR driver
 *
 * IMR stand for Insolate Memory Region, supported by Clanton SoC.
 *
 * A total number of 8 IMRs have implemented by Clanton SoC,
 * Some IMRs might be already occupied by BIOS or Linux during
 * booting time.
 *
 * user can cat /proc/driver/imr/status for current imr usage report.
 *
 * To allocate imr, the input memory address format has to match the 1k algined
 * actual Physical address required by HW.
 *
 * The IMR alloc API will locate the next available IMR slot set up
 * with input memory region, then apply the input access right masks
 *
 * The IMR can be freed with the pre-allocated memory addresses.
 */

#include <asm-generic/uaccess.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/printk.h>
#include <linux/proc_fs.h>

#include "intel_cln_imr.h"
#include <asm/imr.h>

#define DRIVER_NAME	"intel-cln-imr"
#define IMR_PROC_DIR	"driver/imr"
#define IMR_PROC_FILE	"status"

#define IMR_READ_MASK	0x1
#define IMR_MAX_ID	7

#ifndef phys_to_virt
#define phys_to_virt __va
#endif

/* IMR HW register address structre */
struct cln_imr_reg_t {
	u8  imr_xl;   /* high address register */
	u8  imr_xh;   /* low address register */
	u8  imr_rm;   /* read mask register */
	u8  imr_wm;   /* write mask register */
} cln_imr_reg_t;

/**
 * struct cln_imr_addr_t
 *
 * IMR memory address structure
 */
struct cln_imr_addr_t {
	u32 addr_low;      /* low boundary memroy address */
	u32 addr_high;     /* high boundary memory address */
	u32 read_mask;     /* read access right mask */
	u32 write_mask;    /* write access right mask */
} cln_imr_addr_t;

/**
 * struct cln_imr_pack
 *
 * local IMR pack structure
 */
struct cln_imr_pack {
	bool occupied;       /* IMR occupied */
	bool locked;         /* IMR lock */
	struct cln_imr_reg_t reg;   /* predefined imr register address */
	struct cln_imr_addr_t addr; /* IMR address region structure */
	unsigned char info[MAX_INFO_SIZE]; /* IMR info */
} cln_imr_pack;


/* Predefined HW register address */
static struct cln_imr_reg_t imr_reg_value[] = {
	{ IMR0L, IMR0H, IMR0RM, IMR0WM },
	{ IMR1L, IMR1H, IMR1RM, IMR1WM },
	{ IMR2L, IMR2H, IMR2RM, IMR2WM },
	{ IMR3L, IMR3H, IMR3RM, IMR3WM },
	{ IMR4L, IMR4H, IMR4RM, IMR4WM },
	{ IMR5L, IMR5H, IMR5RM, IMR5WM },
	{ IMR6L, IMR6H, IMR6RM, IMR6WM },
	{ IMR7L, IMR7H, IMR7RM, IMR7WM }
};

/* proc directory */
struct proc_dir_entry *pdir;

/* proc file interface /proc/drivers/imr/stats */
struct proc_dir_entry *pstat;

/**
 * module parameter
 * IMR slot should repersant the available IMR region from
 * linux boot and BIOS.
 *
 * For example: imr_bit_mask = 0x10111001
 * occupied IMR: 0, 3, 4, 5, 7
 * un-occupied IMR: 1, 2, 6
 */
static int imr_bit_mask = 0xFF;
module_param(imr_bit_mask, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(imr_bit_mask, "IMR bit mask");

/**
 * module parameter
 * if IMR lock is a nozero value, all unlocked
 * imrs will be locked regardless the usage.
 */
static int imr_lock = 1;
module_param(imr_lock, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(imr_lock, "switch to lock unused IMR");

/* local IMR data structure */
struct cln_imr_pack local_imr[IMR_MAXID];

/**
 * intel_cln_imr_read_reg
 *
 * @param reg: register address
 * @return nothing
 *
 * return register value from input address.
 */
static inline uint32_t intel_cln_imr_read_reg(uint8_t reg)
{
	uint32_t temp = 0;

	intel_cln_sb_read_reg(SB_ID_ESRAM, CFG_READ_OPCODE, reg, &temp, 0);
	return temp;
}

/**
 * intel_cln_imr_update_local_data
 *
 * @return nothing
 *
 * Populate IMR data structure from HW.
 */
static inline void intel_cln_imr_update_local_data(void)
{
	int i = 0;

	for (i = 0; i < IMR_MAXID; i++) {

		local_imr[i].addr.addr_low =
			intel_cln_imr_read_reg(imr_reg_value[i].imr_xl);
		local_imr[i].addr.addr_high =
			intel_cln_imr_read_reg(imr_reg_value[i].imr_xh);
		local_imr[i].addr.read_mask =
			intel_cln_imr_read_reg(imr_reg_value[i].imr_rm);
		local_imr[i].addr.write_mask =
			intel_cln_imr_read_reg(imr_reg_value[i].imr_wm);

		if (local_imr[i].addr.addr_low & IMR_LOCK_BIT)
			local_imr[i].locked = true;

		if (local_imr[i].addr.read_mask > 0 &&
			local_imr[i].addr.read_mask < IMR_READ_ENABLE_ALL){
			local_imr[i].occupied = true;
		} else {
			local_imr[i].occupied = false;
			memcpy(local_imr[i].info, "NOT USED", MAX_INFO_SIZE);
		}
	}
}

/**
 * prepare_input_addr
 *
 * @param addr: input physical memory address
 * @return formated memory address
 *
 * 1. verify input memory address alignment
 * 2. apply IMR_REG_MASK to match the format required by HW
 */
static inline uint32_t prepare_input_addr(uint32_t addr)
{
	if (addr & (IMR_MEM_ALIGN - 1))
		return 0;

	addr = (addr >> 8) & IMR_REG_MASK;
	return addr;
}

/**
 * intel_cln_imr_find_free_entry
 *
 * @return the next free imr slot
 */
int intel_cln_imr_find_free_entry(void)
{
	int i = 0;

	intel_cln_imr_update_local_data();

	for (i = 0; i < IMR_MAXID; i++) {
		if ((!local_imr[i].occupied) && (!local_imr[i].locked))
			return i;
	}

	pr_err("%s: No more free IMR available.\n", __func__);
	return -ENOMEM;
}


/**
 * intel_cln_remove_imr_entry
 *
 * @param id: imr slot id
 * @return nothing
 *
 * remove imr slot base on input id
 */
void intel_cln_remove_imr_entry(int id)
{
	intel_cln_sb_write_reg(SB_ID_ESRAM, CFG_WRITE_OPCODE,
				imr_reg_value[id].imr_rm, IMR_READ_ENABLE_ALL,
				0);
	intel_cln_sb_write_reg(SB_ID_ESRAM, CFG_WRITE_OPCODE,
				imr_reg_value[id].imr_wm, IMR_WRITE_ENABLE_ALL,
				0);
	intel_cln_sb_write_reg(SB_ID_ESRAM, CFG_WRITE_OPCODE,
				imr_reg_value[id].imr_xl, IMR_BASE_ADDR, 0);
	intel_cln_sb_write_reg(SB_ID_ESRAM, CFG_WRITE_OPCODE,
				imr_reg_value[id].imr_xh, IMR_BASE_ADDR, 0);
}

/**
 * intel_cln_imr_add_entry
 *
 * @param id: imr slot id
 * @param hi: hi memory address
 * @param lo: lo memory address
 * @param read: read access mask
 * @param write: write access mask
 * @return nothing
 *
 * Setup an IMR entry
 */
static void intel_cln_imr_add_entry(int id, uint32_t hi,
		uint32_t lo, uint32_t read, uint32_t write, bool lock)
{
	intel_cln_sb_write_reg(SB_ID_ESRAM, CFG_WRITE_OPCODE,
				imr_reg_value[id].imr_xl, lo, 0);
	intel_cln_sb_write_reg(SB_ID_ESRAM, CFG_WRITE_OPCODE,
				imr_reg_value[id].imr_xh, hi, 0);
	intel_cln_sb_write_reg(SB_ID_ESRAM, CFG_WRITE_OPCODE,
				imr_reg_value[id].imr_rm, read, 0);
	intel_cln_sb_write_reg(SB_ID_ESRAM, CFG_WRITE_OPCODE,
				imr_reg_value[id].imr_wm, write, 0);
	if (lock) {
		lo |= IMR_LOCK_BIT;
		intel_cln_sb_write_reg(SB_ID_ESRAM, CFG_WRITE_OPCODE,
					imr_reg_value[id].imr_xl, lo, 0);
	}
}

/**
 * intel_cln_imr_alloc
 *
 * @param high: high boundary of memory address
 * @param low: low boundary of memorry address
 * @param read: IMR read mask value
 * @param write: IMR write mask value
 * @return nothing
 *
 * setup the next available IMR with customized read and write masks
 */
int intel_cln_imr_alloc(uint32_t high, uint32_t low, uint32_t read,
			uint32_t write, unsigned char *info, bool lock)
{
	int id = 0;

	if (info == NULL)
		return -EINVAL;

	if ((low & IMR_LOCK_BIT) || (read == 0 || write == 0)) {
		pr_err("%s: Invalid acces mode\n", __func__);
		return -EINVAL;
	}

	/* Calculate aligned addresses and validate range */
	high = prepare_input_addr(high);
	low = prepare_input_addr(low);

	/* Find a free entry */
	id = intel_cln_imr_find_free_entry();
	if (id < 0)
		return -ENOMEM;

	/* Add entry - locking as necessary */
	intel_cln_imr_add_entry(id, high, low, (read & IMR_READ_ENABLE_ALL),
				write, lock);

	/* Name the new entry */
	memcpy(local_imr[id].info, info, MAX_INFO_SIZE);

	/* Update local data structures */
	intel_cln_imr_update_local_data();

	DBG("%s: allocate IMR %d %s\n", __func__, id,
		lock ? "locked" : "unlocked");

	return 0;
}
EXPORT_SYMBOL(intel_cln_imr_alloc);

/**
 * get_phy_addr
 * @return phy address value
 *
 * convert register format to physical address format.
 */
static uint32_t get_phy_addr(uint32_t reg_value)
{
	reg_value = ((reg_value & IMR_REG_MASK) << 8);
	return reg_value;
}

/**
 * intel_cln_imr_proc_read
 *
 * @param page: buffer to write data into
 * @param start: where the data has been written in the page
 * @param offset: same meaning as read system call
 * @param count: same meaning as read system call
 * @param eof: set if no more data needs to be returned
 * @return number of bytes successfully read
 *
 * Populates IMR state via /proc/driver/imr
 */
static int intel_cln_imr_proc_read(char *page, char **start, off_t off,
		int count, int *eof, void *data)
{
	int len = 0;
	int i = 0;
	int size;
	uint32_t hi_phy_addr, lo_phy_addr;

	/* get the latest imr settings */
	intel_cln_imr_update_local_data();

	for (i = 0; i < IMR_MAXID; i++) {

		/* read back the actual  input physical memory address */
		hi_phy_addr = get_phy_addr(local_imr[i].addr.addr_high);
		lo_phy_addr = get_phy_addr(local_imr[i].addr.addr_low);

		/* the IMR always protect extra 1k memory size above the input
		 * high reg value
		 */
		size = ((hi_phy_addr - lo_phy_addr) / IMR_MEM_ALIGN) + 1;

		len += snprintf(page+len, count,
				"imr - id : %d\n"
				"info     : %s\n"
				"occupied : %s\n"
				"locked   : %s\n"
				"size     : %d kb\n"
				"hi addr (phy): 0x%08x\n"
				"lo addr (phy): 0x%08x\n"
				"hi addr (vir): 0x%08x\n"
				"lo addr (vir): 0x%08x\n"
				"read mask  : 0x%08x\n"
				"write mask : 0x%08x\n\n",
				i,
				local_imr[i].info,
				local_imr[i].occupied ? "yes" : "no",
				local_imr[i].locked ? "yes" : "no",
				size,
				hi_phy_addr,
				lo_phy_addr,
				(uint32_t)phys_to_virt(hi_phy_addr),
				(uint32_t)phys_to_virt(lo_phy_addr),
				local_imr[i].addr.read_mask,
				local_imr[i].addr.write_mask);
	}
	*eof = 1;
	return len;
}

/**
 * intel_cln_imr_proc_read
 *
 * @param page: buffer to write data into
 * @param start: where the data has been written in the page
 * @param offset: same meaning as read system call
 * @param count: same meaning as read system call
 * @param eof: set if no more data needs to be returned
 * @return number of bytes successfully read
 *
 * Function allows user-space to operate IMRs through proc interface
 * e.g. "echo imr off 7 > /proc/drivers/imr/stats" to switch IMR 7 off
 * if its unlocked.
 */
static int intel_cln_imr_proc_write(struct file *file, const char __user *buf,
		unsigned long count, void *data)
{
	ssize_t ret = 0;
	int id = 0;
	char *cbuf = NULL;
	char *sbuf = NULL;
	char *cmd_id = NULL;
	char *cmd = NULL;

	if (count <= 1)
		return -EINVAL;

	/* Get input */
	sbuf = cbuf = kzalloc(count+1, GFP_KERNEL);
	if (cbuf == NULL)
		return -ENOMEM;

	ret = -EFAULT;
	if (copy_from_user(cbuf, (char *)buf, count))
		goto done;

	DBG("%s: input read: %s !\n", __func__, cbuf);
	/* Parse for format "entity on" "entity off" */
	while (sbuf < (cbuf + count)) {
		if (*sbuf == ' ') {
			*sbuf = 0;
			sbuf++;

			if (cmd_id && cmd)
				break;
			if (cmd == NULL)
				cmd = sbuf;
			else
				cmd_id = sbuf;
		}
		sbuf++;
	}

	ret = -EINVAL;
	if (cmd == NULL || cmd_id == NULL)
		goto done;

	if (strstr(cmd, "off"))
		DBG("try to remove IMR: %s.\n", cmd_id);
	else
		goto done;

	id = (int)(cmd_id[0]-'0');
	if (id < 0 || id > IMR_MAX_ID)
		goto done;

	/* get latest imr hardware settings */
	intel_cln_imr_update_local_data();

	if (local_imr[id].locked) {
		pr_err("%s: IMR locked !\n", __func__);
		goto done;
	}

	intel_cln_remove_imr_entry(id);
	intel_cln_imr_update_local_data();

	ret = 0;
done:
	if (ret == 0)
		ret = (ssize_t)count;

	kfree(cbuf);
	return ret;
}


/**
 * intel_cln_imr_free
 *
 * @param high: high boundary of memory address
 * @param low: low boundary of memorry address
 * @return nothing
 *
 * remove the imr based on input memory region
 */
int intel_cln_imr_free(uint32_t high, uint32_t low)
{
	int i = 0;

	if (low > high) {
		pr_err("%s: Invalid input address values.\n", __func__);
		return -EINVAL;
	}

	high = prepare_input_addr(high);
	if (!high) {
		pr_err("%s: Invalid input memory address.\n", __func__);
		return -EINVAL;
	}

	low = prepare_input_addr(low);
	if (!low) {
		pr_err("%s: Invalid input memory address.\n", __func__);
		return -EINVAL;
	}

	for (i = 0; i < IMR_MAXID; i++) {
		if (local_imr[i].occupied
			&& (local_imr[i].addr.addr_low == low)
			&& (local_imr[i].addr.addr_high == high)
			&& (!local_imr[i].locked)) {
				intel_cln_remove_imr_entry(i);
				intel_cln_imr_update_local_data();
				return 0;
			}
	}

	return -EINVAL;
}
EXPORT_SYMBOL(intel_cln_imr_free);

/**
 * intel_cln_imr_proc_remove
 *
 * @return nothing
 * Removes /proc entires
 */
static void intel_cln_imr_proc_remove(void)
{
	remove_proc_entry(IMR_PROC_FILE, pdir);
	remove_proc_entry(IMR_PROC_DIR, NULL);
}

/**
 * intel_cln_imr_proc_add
 *
 * @return nothing
 * add /proc/stat entry
 */
static int intel_cln_imr_proc_add(void)
{
	pdir = proc_mkdir(IMR_PROC_DIR, NULL);
	if (pdir == NULL) {
		pr_err("%s: PROC dir create failed.\n", __func__);
		return -ENOMEM;
	}

	pstat = create_proc_entry(IMR_PROC_FILE, 0, pdir);
	if (pstat == NULL) {
		pr_err("%s: PROC imr file create failed.\n", __func__);
		return -EIO;
	}
	pstat->read_proc = intel_cln_imr_proc_read;
	pstat->write_proc = intel_cln_imr_proc_write;
	pstat->data = NULL;

	return 0;
}

/**
 * intel_cln_imr_load_local_data
 *
 * @param mask: module parameter
 * @return nothing
 *
 * prepare local IMR data structure from input module parameter.
 */
static void intel_cln_imr_load_local_data(int mask)
{
	int i = 0;

	BUG_ON((mask > 255 || mask < 0));

	for (i = 0; i < IMR_MAXID; i++) {
		local_imr[i].addr.addr_low =
			intel_cln_imr_read_reg(imr_reg_value[i].imr_xl);

		/* mask bit 1 means imr occupied*/
		if (((mask>>i) & IMR_READ_MASK) == 0) {
			if (!(local_imr[i].addr.addr_low & IMR_LOCK_BIT))
				intel_cln_remove_imr_entry(i);
		}
	}

	intel_cln_imr_update_local_data();
}

/**
 * intel_cln_imr_init_data
 *
 * @return nothing
 * initialize local_imr data structure
 */
static void intel_cln_imr_init_data(void)
{
	int i = 0;

	intel_cln_imr_update_local_data();

	for (i = 0; i < IMR_MAXID; i++) {
		local_imr[i].reg = imr_reg_value[i];
		memcpy(local_imr[i].info, "System Reserved Region",
			MAX_INFO_SIZE);
	}
}

/**
 * intel_cln_imr_lockall
 *
 * @param mask: module parameter
 * @return nothing
 *
 * lock up all un-locked IMRs
 */
int intel_cln_imr_lockall(void)
{
	int i = 0;
	uint32_t temp_addr;

	/* Enumerate IMR data structures */
	intel_cln_imr_init_data();
	intel_cln_imr_load_local_data(imr_bit_mask);

	/* Nothing else to do */
	if (imr_lock == 0)
		return -EINVAL;

	/* Cycle through IMRs locking whichever are unlocked */
	for (i = 0; i < IMR_MAXID; i++) {

		temp_addr = local_imr[i].addr.addr_low;
		if (!(temp_addr & IMR_LOCK_BIT)) {

			DBG("%s: locking IMR %d\n", __func__, i);
			temp_addr |= IMR_LOCK_BIT;
			intel_cln_sb_write_reg(SB_ID_ESRAM, CFG_WRITE_OPCODE,
						local_imr[i].reg.imr_xl,
						temp_addr, 0);
		}
	}

	return 0;
}
EXPORT_SYMBOL(intel_cln_imr_lockall);

/**
 * intel_cln_imr_init
 *
 * @return 0 success < 0 failue
 *
 * module entry point
 */
static int  __init intel_cln_imr_probe(struct platform_device * pdev)
{
	int ret;

	/* initialise local imr data structure */
	intel_cln_imr_init_data();

	ret = intel_cln_imr_proc_add();
	BUG_ON(ret != 0);

	if(intel_cln_imr_runt_setparams() == 0){
                intel_cln_imr_lockall();
        }

	pr_info("IMR setup complete\n");

	return 0;
}

/**
 * intel_cln_imr_exit
 *
 * @return nothing
 * Module exit
 */
static int intel_cln_imr_remove(struct platform_device * pdev)
{
	/* this will disable all the unlocked IMRs */
	intel_cln_imr_proc_remove();

	return 0;
}

/*
 * Platform structures useful for interface to PM subsystem
 */
static struct platform_driver intel_cln_imr_driver = {
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
	},
	.probe = intel_cln_imr_probe,
	.remove = intel_cln_imr_remove,
};

module_platform_driver(intel_cln_imr_driver);

MODULE_DESCRIPTION("Intel Clanton SOC IMR API ");
MODULE_AUTHOR("Intel Corporation");
MODULE_LICENSE("Dual BSD/GPL");

