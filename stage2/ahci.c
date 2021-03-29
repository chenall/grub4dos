
// save GCC options
#pragma GCC push_options
// turn off optimization
#pragma GCC optimize ("O0")

//------------------------------------------------------------------------------
//
// AHCI driver for GRUB
//
// Copyright (C) 2018 Kai Schtrom
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// Many thanks to OSDev AHCI Wiki, which explained the details of AHCI
// and helped with the source code. Also many thanks to Microsoft for
// their excellent StorAhci StorPort Miniport Driver sources.
//
//------------------------------------------------------------------------------

#include "shared.h"
#include "ahci.h"

#define printf grub_printf
#define malloc grub_malloc
#define free grub_free
#define strlen grub_strlen
#define strcpy grub_strcpy

// initialize structure before use
AHCI_GLOBALS ahcig = {0,0,0x80,-1,-1,0,0,0,0,0};


//------------------------------------------------------------------------------
// functions that have to be declared
//------------------------------------------------------------------------------
static BOOL P_Running_WaitOnFRE(AHCI_MEMORY_REGISTERS *abar,ULONG portno,AHCI_PORT_EXT *portExt);


//------------------------------------------------------------------------------
// save AHCI BIOS configuration
//------------------------------------------------------------------------------
static void SaveAhciBiosConfig(AHCI_HBA_EXT *HbaExt)
{
	memcpy(HbaExt->BiosConfig,(const void*)HbaExt->ABAR_Address,0x1100);
}


//------------------------------------------------------------------------------
// restore AHCI BIOS configuration
//------------------------------------------------------------------------------
static void RestoreAhciBiosConfig(AHCI_HBA_EXT *HbaExt)
{
	memcpy((void*)HbaExt->ABAR_Address,HbaExt->BiosConfig,0x1100);
}


//------------------------------------------------------------------------------
// read DWORD from port
//------------------------------------------------------------------------------
static int inpd(int port)
{
	int ret_val;
	__asm__ volatile ("inl %%dx,%%eax" : "=a" (ret_val) : "d"(port));
	return ret_val;
}


//------------------------------------------------------------------------------
// write DWORD to port
//------------------------------------------------------------------------------
static void outpd(int port, int val)
{
	__asm__ volatile ("outl %%eax,%%dx" : : "a"(val), "d"(port));
}


//------------------------------------------------------------------------------
// delay function
//------------------------------------------------------------------------------
static int delay(unsigned long int ms)
{
	// CPU frequency in kHz
	static unsigned long int kHz = 0;
	// do not use more than 50 ms here, otherwise we get an overflow in the
	// latch, use only values that 1000 can be evenly devided by, otherwise we
	// get rounding differences
	unsigned long int pitms = 50;
	// PIT (Programmable Interval Timer) tick rate is 1,193182 MHz
	unsigned long int latch = 1193182ul / (1000 / pitms);

	// try to calibrate the TSC against the Programmable Interrupt Timer and
	// return the frequency of the TSC in kHz
	// if kHz is zero we have to first calibrate the TSC
	// we also do a recalibrate if the specified milliseconds argument is zero
	unsigned long int tsclow = 0;
	unsigned long int tschigh = 0;
	unsigned long long int t1 = 0;
	unsigned long long int t2 = 0;
	if(kHz == 0 || ms == 0)
	{
		// CPUID supported if can set/clear ID flag (EFLAGS bit 21).
		int i = 0;
		__asm__ volatile
		(
			"pushfl\n\t"						// push original EFLAGS
			"popl %%edx\n\t"					// get original EFLAGS
			"pushfl\n\t"						// push original EFLAGS
			"popl %%eax\n\t"					// get original EFLAGS
			"xorl $0x200000,%%eax\n\t"			// flip ID bit in EFLAGS
			"pushl %%eax\n\t"					// save new EFLAGS value on stack
			"popfl\n\t"							// replace current EFLAGS value
			"pushfl\n\t"						// get new EFLAGS value
			"popl %%eax\n\t"					// store new EFLAGS in EAX
			"xorl %%edx,%%eax\n\t":"=a"(i)		// can not toggle ID bit, i = result
		);

		// CPUID is not supported
		if(i == 0)
		{
			return 1;
		}	
	
		// TSC (Time Stamp Counter) supported if CPUID func 1 returns with bit 4 of
		// EDX set.
		__asm__ volatile
		(
			"movl	$1,%%eax\n\t"				// call CPUID function 1
			"cpuid\n\t"							// call CPUID
			"andl	$0x10,%%edx\n\t":"=d"(i)	// check TSC bit 4, i = result
		);

		// TSC is not supported
		if(i == 0)
		{
			return 2;
		}	
	
		// set the Gate high, disable speaker
		outpd(0x61,(inpd(0x61) & ~0x02) | 0x01);
	
		// Program timer 2 for LSB then MSB, mode 0, binary.
		// - bit 7-6:  10 = timer 2
		// - bit 5-4:  11 = R/W LSB then MSB
		// - bit 3-1: 000 = single timeout
		// - bit   0:   0 = binary
		outpd(0x43,0xB0);
		// Load the starting value, LSB then MSB. This value will cause the timer
		// to time out after X cycles of it's 1193182 Hz clock.
		outpd(0x42,latch & 0xFF);
		outpd(0x42,latch >> 8);
		
		// serialize with cpuid instruction and
		// read 64 bit TSC (Time Stamp Counter)
		__asm__ volatile
		(
			"cpuid\n\t"
			"rdtsc\n\t":"=a"(tsclow),"=d"(tschigh)
		);

		// convert low and high values to 64 bit
		t1 = tschigh;
		t1 = t1 << 32;
		t1 = t1 | tsclow;
	
		// wait until the output bit (bit 5 at I/O port 61h) is set
		while((inpd(0x61) & 0x20) == 0)
		{
			// serialize with cpuid instruction and
			// read 64 bit TSC (Time Stamp Counter)
			__asm__ volatile
			(
				"cpuid\n\t"
				"rdtsc\n\t":"=a"(tsclow),"=d"(tschigh)
			);
			
			// convert low and high values to 64 bit
			t2 = tschigh;
			t2 = t2 << 32;
			t2 = t2 | tsclow;
		}
	
		// calculate CPU frequency in kHz
		kHz = ((unsigned long int)(t2 - t1) / pitms);
	}
	
	// if the specified milliseconds argument is zero the call was only done to
	// calibrate the TSC against the PIT
	if(ms == 0)
	{
		// leave delay function without doing a delay
		return 0;
	}

	// serialize with cpuid instruction and
	// read 64 bit TSC (Time Stamp Counter)
	__asm__ volatile
	(
		"cpuid\n\t"
		"rdtsc\n\t":"=a"(tsclow),"=d"(tschigh)
	);

	// convert low and high values to 64 bit
	t1 = tschigh;
	t1 = t1 << 32;
	t1 = t1 | tsclow;	

	// calculate end time
	// to use a microsecond sleep use the following code:
	// t1 += (unsigned long long int)(kHz / 1000) * ms;
	// to use a nanosecond sleep use the following code:
	// t1 += (unsigned long long int)(kHz / 1000000) * ms;
	t1 += (unsigned long long int)kHz * ms;

	// calculate actual time
	do
	{
		// serialize with cpuid instruction and
		// read 64 bit TSC (Time Stamp Counter)
		__asm__ volatile
		(
			"cpuid\n\t"
			"rdtsc\n\t":"=a"(tsclow),"=d"(tschigh)
		);
	
		// convert low and high values to 64 bit
		t2 = tschigh;
		t2 = t2 << 32;
		t2 = t2 | tsclow;
	}while(t1 > t2);

	return 0;
}


//------------------------------------------------------------------------------
// remove leading and trailing spaces from a string
//------------------------------------------------------------------------------
static int RemoveLeadingAndTrailingSpaces(char *string)
{
	unsigned int len = strlen(string);
	unsigned int i;

	// delete all spaces from the start of the string
	for(i = 0; i < len ; i++)
	{
		// if a space is found, increment the copy start position
		if(string[i] != 0x20)
		{
			strcpy(string,string+i);
			break;
		}
	}       
		
	// delete all spaces from the end of the string
	for(i = len; i > 0 ; i--)
	{
		// if a space is found, replace it with a terminating null
		if(string[i] == 0x20 || string[i] == 0x00)
		{
	    	string[i] = '\0';
		}
		else
		{
			break;
		}
	}       

	return 0;
}


//------------------------------------------------------------------------------
// swap bytes of unsigned short int (WORD)
//------------------------------------------------------------------------------
static void SwapBytes(unsigned char *buf,unsigned short int len)
{
	// do this for every word in the buffer
	unsigned short int i;
	for(i = 0; i < len; i += 2)
	{
		// get next word from buffer
		unsigned short int *word = (unsigned short int*)(buf + i);
		
		// swap bytes and write them back to the buffer
		*word = (*word >> 8) | ((*word << 8) & 0xFF00);
	}
}


//------------------------------------------------------------------------------
// reallocate memory, cheap implementation
//------------------------------------------------------------------------------
static void *realloc(void *ptr,unsigned long int size)
{
	void *newptr = malloc(size);
	if(newptr)
	{
		memcpy(newptr,ptr,size);
		free(ptr);
	}

	return newptr;
}


//------------------------------------------------------------------------------
// allocate aligned memory
//------------------------------------------------------------------------------
static void *aligned_malloc(unsigned long int size,int align)
{
	void *ptr;
	void *p;

	// alignment could not be less than zero
	if(align < 0)
	{
		return NULL;
	}

	// Allocate necessary memory area client request - size parameter - plus area
	// to store the address of the memory returned by standard malloc().
	p = malloc(size + align - 1 + sizeof(void*));

	if(p == NULL)
	{
		return NULL;
	}

	// Address of the aligned memory according to the align parameter
	ptr = (void*)(((unsigned int)p + sizeof(void*) + align -1) & ~(align-1));
	// store the address of the malloc() above at the beginning of our total
	// memory area.
	// You can also use *((void **)ptr-1) = p instead of the one below.
	*((void**)((unsigned int)ptr - sizeof(void*))) = p;

	// Return the address of aligned memory
	return ptr;
}


//------------------------------------------------------------------------------
// free aligned memory
//------------------------------------------------------------------------------
static void aligned_free(void *p)
{
	// Get the address of the memory, stored at the start of our total memory
	// area. Alternatively, you can use void *ptr = *((void **)p-1) instead of the
	// one below.
	void *ptr = *((void**)((unsigned int)p - sizeof(void*)));

	free(ptr);

	return;
}


//------------------------------------------------------------------------------
// find free command list slot
//------------------------------------------------------------------------------
static int FindFreeCmdListSlot(AHCI_MEMORY_REGISTERS *abar,ULONG portno)
{
	// get port address
	AHCI_PORT *port = &abar->PortList[portno];
	
	// maximum number of command list slots
	int cmdslots = 32;

	// if not set in SACT and CI, the slot is free
	// Offset 34h: PxSACT ? Port x Serial ATA Active (SCR3: SActive)
	// Offset 38h: PxCI ? Port x Command Issue
	DWORD slots = (port->SACT | port->CI);
	
	// do this for all command list slots
	int i;
	for(i = 0; i < cmdslots; i++)
	{
		// check for free slot
		if((slots & 1) == 0)
		{
			// free slot found
			return i;
		}

		// check next slot
		slots >>= 1;
	}

	return -1;
}


//------------------------------------------------------------------------------
// send FIS (Frame Information Structure) command to device
//------------------------------------------------------------------------------
static int SendFis(AHCI_FIS_CMD *fis)
{
	// set port
	AHCI_PORT *port = &fis->ABAR_Address->PortList[fis->PortNo];

	// Offset 10h: PxIS ? Port x Interrupt Status
	// clear pending interrupt bits
	port->IS.AsUlong = (ULONG)~0;

	// find free command list slot
	int slot = FindFreeCmdListSlot(fis->ABAR_Address,fis->PortNo);
	if(slot == -1)
	{
		// no free command list slot found
		return 1;
	}

	// fill command header
	AHCI_COMMAND_HEADER *cmdheader = (AHCI_COMMAND_HEADER*)port->CLB.AsUlong;
	// advance to command header of free slot
	cmdheader += slot;
	// set command FIS size in DWORDs
	cmdheader->DI.CFL = sizeof(AHCI_H2D_REGISTER_FIS) / sizeof(DWORD);
	// - 0 = read buffer from device
	// - 1 = write buffer to device
	cmdheader->DI.W = fis->Direction;
	// set PRDT entries count
	// 4 MB is the maximum transfer buffer size
	// 4 * 1024 * 1024 = 4.194.304 Bytes
	cmdheader->DI.PRDTL = (WORD)((fis->BufSize - 1) / (4194304)) + 1;

	// fill command table
	AHCI_COMMAND_TABLE *cmdtbl = (AHCI_COMMAND_TABLE*)(cmdheader->CTBA.AsUlong);
	// zero command table
	memset(cmdtbl,0,sizeof(AHCI_COMMAND_TABLE) + (cmdheader->DI.PRDTL - 1) * sizeof(AHCI_PRDT));

	// 4 MB (8192 sectors) per PRDT
	unsigned int i;
	for(i = 0; i < cmdheader->DI.PRDTL - 1; i++)
	{
		// lower Data Base Address (DBA)
		cmdtbl->PRDT[i].DBA.AsUlong = (DWORD)fis->Buf;
		// upper Data Base Address (DBAU)
		cmdtbl->PRDT[i].DBAU = (DWORD)(fis->Buf >> 32);
		// Data Byte Count (DBC)
		// This is 1 byte less than 4 MB, the AHCI controller will increase it to
		// an even value of exactly 4 MB. We use 1 byte less because otherwise we
		// will have zero in this field and no data will be transferred.
		cmdtbl->PRDT[i].DI.DBC = 4194303;
		// Interrupt on Completion (I)
		cmdtbl->PRDT[i].DI.I = 0;
		// increase buffer pointer by 4 MB
		fis->Buf += 4194304;
		// decrease buffer size by 4 MB
		fis->BufSize -= 4194304;
	}
	// last PRDT entry
	// lower Data Base Address (DBA)
	cmdtbl->PRDT[i].DBA.AsUlong = (DWORD)fis->Buf;
	// upper Data Base Address (DBAU)
	cmdtbl->PRDT[i].DBAU = (DWORD)(fis->Buf >> 32);
	// Data Byte Count (DBC)
	// This is 1 byte less than the buffer size, the AHCI controller will increase
	// it to an even value. We use 1 byte less because otherwise we will have zero
	// in this field and no data will be transferred for a 4 MB buffer. Also some
	// AMD AHCI controllers have problems if we do an identify device command and
	// place 0x200 instead of 0x1FF in this field. These AMD AHCI controllers will
	// hang after the first identify device command.
	cmdtbl->PRDT[i].DI.DBC = fis->BufSize - 1;
	// Interrupt on Completion (I)
	cmdtbl->PRDT[i].DI.I = 0;

	// fill FIS command
	AHCI_H2D_REGISTER_FIS *cmdfis = (AHCI_H2D_REGISTER_FIS*)(&cmdtbl->CFIS);
	// Register FIS ? Host to Device
	cmdfis->FisType = 0x27;
	// send a command
	cmdfis->C = 1;

	// copy registers
	// Command register
	cmdfis->Command = fis->Command;
	// Feature register, 7:0
	cmdfis->Feature7_0 = fis->Feature7_0;
	// LBA low register, 7:0
	cmdfis->LBA7_0 = fis->LBA7_0;
	// LBA mid register, 15:8
	cmdfis->LBA15_8 = fis->LBA15_8;
	// LBA high register, 23:16
	cmdfis->LBA23_16 = fis->LBA23_16;
	// Device register
	cmdfis->Device = fis->Device;
	// LBA register, 31:24
	cmdfis->LBA31_24 = fis->LBA31_24;
	// LBA register, 39:32
	cmdfis->LBA39_32 = fis->LBA39_32;
	// LBA register, 47:40
	cmdfis->LBA47_40 = fis->LBA47_40;
	// Feature register, 15:8
	cmdfis->Feature15_8 = fis->Feature15_8;
	// Count register, 7:0
	cmdfis->Count7_0 = fis->Count7_0;
	// Count register, 15:8
	cmdfis->Count15_8 = fis->Count15_8;

	// the below loop waits until the port is no longer busy before issuing a new
	// command
	AHCI_TASK_FILE_DATA tfd;
	do
	{
		tfd.AsUlong = port->TFD.AsUlong;
	// check busy (BSY) and data transfer requested status (DRQ)
	}while(tfd.STS.BSY || tfd.STS.DRQ);

	// Offset 38h: PxCI ? Port x Command Issue
	// send command in command slot
	port->CI = 1 << slot;

	// wait for completion
	// for some ATA commands this can take a very long time e.g. SECURITY ERASE
	// UNIT may need half an hour to complete on a platter HDD
	AHCI_INTERRUPT_STATUS is;
	while(1)
	{
		// In some longer duration reads, it may be helpful to spin on the
		// Descriptor Processed (DPS) bit in the PxIS port field as well (1 << 5)
		if((port->CI & (1 << slot)) == 0)
		{
			break;
		}

		// Task File Error Status (TFES)
		is.AsUlong = port->IS.AsUlong;
		if(is.TFES)
		{
			// error bit (bit 0) is set
			return 3;
		}
	}

	// check again
	// Task File Error Status (TFES)
	is.AsUlong = port->IS.AsUlong;
	if(is.TFES)
	{
		// error bit (bit 0) is set
		return 4;
	}	

	return 0;
}


//------------------------------------------------------------------------------
// read data from the device using the DMA data transfer protocol
//------------------------------------------------------------------------------
int AhciRead(AHCI_MEMORY_REGISTERS *abar,ULONG portno,QWORD start,QWORD buf,USHORT count)
{
	// fill FIS command structure
	AHCI_FIS_CMD fis;
	memset(&fis,0,sizeof(AHCI_FIS_CMD));
	fis.ABAR_Address = abar;
	fis.PortNo = portno;
	fis.Direction = 0;
	fis.Buf = buf;
	fis.BufSize = count * 512;
	// The number of logical sectors to be transferred. A value of 0000h indicates
	// that 65.536 logical sectors are to be transferred.
	fis.Count7_0  = (BYTE)count;
	fis.Count15_8 = (BYTE)(count >> 8);
	// address of first logical sector to be transferred
	fis.LBA7_0   = (BYTE)start;
	fis.LBA15_8  = (BYTE)(start >>  8);
	fis.LBA23_16 = (BYTE)(start >> 16);
	fis.LBA31_24 = (BYTE)(start >> 24);
	fis.LBA39_32 = (BYTE)(start >> 32);
	fis.LBA47_40 = (BYTE)(start >> 40);
	// LBA mode
	fis.Device = 0x40;
	// READ DMA EX
	fis.Command = 0x25;

	// send FIS command
	if(SendFis(&fis) != 0)
	{
		return 1;
	}	

	return 0;
}


//------------------------------------------------------------------------------
// identify ATA / ATAPI device
//------------------------------------------------------------------------------
static int IdentifyDevice(AHCI_MEMORY_REGISTERS *abar,ULONG portno,unsigned char atapi,unsigned char *buf)
{
	// fill FIS command structure
	AHCI_FIS_CMD fis;
	memset(&fis,0,sizeof(AHCI_FIS_CMD));
	fis.ABAR_Address = abar;
	fis.PortNo = portno;
	fis.Direction = 0;
	fis.Buf = (DWORD)buf;
	fis.BufSize = 512;

	// ATAPI device
	if(atapi == 1)
	{
		// IDENTIFY ATAPI DEVICE
		fis.Command = 0xA1;
	}
	// ATA device
	else
	{
		// IDENTIFY ATA DEVICE
		fis.Command = 0xEC;
	}

	// send FIS command
	if(SendFis(&fis) != 0)
	{
		return 1;
	}	

	return 0;
}


//------------------------------------------------------------------------------
// get identify data from device
//------------------------------------------------------------------------------
static int GetIdentifyData(AHCI_MEMORY_REGISTERS *abar,ULONG portno,unsigned char atapi,char *model,char *fw,char *serial,unsigned short int *securityStatus,unsigned short int *masterPwIdentifier,unsigned short int *normalEraseTime,unsigned short int *enhancedEraseTime)
{
	// transfer buffer
	unsigned char buf[512];
	memset(buf,0x00,512);
	
	// identify device
	if(IdentifyDevice(abar,portno,atapi,buf) != 0)
	{
		return 1;
	}

	// We swap only the bytes of the model, firmware and serial number. If we
	// would swap the complete buffer we would have to swap the bytes once again
	// for security status, master password id and erase times.

	// get model number with spaces
	// swap received bytes
	SwapBytes(buf + 54,40);
	memcpy(model,buf + 54,40);
	model[40] = '\0';
	// delete spaces from model number
	RemoveLeadingAndTrailingSpaces(model);

	// get firmware revision with spaces
	// swap received bytes
	SwapBytes(buf + 46,8);
	memcpy(fw,buf + 46,8);
	fw[8] = '\0';
	// delete spaces from firmware revision
	RemoveLeadingAndTrailingSpaces(fw);
	
	// get serial number with spaces
	// swap received bytes
	SwapBytes(buf + 20,20);
	memcpy(serial,buf + 20,20);
	serial[20] = '\0';
	// delete spaces from serial number
	RemoveLeadingAndTrailingSpaces(serial);

	// copy security status WORD 128
	*securityStatus = *(unsigned short int*)(buf + 256);

	// copy master password identifier WORD 92
	*masterPwIdentifier = *(unsigned short int*)(buf + 184);

	// copy time required for security erase unit completion WORD 89
	*normalEraseTime = *(unsigned short int*)(buf + 178) * 2;
	// copy time required for enhanced security erase completion WORD 90
	*enhancedEraseTime = *(unsigned short int*)(buf + 180) * 2;

	return 0;
}


//------------------------------------------------------------------------------
// get last PCI bus number in the system
// this has the same effect as using the following code with interrupts:
// union REGS regs;
// regs.x.ax = 0xB101;
// int86(0x1A,&regs,&regs);
// int lastBusNum = regs.h.cl;
//------------------------------------------------------------------------------
static unsigned short int GetLastPCIBusNumber(void)
{
	// last bus number
	unsigned short int lastBusNum = 0xFF;
	
	// PCI vendor ID
	// use 4 chars, because we receive a DWORD
	unsigned char vendorID[4];

	// do this for all possible buses, normally there is not much more as bus 0 and 1
	unsigned short int bus;
	for(bus = 0x00; bus <= 0xFF; bus++)
	{
		// do this for all devices
		unsigned short int dev;
		for(dev = 0x00; dev <= 0x1F; dev++)
		{
			// do this for all functions
			unsigned short int func;
			for(func = 0x00; func <= 0x07; func++)
			{
				// read PCI configuration DWORD
				// 32 Bit CONFIG_ADDRESS register format
				// Bit     Usage
				// -----------------------------------------------
				// 31      Enable Bit, 1 = enabled, 0 = disabled
				// 30 - 24 Reserved, read only, must return 0's when read
				// 23 - 16 Bus Number, choose a specific PCI bus in the system
				// 15 - 11 Device Number, choose a specific device on the bus
				// 10 -  8 Function Number, choose a specific function in a device
				//  7 -  2 Register Number, choose a DWORD in the device's
				//         Configuration Space
				//  1 -  0 read-only and must return 0's when read
				// because bits 0 and 1 are always zero we use (reg & ~3)
				// 0x00 is the value for the first register value to read
				outpd(0xCF8,0x80000000L | ((unsigned long int)bus << 16) | ((unsigned short int)dev << 11) | ((unsigned short int)func << 8) | (0x00 & ~3));
				
				// read CONFIG_DATA register
				*(unsigned long int*)vendorID = inpd(0xCFC);

				// get next function on invalid vendor ID
				if(vendorID[0x00] == 0xFF && vendorID[0x01] == 0xFF)
				{
					continue;
				}
				// valid vendor ID
				else
				{
					// set last valid bus number
					lastBusNum = bus;
					goto label_check_next_bus;
				}
			}
		}
label_check_next_bus:
		;
	}
	
	return lastBusNum;
}


//------------------------------------------------------------------------------
// find AHCI base address on any AHCI Mass Storage Controller
// arg0: pointer to a PAHCI_HBA_EXT array
// arg1: number of controllers found
//------------------------------------------------------------------------------
static int AhciFindBaseAddress(AHCI_HBA_EXT **HbaExt,unsigned short int *foundControllers)
{
	// set AHCI_HBA_EXT structure pointer to zero
	*HbaExt = NULL;
	// set found controllers to zero
	*foundControllers = 0;	
	// PCI config space info table
	unsigned char infoTable[0x28];
	// actual number of controllers, this is incremented after one controller is
	// found
	unsigned short int contr = 0;
	// get last PCI bus number
	unsigned short int lastBusNum = GetLastPCIBusNumber();

	// do this for all possible buses, normally there is not much more as bus 0
	// and 1
	unsigned short int bus;
	for(bus = 0x00; bus <= lastBusNum; bus++)
	{
		// do this for all devices
		unsigned short int dev;
		for(dev = 0x00; dev <= 0x1F; dev++)
		{
			// do this for all functions
			unsigned short int func;
			for(func = 0x00; func <= 0x07; func++)
			{
				// zero info table
				memset(infoTable,0,0x28);

				// read all registers from pci configuration space and fill info
				// table, for performance increase we read only the 1st 0x28 bytes
				// of pci config space, this is enough to get the port addresses
				unsigned short int reg;
				for(reg = 0x00; reg <= 0x24; reg = reg + 4)
				{
					// read PCI configuration DWORD
					// 32 Bit CONFIG_ADDRESS register format
					// Bit     Usage
					// -----------------------------------------------
					// 31      Enable Bit, 1 = enabled, 0 = disabled
					// 30 - 24 Reserved, read only, must return 0's when read
					// 23 - 16 Bus Number, choose a specific PCI bus in the system
					// 15 - 11 Device Number, choose a specific device on the bus
					// 10 -  8 Function Number, choose a specific function in a
					//         device
					//  7 -  2 Register Number, choose a DWORD in the device's
					//         Configuration Space
					//  1 -  0 read-only and must return 0's when read
					// because bits 0 and 1 are always zero we use (reg & ~3)
					outpd(0xCF8,0x80000000L | ((unsigned long int)bus << 16) | ((unsigned short int)dev << 11) | ((unsigned short int)func << 8) | (reg & ~3));

					// read CONFIG_DATA register
					*(unsigned long int*)(infoTable + reg) = inpd(0xCFC);

					// get next function on invalid vendor ID
					if(infoTable[0x00] == 0xFF && infoTable[0x01] == 0xFF)
					{
						goto label_check_next_func;
					}
				}
	
				// Mass Storage AHCI Controller detected
				if(infoTable[0x0B] == 0x01 && infoTable[0x0A] == 0x06)
				{
					// if bit 7 of the Header Type is 0 this is a multi function
					// device
					if((infoTable[0x0E] & 0x7F) == 0)
					{
						// set AHIC base address to zero
						unsigned long int ulAhciBaseAddress = 0;
						
						// Bit 0 in all Base Address registers is read-only and
						// used to determine whether the register maps into Memory
						// or I/O Space. Base Address registers that map to Memory
						// Space must return a 0 in bit 0. Base Address registers
						// that map to I/O Space must return a 1 in bit 0.
						// register maps into memory space
						if((infoTable[0x24] & 1) == 0)
						{
							// get AHCI base address from info table
							ulAhciBaseAddress = (infoTable[0x27] << 24) + (infoTable[0x26] << 16) + (infoTable[0x25] << 8) + (infoTable[0x24] & 0xFC);
						}

						// if the address is zero leave the function for loop
						if(ulAhciBaseAddress == 0) break;

						// allocate space for one more AHCI_HBA_EXT structure
						*HbaExt = (AHCI_HBA_EXT*)realloc(*HbaExt,(contr + 1) * sizeof(AHCI_HBA_EXT));
						if(*HbaExt == NULL)
						{
							printf("AhciFindBaseAddress Error: Can't reallocate memory for AHCI_INFO!\n");
							return 1;
						}
						
						// zero added AHCI_HBA_EXT structure
						memset(*HbaExt + contr,0,sizeof(AHCI_HBA_EXT));

						// save AHCI base address
						(*HbaExt)[contr].ABAR_Address = (AHCI_MEMORY_REGISTERS*)ulAhciBaseAddress;
						// save bus, function and device number
						(*HbaExt)[contr].Bus = bus;
						(*HbaExt)[contr].Dev = dev;
						(*HbaExt)[contr].Func = func;
						// convert PCI table vendor ID to WORD
						(*HbaExt)[contr].PciVendorID = (infoTable[1] << 8) | infoTable[0];
						// convert PCI table device ID to WORD
						(*HbaExt)[contr].PciDeviceID = (infoTable[3] << 8) | infoTable[2];

						// increment number of valid controllers found
						contr++;
					}
				}
label_check_next_func:
				;
			}
		}
	}

	// return number of found AHCI controllers
	*foundControllers = contr;
	
	return 0;
}


//------------------------------------------------------------------------------
// corresponds to StorAhci function AhciAdapterReset in hbastat.c
// This function brings the HBA and all ports to the H:Idle and P:Idle states by
// way of HBA Reset and P:Reset
//------------------------------------------------------------------------------
static int AhciAdapterReset(AHCI_MEMORY_REGISTERS *abar)
{
	// 10.4.3 HBA Reset
	// If the HBA becomes unusable for multiple ports, and a software reset or
	// port reset does not correct the problem, software may reset the entire HBA
	// by setting GHC.HR to ?1?. When software sets the GHC.HR bit to ?1?, the HBA
	// shall perform an internal reset action.
	AHCI_Global_HBA_CONTROL ghc;
	ghc.HR = 1;
	abar->GHC.AsUlong = ghc.AsUlong;

	// The bit shall be cleared to ?0? by the HBA when the reset is complete. A
	// software write of ?0? to GHC.HR shall have no effect. To perform the HBA
	// reset, software sets GHC.HR to ?1? and may poll until this bit is read to
	// be ?0?, at which point software knows that the HBA reset has completed.
	ghc.AsUlong = abar->GHC.AsUlong;
	// 5.2.2.1 H:Init
	// 5.2.2.2 H:WaitForAhciEnable
	int i;
	for(i = 0;(i < 50) && (ghc.HR == 1); i++)
	{
		// 20 milliseconds
		delay(20);
		ghc.AsUlong = abar->GHC.AsUlong;
	}

	// If the HBA has not cleared GHC.HR to ?0? within 1 second of software
	// setting GHC.HR to ?1?, the HBA is in a hung or locked state.
	if(i == 50)
	{
		printf("AhciAdapterReset Error: HBA reset time out!\n");
		return 1;
	}

	// When GHC.HR is set to ?1?, GHC.AE, GHC.IE, the IS register, and all port
	// register fields (except PxFB/PxFBU/PxCLB/PxCLBU) that are not HwInit in the
	// HBA抯 register memory space are reset. The HBA抯 configuration space and
	// all other global registers/bits are not affected by setting GHC.HR to ?1?.
	// Any HwInit bits in the port specific registers are not affected by setting
	// GHC.HR to ?1?. The port specific registers PxFB, PxFBU, PxCLB, and PxCLBU
	// are not affected by setting GHC.HR to ?1?. If the HBA supports staggered
	// spin-up, the PxCMD.SUD bit will be reset to ?0?; software is responsible
	// for setting the PxCMD.SUD and PxSCTL.DET fields appropriately such that
	// communication can be established on the Serial ATA link. If the HBA does
	// not support staggered spin-up, the HBA reset shall cause a COMRESET to be
	// sent on the port

	return 0;
}


//------------------------------------------------------------------------------
// corresponds to StorAhci function PortClearPendingInterrupt in util.h
//------------------------------------------------------------------------------
static __inline void PortClearPendingInterrupt(AHCI_MEMORY_REGISTERS *abar,ULONG portno)
{
	// get port address
	AHCI_PORT *port = &abar->PortList[portno];
	
	port->SERR.AsUlong = (ULONG)~0;
	port->IS.AsUlong = (ULONG)~0;
	abar->IS = 1 << portno;
}


//------------------------------------------------------------------------------
// corresponds to StorAhci function AhciPortInitialize in pnppower.c
// This function is used to start an AHCI port
//------------------------------------------------------------------------------
static int AhciPortInitialize(AHCI_MEMORY_REGISTERS *abar,ULONG portno,UCHAR *pSystemMemory)
{
	// get port address
	AHCI_PORT *port = &abar->PortList[portno];

	// Setup the CommandList
	// PxCLB and PxCLBU (AHCI 1.1 Section 10.1.2 - 5)
	// Command list offset: 1K * portno
	// Command list entry size = 32
	// Command list entry maximum count = 32
	// Command list maximum size = 32 * 32 = 1K per port
	// Offset 00h: PxCLB ? Port x Command List Base Address
	port->CLB.AsUlong = (DWORD)pSystemMemory + (portno << 10);
	// Offset 04h: PxCLBU ? Port x Command List Base Address Upper 32-bits
	port->CLBU = 0;
	// zero command list
	memset((void*)(port->CLB.AsUlong),0,1024);
	
	// Setup the Receive FIS buffer
	// PxFB and PxFBU (AHCI 1.1 Section 10.1.2 - 5)
	// FIS offset: 32K + 256 * portno
	// FIS entry size = 256 bytes per port
	// Offset 08h: PxFB ? Port x FIS Base Address
	port->FB.AsUlong = (DWORD)pSystemMemory + (32 << 10) + (portno << 8);
	// Offset 0Ch: PxFBU ? Port x FIS Base Address Upper 32-bits
	port->FBU = 0;
	// zero FIS structure
	memset((void*)(port->FB.AsUlong),0,256);

	// Command table offset: 40K + 8K * portno
	// Command table size = (128 + (8 * 16)) * 32 = 8K per port
	// 128 -> CFIS + ACMD + Reserved
	// 8 PRDT entries * 16 bytes per entry
	// 32 ports
	AHCI_COMMAND_HEADER *cmdheader = (AHCI_COMMAND_HEADER*)(port->CLB.AsUlong);
	int i;
	for(i = 0; i < 32; i++)
	{
		// 8 prdt entries per command table
		cmdheader[i].DI.PRDTL = 8;
		// 256 bytes per command table, 64 + 16 + 48 + 16 * 8
		// Command table offset: 40K + 8K * portno + cmdheader_index * 256
		cmdheader[i].CTBA.AsUlong = (DWORD)pSystemMemory + (40 << 10) + (portno << 13) + (i << 8);
		cmdheader[i].CTBAU = 0;
		// zero command table descriptor base address
		memset((void*)cmdheader[i].CTBA.AsUlong,0,256);
	}

	// Clear Enable Interrupts on the Channel (AHCI 1.1 Section 10.1.2 - 7)
	// We will enable interrupt after channel started
	PortClearPendingInterrupt(abar,portno);

	return 0;
}


//------------------------------------------------------------------------------
// corresponds to StorAhci function AhciCOMRESET in hbastat.c
// PHY Reset:COMRESET
// SCTL.DET Controls the HBA抯 device detection and interface initialization.
// DET=1 Performs interface communication initialization sequence to establish
// communication. This is functionally equivalent to a hard reset and results in
// the interface being reset and communications reinitialized. While this field is
// 1h, COMRESET is transmitted on the interface. Software should leave the DET
// field set to 1h for a minimum of 1 millisecond to ensure that a COMRESET is
// sent on the interface. since we are in 5.3.2.3 P:NotRunning and PxCMD.SUD = ?0?
// does this still take us to P:StartComm?
//------------------------------------------------------------------------------
static int AhciCOMRESET(AHCI_MEMORY_REGISTERS *abar,ULONG portno)
{
	// get port address
	AHCI_PORT *port = &abar->PortList[portno];

	// make sure ST is 0. DET cannot be altered while ST == 1 as per AHCI 1.1
	// section 5.3.2.3.
	AHCI_COMMAND cmd;
	cmd.AsUlong = port->CMD.AsUlong;
	if(cmd.ST == 1)
	{
		// PxCMD.ST is 1. Abort
		return 1;
	}

	// Don't allow a COMINIT to trigger a hotplug
	AHCI_INTERRUPT_ENABLE ieOrig;
	AHCI_INTERRUPT_ENABLE ieTemp;
	ieOrig.AsUlong = port->IE.AsUlong;
	ieTemp.AsUlong = ieOrig.AsUlong;
	// "PhyRdy Change Interrupt Enable", no generate interrupt with PxIS.PRCS
	ieTemp.PRCE = 0;
	// "Port Change Interrupt Enable", no generate interrupt with PxIS.PCS
	ieTemp.PCE = 0;
	port->IE.AsUlong = ieTemp.AsUlong;
	
	// Perform COMRESET
	AHCI_SERIAL_ATA_CONTROL sctl;
	sctl.AsUlong = port->SCTL.AsUlong;
	sctl.DET = 1;
	port->SCTL.AsUlong = sctl.AsUlong;
	// DET = 1 Performs interface communication initialization sequence to
	// establish communication. This is functionally equivalent to a hard reset
	// and results in the interface being reset and communications reinitialized.
	// While this field is 1h, COMRESET is transmitted on the interface. Software
	// should leave the DET field set to 1h for a minimum of 1 millisecond to
	// ensure that a COMRESET is sent on the interface.
	delay(1);

	sctl.AsUlong = port->SCTL.AsUlong;
	sctl.DET = 0;
	port->SCTL.AsUlong = sctl.AsUlong;

	// Wait for DET to be set
	// AHCI 10.4.2 After clearing PxSCTL.DET to 0h, software should wait for
	// communication to be re-established as indicated by bit 0 of PxSSTS.DET
	// being set to ?1?.
	// typically, it will be done in 10ms. max wait 50ms to be safe
	delay(50);
	AHCI_SERIAL_ATA_STATUS ssts;
	ssts.AsUlong = port->SSTS.AsUlong;

	if(ssts.DET == 0)
	{
		// wait 30 ms on DET
		int i;
		for(i = 0; i < 30; i++)
		{
			delay(1);
			ssts.AsUlong = port->SSTS.AsUlong;
			if(ssts.DET != 0)
			{
				break;
			}
		}
	}

	// Enable Hotplug again if it was enabled before
	port->IE.AsUlong = ieOrig.AsUlong;

	// Clear SERR
	// AHCI 10.4.2 software should write all 1s to the PxSERR register to clear
	// any bits that were set as part of the port reset.
	port->SERR.AsUlong = (ULONG)~0;

	return 0;
}


//------------------------------------------------------------------------------
// corresponds to StorAhci function P_NotRunning in hbastat.c
// This function does the following:
// - Clear CMD.ST
// - Verify CR cleared
// - Verify CI cleared
// - Clear CMD.FRE
// - Verify FR cleared
//------------------------------------------------------------------------------
static BOOL P_NotRunning(AHCI_MEMORY_REGISTERS *abar,ULONG portno)
{
	// get port address
	AHCI_PORT *port = &abar->PortList[portno];

	// Clear CMD.ST
	AHCI_COMMAND cmd;
	cmd.AsUlong = port->CMD.AsUlong;
	// AHCI 10.3.2 on FRE it says:
	// Software shall not clear this [FRE] bit while PxCMD.ST remains set to ?1?.
	// System software places a port into the idle state by clearing PxCMD.ST and
	// waiting for PxCMD.CR to return ?0? when read.
	cmd.ST = 0;
	port->CMD.AsUlong = cmd.AsUlong;

	// HBA supports command list override
	int i;
	AHCI_TASK_FILE_DATA tfd;
	if(abar->CAP.SCLO == TRUE)
	{
		// AHCI 3.3.7 make sure PxCMD.ST is 0.
		for(i = 1; i < 101; i++)
		{
			cmd.AsUlong = port->CMD.AsUlong;
			if(cmd.ST == 0)
			{
				break;
			}
		
			// 5 milliseconds
			delay(5);
		}
		
		if(i == 101)
		{
			// P_NotRunning, After 500ms of writing 0 to PxCMD.ST, it's still 1.
		}
		
		tfd.AsUlong = port->TFD.AsUlong;
		if(tfd.STS.BSY)
		{
			// AHCI 3.3.7 Command List Override (CLO): Setting this bit to ?1?
			// causes PxTFD.STS.BSY and PxTFD.STS.DRQ to be cleared to ?0?. This
			// allows a software reset to be transmitted to the device regardless
			// of whether the BSY and DRQ bits are still set in the PxTFD.STS
			// register.
		
			// Do this to make sure the port can stop
			cmd.CLO = 1;
			port->CMD.AsUlong = cmd.AsUlong;
		}		
	}
	
	// Verify CR cleared
	// AHCI 10.1.2 - 3: wait cmd.CR to be 0. Software should wait at least 500
	// milliseconds for this to occur.
	for(i = 1; i < 101; i++)
	{
		cmd.AsUlong = port->CMD.AsUlong;
		if(cmd.CR == 0 && cmd.ST == 0)
		{
			break;
		}
	
		// 5 milliseconds
		delay(5);
	}

	// AHCI 10.4.2 If PxCMD.CR or PxCMD.FR do not clear to ?0? correctly, then
	// software may attempt a port reset or a full HBA reset to recover.
	if(i == 101)
	{
		// P_NotRunning, PxCMD.CR or PxCMD.FR do not clear to ?0? correctly
		return FALSE;
	}	

	// Verify CI cleared
	// This must have the effect of clearing CI as per AHCI section 3.3.14
	for(i = 1; i < 101; i++)
	{
		// Port x Command Issue
		ULONG ci;
		ci = port->CI;
		if(ci == 0)
		{
			break;
		}
	
		// 50 microseconds normally, we use 1 ms here
		delay(1);
	}

	// If CI does not clear to ?0? correctly abort the stop
	if(i == 101)
	{
		// P_NotRunning, CI does not clear to ?0?
		return FALSE;
	}

	// Clear CMD.FRE
	cmd.AsUlong = port->CMD.AsUlong;
	if((cmd.FRE | cmd.FR) != 0)
	{
		// If PxCMD.FRE is set to ?1?, software should clear it to ?0? and wait at
		// least 500 milliseconds for PxCMD.FR to return ?0? when read.
		// AHCI 10.3.2 Software shall not clear this bit while PxCMD.ST or
		// PxCMD.CR is set to ?1?.
		cmd.FRE = 0;
		port->CMD.AsUlong = cmd.AsUlong;
	
		if(abar->CAP.SCLO == TRUE)
		{
			tfd.AsUlong = port->TFD.AsUlong;
			if(tfd.STS.BSY)
			{
				// don't expect BSY is set again
				cmd.CLO = 1;
				port->CMD.AsUlong = cmd.AsUlong;
			}
		}

		// Software should wait at least 500 milliseconds for this to occur.
		for(i = 1; i < 101; i++)
		{
			cmd.AsUlong = port->CMD.AsUlong;
			if(cmd.CR == 0 && cmd.FR == 0 && cmd.ST == 0 && cmd.FRE == 0)
			{
				break;
			}
			
			// 5 milliseconds
			delay(5);
		}

		if(i == 101)
		{
			// If PxCMD.CR or PxCMD.FR do not clear to ?0? correctly, then
			// software may attempt a port reset or a full HBA reset to recover.
			//if(supportsCLO)
			//{
				// P_NotRunning, PxCMD.CR or PxCMD.FR do not clear to ?0?, CLO
				// enabled
			//}
			//else
			//{
				// P_NotRunning, PxCMD.CR or PxCMD.FR do not clear to ?0?, CLO not
				// enabled
			//}

			return FALSE;
		}
	}

	// wait for CLO to clear
	// AHCI 3.3.7 Software must wait for CLO to be cleared to ?0? before setting
	// PxCMD.ST to ?1?.
	// Register bit CLO might be set in this function, and it should have been
	// cleared before function exit.
	if(abar->CAP.SCLO == TRUE && cmd.CLO == 0)
	{
		for(i = 1; i < 101; i++)
		{
			cmd.AsUlong = port->CMD.AsUlong;
			if(cmd.CLO == 0)
			{
				break;
			}

			// 5 milliseconds
			delay(5);
		}

		if(i == 101)
		{
			// P_NotRunning, PxCMD.CLO not clear to ?0?, CLO enabled
			return FALSE;
		}
	}
	
	return TRUE;
}


//------------------------------------------------------------------------------
// corresponds to StorAhci function Set_PxIE in util.c
// This is just a space saver function that makes the interrupt configuration
// more readable
//------------------------------------------------------------------------------
static VOID Set_PxIE(AHCI_MEMORY_REGISTERS *abar,ULONG portno)
{
	// get port address
	AHCI_PORT *port = &abar->PortList[portno];	

	AHCI_INTERRUPT_ENABLE ie;
	ie.AsUlong = port->IE.AsUlong;
	// Device to Host Register FIS Interrupt (DHRS):  A D2H Register FIS has been
	// received with the 慖? bit set, and has been copied into system memory.
	ie.DHRE = 1;
	// PIO Setup FIS Interrupt (PSS):  A PIO Setup FIS has been received with the
	// 慖? bit set, it has been copied into system memory, and the data related to
	// that FIS has been transferred. This bit shall be set even if the data
	// transfer resulted in an error.
	ie.PSE = 1;
	// DMA Setup FIS Interrupt (DSS): A DMA Setup FIS has been received with the
	// 慖? bit set and has been copied into system memory.
	ie.DSE = 1;
	// Set Device Bits Interrupt (SDBS):  A Set Device Bits FIS has been received
	// with the 慖? bit set and has been copied into system memory.
	ie.SDBE = 1;
	// Unknown FIS Interrupt (UFS): When set to ?1?, indicates that an unknown FIS
	// was received and has been copied into system memory. This bit is cleared to
	// ?0? by software clearing the PxSERR.DIAG.F bit to ?0?.  Note that this bit
	// does not directly reflect the PxSERR.DIAG.F bit. PxSERR.DIAG.F is set
	// immediately when an unknown FIS is detected, whereas this bit is set when
	// that FIS is posted to memory. Software should wait to act on an unknown FIS
	// until this bit is set to ?1? or the two bits may become out of sync.
	ie.UFE = 0;
	// Descriptor Processed (DPS):  A PRD with the 慖? bit set has transferred all
	// of its data. Refer to section 5.4.2.
	ie.DPE = 0;
	// Port Connect Change Status (PCS): 1=Change in Current Connect Status. 0=No
	// change in Current Connect Status. This bit reflects the state of
	// PxSERR.DIAG.X. This bit is only cleared when PxSERR.DIAG.X is cleared.
	ie.PCE = 1;
	// Supports Mechanical Presence Switch (SMPS)
	if(abar->CAP.SMPS)
	{
		// Device Mechanical Presence Status (DMPS): When set, indicates that a
		// mechanical presence switch attached to this port has been opened or
		// closed, which may lead to a change in the connection state of the
		// device. This bit is only valid if both CAP.SMPS and P0CMD.MPSP are set
		// to ?1?.
		ie.DMPE  = 1;
	}
	else
	{
		ie.DMPE  = 0;
	}

	// Reserved:14;
	// PhyRdy Change Status (PRCS): When set to ?1? indicates the internal PhyRdy
	// signal changed state. This bit reflects the state of P0SERR.DIAG.N. To
	// clear this bit, software must clear P0SERR.DIAG.N to ?0?.
	ie.PRCE = 1;
	// Incorrect Port Multiplier Status (IPMS): Indicates that the HBA received a
	// FIS from a device whose Port Multiplier field did not match what was
	// expected. The IPMS bit may be set during enumeration of devices on a Port
	// Multiplier due to the normal Port Multiplier enumeration process. It is
	// recommended that IPMS only be used after enumeration is complete on the
	// Port Multiplier.
	ie.IPME = 0;
	// Overflow Status (OFS): Indicates that the HBA received more bytes from a
	// device than was specified in the PRD table for the command.
	ie.OFE = 1;
	// Reserved2:1;
	// Interface Non-fatal Error Status (INFS): Indicates that the HBA encountered
	// an error on the Serial ATA interface but was able to continue operation.
	// Refer to section 6.1.2.
	ie.INFE = 1;
	// Interface Fatal Error Status (IFS): Indicates that the HBA encountered an
	// error on the Serial ATA interface which caused the transfer to stop. Refer
	// to section 6.1.2.
	ie.IFE = 1;
	// Host Bus Data Error Status (HBDS): Indicates that the HBA encountered a
	// data error (uncorrectable ECC / parity) when reading from or writing to
	// system memory.
	ie.HBDE = 1;
	// Host Bus Fatal Error Status (HBFS): Indicates that the HBA encountered a
	// host bus error that it cannot recover from, such as a bad software pointer.
	// In PCI, such an indication would be a target or master abort.
	ie.HBFE = 1;
	// Task File Error Status (TFES): This bit is set whenever the status register
	// is updated by the device and the error bit (bit 0) is set.
	ie.TFEE = 1;
	
	AHCI_COMMAND cmd;
	cmd.AsUlong = port->CMD.AsUlong;
	if(cmd.CPD)
	{
		// check for PxCMD.CPD set to ?1? before setting CPDE
		// Cold Port Detect Status (CPDS): When set, a device status has changed
		// as detected by the cold presence detect logic. This bit can either be
		// set due to a non-connected port receiving a device, or a connected port
		// having its device removed. This bit is only valid if the port supports
		// cold presence detect as indicated by PxCMD.CPD set to ?1?.
		ie.CPDE = 1;
	}
	else
	{
		ie.CPDE = 0;
	}

	port->IE.AsUlong = ie.AsUlong;
}


//------------------------------------------------------------------------------
// corresponds to StorAhci function P_Running_StartFailed in hbastat.c
//------------------------------------------------------------------------------
static BOOL P_Running_StartFailed(AHCI_MEMORY_REGISTERS *abar,ULONG portno)
{
	// Enable Interrupts on the Channel (AHCI 1.1 Section 10.1.2 - 7)
	PortClearPendingInterrupt(abar,portno);
	Set_PxIE(abar,portno);
	
	return FALSE;
}


//------------------------------------------------------------------------------
// corresponds to StorAhci function P_Running_WaitOnDET3 in hbastat.c
// From here on out only a DET of 3 will do
// - Look for signs of life which are DET == 3
// - Wait for 1 second until DET becomes 3
// - After waiting for 1 second, give up on starting the channel
// - If DET == 3 we are ready for WaitOnFRE
//------------------------------------------------------------------------------
static BOOL P_Running_WaitOnDET3(AHCI_MEMORY_REGISTERS *abar,ULONG portno,AHCI_PORT_EXT *portExt)
{
	// get port address
	AHCI_PORT *port = &abar->PortList[portno];

	AHCI_SERIAL_ATA_STATUS ssts;

WaitOnDET3_Start:
	// After waiting for 1 second, give up on starting the channel
	ssts.AsUlong = port->SSTS.AsUlong;
	
	if(ssts.DET != 3)
	{
		if(portExt->StateDET3Count > 100)
		{
			// P_Running_WaitOnDET3 timed out
			return P_Running_StartFailed(abar,portno);
		}
		// Wait for 1 second until DET becomes 3
		else
		{
			// P_Running_WaitOnDET3 still waiting
			portExt->StateDET3Count++;

			// wait 10 milliseconds
			delay(10);
			goto WaitOnDET3_Start;
		}
	}
	// If DET==3 we are ready for WaitOnFRE
	else
	{
		// P_Running_WaitOnDET3 Success
		return P_Running_WaitOnFRE(abar,portno,portExt);
	}
}


//------------------------------------------------------------------------------
// corresponds to StorAhci function P_Running_WaitOnBSYDRQ in hbastat.c
// - Clear serr.DIAG.X
// - Wait for the rest of the 10 seconds for BSY and DRQ to clear
// - After a total amount of 3 seconds working on the start COMRESET
// - After waiting for the rest of the 4 seconds for BSY and DRQ to clear, give up
// - Set ST to 1
//------------------------------------------------------------------------------
static BOOL P_Running_WaitOnBSYDRQ(AHCI_MEMORY_REGISTERS *abar,ULONG portno,AHCI_PORT_EXT *portExt)
{
	// get port address
	AHCI_PORT *port = &abar->PortList[portno];	

	AHCI_TASK_FILE_DATA tfd;

WaitOnBSYDRQ_Start:
	// P_Running_WaitOnBSYDRQ
	tfd.AsUlong = port->TFD.AsUlong;

	// Enable BSY and DRQ to go to 0
	if(tfd.STS.BSY || tfd.STS.DRQ)
	{
		// When [serr.DIAG.X is] set to one this bit indicates a COMINIT signal
		// was received. This bit is reflected in the P0IS.PCS bit.
		// to allow the TFD to be updated serr.DIAG.X must be cleared.
		AHCI_SERIAL_ATA_ERROR serr;
		serr.AsUlong = 0;
		serr.DIAG.X = 1;
		port->SERR.AsUlong = serr.AsUlong;
	}
	
	// Set ST to 1
	if(tfd.STS.BSY == 0 && tfd.STS.DRQ == 0)
	{
		// Enable Interrupts on the channel (AHCI 1.1 Section 10.1.2 - 7)
		PortClearPendingInterrupt(abar,portno);
		Set_PxIE(abar,portno);
	
		// We made it! Set ST and start the IO we have collected!
		AHCI_COMMAND cmd;
		cmd.AsUlong = port->CMD.AsUlong;
		cmd.ST = 1;
		port->CMD.AsUlong = cmd.AsUlong;
	
		// start is complete

		// Exit P_Running_WaitOnBSYDRQ Succeeded
		return TRUE;
	}
	// After waiting for the remainder of the 60 second maximum Channel Start time
	// for BSY and DRQ to clear, give up.
	else
	{
		// Some big HDDs takes close to 20 second to spin up
		ULONG portStartTimeoutIn10MS = (60 * 100);

		// calculate the total time in unit of 10 ms.
		USHORT totalstarttime;
		totalstarttime = portExt->StateDETCount + portExt->StateDET1Count + portExt->StateDET3Count +
		portExt->StateFRECount + (portExt->StateBSYDRQCount * 2);

		if(totalstarttime > portStartTimeoutIn10MS)
		{
			// P_Running_WaitOnBSYDRQ timed out
			return P_Running_StartFailed(abar,portno);
		}
		// After a total amount of 1 second working on the start COMRESET
		else if(portExt->StateBSYDRQCount == 50)
		{
			// Stop FRE, FR in preperation for RESET
			if(P_NotRunning(abar,portno) == FALSE)
			{
				// It takes 1/2 second for stop to fail.
				// This is taking way too long and the controller is not
				// responding properly. Abort the start.
				return P_Running_StartFailed(abar,portno);
				// P_Running_WaitOnBSYDRQ Stop Failed on COMRESET
			}

			// All set, bring down the hammer
			AhciCOMRESET(abar,portno);

			// Set the timers for time remaining. This is a best case scenario and
			// the best that can be offered.
			portExt->StateDETCount = 0;
			portExt->StateDET1Count = 0;
			// won't exceed 100 * 10ms for 1 second
			portExt->StateDET3Count = 0;
			// won't exceed 5 * 10ms for 50 ms
			portExt->StateFRECount = 0;
			// won't exceed 3000 * 20ms for 60 seconds minus what DET3 and FRE
			// use.
			portExt->StateBSYDRQCount = 51;

			// P_Running_WaitOnBSYDRQ crossing 1 second. COMRESET done, going to
			// WaitOnDET3
			// Go back to WaitOnDet3
			return P_Running_WaitOnDET3(abar,portno,portExt);
		}
		// Wait for the rest of the 4 seconds for BSY and DRQ to clear
		else
		{
			// P_Running_WaitOnBSYDRQ still waiting
			portExt->StateBSYDRQCount++;

			// wait 20 milliseconds
			delay(20);
			goto WaitOnBSYDRQ_Start;
		}
	}
}


//------------------------------------------------------------------------------
// corresponds to StorAhci function P_Running_WaitOnFRE in hbastat.c
// - Set FRE
// - Wait for 50ms for FR to reflect running status
// - Move to WaitOnBSYDRQ
//------------------------------------------------------------------------------
static BOOL P_Running_WaitOnFRE(AHCI_MEMORY_REGISTERS *abar,ULONG portno,AHCI_PORT_EXT *portExt)
{
	// get port address
	AHCI_PORT *port = &abar->PortList[portno];	

	AHCI_COMMAND cmd;

WaitOnFRE_Start:
	// Set FRE
	cmd.AsUlong = port->CMD.AsUlong;
	cmd.FRE = 1;
	port->CMD.AsUlong = cmd.AsUlong;
	
	cmd.AsUlong = port->CMD.AsUlong;
	// Move to WaitOnBSYDRQ
	if(cmd.FR == 1)
	{
		// P_Running_WaitOnFRE Success
		return P_Running_WaitOnBSYDRQ(abar,portno,portExt);
	}
	// Move to WaitOnBSYDRQ
	else
	{
		if(portExt->StateFRECount > 5)
		{
			// P_Running_WaitOnFRE timed out
			return P_Running_WaitOnBSYDRQ(abar,portno,portExt);
		}
		// Wait for 50ms for FR to reflect running status
		else
		{
			// P_Running_WaitOnFRE still waiting
			portExt->StateFRECount++;
	
			// wait 10 milliseconds
			delay(10);
			goto WaitOnFRE_Start;
		}
	}
}


//------------------------------------------------------------------------------
// corresponds to StorAhci function ForceGenxSpeed in hbastat.c
//------------------------------------------------------------------------------
static __inline VOID ForceGenxSpeed(AHCI_MEMORY_REGISTERS *abar,ULONG portno,ULONG GenNumber)
{
	// get port address
	AHCI_PORT *port = &abar->PortList[portno];	
	
	if(GenNumber == 0)
	{
		return;
	}
	
	UCHAR spd;
	if(GenNumber <= abar->CAP.ISS)
	{
		spd = (UCHAR)GenNumber;
	}
	else
	{
		spd = (UCHAR)abar->CAP.ISS;	
	}

	AHCI_SERIAL_ATA_CONTROL sctl;
	sctl.AsUlong = port->SCTL.AsUlong;
	if(sctl.SPD == spd)
	{
		return;
	}
	
	// Set speed limitation
	sctl.SPD = spd;
	port->SCTL.AsUlong = sctl.AsUlong;
	// Reset the port
	AhciCOMRESET(abar,portno);
	// and clear out SERR
	AHCI_SERIAL_ATA_ERROR serr;
	serr.AsUlong = (ULONG)~0;
	port->SERR.AsUlong = serr.AsUlong;

	return;
}


//------------------------------------------------------------------------------
// corresponds to StorAhci function P_Running_WaitWhileDET1 in hbastat.c
// Waiting on establishment of link level communications phase.
// A '1' should become a '3'. If it doesn't, help it along.
// NOTE: When a COMINIT is received, the PxSSTS.DET field shall be set to 1h.
//       That means a device is detected, but communications has not finished
//       Polled 100 times in 1 second
// - If DET moves from 1 to 3, go to WaitOnFRE
// - Wait for 1 second for DET to become 3
// - After 1 second of waiting, force the controller to 150MB/s speeds and go to
//   WaitOnDET3
//------------------------------------------------------------------------------
static BOOL P_Running_WaitWhileDET1(AHCI_MEMORY_REGISTERS *abar,ULONG portno,AHCI_PORT_EXT *portExt)
{
	// get port address
	AHCI_PORT *port = &abar->PortList[portno];		
	
	AHCI_SERIAL_ATA_STATUS ssts;

WaitWhileDET1_Start:
	// If DET moves from 1 to 3, go to WaitOnFRE
	ssts.AsUlong = port->SSTS.AsUlong;

	if(ssts.DET == 3)
	{
		// P_Running_WaitWhileDET1 done
		return P_Running_WaitOnFRE(abar,portno,portExt);
	}
	else
	{
		// After 1 second of waiting, force the controller to 150MB/s speeds and
		// go to WaitOnDET3
		if(portExt->StateDET1Count > 100)
		{
			// A very wise woman once taught me this trick
			// it is possible that the device is not handling speed negotation
			// very well
			// help it out by allowing only 150MB/s
			ForceGenxSpeed(abar,portno,1);
	
			// P_Running_WaitWhileDET1 timed out, speed stepping down
	
			// wait 10 milliseconds
			delay(10);
			return P_Running_WaitOnDET3(abar,portno,portExt);
		}
		// Wait for DET to become 3 for 1 second
		else
		{
			// P_Running_WaitWhileDET1 still waiting
			portExt->StateDET1Count++;
			
			// wait 10 milliseconds
			delay(10);
			goto WaitWhileDET1_Start;
		}
	}
}


//------------------------------------------------------------------------------
// corresponds to StorAhci function P_Running_WaitOnDET in hbastat.c
// Search for Device Activity phase.  Use DET and IPM for any signs of life as
// defined in AHCI 1.2 section 10.1.2 and 10.3.1.
// Polled 3 times in 30 ms
// - When looking for device presence, seeing link level power managment shall
//   count as device present as per 10.3.1
// - Wait for 1 second for DET to show signs of life
// - After 30 ms give up
// - When looking for device presence, seeing link level power managment shall
//   count as device present as per 10.3.1
// - Otherwise continue on to WaitOnDET1
// - If DET == 3 we are ready for WaitOnFRE already.
//------------------------------------------------------------------------------
static BOOL P_Running_WaitOnDET(AHCI_MEMORY_REGISTERS *abar,ULONG portno,AHCI_PORT_EXT *portExt)
{
	// get port address
	AHCI_PORT *port = &abar->PortList[portno];	
	
	// 30 milliseconds
	UCHAR waitMaxCount = 30;

	AHCI_SERIAL_ATA_STATUS ssts;

WaitOnDET_Start:
	// Look for signs of life while DET == 0
	ssts.AsUlong = port->SSTS.AsUlong;

	// When a COMRESET is sent to the device the PxSSTS.DET field shall be cleared
	// to 0h.
	if(ssts.DET == 0)
	{
		// When looking for device presence, seeing link level power managment
		// shall count as device present as per 10.3.1
		if((ssts.IPM == 0x2) || (ssts.IPM == 0x6) || (ssts.IPM == 0x8))
		{
			// P_Running_WaitOnDET is 0 w/ LPM activity, goto FRE
			return P_Running_WaitOnFRE(abar,portno,portExt);
		}
		// After 30ms give up (spec requires 10ms, use 30ms for safety). There is
		// no device connected.
		// From SATA-IO spec 3.0 - 15.2.2.2:
		// If a device is present, the Phy shall detect device presence within 10
		// ms of a power-on reset (i.e. COMINIT shall be returned within 10 ms of
		// an issued COMRESET).
		else if(portExt->StateDETCount > waitMaxCount)
		{
			// P_Running_WaitOnDET Timed out, No Device
			return P_Running_StartFailed(abar,portno);
		}
		// Wait for 30ms for DET to show signs of life
		else
		{
			// P_Running_WaitOnDET is 0, still waiting
			portExt->StateDETCount++;
			
			// wait 10 milliseconds
			delay(10);
			goto WaitOnDET_Start;
		}
	}
	else
	{
		// Look for signs of life while DET == 1
		if(ssts.DET == 1)
		{
			// When looking for device presence, seeing link level power managment
			// shall count as device present as per 10.3.1
			if((ssts.IPM == 0x2) || (ssts.IPM == 0x6) || (ssts.IPM == 0x8))
			{
				// P_Running_WaitOnDET is 1 w/ LPM activity, goto FRE
				return P_Running_WaitOnFRE(abar,portno,portExt);
			}
			// Otherwise continue on to WaitOnDET1
			else
			{
				// P_Running_WaitOnDET is 1
				return P_Running_WaitWhileDET1(abar,portno,portExt);
			}
		
		}
		// If DET == 3 we are ready for WaitOnFRE already.
		else if(ssts.DET == 3)
		{
			// P_Running_WaitOnDET is 3
			return P_Running_WaitOnFRE(abar,portno,portExt);
		}
		else
		{
			// P_Running_WaitOnDET is Bogus, aborting
			return P_Running_StartFailed(abar,portno);
		}
	}
}


//------------------------------------------------------------------------------
// corresponds to StorAhci function P_Running in hbastat.c
// The purpose of this function is to verify and drive the Start Channel state
// machine
// - Is the port somehow already running?
// - Make sure the device knows it is supposed to be spun up
// - Check to make sure that FR and CR are both 0. Attempt to bring the controller
//   into a consistent state by stopping the controller.
// - CMD.ST has to be set, when that happens check to see that PxSSTS.DET is not
//   ?4h?
//------------------------------------------------------------------------------
static BOOL P_Running(AHCI_MEMORY_REGISTERS *abar,ULONG portno,AHCI_PORT_EXT *portExt)
{
	// get port address
	AHCI_PORT *port = &abar->PortList[portno];

	// check if the port is already running
	AHCI_COMMAND cmd;
	cmd.AsUlong = port->CMD.AsUlong;
	if(cmd.ST == 1 && cmd.CR == 1 && cmd.FRE == 1 && cmd.FR == 1)
	{
		// the port is already running
		return TRUE;		
	}
	
	// Make sure the device knows it is supposed to be spun up
	cmd.SUD = 1;
	port->CMD.AsUlong = cmd.AsUlong;
	
	// Check to make sure that FR and CR are both 0. If not then ST and/or FRE are
	// 0 which is a bad scenario.
	if((cmd.FR == 1 && cmd.FRE == 0) || (cmd.CR == 1 && cmd.ST == 0))
	{
		// Attempt to bring the controller into a consistent state by stopping the
		// controller.
		if(P_NotRunning(abar,portno) == FALSE)
		{
			// Issue COMRESET to recover
			AhciCOMRESET(abar,portno);
		}
	}

	// CMD.ST has to be set, when that happens check to see that PxSSTS.DET is not
	// ?4h?
	AHCI_SERIAL_ATA_STATUS ssts;
	ssts.AsUlong = port->SSTS.AsUlong;
	if(ssts.DET == 0x04)
	{
		// Phy in offline mode as a result of the interface being disabled or
		// running in a BIST loopback mode
		return P_Running_StartFailed(abar,portno);
	}

	// Ok, run the Start Channel State Machine
	return P_Running_WaitOnDET(abar,portno,portExt);
}


//------------------------------------------------------------------------------
// corresponds to StorAhci function AhciHwFindAdapter in entrypts.c
//------------------------------------------------------------------------------
static int AhciHwFindAdapter(AHCI_HBA_EXT *HbaExt)
{
	// get AHCI base address of HBA
	AHCI_MEMORY_REGISTERS *abar = HbaExt->ABAR_Address;
	
	// Turn on AE (AHCI 1.1 Section 10.1.2 - 1)
	AHCI_Global_HBA_CONTROL ghc;
	ghc.AsUlong = abar->GHC.AsUlong;
	if(ghc.AE == 1)
	{
		// Microsoft only resets the controller if AHCI enable is set, if it is
		// clear the do not reset it, maybe because the controller is not
		// configured if AE bit is not set
		if(AhciAdapterReset(abar) != 0)
		{
			return 1;
		}
	}

	// AE is 0. Either through power up or reset we are now pretty sure the
	// controller is in 5.2.2.1 H:Init
	ghc.AsUlong = 0;
	ghc.AE = 1;
	abar->GHC.AsUlong = ghc.AsUlong;

	// get implemented ports
	ULONG pi = abar->PI;
	
	// AHCI specification requires that at least one bit is set in PI register. In
	// other words, at least one port must be implemented.
	if(pi == 0)
	{
		printf("AhciHwFindAdapter Error: No implemented ports present!\n");
		return 2;	
	}

	// enable interrupts from the HBA
	// Turn on IE, pending interrupts will be cleared when port starts
	ghc.IE = 1;
	abar->GHC.AsUlong = ghc.AsUlong;

	// search all possible 32 ports for devices
	int sig;
	int i;
	for(i = 0; i < 32; i++)
	{
		// get AHCI port extension
		AHCI_PORT_EXT *portExt = &HbaExt->PortExt[i];
		
		// check if port is implemented
		if((pi & 1) == 0)
		{
			// port is not implemented, get next port
			goto label_check_next_port;
		}

		// port is implemented
		HbaExt->PortExt[i].PortImplemented = 1;
		HbaExt->PortsImplemented++;

		// set values to no device connected and no device detected first, if
		// anything goes wrong we can still display the port with the device type
		// "No device detected"
		HbaExt->PortExt[i].DeviceConnected = 0;
		strcpy(HbaExt->PortExt[i].DeviceType,"No device detected");

		// initialize port
		if(AhciPortInitialize(abar,i,HbaExt->pSystemMemory) != 0)
		{
			// port initialization failed, get next port
			goto label_check_next_port;
		}

		// set initial port state and state counter values
		portExt->StateDETCount = 0;
		portExt->StateDET1Count = 0;
		portExt->StateDET3Count = 0;
		portExt->StateFRECount = 0;
		portExt->StateBSYDRQCount = 0;

		// get port into running state
		if(P_Running(abar,i,portExt) == FALSE)
		{
			// port start failed, get next port
			goto label_check_next_port;			
		}

		// Offset 24h: PxSIG ? Port x Signature
		// get device signature
		sig = HbaExt->ABAR_Address->PortList[i].SIG.AsUlong;
		if(sig == 0x00000101 || sig == 0xEB140101)
		{
			// SATA or SATAPI device found
				
			// set device connected on this port to 1
			HbaExt->PortExt[i].DeviceConnected = 1;
				
			// copy corresponding device type string
			if(sig == 0x00000101)
			{
				strcpy(HbaExt->PortExt[i].DeviceType,"Hard Disk");

				// send ATA IDENTIFY DEVICE command
				GetIdentifyData(HbaExt->ABAR_Address,i,0,HbaExt->PortExt[i].ModelNumber,HbaExt->PortExt[i].FirmwareRevision,HbaExt->PortExt[i].SerialNumber,&HbaExt->PortExt[i].SecurityStatus,&HbaExt->PortExt[i].MasterPwIdentifier,&HbaExt->PortExt[i].NormalEraseTime,&HbaExt->PortExt[i].EnhancedEraseTime);					
			}
			else if(sig == 0xEB140101)
			{
				strcpy(HbaExt->PortExt[i].DeviceType,"CDROM");

				// send ATAPI IDENTIFY PACKET DEVICE command
				GetIdentifyData(HbaExt->ABAR_Address,i,1,HbaExt->PortExt[i].ModelNumber,HbaExt->PortExt[i].FirmwareRevision,HbaExt->PortExt[i].SerialNumber,&HbaExt->PortExt[i].SecurityStatus,&HbaExt->PortExt[i].MasterPwIdentifier,&HbaExt->PortExt[i].NormalEraseTime,&HbaExt->PortExt[i].EnhancedEraseTime);
			}

			// increment the connected devices for this AHCI controller
			HbaExt->DevicesConnected++;
		}

label_check_next_port:		
		// advance to next port
		pi >>= 1;
	}

	return 0;
}


//------------------------------------------------------------------------------
// AHCI initialization
//------------------------------------------------------------------------------
int AhciInit(AHCI_HBA_EXT **HbaExt,unsigned short int *foundControllers)
{
	// explicit calibration by using zero delay
	// this will cause to calibrate the TSC (Time Stamp Counter) against the PIT
	// (Programmable Interval Timer)
	if(delay(0) != 0)
	{
		printf("AhciInit Error: Can't calibrate TSC against PIT!\n");
		return 1;
	}

	// find all AHCI controllers on the system
	if(AhciFindBaseAddress(HbaExt,foundControllers) != 0)
	{
		printf("AhciInit Error: Can't find AHCI base address!\n");
		return 2;	
	}

	// do this for all found controllers
	unsigned short int i;
	for(i = 0; i < *foundControllers; i++)
	{
		// save AHCI BIOS configuration
		SaveAhciBiosConfig(&(*HbaExt)[i]);

		// allocate aligned system memory for 8 PRDTs per port
		unsigned long int memSize = (40 << 10) + (32 << 13) + (32 << 8);
		(*HbaExt)[i].pSystemMemory = (unsigned char*)aligned_malloc(memSize,1024);
		if((*HbaExt)[i].pSystemMemory == NULL)
		{
			printf("AhciInit Error: Can't allocate aligned system memory for 8 PRDTs!\n");
			return 3;
		}
		
		// turn on AE, reset HBA, initialize ports etc.
		if(AhciHwFindAdapter(&(*HbaExt)[i]) != 0)
		{
			// error handling inside
			return 4;
		}
	}

	return 0;
}


//------------------------------------------------------------------------------
// AHCI uninitialization
//------------------------------------------------------------------------------
void AhciUninit(AHCI_HBA_EXT **HbaExt,unsigned short int *foundControllers)
{
	// restore AHCI BIOS configuration for all controllers
	unsigned short int i;
	for(i = 0; i < *foundControllers; i++)
	{
		// free aligned system memory
		if((*HbaExt)[i].pSystemMemory != NULL)
		{
			aligned_free((*HbaExt)[i].pSystemMemory);
			(*HbaExt)[i].pSystemMemory = NULL;
		}

		// restore AHCI BIOS configuration
		RestoreAhciBiosConfig(HbaExt[i]);
	}

	// free AHCI_HBA_EXT structures
	if(*HbaExt != NULL)
	{
		free(*HbaExt);
		*HbaExt = NULL;
	}

	// set found controllers to zero
	*foundControllers = 0;
}


//------------------------------------------------------------------------------
// AHCI raw data read
//------------------------------------------------------------------------------
int AhciRawRead(unsigned long drive,unsigned long long sector,unsigned long byte_offset,unsigned long long byte_len,unsigned long long buf,unsigned long write)
{
	unsigned char *buffer;
	USHORT count;	

	// AHCI_HBA_EXT structure is not present
	// no valid AHCI controller or port is selected
	// read blocks only from the selected BIOS drive
	if(ahcig.HbaExt == NULL || ahcig.SelectedController == -1 || ahcig.SelectedPort == -1 || ahcig.SelectedDrive != drive)
	{
		// no printf, because we may use another BIOS read function
		return 1;
	}

	// buf address should be valid
	// write variable should be GRUB_READ which is defined as follows:
	// #define GRUB_READ 0xedde0d90
	if(buf == 0 || write != 0xedde0d90)
	{
		// no printf, because we may use another BIOS read function
		return 2;
	}

	// add map start sector to sector if drive mapping is active
	if(ahcig.MapActive == 1)
	{
		sector += ahcig.StartSector;
	}

	// byte_len is a multiple of sector size and byte_offset is zero
	if(byte_len % 512 == 0 && byte_offset == 0)
	{
		// AHCI read
		if(AhciRead(ahcig.HbaExt[ahcig.SelectedController].ABAR_Address,ahcig.SelectedPort,sector,buf,(USHORT)(byte_len >> 9)) != 0)
		{
			printf("AhciRawRead Error: Can't read sectors!\n");
			return 3;
		}
	}
	else
	{
		// calculate number of sectors to read
		count = (byte_len / 512) + 1;
		// allocate memory for sector read
		buffer = (unsigned char*)aligned_malloc(count * 512,4096);
		
		// AHCI read
		if(AhciRead(ahcig.HbaExt[ahcig.SelectedController].ABAR_Address,ahcig.SelectedPort,sector,(unsigned long long)(unsigned int)(unsigned char*)buffer,count) != 0)
		{
			// free memory
			aligned_free(buffer);
			printf("AhciRawRead Error: Can't read sectors!\n");
			return 4;
		}

		// copy memory to original buffer
		grub_memmove64(buf + byte_offset,(unsigned long long)(unsigned int)(unsigned char*)buffer,byte_len);
		// free memory
		aligned_free(buffer);
	}

	return 0;
}


//------------------------------------------------------------------------------
// show all found controllers, ports and connected devices
//------------------------------------------------------------------------------
void AhciShowAllDevices(AHCI_HBA_EXT *HbaExt,unsigned short int foundControllers)
{
	// show all found controllers, ports and connected devices
	unsigned short int i;
	for(i = 0; i < foundControllers; i++)
	{
		printf("%2.1d) AHCI Controller VendorID#%.4X, DeviceID#%.4X, Base Address#%.8X\n"
			   "    Bus#%.2X, Device#%.2X, Function#%.2X: %.2d Ports, %.2d Devices\n"
			   ,i,HbaExt[i].PciVendorID,HbaExt[i].PciDeviceID,HbaExt[i].ABAR_Address
			   ,HbaExt[i].Bus,HbaExt[i].Func,HbaExt[i].Dev,HbaExt[i].PortsImplemented,HbaExt[i].DevicesConnected);

		// check all ports for devices
		unsigned short int k;
		for(k = 0; k < 32; k++)
		{
			// port is implemented and device is connected
			if(HbaExt[i].PortExt[k].PortImplemented == 1 && HbaExt[i].PortExt[k].DeviceConnected == 1)
			{
				printf("    Port-%.2d: %s, %s %s %s\n",k,HbaExt[i].PortExt[k].DeviceType,HbaExt[i].PortExt[k].ModelNumber,HbaExt[i].PortExt[k].FirmwareRevision,HbaExt[i].PortExt[k].SerialNumber);
			}
			// port is implemented but no device is connected
			else if(HbaExt[i].PortExt[k].PortImplemented == 1 && HbaExt[i].PortExt[k].DeviceConnected == 0)
			{
				printf("    Port-%.2d: %s\n",k,HbaExt[i].PortExt[k].DeviceType);
			}
		}
	}
	printf("\n");
}


//------------------------------------------------------------------------------
// show selected controller, port and connected device
//------------------------------------------------------------------------------
void AhciShowSelectedDevice(AHCI_HBA_EXT *HbaExt,unsigned short int i,unsigned short int k)
{
	printf("%2.1d) AHCI Controller VendorID#%.4X, DeviceID#%.4X, Base Address#%.8X\n"
		   "    Bus#%.2X, Device#%.2X, Function#%.2X: %.2d Ports, %.2d Devices\n"
		   ,i,HbaExt[i].PciVendorID,HbaExt[i].PciDeviceID,HbaExt[i].ABAR_Address
		   ,HbaExt[i].Bus,HbaExt[i].Func,HbaExt[i].Dev,HbaExt[i].PortsImplemented,HbaExt[i].DevicesConnected);

	// port is implemented and device is connected
	if(HbaExt[i].PortExt[k].PortImplemented == 1 && HbaExt[i].PortExt[k].DeviceConnected == 1)
	{
		printf("    Port-%.2d: %s, %s %s %s\n",k,HbaExt[i].PortExt[k].DeviceType,HbaExt[i].PortExt[k].ModelNumber,HbaExt[i].PortExt[k].FirmwareRevision,HbaExt[i].PortExt[k].SerialNumber);
	}
	// port is implemented but no device is connected
	else if(HbaExt[i].PortExt[k].PortImplemented == 1 && HbaExt[i].PortExt[k].DeviceConnected == 0)
	{
		printf("    Port-%.2d: %s\n",k,HbaExt[i].PortExt[k].DeviceType);
	}
	printf("\n");
}

// restore GCC options
#pragma GCC pop_options

