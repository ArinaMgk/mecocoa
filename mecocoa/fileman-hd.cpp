// ASCII g++ TAB4 LF
// Attribute: 
// AllAuthor: @ArinaMgk
// ModuTitle: [Service] File Manage - ELF32-C++ x86 Bare-Metal
// Copyright: Dosconio Mecocoa, BSD 3-Clause License
#define _STYLE_RUST
#undef _DEBUG
#define _DEBUG

#include <c/consio.h>
#include <c/storage/harddisk.h>


use crate uni;
#ifdef _ARC_x86 // x86:
#include "../include/atx-x86-flap32.hpp"
#include "cpp/Device/Storage/HD-DEPEND.h"


Harddisk_PATA* disks[2];// referenced
static char hdd_buf[byteof(**disks) * numsof(disks)];

struct HD_Info {
    Slice primary[NR_PRIM_PER_DRIVE];
    Slice logical[NR_SUB_PER_DRIVE];
};
static HD_Info hd_info[4] = { 0 };//{TEMP} 0:0 0:1 1:0 1:1
static bool hd_info_valid[4] = { 0 };
static char* single_sector = NULL;
bool task_run = false;

void fileman_hd() {
	for0a(i, disks) {
		disks[i] = new (hdd_buf + i * byteof(**disks)) Harddisk_PATA(i);
	}
	disks[0]->setInterrupt(NULL);
	single_sector = (char*)malloc(0x1000);
}


static byte lock = 1;


#define	MAKE_DEVICE_REG(lba,drv,lba_highest) (((lba) << 6) | ((drv) << 4) | ((lba_highest) & 0xF) | 0xA0)


void Handint_HDD()// HDD Master
{
	innpb(REG_STATUS);
	if (lock) return;
	lock = 1;
		rupt_proc(Task_Hdd_Serv, IRQ_ATA_DISK0);
}

static bool hd_cmd_wait() {
	return waitfor(STATUS_BSY, 0, HD_TIMEOUT / 1000);
}

static void hd_int_wait() {
	CommMsg msg;
	syscall(syscall_t::COMM, 0b10, INTRUPT, &msg);
}

namespace uni {
	bool Harddisk_PATA::Read(stduint BlockIden, void* Dest) {
		if (task_run) {
			usize sect_nr = BlockIden;
			HdiskCommand cmd;
			cmd.feature = 0;
			cmd.count = 1;// number of sectors
			for0(i, 3) cmd.LBA[i] = (sect_nr >> (i * 8));
			cmd.device = MAKE_DEVICE_REG(1, getLowID(), (sect_nr >> 24) & 0xF);
			cmd.command = ATA_READ;
			lock = 0;
			Harddisk_PATA::Hdisk_OUT(&cmd, hd_cmd_wait);
			if (true) {
				hd_int_wait();
				if (!waitfor(STATUS_DRQ, STATUS_DRQ, HD_TIMEOUT)) {
					return false;
				}
				IN_wn(REG_DATA, (word*)Dest, Block_Size);
				// slice.length--;
			}
		}
		else {
			stduint C, B;
			__asm volatile("mov %%ecx, %0" : "=r" (C));// will break GNU stack judge: __asm ("push %ecx");
			__asm volatile("mov %%ebx, %0" : "=r" (B));// will break GNU stack judge: __asm ("push %ebx");
			__asm volatile("mov %0, %%ebx" : : "r" _IMM(Dest));// gcc use mov %eax->%ebx to assign
			__asm volatile("mov %0, %%eax": : "r" (BlockIden));
			__asm volatile("mov $1, %ecx");
			__asm volatile("call HdiskLBA28Load");
			__asm volatile("mov %0, %%ebx" : : "r" (B));// rather __asm ("pop %ebx");
			__asm volatile("mov %0, %%ecx" : : "r" (C));// rather __asm ("pop %ecx");
		}
		return true;
	}
	bool Harddisk_PATA::Write(stduint BlockIden, const void* Sors) {
		if (task_run) {
			usize sect_nr = BlockIden;
			HdiskCommand cmd;
			cmd.feature = 0;
			cmd.count = 1;// number of sectors
			for0(i, 3) cmd.LBA[i] = (sect_nr >> (i * 8));
			cmd.device = MAKE_DEVICE_REG(1, getLowID(), (sect_nr >> 24) & 0xF);
			cmd.command = ATA_WRITE;
			lock = 0;
			Harddisk_PATA::Hdisk_OUT(&cmd, hd_cmd_wait);
			if (true) {
				if (!waitfor(STATUS_DRQ, STATUS_DRQ, HD_TIMEOUT)) {
					return false;
				}
				OUT_wn(REG_DATA, (word*)Sors, Block_Size);
				hd_int_wait();
				// slice.length--;
			}
		}
		else return false;
		return true;
	}
}


struct iden_info_ascii {
	int idx;
	int len;
	rostr desc;
} iinfo[] = {
	{10, 20, "HD SN"}, // Serial number in ASCII
	{27, 40, "Model"} // Model number in ASCII
};

#define	DRV_OF_DEV(dev) (dev <= MAX_PRIM ? \
	dev / NR_PRIM_PER_DRIVE : \
	(dev - MINOR_hd1a) / NR_SUB_PER_DRIVE)


static void hd_read(Harddisk_PATA& hd, Slice slice, stduint pg_task = nil);
static void get_partition_table(Harddisk_PATA& drv, unsigned partable_sectposi, PartitionTableX86* pt)
{
	drv.Read(partable_sectposi, single_sector);
	MemCopyN(pt, single_sector + PARTITION_TABLE_OFFSET, sizeof(*pt) * NR_PART_PER_DRIVE);
}

#define NO_PART 0x00
#define EX_PART 0x05
#define ORANGES_PART 0x99// use its method

void partition(unsigned device, bool primary_but_logical = true) {
	unsigned drive = DRV_OF_DEV(device);
	HD_Info& hdi = hd_info[drive];
	PartitionTableX86* part_tbl = (PartitionTableX86*)single_sector;
	if (primary_but_logical) {
		get_partition_table(*disks[drive], 0, part_tbl);
		int nr_prim_parts = 0;
		for0 (i, NR_PART_PER_DRIVE) {
			if (part_tbl[i].type == NO_PART) 
				continue;
			nr_prim_parts++;
			hdi.primary[i + 1].address = part_tbl[i].lba_start;
			hdi.primary[i + 1].length = part_tbl[i].lba_count;
			if (part_tbl[i].type == EX_PART)
				partition(device + i + 1, false);
		}
		// assert(nr_prim_parts != 0);
	}
	else {
		int j = device % NR_PRIM_PER_DRIVE; /* 1~4 */
		int ext_start_sect = hdi.primary[j].address;
		int s = ext_start_sect;
		int nr_1st_sub = (j - 1) * NR_SUB_PER_PART; /* 0/16/32/48 */
		for0 (i, NR_SUB_PER_PART) {
			int dev_nr = nr_1st_sub + i;/* 0~15/16~31/32~47/48~63 */
			get_partition_table(*disks[drive], s, part_tbl);
			hdi.logical[dev_nr].address = s + part_tbl[0].lba_start;
			hdi.logical[dev_nr].length = part_tbl[0].lba_count;
			s = ext_start_sect + part_tbl[1].lba_start;
			if (part_tbl[1].type == NO_PART) {
				// ploginfo("end loop at %u", i);
				break;
			}
		}
	}
}


static void print_identify_info(uint16* hdinfo, const Harddisk_PATA& hd)
{
	int i, k;
	char s[64];

	for0a(k, iinfo) {
		char* p = (char*)&hdinfo[iinfo[k].idx];
		for (i = 0; i < iinfo[k].len / 2; i++) {
			s[i * 2 + 1] = *p++;
			s[i * 2] = *p++;
		}
		s[i * 2] = 0;
		if (_TEMP false) outsfmt("[Hrddisk] %s: %s\n\r", iinfo[k].desc, s);
	}

	int capabilities = hdinfo[49];
	int cmd_set_supported = hdinfo[83];
	if (_TEMP false) outsfmt("[Hrddisk] LBA  : %s\n\r",
		(capabilities & 0x0200) ? (cmd_set_supported & 0x0400) ? "Supported LBA48" : "Supported" : "No");

	int sectors = ((int)hdinfo[61] << 16) + hdinfo[60];
	outsfmt("[Hrddisk] Size : %lf MB\n\r", (double)sectors* hd.getUnits() / 1024 / 1024);// care for #NM FPU Loss

	hd_info[hd.getHigID() * 2 + hd.getLowID()].primary[0].address = nil;
	hd_info[hd.getHigID() * 2 + hd.getLowID()].primary[0].length = sectors;
}

void print_hdinfo(Harddisk_PATA& hd)
{
	HD_Info& hdinfo = hd_info[hd.getHigID() * 2 + hd.getLowID()];
	Console.OutFormat("device LBA\n\r");
	for0(i, NR_PART_PER_DRIVE + 1) {
		if (hdinfo.primary[i].length) {
			Console.OutFormat("%u      %u..%u\n\r", i,
				hdinfo.primary[i].address,
				hdinfo.primary[i].address + hdinfo.primary[i].length
			);
		}
		if (i) for0(j, NR_SUB_PER_PART) {
			unsigned jj = j + NR_SUB_PER_PART * (i - 1);
			if (hdinfo.logical[jj].length) {
				Console.OutFormat("%u      %u..%u\n\r", jj,
					hdinfo.logical[jj].address,
					hdinfo.logical[jj].address + hdinfo.logical[jj].length
				);
			}
		}
	}
}

static void hd_open(Harddisk_PATA& hd) { // 0x00
	HdiskCommand cmd;
	cmd.command = ATA_IDENTIFY;
	cmd.device = MAKE_DEVICE_REG(0, hd.getLowID(), 0);
	lock = 0;
	Harddisk_PATA::Hdisk_OUT(&cmd, hd_cmd_wait);
	hd_int_wait();
	IN_wn(REG_DATA, (word*)single_sector, hd.Block_Size);
	print_identify_info((uint16*)single_sector, hd);
	if (!hd_info_valid[hd.getHigID() * 2 + hd.getLowID()]) {
		if (_TEMP hd.getID() == 0x01) {
			partition(hd.getID() * (NR_PART_PER_DRIVE + 1));
			if (0) print_hdinfo(hd);
		}
		hd_info_valid[hd.getHigID() * 2 + hd.getLowID()] = true;
	}
}

static void hd_close(Harddisk_PATA& hd) { // 0x02
	hd_info_valid[hd.getHigID() * 2 + hd.getLowID()] = false;
}

static void hd_read(Harddisk_PATA& hd, Slice slice, stduint pg_task)// 0x03
{
	usize sect_nr = slice.address;
	HdiskCommand cmd;
	cmd.feature = 0;
	cmd.count = slice.length;// number of sectors
	for0(i, 3) cmd.LBA[i] = (sect_nr >> (i * 8));
	cmd.device	= MAKE_DEVICE_REG(1, hd.getLowID(), (sect_nr >> 24) & 0xF);
	cmd.command = ATA_READ;
	lock = 0;
	Harddisk_PATA::Hdisk_OUT(&cmd, hd_cmd_wait);

	CommMsg msg;
	msg.type = 0;
	msg.src = Task_Hdd_Serv;
	msg.data.address = _IMM(single_sector);
	msg.data.length = hd.Block_Size;

	while (slice.length) {
		hd_int_wait();
		if (!waitfor(STATUS_DRQ, STATUS_DRQ, HD_TIMEOUT)) {
			plogerro("hd writing error.");
			return;
		}
		IN_wn(REG_DATA, (word*)single_sector, hd.Block_Size);
		if (pg_task) syscall(syscall_t::COMM, COMM_SEND, pg_task, &msg);
		// syssend(pg_task, single_sector, hd.Block_Size, 0);
		slice.length--;
		// dst += hd.Block_Size;
	}
}

static void hd_write(Harddisk_PATA& hd, Slice slice, stduint pg_task)
{
	usize sect_nr = slice.address;
	HdiskCommand cmd;
	cmd.feature = 0;
	cmd.count = slice.length;// number of sectors
	for0(i, 3) cmd.LBA[i] = (sect_nr >> (i * 8));
	cmd.device	= MAKE_DEVICE_REG(1, hd.getLowID(), (sect_nr >> 24) & 0xF);
	cmd.command = ATA_WRITE;
	lock = 0;
	Harddisk_PATA::Hdisk_OUT(&cmd, hd_cmd_wait);

	CommMsg msg;
	msg.type = 0;
	msg.src = Task_Hdd_Serv;
	msg.data.address = _IMM(single_sector);
	msg.data.length = hd.Block_Size;
	
	while (slice.length) {
		if (!waitfor(STATUS_DRQ, STATUS_DRQ, HD_TIMEOUT)) {
			plogerro("hd writing error.");
			return;
		}
		if (pg_task) syscall(syscall_t::COMM, COMM_RECV, pg_task, &msg);
		// sysrecv(pg_task, single_sector, hd.Block_Size);
		OUT_wn(REG_DATA, (word*)single_sector, hd.Block_Size);
		hd_int_wait();
		slice.length--;
		// src += hd.Block_Size;
		// ploginfo("%s OK", __FUNCIDEN__);
	}
}

static void GetPartitionSlice(Harddisk_PATA& hd, unsigned device, stduint pg_task)
{
	//{} hd_info_valid
	Slice* retp;
	HD_Info& hdinfo = hd_info[hd.getHigID() * 2 + hd.getLowID()];
	retp = device < NR_PRIM_PER_DRIVE ?
	&hdinfo.primary[device % NR_PRIM_PER_DRIVE] :
	&hdinfo.logical[(device - NR_PRIM_PER_DRIVE) % NR_SUB_PER_DRIVE];
	// if (retp->length && device >= NR_PRIM_PER_DRIVE)
	// 	ploginfo("[Hrddisk] GetPartitionSlice(%u -> log[%u])", device, (device - NR_PRIM_PER_DRIVE) % NR_SUB_PER_DRIVE);
	Slice ret = *retp;
	CommMsg msg{
		.data = {.address = _IMM(&ret), .length = byteof(ret) },
		// .type = 0, .src = Task_Hdd_Serv
	};
	syscall(syscall_t::COMM, COMM_SEND, pg_task, &msg);
	// ploginfo("[Hrddisk] GetPartitionSlice %u %u", ret.address, ret.length);
}

static stduint args[4];
void serv_dev_hd_loop()
{
	task_run = true;
	if (_IMM(&bda->hdisk_number) != 0x475) {
		plogerro("Invalid BIOS_DataArea");
		while (1);
	}
	
	Console.OutFormat("[Hrddisk] detect %u disks\n\r", bda->hdisk_number);
	struct CommMsg msg;
	msg.data.address = _IMM(args);
	msg.data.length = sizeof(args);
	Slice slice;
	slice.length = 1;
	while (true) {
		syscall(syscall_t::COMM, COMM_RECV, 0, &msg);
		switch (msg.type)
		{
		case 0:// TEST (no-feedback)
			for0(i, bda->hdisk_number) {
				Console.OutFormat("[Hrddisk] %u:%u:\n\r", i / MAX_DRIVES, i % MAX_DRIVES);
				hd_open(*disks[i]);
			}
			break;
		case 1:// HARDRUPT (usercall-forbidden)
			break;
		case 2:// CLOSE[diskno]
			ploginfo("[Hrddisk] close %u", (unsigned)(byte)args[0]);
			hd_close(*disks[(byte)args[0]]);// assert msg.data.length == 0
			break;
		case 3:// READ[diskno, lba]
			slice.address = (usize)args[1];
			// ploginfo("[Hrddisk] device %u: read %u", (unsigned)(byte)args[0], slice.address);
			hd_read(*disks[(byte)args[0]], slice, msg.src);
			break;
		case 4:// WRITE[diskno, lba]
			slice.address = (usize)args[1];
			// ploginfo("[Hrddisk] device %u: write %u", (unsigned)(byte)args[0], slice.address);
			hd_write(*disks[(byte)args[0]], slice, msg.src);
			break;
		case 5:// GetPartitionSlice aka geometry (device 0 for all, 1~4 for primary, 5+ for logical)
			GetPartitionSlice(*disks[(byte)args[0]], args[1], msg.src);
			break;


		default:
			plogerro("Bad TYPE in %s %s", __FILE__, __FUNCIDEN__);
			break;
		}
	}
}

#endif
