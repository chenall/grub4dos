
// save GCC options
#pragma GCC push_options
// turn off optimization
#pragma GCC optimize ("O0")

//------------------------------------------------------------------------------
//
// NVMe driver for GRUB
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
// Many thanks to OFA for their excellent NVMe Driver sources.
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// our custom stuff starts here
//------------------------------------------------------------------------------

#include "shared.h"
#include "nvme.h"

#define printf grub_printf
#define malloc grub_malloc
#define free grub_free
#define strlen grub_strlen
#define strcpy grub_strcpy

// initialize structure before use
NVME_GLOBALS nvmeg = {0,0,0x80,-1,0,0,0,0,0};


//--------------------------------------------------------------------------------
// functions that have to be declared
//--------------------------------------------------------------------------------
static VOID NVMeRunning(PNVME_DEVICE_EXTENSION pAE);


//------------------------------------------------------------------------------
// acquire the specified spin lock
//------------------------------------------------------------------------------
static VOID StorPortAcquireSpinLock(PVOID DeviceExtension,STOR_SPINLOCK SpinLock,PVOID LockContext,PSTOR_LOCK_HANDLE LockHandle)
{
	return;
}


//------------------------------------------------------------------------------
// release a spinlock
//------------------------------------------------------------------------------
static VOID StorPortReleaseSpinLock(PVOID DeviceExtension,PSTOR_LOCK_HANDLE LockHandle)
{
	return;
}


//------------------------------------------------------------------------------
// notify the port driver of certain events
//------------------------------------------------------------------------------
static VOID StorPortNotification(SCSI_NOTIFICATION_TYPE NotificationType,PVOID DeviceExtension,...)
{
	return;
}


//------------------------------------------------------------------------------
// indicates whether a doubly linked list of LIST_ENTRY structures is empty
//------------------------------------------------------------------------------
static BOOLEAN IsListEmpty(const LIST_ENTRY *ListHead)
{
    return (BOOLEAN)(ListHead->Flink == ListHead);
}


//------------------------------------------------------------------------------
// removes an entry from the beginning of a doubly linked list
//------------------------------------------------------------------------------
static PLIST_ENTRY RemoveHeadList(PLIST_ENTRY ListHead)
{
    PLIST_ENTRY Flink;
    PLIST_ENTRY Entry;

    Entry = ListHead->Flink;
    Flink = Entry->Flink;
    ListHead->Flink = Flink;
    Flink->Blink = ListHead;
    return Entry;
}


//------------------------------------------------------------------------------
// writes a ULONG value to the specified register address
//------------------------------------------------------------------------------
static __inline VOID WRITE_REGISTER_ULONG(volatile ULONG *Register,ULONG Value)
{
    *Register = Value;
    return;
}


//------------------------------------------------------------------------------
// reads a ULONG value from the specified register address
//------------------------------------------------------------------------------
static __inline ULONG READ_REGISTER_ULONG(volatile ULONG *Register)
{
	return *Register;
}


#define StorPortWriteRegisterUlong(h, r, v) WRITE_REGISTER_ULONG(r, v)
#define StorPortReadRegisterUlong(h, r) READ_REGISTER_ULONG(r)


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
// find NVMe base address on any Mass Storage NVMe Controller
// arg0: pointer to a PNVME_DEVICE_EXTENSION array
// arg1: number of controllers found
//------------------------------------------------------------------------------
static int NVMeFindBaseAddress(NVME_DEVICE_EXTENSION **DevExt,unsigned short int *foundControllers)
{
	// PCI config space info table
	unsigned char infoTable[0x28];
	unsigned short int contr;
	unsigned short int lastBusNum;
	unsigned short int bus;
	unsigned short int dev;
	unsigned short int func;
	unsigned short int reg;

	// set NVME_DEVICE_EXTENSION structure pointer to zero
	*DevExt = NULL;
	// set found controllers to zero
	*foundControllers = 0;  
	// actual number of controllers, this is incremented after a controller is found
	contr = 0;
	// get last PCI bus number
	lastBusNum = GetLastPCIBusNumber();

	// do this for all possible buses, normally there is not much more as bus 0 and 1
	for(bus = 0x00; bus <= lastBusNum; bus++)
	{
		// do this for all devices
		for(dev = 0x00; dev <= 0x1F; dev++)
		{
			// do this for all functions
			for(func = 0x00; func <= 0x07; func++)
			{
				// zero info table
				memset(infoTable,0,0x28);

				// read all registers from pci configuration space and fill info
				// table, for performance increase we read only the 1st 0x28 bytes
				// of pci config space, this is enough to get the port addresses
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

				// Mass Storage NVMe Controller detected
				// Base Class 0x01
				// -> Mass Storage Controller
				// Sub Class 0x08
				// Interface 0x02
				// -> NVMe Controller
				if(infoTable[0x0B] == 0x01 && infoTable[0x0A] == 0x08 && infoTable[0x09] == 0x02)
				{
					// if bit 7 of the Header Type is 0 this is a multi function device
					if((infoTable[0x0E] & 0x7F) == 0)
					{
						// set NVMe base address to zero
						unsigned long int ulNVMeBaseAddress = 0;

						// Bit 0 in all Base Address registers is read-only and
						// used to determine whether the register maps into Memory
						// or I/O Space. Base Address registers that map to Memory
						// Space must return a 0 in bit 0. Base Address registers
						// that map to I/O Space must return a 1 in bit 0.
						// register maps into memory space
						if((infoTable[0x10] & 1) == 0)
						{
							// get NVMe base address from info table
							ulNVMeBaseAddress = (infoTable[0x13] << 24) + (infoTable[0x12] << 16) + ((infoTable[0x11] & 0xE0) << 8);
						}

						// if the address is zero leave the function for loop
						if(ulNVMeBaseAddress == 0) break;

						// allocate space for one more NVME_DEVICE_EXTENSION structures
						*DevExt = (NVME_DEVICE_EXTENSION*)realloc(*DevExt,(contr + 1) * sizeof(NVME_DEVICE_EXTENSION));
						if(*DevExt == NULL)
						{
							return 1;
						}

						// zero added NVME_DEVICE_EXTENSION structure
						memset(*DevExt + contr,0,sizeof(NVME_DEVICE_EXTENSION));

						// save NVMe base address
						(*DevExt)[contr].pCtrlRegister = (NVMe_CONTROLLER_REGISTERS*)ulNVMeBaseAddress;
						// save bus, function and device number
						(*DevExt)[contr].Bus = bus;
						(*DevExt)[contr].Dev = dev;
						(*DevExt)[contr].Func = func;
						// convert PCI table vendor ID to WORD
						(*DevExt)[contr].PciVendorID = (infoTable[1] << 8) | infoTable[0];
						// convert PCI table device ID to WORD
						(*DevExt)[contr].PciDeviceID = (infoTable[3] << 8) | infoTable[2];
						
						// increment number of valid controllers found
						contr++;
					}
				}
label_check_next_func:
				;
			}
		}
	}

	// return number of found NVMe controllers
	*foundControllers = contr;

	return 0;
}


//------------------------------------------------------------------------------
// NVMeCrashDelay
//
// @brief Will delay, by spinning, the amount of time specified without relying
//        on API not available in crashdump mode.  Should not be used outside of
//        crashdump mode.
//
// @param delayInUsec - uSec to delay
//
// @return VOID
//------------------------------------------------------------------------------
static VOID NVMeCrashDelay(ULONG delayInUsec,BOOLEAN ntldrDump)
{
	delay(delayInUsec / 1000);
}


//------------------------------------------------------------------------------
// NVMeStallExecution
//
// @brief Stalls for the # of usecs specified
//
// @param pAE - Pointer to adapter device extension.
// @param microSeconds - time to stall
//
// @return VOID
//------------------------------------------------------------------------------
static VOID NVMeStallExecution(PNVME_DEVICE_EXTENSION pAE,ULONG microSeconds)
{
	NVMeCrashDelay(microSeconds,pAE->ntldrDump);
}


//------------------------------------------------------------------------------
// deallocates a range of noncached memory in the nonpaged portion of the system address space
//------------------------------------------------------------------------------
static ULONG StorPortPatchFreeContiguousMemorySpecifyCache(PVOID HwDeviceExtension,PVOID BaseAddress,SIZE_T NumberOfBytes,MEMORY_CACHING_TYPE CacheType)
{
	aligned_free(BaseAddress);

	return STOR_STATUS_SUCCESS;
}


//------------------------------------------------------------------------------
// frees a block of memory that was previously allocated
//------------------------------------------------------------------------------
static ULONG StorPortFreePool(PVOID HwDeviceExtension,PVOID BufferPointer)
{
	aligned_free(BufferPointer);
	return STOR_STATUS_SUCCESS;
}


//------------------------------------------------------------------------------
// allocates a range of physically contiguous noncached, nonpaged memory
//------------------------------------------------------------------------------
static ULONG StorPortPatchAllocateContiguousMemorySpecifyCacheNode(PVOID HwDeviceExtension,SIZE_T NumberOfBytes,PHYSICAL_ADDRESS LowestAcceptableAddress,PHYSICAL_ADDRESS HighestAcceptableAddress,PHYSICAL_ADDRESS BoundaryAddressMultiple,MEMORY_CACHING_TYPE CacheType,NODE_REQUIREMENT PreferredNode,PVOID* BufferPointer)
{
	*BufferPointer = aligned_malloc(NumberOfBytes,4096);
	if(*BufferPointer == NULL)
	{
		return STOR_STATUS_INSUFFICIENT_RESOURCES;
	}

	return STOR_STATUS_SUCCESS;
}


//------------------------------------------------------------------------------
// allocates a block of non-contiguous, non-paged pool memory
//------------------------------------------------------------------------------
static ULONG StorPortAllocatePool(PVOID HwDeviceExtension,ULONG NumberOfBytes,ULONG Tag,PVOID *BufferPointer)
{
	*BufferPointer = aligned_malloc(NumberOfBytes,4096);
	if(*BufferPointer == NULL)
	{
		return STOR_STATUS_INSUFFICIENT_RESOURCES;
	}

	return STOR_STATUS_SUCCESS;
}


//------------------------------------------------------------------------------
// notifies the port driver that the adapter is currently busy
//------------------------------------------------------------------------------
static BOOLEAN StorPortBusy(PVOID HwDeviceExtension,ULONG RequestsToComplete)
{
	return TRUE;
}


//------------------------------------------------------------------------------
// notifies the port driver that the adapter is no longer busy
//------------------------------------------------------------------------------
static BOOLEAN StorPortReady(PVOID HwDeviceExtension)
{
	return TRUE;
}


//------------------------------------------------------------------------------
// retrieves the message signaled interrupt (MSI) information for the specified message
//------------------------------------------------------------------------------
static ULONG StorPortGetMSIInfo(PVOID HwDeviceExtension,ULONG MessageId,PMESSAGE_INTERRUPT_INFORMATION InterruptInfo)
{
	return STOR_STATUS_NOT_IMPLEMENTED;
}


//------------------------------------------------------------------------------
// initialize the performance optimizations 
//------------------------------------------------------------------------------
static ULONG StorPortInitializePerfOpts(PVOID HwDeviceExtension,BOOLEAN Query,PPERF_CONFIGURATION_DATA PerfConfigData)
{
	return STOR_STATUS_NOT_IMPLEMENTED;
}


//------------------------------------------------------------------------------
// this routine calls the callback routine provided by the caller
//------------------------------------------------------------------------------
static BOOLEAN StorPortEnablePassiveInitialization(PVOID DeviceExtension,PHW_PASSIVE_INITIALIZE_ROUTINE HwPassiveInitializeRoutine)
{
	return HwPassiveInitializeRoutine(DeviceExtension);
}


//------------------------------------------------------------------------------
// initialize a StorPort DPC
//------------------------------------------------------------------------------
static VOID StorPortInitializeDpc(PVOID DeviceExtension,PSTOR_DPC Dpc,PHW_DPC_ROUTINE HwDpcRoutine)
{
	Dpc->Dpc.DeferredRoutine = HwDpcRoutine;
}


//------------------------------------------------------------------------------
// issues a deferred procedure call
//------------------------------------------------------------------------------
static BOOLEAN StorPortIssueDpc(PVOID DeviceExtension,PSTOR_DPC Dpc,PVOID SystemArgument1,PVOID SystemArgument2)
{
	PHW_DPC_ROUTINE ProcAdd;

	ProcAdd = (PHW_DPC_ROUTINE)Dpc->Dpc.DeferredRoutine;
	(ProcAdd)(NULL,DeviceExtension,SystemArgument1,SystemArgument2);

	return TRUE;
}


//------------------------------------------------------------------------------
// retrieves the current processor number from the kernel
//------------------------------------------------------------------------------
static ULONG StorPortPatchGetCurrentProcessorNumber(PVOID HwDeviceExtension,PPROCESSOR_NUMBER ProcNumber)
{
	if(ProcNumber == NULL)
	{
		return STOR_STATUS_INVALID_PARAMETER;
	}

	ProcNumber->Group = 0;
	ProcNumber->Number = 0;
	ProcNumber->Reserved = 0;

	return STOR_STATUS_SUCCESS;
}


//------------------------------------------------------------------------------
// initialize the head of a doubly linked list
// C:\WinDDK\7600.16385.1\inc\ddk\wdm.h
//------------------------------------------------------------------------------
static VOID InitializeListHead(PLIST_ENTRY ListHead)
{
	ListHead->Flink = ListHead->Blink = ListHead;
}


//------------------------------------------------------------------------------
// insert an entry at the tail of a doubly linked list 
// C:\WinDDK\7600.16385.1\inc\ddk\wdm.h
//------------------------------------------------------------------------------
static VOID InsertTailList(PLIST_ENTRY ListHead,PLIST_ENTRY Entry)
{
	PLIST_ENTRY Blink;

	Blink = ListHead->Blink;
	Entry->Flink = ListHead;
	Entry->Blink = Blink;
	Blink->Flink = Entry;
	ListHead->Blink = Entry;
}


//------------------------------------------------------------------------------
// NVMeGetPhysAddr
//
// @brief Helper routine for converting virtual address to physical address by
//        calling StorPortGetPhysicalAddress.
//
// @param pAE - Pointer to hardware device extension
// @param pVirtAddr - Virtual address to convert
//
// @return STOR_PHYSICAL_ADDRESS
//     Physical Address - If all resources are allocated and initialized
//     NULL - If anything goes wrong
//------------------------------------------------------------------------------
static STOR_PHYSICAL_ADDRESS NVMeGetPhysAddr(PNVME_DEVICE_EXTENSION pAE,PVOID pVirtAddr)
{
	STOR_PHYSICAL_ADDRESS PhysAddr;

	//PhysAddr.QuadPart = (ULONGLONG)pVirtAddr;
	PhysAddr.LowPart = (ULONG)pVirtAddr;
	PhysAddr.HighPart = 0;

	return PhysAddr;
}


//------------------------------------------------------------------------------
// NVMeLogError
//
// @brief Logs error to storport.
//
// @param pAE - Pointer to device extension
// @param ErrorNum - Code to log.  The error code will show up in the 4th DWORD
//        (starting with 0) in the system event log detailed data
//
// @return VOID
//------------------------------------------------------------------------------
static VOID NVMeLogError(PNVME_DEVICE_EXTENSION pAE,ULONG ErrorNum)
{
	StorPortDebugPrint(INFO,"NvmeLogError: logging error (0x%x)\n",ErrorNum);
}


//------------------------------------------------------------------------------
// NVMeCallArbiter
//
// @brief Calls the init state machine arbiter either via timer callback or
//        directly depending on crashdump or not
//
// @param pAE - Pointer to adapter device extension.
//
// @return VOID
//------------------------------------------------------------------------------
static VOID NVMeCallArbiter(PNVME_DEVICE_EXTENSION pAE)
{
	if(pAE->ntldrDump == FALSE)
	{
		// we can not call the function NVMeRunning, this leads to a stack overflow
		// because we call into NVMeRunning again before it has finished the last task
		//NVMeRunning(pAE);
	}
	else
	{
		// NoOp in dump mode because NVMeRunningStartAttempt() steps through
		// the initialization state machine in a while loop.
	}
}


//------------------------------------------------------------------------------
// NVMeRunningStartAttempt
//
// @brief NVMeRunningStartAttempt is the entry point of state machine. It is
//        called to initialize and start the state machine. It returns the
//        returned status from NVMeRunning to the callers.
//
// @param pAE - Pointer to adapter device extension.
// @param resetDriven - Boolean to determine if reset driven
// @param pResetSrb - Pointer to SRB for reset
//
// @return BOOLEAN
//     TRUE: When the starting state is completed successfully
//     FALSE: If the starting state had been determined as a failure
//------------------------------------------------------------------------------
static BOOLEAN NVMeRunningStartAttempt(
    PNVME_DEVICE_EXTENSION pAE,
    BOOLEAN resetDriven,
#if (NTDDI_VERSION > NTDDI_WIN7)
    PSTORAGE_REQUEST_BLOCK pResetSrb
#else
    PSCSI_REQUEST_BLOCK pResetSrb
#endif
)
{
    // Set up the timer interval (time per DPC callback)
    pAE->DriverState.CheckbackInterval = STORPORT_TIMER_CB_us;

    // Initializes the state machine and its variables
    pAE->DriverState.DriverErrorStatus = 0;
    pAE->DriverState.NextDriverState = NVMeWaitOnRDY;
    pAE->DriverState.StateChkCount = 0;
    pAE->DriverState.IdentifyNamespaceFetched = 0;
    pAE->DriverState.CurrentNsid = 0;
    pAE->DriverState.InterruptCoalescingSet = FALSE;
    pAE->DriverState.ConfigLbaRangeNeeded = FALSE;
    pAE->DriverState.TtlLbaRangeExamined = 0;
    pAE->DriverState.NumAERsIssued = 0;
    pAE->DriverState.TimeoutCounter = 0;
    pAE->DriverState.resetDriven = resetDriven;
    pAE->DriverState.pResetSrb = pResetSrb;
    pAE->DriverState.VisibleNamespacesExamined = 0;
#if DBG
    pAE->LearningComplete = FALSE;
#endif


    // Zero out SQ cn CQ counters
    pAE->QueueInfo.NumSubIoQCreated = 0;
    pAE->QueueInfo.NumCplIoQCreated = 0;
    pAE->QueueInfo.NumSubIoQAllocFromAdapter = 0;
    pAE->QueueInfo.NumCplIoQAllocFromAdapter = 0;

    // Zero out the LUN extensions and reset the counter as well
    memset((PVOID)pAE->pLunExtensionTable[0],
           0,
           sizeof(NVME_LUN_EXTENSION) * MAX_NAMESPACES);

    // Now, starts state machine by calling NVMeRunning
    // We won't accept IOs until the machine finishes and if it
    // fails to finish we'll never accept IOs and simply log an error
    if (pAE->ntldrDump == FALSE) {
		// the following code was changed, because we have no StorPort timer and poll for completion
		while (pAE->DriverState.NextDriverState != NVMeStateFailed && pAE->DriverState.NextDriverState != NVMeStartComplete)
		{
			NVMeRunning(pAE);
		}
    } else {
        PRES_MAPPING_TBL pRMT = &pAE->ResMapTbl;
        pAE->LearningCores = pRMT->NumActiveCores; // no need to learn cores in dump mode
        
        // In dump mode there is no timer. We have to poll for completion at each state.
        while (pAE->DriverState.NextDriverState != NVMeStateFailed && 
               pAE->DriverState.NextDriverState != NVMeStartComplete) {    
            NVMeRunning(pAE);
            NVMeCrashDelay(pAE->DriverState.CheckbackInterval, pAE->ntldrDump);
        }
    }

    return (TRUE);
} // NVMeRunningStartAttempt


//------------------------------------------------------------------------------
// NVMeFetchRegistry
//
// @brief NVMeFetchRegistry gets called to fetch all the default values from
//        Registry when the driver first loaded. The sub-keys are:
//
//        Namespace: The supported number of Namespace
//        TranSize: Max transfer size in bytes with one request
//        AdQueueEntries: The number of Admin queue entries, 128 by default
//        IoQueueEntries: The number of IO queue entries, 1024 by default
//        IntCoalescingTime: The frequency of interrupt coalescing time in 100
//                           ms increments
//        IntCoalescingEntry: The frequency of interrupt coalescing entries
//
// @param pAE - Device Extension
//
// @return BOOLEAN
//     Returns TRUE when no error encountered, otherwise, FALSE.
//------------------------------------------------------------------------------
static BOOLEAN NVMeFetchRegistry(PNVME_DEVICE_EXTENSION pAE)
{
	return TRUE;
}


//------------------------------------------------------------------------------
// our custom stuff ends here
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// original NVMe OFA driver code starts here
//------------------------------------------------------------------------------


/* Generic Command Status Lookup Table */
SNTI_RESPONSE_BLOCK genericCommandStatusTable[] = {
    /* SUCCESSFUL_COMPLETION - 0x0 */
    {SRB_STATUS_SUCCESS,
        SCSISTAT_GOOD,
        SCSI_SENSE_NO_SENSE,
        SCSI_ADSENSE_NO_SENSE,
        SCSI_ADSENSE_NO_SENSE},

    /* INVALID_COMMAND_OPCODE - 0x1 */
    {SRB_STATUS_ERROR,
        SCSISTAT_CHECK_CONDITION,
        SCSI_SENSE_ILLEGAL_REQUEST,
        SCSI_ADSENSE_ILLEGAL_COMMAND,
        SCSI_ADSENSE_NO_SENSE},

    /* INVALID_FIELD_IN_COMMAND - 0x2 */
    {SRB_STATUS_ERROR,
        SCSISTAT_CHECK_CONDITION,
        SCSI_SENSE_ILLEGAL_REQUEST,
        SCSI_ADSENSE_INVALID_CDB,
        SCSI_ADSENSE_NO_SENSE},

    /* COMMAND_ID_CONFLICT - 0x3 */
    {SRB_STATUS_ERROR,
        SCSISTAT_CHECK_CONDITION,
        SCSI_SENSE_ILLEGAL_REQUEST,
        SCSI_ADSENSE_INVALID_CDB,
        SCSI_ADSENSE_NO_SENSE},

    /* DATA_TRANSFER_ERROR - 0x4 */
    {SRB_STATUS_ERROR,
        SCSISTAT_CHECK_CONDITION,
        SCSI_SENSE_MEDIUM_ERROR,
        SCSI_ADSENSE_NO_SENSE,
        SCSI_ADSENSE_NO_SENSE},

    /* COMMANDS_ABORTED_DUE_TO_POWER_LOSS_NOTIFICATION - 0x5 */
    {SRB_STATUS_ABORTED,
        SCSISTAT_TASK_ABORTED,
        SCSI_SENSE_ABORTED_COMMAND,
        SCSI_ADSENSE_NO_SENSE,
        SCSI_ADSENSE_NO_SENSE},

    /* INTERNAL_DEVICE_ERROR - 0x6 */
    {SRB_STATUS_ERROR,
        SCSISTAT_CHECK_CONDITION,
        SCSI_SENSE_HARDWARE_ERROR,
        SCSI_ADSENSE_INTERNAL_TARGET_FAILURE,
        SCSI_ADSENSE_NO_SENSE},

    /* COMMAND_ABORT_REQUESTED - 0x7 */
    {SRB_STATUS_ABORTED,
        SCSISTAT_TASK_ABORTED,
        SCSI_SENSE_ABORTED_COMMAND,
        SCSI_ADSENSE_NO_SENSE,
        SCSI_ADSENSE_NO_SENSE},

    /* COMMAND_ABORTED_DUE_TO_SQ_DELETION - 0x8 */
    {SRB_STATUS_ABORTED,
        SCSISTAT_TASK_ABORTED,
        SCSI_SENSE_ABORTED_COMMAND,
        SCSI_ADSENSE_NO_SENSE,
        SCSI_ADSENSE_NO_SENSE},

    /* COMMAND_ABORTED_DUE_TO_FAILED_FUSED_COMMAND - 0x9 */
    {SRB_STATUS_ABORTED,
        SCSISTAT_TASK_ABORTED,
        SCSI_SENSE_ABORTED_COMMAND,
        SCSI_ADSENSE_NO_SENSE,
        SCSI_ADSENSE_NO_SENSE},

    /* COMMAND_ABORTED_DUE_TO_MISSING_FUSED_COMMAND - 0xA */
    {SRB_STATUS_ABORTED,
        SCSISTAT_TASK_ABORTED,
        SCSI_SENSE_ABORTED_COMMAND,
        SCSI_ADSENSE_NO_SENSE,
        SCSI_ADSENSE_NO_SENSE},

    /* INVALID_NAMESPACE_OR_FORMAT - 0xB */
    {SRB_STATUS_ERROR,
        SCSISTAT_CHECK_CONDITION,
        SCSI_SENSE_ILLEGAL_REQUEST,
        SCSI_ADSENSE_ILLEGAL_COMMAND,
        SCSI_SENSEQ_INVALID_LUN_ID},

    /* Generic Command Status (NVM Command Set) Lookup Table */

    /* LBA_OUT_OF_RANGE - 0x80 (0xC) */
    {SRB_STATUS_ERROR,
        SCSISTAT_CHECK_CONDITION,
        SCSI_SENSE_ILLEGAL_REQUEST,
        SCSI_ADSENSE_ILLEGAL_BLOCK,
        SCSI_ADSENSE_NO_SENSE},

    /* CAPACITY_EXCEEDED - 0x81 (0xD) */
    {SRB_STATUS_ERROR,
        SCSISTAT_CHECK_CONDITION,
        SCSI_SENSE_MEDIUM_ERROR,
        SCSI_ADSENSE_NO_SENSE,
        SCSI_ADSENSE_NO_SENSE},

    /* NAMESPACE_NOT_READY - 0x82 (0xE) */
    {SRB_STATUS_ERROR,
        SCSISTAT_CHECK_CONDITION,
        SCSI_SENSE_NOT_READY,
        SCSI_ADSENSE_LUN_NOT_READY,
        SCSI_ADSENSE_NO_SENSE}
};

/* Command Specific Status Lookup Table */
SNTI_RESPONSE_BLOCK commandSpecificStatusTable[] = {
    /* COMPLETION_QUEUE_INVALID - 0x0 */
    {SRB_STATUS_ERROR,
        SCSISTAT_CHECK_CONDITION,
        SCSI_SENSE_ILLEGAL_REQUEST,
        SCSI_ADSENSE_NO_SENSE,
        SCSI_ADSENSE_NO_SENSE},

    /* INVALID_QUEUE_IDENTIFIER - 0x1 */
    {SRB_STATUS_ERROR,
        SCSISTAT_CHECK_CONDITION,
        SCSI_SENSE_UNIT_ATTENTION,
        SCSI_ADSENSE_INTERNAL_TARGET_FAILURE,
        SCSI_ADSENSE_NO_SENSE},

    /* MAXIMUM_QUEUE_SIZE_EXCEEDED - 0x2 */
    {SRB_STATUS_ERROR,
        SCSISTAT_CHECK_CONDITION,
        SCSI_SENSE_UNIT_ATTENTION,
        SCSI_ADSENSE_INTERNAL_TARGET_FAILURE,
        SCSI_ADSENSE_NO_SENSE},

    /* ABORT_COMMAND_LIMIT_EXCEEDED - 0x3 */
    {SRB_STATUS_ERROR,
        SCSISTAT_CHECK_CONDITION,
        SCSI_SENSE_ILLEGAL_REQUEST,
        SCSI_ADSENSE_NO_SENSE,
        SCSI_ADSENSE_NO_SENSE},

    /* RESERVED (REQUESTED_COMMAND_TO_ABORT_NOT_FOUND) - 0x4 */
    {SRB_STATUS_ERROR,
        SCSISTAT_CHECK_CONDITION,
        SCSI_SENSE_UNIT_ATTENTION,
        SCSI_ADSENSE_INTERNAL_TARGET_FAILURE,
        SCSI_ADSENSE_NO_SENSE},

    /* ASYNCHRONOUS_EVENT_REQUEST_LIMIT_EXCEEDED - 0x5 */
    {SRB_STATUS_ERROR,
        SCSISTAT_CHECK_CONDITION,
        SCSI_SENSE_UNIT_ATTENTION,
        SCSI_ADSENSE_INTERNAL_TARGET_FAILURE,
        SCSI_ADSENSE_NO_SENSE},

    /* INVALID_FIRMWARE_SLOT - 0x6 */
    {SRB_STATUS_ERROR,
        SCSISTAT_CHECK_CONDITION,
        SCSI_SENSE_UNIT_ATTENTION,
        SCSI_ADSENSE_INTERNAL_TARGET_FAILURE,
        SCSI_ADSENSE_NO_SENSE},

    /* INVALID_FIRMWARE_IMAGE - 0x7 */
    {SRB_STATUS_ERROR,
        SCSISTAT_CHECK_CONDITION,
        SCSI_SENSE_UNIT_ATTENTION,
        SCSI_ADSENSE_INTERNAL_TARGET_FAILURE,
        SCSI_ADSENSE_NO_SENSE},

    /* INVALID_INTERRUPT_VECTOR - 0x8 */
    {SRB_STATUS_ERROR,
        SCSISTAT_CHECK_CONDITION,
        SCSI_SENSE_UNIT_ATTENTION,
        SCSI_ADSENSE_INTERNAL_TARGET_FAILURE,
        SCSI_ADSENSE_NO_SENSE},

    /* INVALID_LOG_PAGE - 0x9 */
    {SRB_STATUS_ERROR,
        SCSISTAT_CHECK_CONDITION,
        SCSI_SENSE_UNIT_ATTENTION,
        SCSI_ADSENSE_INTERNAL_TARGET_FAILURE,
        SCSI_ADSENSE_NO_SENSE},

    /* INVALID_FORMAT - 0xA */
    {SRB_STATUS_ERROR,
        SCSISTAT_CHECK_CONDITION,
        SCSI_SENSE_ILLEGAL_REQUEST,
        SCSI_ADSENSE_FORMAT_COMMAND_FAILED,
        SCSI_SENSEQ_FORMAT_COMMAND_FAILED},

    /* Command Specific Status (NVM Command Set) Lookup Table */

    /* CONFLICTING_ATTRIBUTES - 0x80 (0xB) */
    {SRB_STATUS_ERROR,
        SCSISTAT_CHECK_CONDITION,
        SCSI_SENSE_ILLEGAL_REQUEST,
        SCSI_ADSENSE_INVALID_CDB,
        SCSI_ADSENSE_NO_SENSE}
};

/* Media Error Lookup Table */
SNTI_RESPONSE_BLOCK mediaErrorTable[] = {
    /* WRITE_FAULT - 0x80 */
    {SRB_STATUS_ERROR,
        SCSISTAT_CHECK_CONDITION,
        SCSI_SENSE_MEDIUM_ERROR,
        SCSI_ADSENSE_PERIPHERAL_DEV_WRITE_FAULT,
        SCSI_ADSENSE_NO_SENSE},

    /* UNRECOVERED_READ_ERROR - 0x81 */
    {SRB_STATUS_ERROR,
        SCSISTAT_CHECK_CONDITION,
        SCSI_SENSE_MEDIUM_ERROR,
        SCSI_ADSENSE_UNRECOVERED_READ_ERROR,
        SCSI_ADSENSE_NO_SENSE},

    /* END_TO_END_GUARD_CHECK_ERROR - 0x82 */
    {SRB_STATUS_ERROR,
        SCSISTAT_CHECK_CONDITION,
        SCSI_SENSE_MEDIUM_ERROR,
        SCSI_ADSENSE_LOG_BLOCK_GUARD_CHECK_FAILED,
        SCSI_SENSEQ_LOG_BLOCK_GUARD_CHECK_FAILED},

    /* END_TO_END_APPLICATION_TAG_CHECK_ERROR - 0x83 */
    {SRB_STATUS_ERROR,
        SCSISTAT_CHECK_CONDITION,
        SCSI_SENSE_MEDIUM_ERROR,
        SCSI_ADSENSE_LOG_BLOCK_APPTAG_CHECK_FAILED,
        SCSI_SENSEQ_LOG_BLOCK_APPTAG_CHECK_FAILED},

    /* END_TO_END_REFERENCE_TAG_CHECK_ERROR - 0x84 */
    {SRB_STATUS_ERROR,
        SCSISTAT_CHECK_CONDITION,
        SCSI_SENSE_MEDIUM_ERROR,
        SCSI_ADSENSE_LOG_BLOCK_REFTAG_CHECK_FAILED,
        SCSI_SENSEQ_LOG_BLOCK_REFTAG_CHECK_FAILED},

    /* COMPARE_FAILURE - 0x85 */
    {SRB_STATUS_ERROR,
        SCSISTAT_CHECK_CONDITION,
        SCSI_SENSE_MISCOMPARE,
        SCSI_ADSENSE_MISCOMPARE_DURING_VERIFY,
        SCSI_ADSENSE_NO_SENSE},

    /* ACCESS_DENIED - 0x86 */
    {SRB_STATUS_ERROR,
        SCSISTAT_CHECK_CONDITION,
        SCSI_SENSE_ILLEGAL_REQUEST,
        SCSI_ADSENSE_ACCESS_DENIED_INVALID_LUN_ID,
        SCSI_SENSEQ_ACCESS_DENIED_INVALID_LUN_ID}
};


/*******************************************************************************
 * NVMeWaitForCtrlRDY
 *
 * @brief helper routine to wait for controller status RDY transition
 *
 * @param pAE - Pointer to device extension
 *
 * @return VOID
 ******************************************************************************/
static VOID NVMeWaitForCtrlRDY(
    __in PNVME_DEVICE_EXTENSION pAE,
    __in ULONG expectedValue
)
{   
    NVMe_CONTROLLER_STATUS CSTS;
    CSTS.AsUlong = 0;
    ULONG time = 0;

     CSTS.AsUlong =
         StorPortReadRegisterUlong(pAE,
                                   &pAE->pCtrlRegister->CSTS.AsUlong);
     while (CSTS.RDY != expectedValue) {
        NVMeCrashDelay(STORPORT_TIMER_CB_us, pAE->ntldrDump);
        time += STORPORT_TIMER_CB_us;
        if (time > pAE->uSecCrtlTimeout) {
            return ;
        }
        CSTS.AsUlong =
            StorPortReadRegisterUlong(pAE,
                                      &pAE->pCtrlRegister->CSTS.AsUlong);
     };
}


/*******************************************************************************
 * NVMeWaitOnReady
 *
 * @brief Polls on the status bit waiting for RDY state
 *
 * @param pAE - Pointer to hardware device extension.
 *
 * @return BOOLEAN
 *     TRUE - If it went RDY before the timeout
 *     FALSE - If anything goes wrong
 ******************************************************************************/
static BOOLEAN NVMeWaitOnReady(
    PNVME_DEVICE_EXTENSION pAE
)
{
    NVMe_CONTROLLER_STATUS CSTS;
    ULONG PollMax = pAE->uSecCrtlTimeout / MAX_STATE_STALL_us;
    ULONG PollCount;

    /*
     * Find out the Timeout value from Controller Capability register,
     * which is in 500 ms.
     * In case the read back unit is 0, make it 1, i.e., 500 ms wait.
     */
    for (PollCount = 0; PollCount < PollMax; PollCount++) {
        CSTS.AsUlong = StorPortReadRegisterUlong(
            pAE, (PULONG)(&pAE->pCtrlRegister->CSTS));

        if (CSTS.RDY == 0)
            return TRUE;

        NVMeStallExecution(pAE, MAX_STATE_STALL_us);
    }

    return FALSE;
} /* NVMeWaitOnReady */


/*******************************************************************************
 * NVMeResetAdapter
 *
 * @brief NVMeResetAdapter gets called to reset the adapter by setting EN bit of
 *        CC register as 0. This causes the adapter to forget about queues --
 *        the internal adapter falls back to initial state.
 *
 * @param pAE - Pointer to hardware device extension.
 *
 * @return BOOLEAN
 *     TRUE - If reset procedure seemed to worky
 *     FALSE - If anything goes wrong
 ******************************************************************************/
static BOOLEAN NVMeResetAdapter(
    PNVME_DEVICE_EXTENSION pAE
)
{
    NVMe_CONTROLLER_CONFIGURATION CC;

    /* Need to to ensure the Controller registers are memory-mapped properly */
    if (pAE->pCtrlRegister == NULL)
        return (FALSE);

    StorPortDebugPrint(INFO, "NVMeResetAdapter:  Clearing EN...\n");
    /*
     * Immediately reset our start state to indicate that the controller
     * is ont ready
     */
    CC.AsUlong= StorPortReadRegisterUlong(pAE,
                                          (PULONG)(&pAE->pCtrlRegister->CC));
    CC.EN = 0;

    StorPortWriteRegisterUlong(pAE,
                               (PULONG)(&pAE->pCtrlRegister->CC),
                               CC.AsUlong);

    if (NVMeWaitOnReady(pAE) == FALSE) {
        StorPortDebugPrint(INFO,
                       "NVMeResetAdapter: EN bit is not getting Cleared\n");
        return (FALSE);
    }


 	pAE->DriverState.NextDriverState = NVMeWaitOnRDY;

    return (TRUE);
} /* NVMeResetAdapter */


/*******************************************************************************
 * NVMeFreeNonContiguousBuffers
 *
 * @brief NVMeFreeNonContiguousBuffer gets called to free buffers that had been
 *        allocated without requiring physically contiguous.
 *
 * @param pAE - Pointer to hardware device extension.
 *
 * @return VOID
 ******************************************************************************/
static VOID NVMeFreeNonContiguousBuffers (
    PNVME_DEVICE_EXTENSION pAE
)
{
    PQUEUE_INFO pQI = &pAE->QueueInfo;
    PRES_MAPPING_TBL pRMT = &pAE->ResMapTbl;

    /* Free the Start State Machine SRB EXTENSION allocated by driver */
    if (pAE->DriverState.pSrbExt != NULL) {
        StorPortFreePool((PVOID)pAE, pAE->DriverState.pSrbExt);
        pAE->DriverState.pSrbExt = NULL;
    }

    /* Free the resource mapping tables if allocated */
    if (pRMT->pMsiMsgTbl != NULL) {
        StorPortFreePool((PVOID)pAE, pRMT->pMsiMsgTbl);
        pRMT->pMsiMsgTbl = NULL;
    }

    if (pRMT->pCoreTbl != NULL) {
        StorPortFreePool((PVOID)pAE, pRMT->pCoreTbl);
        pRMT->pCoreTbl = NULL;
    }

    if (pRMT->pNumaNodeTbl != NULL) {
        StorPortFreePool((PVOID)pAE, pRMT->pNumaNodeTbl);
        pRMT->pNumaNodeTbl = NULL;
    }

    if (pRMT->pProcGroupTbl != NULL) {
        StorPortFreePool((PVOID)pAE, pRMT->pProcGroupTbl);
        pRMT->pProcGroupTbl = NULL;
    }

    /* Free the allocated SUB/CPL_QUEUE_INFO structures memory */
    if ( pQI->pSubQueueInfo != NULL ) {
        StorPortFreePool((PVOID)pAE, pQI->pSubQueueInfo);
        pQI->pSubQueueInfo = NULL;
    }

    if ( pQI->pCplQueueInfo != NULL ) {
        StorPortFreePool((PVOID)pAE, pQI->pCplQueueInfo);
        pQI->pCplQueueInfo = NULL;
    }

    /* Free the DPC array memory */
    if (pAE->pDpcArray != NULL) {
        StorPortFreePool((PVOID)pAE, pAE->pDpcArray);
        pAE->pDpcArray = NULL;
    }


} /* NVMeFreeNonContiguousBuffer */


/*******************************************************************************
 * NVMeFreeBuffers
 *
 * @brief NVMeFreeBuffers gets called to free buffers that had been allocated.
 *        The buffers are required to be physically contiguous.
 *
 * @param pAE - Pointer to hardware device extension.
 *
 * @return VOID
 ******************************************************************************/
static VOID NVMeFreeBuffers (
    PNVME_DEVICE_EXTENSION pAE
)
{
    USHORT QueueID;
    PQUEUE_INFO pQI = &pAE->QueueInfo;
    PRES_MAPPING_TBL pRMT = &pAE->ResMapTbl;
    PSUB_QUEUE_INFO pSQI = NULL;

    /* First, free the Start State Data buffer memory allocated by driver */
    if (pAE->DriverState.pDataBuffer != NULL) {
        StorPortPatchFreeContiguousMemorySpecifyCache((PVOID)pAE,
                                                 pAE->DriverState.pDataBuffer,
                                                 PAGE_SIZE, MmCached);
        pAE->DriverState.pDataBuffer = NULL;
    }
    /* Free the NVME_LUN_EXTENSION memory allocated by driver */
    if (pAE->pLunExtensionTable[0] != NULL) {
        StorPortPatchFreeContiguousMemorySpecifyCache((PVOID)pAE,
                                                 pAE->pLunExtensionTable[0],
                                                 pAE->LunExtSize,
                                                 MmCached);
        pAE->pLunExtensionTable[0] = NULL;
    }

    /* Free the allocated queue entry and PRP list buffers */
    if (pQI->pSubQueueInfo != NULL) {
        for (QueueID = 0; QueueID <= pRMT->NumActiveCores; QueueID++) {
            pSQI = pQI->pSubQueueInfo + QueueID;
            if (pSQI->pQueueAlloc != NULL) {
                StorPortPatchFreeContiguousMemorySpecifyCache((PVOID)pAE,
                                                         pSQI->pQueueAlloc,
                                                         pSQI->QueueAllocSize,
                                                         MmCached);
                pSQI->pQueueAlloc = NULL;
            }

            if (pSQI->pPRPListAlloc != NULL) {
                StorPortPatchFreeContiguousMemorySpecifyCache((PVOID)pAE,
                                                         pSQI->pPRPListAlloc,
                                                         pSQI->PRPListAllocSize,
                                                         MmCached);
                pSQI->pPRPListAlloc = NULL;
            }


#ifdef DUMB_DRIVER
            if (pSQI->pDblBuffAlloc != NULL)
                StorPortPatchFreeContiguousMemorySpecifyCache((PVOID)pAE,
                                                         pSQI->pDblBuffAlloc,
                                                         pSQI->dblBuffSz,
                                                         MmCached);
            if (pSQI->pDblBuffListAlloc != NULL)
                StorPortPatchFreeContiguousMemorySpecifyCache((PVOID)pAE,
                                                         pSQI->pDblBuffListAlloc,
                                                         pSQI->dblBuffListSz,
                                                         MmCached);
#endif /* DUMB_DRIVER */
        }
    }

    /* Lastly, free the allocated non-contiguous buffers */
    NVMeFreeNonContiguousBuffers(pAE);
} /* NVMeFreeBuffers */


/*******************************************************************************
 * NVMeAllocateMem
 *
 * @brief Helper routoine for Buffer Allocation.
 *        StorPortPatchAllocateContiguousMemorySpecifyCacheNode is called to allocate
 *        memory from the preferred NUMA node. If succeeded, zero out the memory
 *        before returning to the caller.
 *
 * @param pAE - Pointer to hardware device extension
 * @param Size - In bytes
 * @param Node - Preferred NUMA node
 *
 * @return PVOID
 *    Buffer Addr - If all resources are allocated and initialized properly
 *    NULL - If anything goes wrong
 ******************************************************************************/
static PVOID NVMeAllocateMem(
    PNVME_DEVICE_EXTENSION pAE,
    ULONG Size,
    ULONG Node
)
{
    PHYSICAL_ADDRESS Low;
    PHYSICAL_ADDRESS High;
    PHYSICAL_ADDRESS Align;
    PVOID pBuf = NULL;
    ULONG Status = 0;
    ULONG NewBytesAllocated;
    
    if (pAE->ntldrDump == FALSE) {
        /* Set up preferred range and alignment before allocating */
        Low.QuadPart = 0;
        High.QuadPart = (-1);
        Align.QuadPart = 0;
        Status = StorPortPatchAllocateContiguousMemorySpecifyCacheNode(
                     pAE, Size, Low, High, Align, MmCached, Node, (PVOID)&pBuf);

        /* It fails, log the error first */
        if ((Status != 0) || (pBuf == NULL)) {
            StorPortDebugPrint(ERROR,
                               "NVMeAllocateMem: Failure from node=%d, sts=0x%x\n",
                               Node, Status);
            /* If the memory allocation fails for the requested node,
		     * we will try to allocate from any available node, which is not
		     * expected to fail.
		     */

            /* Give it another try from any nodes that can grant the request */
            Status = 0;
            pBuf = NULL;
            Status = StorPortPatchAllocateContiguousMemorySpecifyCacheNode(
                     pAE, Size, Low, High, Align, MmCached, MM_ANY_NODE_OK, (PVOID)&pBuf);
            if ((Status != 0) || (pBuf == NULL)) {
                StorPortDebugPrint(ERROR,
                               "NVMeAllocateMem: Failure from Node 0, sts=0x%x\n",
                               Status);
                return NULL;
            }
        }
    } else {     
        NewBytesAllocated = pAE->DumpBufferBytesAllocated + Size;
        if (NewBytesAllocated <= DUMP_BUFFER_SIZE) {
            pBuf = pAE->DumpBuffer + pAE->DumpBufferBytesAllocated;               
            pAE->DumpBufferBytesAllocated = NewBytesAllocated;
        } else {
            StorPortDebugPrint(ERROR,
                               "Unable to allocate %d bytes at DumpBuffer offset %d.\n",
                               Size,
                               pAE->DumpBufferBytesAllocated);
            return NULL;
        }
    }



    StorPortDebugPrint(INFO, "NVMeAllocateMem: Succeeded!\n");
    /* Zero out the buffer before return */
    memset(pBuf, 0, Size);

    return pBuf;
} /* NVMeAllocateMem */


/*******************************************************************************
 * NVMeAllocQueues
 *
 * @brief NVMeAllocQueues gets called to allocate buffers in
 *        non-paged, contiguous memory space for Submission/Completion queues.
 *
 * @param pAE - Pointer to hardware device extension.
 * @param QueueID - Which queue to allocate memory for
 * @param QEntries - the number of entries to allocate for this queue
 * @param NumaNode - Which NUMA node associated memory to allocate from
 *
 * @return ULONG
 *     STOR_STATUS_SUCCESS - If all resources are allocated properly
 *     Otherwise - If anything goes wrong
 ******************************************************************************/
static ULONG NVMeAllocQueues (
    PNVME_DEVICE_EXTENSION pAE,
    USHORT QueueID,
    ULONG QEntries,
    USHORT NumaNode
)
{

    /* The number of Submission entries makes up exact one system page size */
    ULONG SysPageSizeInSubEntries;
    PQUEUE_INFO pQI = &pAE->QueueInfo;
    PSUB_QUEUE_INFO pSQI = NULL;
    PRES_MAPPING_TBL pRMT = &pAE->ResMapTbl;
    ULONG SizeQueueEntry = 0;
    ULONG NumPageToAlloc = 0;

    /* Ensure the QueueID is valid via the number of active cores in system */
    if (QueueID > pRMT->NumActiveCores)
        return (STOR_STATUS_INVALID_PARAMETER);

    /* Locate the target SUB_QUEUE_STRUCTURE via QueueID */
    pSQI = pQI->pSubQueueInfo + QueueID;

    /*
     * To ensure:
     *   1. Allocating enough memory and
     *   2. The starting addresses of Sub/Com queues are system page aligned.
     *
     *      Need to:
     *        1. Round up the allocated size of all Submission entries to be
     *           multiple(s) of system page size.
     *        2. Add one extra system page to allocation size
     */
    SysPageSizeInSubEntries = PAGE_SIZE / sizeof (NVMe_COMMAND);
    if ((QEntries % SysPageSizeInSubEntries) != 0)
        QEntries = (QEntries + SysPageSizeInSubEntries) &
                  ~(SysPageSizeInSubEntries - 1);

    /*
     * Determine the allocation size in bytes
     *   1. For Sub/Cpl/Cmd entries
     *   2. For PRP Lists
     */
    SizeQueueEntry = QEntries * (sizeof(NVMe_COMMAND) +
                                 sizeof(NVMe_COMPLETION_QUEUE_ENTRY) +
                                 sizeof(CMD_ENTRY));

    /* Allcate memory for Sub/Cpl/Cmd entries first */
    pSQI->pQueueAlloc = NVMeAllocateMem(pAE,
                                        SizeQueueEntry + PAGE_SIZE,
                                        NumaNode);

    if (pSQI->pQueueAlloc == NULL)
        return ( STOR_STATUS_INSUFFICIENT_RESOURCES );

    /* Save the size if needed to free the unused buffers */
    pSQI->QueueAllocSize = SizeQueueEntry + PAGE_SIZE;

#ifdef DUMB_DRIVER
    pSQI->pDblBuffAlloc = NVMeAllocateMem(pAE,
                                          (QEntries * DUMB_DRIVER_SZ) + PAGE_SIZE,
                                          NumaNode);

    if (pSQI->pDblBuffAlloc == NULL)
        return ( STOR_STATUS_INSUFFICIENT_RESOURCES );

    /* Save the size if needed to free the unused buffers */
    pSQI->dblBuffSz = (QEntries * DUMB_DRIVER_SZ) + PAGE_SIZE;

    /* now the PRP list mem for this SQ to use */
    pSQI->pDblBuffListAlloc = NVMeAllocateMem(pAE,
                                          (QEntries * PAGE_SIZE) + PAGE_SIZE,
                                          NumaNode);

    if (pSQI->pDblBuffListAlloc == NULL)
        return ( STOR_STATUS_INSUFFICIENT_RESOURCES );

    /* Save the size if needed to free the unused buffers */
    pSQI->dblBuffListSz = (QEntries * PAGE_SIZE) + PAGE_SIZE;
#endif

    /*
     * Allcate memory for PRP Lists In order not crossing page boundary, need to
     * calculate how many lists one page can accommodate.
     */
    pSQI->NumPRPListOnePage = PAGE_SIZE / pAE->PRPListSize;
    NumPageToAlloc = (QEntries % pSQI->NumPRPListOnePage) ?
        (QEntries / pSQI->NumPRPListOnePage) + 1 :
        (QEntries / pSQI->NumPRPListOnePage);

    pSQI->pPRPListAlloc = NVMeAllocateMem(pAE,
                                          (NumPageToAlloc + 1) * PAGE_SIZE,
                                          NumaNode);

    if (pSQI->pPRPListAlloc == NULL) {
        /* Free the allcated memory for Sub/Cpl/Cmd entries before returning */
        StorPortPatchFreeContiguousMemorySpecifyCache((PVOID)pAE,
                                                 pSQI->pQueueAlloc,
                                                 pSQI->QueueAllocSize,
                                                 MmCached);
        pSQI->pQueueAlloc = NULL;
        return ( STOR_STATUS_INSUFFICIENT_RESOURCES );
    }

    /* Save the size if needed to free the unused buffers */
    pSQI->PRPListAllocSize = (NumPageToAlloc + 1) * PAGE_SIZE;

    /* Mark down the number of entries allocated successfully */
    if (QueueID != 0) {
        pQI->NumIoQEntriesAllocated = (USHORT)QEntries;
    } else {
        pQI->NumAdQEntriesAllocated = (USHORT)QEntries;
    }

    return (STOR_STATUS_SUCCESS);
} /* NVMeAllocQueues */


/*******************************************************************************
 * NVMeInitSubQueue
 *
 * @brief NVMeInitSubQueue gets called to initialize the SUB_QUEUE_INFO
 *        structure of the specific submission queue associated with the QueueID
 *
 * @param pAE - Pointer to hardware device extension.
 * @param QueueID - Which submission queue structure to initialize
 *
 * @return ULONG
 *     STOR_STATUS_SUCCESS - If all resources are allocated properly
 *     Otherwise - If anything goes wrong
 ******************************************************************************/
static ULONG NVMeInitSubQueue(
    PNVME_DEVICE_EXTENSION pAE,
    USHORT QueueID
)
{
    PQUEUE_INFO pQI = &pAE->QueueInfo;
    PSUB_QUEUE_INFO pSQI = pQI->pSubQueueInfo + QueueID;
    PRES_MAPPING_TBL pRMT = &pAE->ResMapTbl;
    //ULONG_PTR PtrTemp = 0;
    //USHORT Entries;
    ULONG dbIndex = 0;
    UCHAR maxCore;

    NVMe_CONTROLLER_CAPABILITIES CAP;
    CAP.AsUlonglong = 0;

    maxCore = (UCHAR)min(pAE->QueueInfo.NumCplIoQAllocFromAdapter,
                        pAE->QueueInfo.NumSubIoQAllocFromAdapter);

    /* Ensure the QueueID is valid via the number of active cores in system */
    if (QueueID > maxCore)
        return ( STOR_STATUS_INVALID_PARAMETER );

#if (NTDDI_VERSION > NTDDI_WIN7) && defined(_WIN64)
    CAP.AsUlonglong = StorPortReadRegisterUlong64(pAE,
        (PULONG64)(&pAE->pCtrlRegister->CAP));
#else
    CAP.HighPart = StorPortReadRegisterUlong(pAE,
        (PULONG)(&pAE->pCtrlRegister->CAP.HighPart));
    CAP.LowPart = StorPortReadRegisterUlong(pAE,
        (PULONG)(&pAE->pCtrlRegister->CAP.LowPart));
#endif

    /* Initialize static fields of SUB_QUEUE_INFO structure */
    pSQI->SubQEntries = (QueueID != 0) ? pQI->NumIoQEntriesAllocated :
                                         pQI->NumAdQEntriesAllocated;
    pSQI->SubQueueID = QueueID;
    pSQI->FreeSubQEntries = pSQI->SubQEntries;

     /* calculate byte offset per 1.0c spec formula */
    dbIndex = 2 * QueueID * (4 << CAP.DSTRD);

    /* convert to index */
    dbIndex = dbIndex / sizeof(NVMe_QUEUE_Y_DOORBELL);
    pSQI->pSubTDBL = (PULONG)(&pAE->pCtrlRegister->IODB[dbIndex].QHT );

    StorPortDebugPrint(INFO,
                       "NVMeInitSubQueue : SQ 0x%x pSubTDBL 0x%x at index  0x%x\n",
                       QueueID, pSQI->pSubTDBL, dbIndex);
    pSQI->Requests = 0;
    pSQI->SubQTailPtr = 0;
    pSQI->SubQHeadPtr = 0;

    /*
     * The queue is shared by cores when:
     *   it's Admin queue, or
     *   not enough IO queues are allocated, or
     *   we are in crashdump.
     */
    if ((QueueID == 0)                                   ||
        (pQI->NumSubIoQAllocated < pRMT->NumActiveCores) ||
        (pAE->ntldrDump == TRUE)) {
        pSQI->Shared = TRUE;
    }

    pSQI->CplQueueID = QueueID;

    /*
     * Initialize submission queue starting point. Per NVMe specification, need
     * to make it system page aligned if it's not.
     */
    pSQI->pSubQStart = PAGE_ALIGN_BUF_PTR(pSQI->pQueueAlloc);

    memset(pSQI->pQueueAlloc, 0, pSQI->QueueAllocSize);

    pSQI->SubQStart = NVMeGetPhysAddr(pAE, pSQI->pSubQStart);
    /* If fails on converting to physical address, return here */
    if (pSQI->SubQStart.QuadPart == 0)
        return ( STOR_STATUS_INSUFFICIENT_RESOURCES );

#ifdef DUMB_DRIVER
    pSQI->pDlbBuffStartVa = PAGE_ALIGN_BUF_PTR(pSQI->pDblBuffAlloc);
    memset(pSQI->pDblBuffAlloc, 0, pSQI->dblBuffSz);

    pSQI->pDlbBuffStartListVa = PAGE_ALIGN_BUF_PTR(pSQI->pDblBuffListAlloc);
    memset(pSQI->pDblBuffListAlloc, 0, pSQI->dblBuffListSz);
#endif

    /*
     * Initialize PRP list starting point. Per NVMe specification, need to make
     * it system page aligned if it's not.
     */
    pSQI->pPRPListStart = PAGE_ALIGN_BUF_PTR(pSQI->pPRPListAlloc);
    memset(pSQI->pPRPListAlloc, 0, pSQI->PRPListAllocSize);

    pSQI->PRPListStart = NVMeGetPhysAddr( pAE, pSQI->pPRPListStart );
    /* If fails on converting to physical address, return here */
    if (pSQI->PRPListStart.QuadPart == 0)
        return ( STOR_STATUS_INSUFFICIENT_RESOURCES );

    /* Initialize list head of the free queue list */
    InitializeListHead(&pSQI->FreeQList);

    return (STOR_STATUS_SUCCESS);
} /* NVMeInitSubQueue */


/*******************************************************************************
 * NVMeInitCplQueue
 *
 * @brief NVMeInitCplQueue gets called to initialize the CPL_QUEUE_INFO
 *        structure of the specific completion queue associated with the QueueID
 *
 * @param pAE - Pointer to hardware device extension.
 * @param QueueID - Which completion queue structure to initialize
 *
 * @return ULONG
 *     STOR_STATUS_SUCCESS - If all resources are allocated properly
 *     Otherwise - If anything goes wrong
 ******************************************************************************/
static ULONG NVMeInitCplQueue(
    PNVME_DEVICE_EXTENSION pAE,
    USHORT QueueID
)
{
    PQUEUE_INFO pQI = &pAE->QueueInfo;
    PSUB_QUEUE_INFO pSQI = NULL;
    PCPL_QUEUE_INFO pCQI = NULL;
    PRES_MAPPING_TBL pRMT = &pAE->ResMapTbl;
    PCORE_TBL pCT = NULL;
    ULONG_PTR PtrTemp;
    //USHORT Entries;
    ULONG queueSize = 0;
    ULONG dbIndex = 0;
    UCHAR maxCore;

    NVMe_CONTROLLER_CAPABILITIES CAP;
    CAP.AsUlonglong = 0;

    maxCore = (UCHAR)min(pAE->QueueInfo.NumCplIoQAllocFromAdapter,
                        pAE->QueueInfo.NumSubIoQAllocFromAdapter);

    /* Ensure the QueueID is valid via the number of active cores in system */
    if (QueueID > maxCore)
        return ( STOR_STATUS_INVALID_PARAMETER );

#if (NTDDI_VERSION > NTDDI_WIN7) && defined(_WIN64)
    CAP.AsUlonglong = StorPortReadRegisterUlong64(pAE,
        (PULONG64)(&pAE->pCtrlRegister->CAP));
#else
    CAP.HighPart = StorPortReadRegisterUlong(pAE,
        (PULONG)(&pAE->pCtrlRegister->CAP.HighPart));
    CAP.LowPart = StorPortReadRegisterUlong(pAE,
        (PULONG)(&pAE->pCtrlRegister->CAP.LowPart));
#endif

    pSQI = pQI->pSubQueueInfo + QueueID;
    pCQI = pQI->pCplQueueInfo + QueueID;

    /* Initialize static fields of CPL_QUEUE_INFO structure */
    pCQI->CplQueueID = QueueID;
    pCQI->CplQEntries = pSQI->SubQEntries;

    /* calculate byte offset per 10.c spec formula  */
    dbIndex = (2 * QueueID + 1) * (4 << CAP.DSTRD);

    /* convert to index */
    dbIndex = dbIndex / sizeof(NVMe_QUEUE_Y_DOORBELL);
    pCQI->pCplHDBL = (PULONG)(&pAE->pCtrlRegister->IODB[dbIndex].QHT );

    StorPortDebugPrint(INFO,
                       "NVMeInitCplQueue : CQ 0x%x pCplHDBL 0x%x at index  0x%x\n",
                       QueueID, pCQI->pCplHDBL, dbIndex);
    pCQI->Completions = 0;
    pCQI->CurPhaseTag = 0;
    pCQI->CplQHeadPtr = 0;

    /**
     * The queue is shared by cores when:
     *   it's Admin queue, or
     *   not enough IO queues are allocated, or
     *   we are in crashdump.
     */
    if ((QueueID == 0)                                   ||
        (pQI->NumCplIoQAllocated < maxCore) ||
        (pAE->ntldrDump == TRUE)) {
        pCQI->Shared = TRUE;
    }

    if (pRMT->InterruptType == INT_TYPE_MSI ||
        pRMT->InterruptType == INT_TYPE_MSIX) {
        if (pRMT->NumMsiMsgGranted < maxCore) {
            /* All completion queueus share the single message */
            pCQI->MsiMsgID = 0;
        } else {
            /*
             * When enough queues allocated, the mappings between cores and
             * queues are set up as core#(n) <=> queue#(n+1)
             * Only need to deal with IO queues since Admin queue uses message#0
             */
            if (QueueID != 0) {
                pCT = pRMT->pCoreTbl + QueueID - 1;
                pCQI->MsiMsgID = pCT->MsiMsgID;
            }
        }
    }

    /*
     * Initialize completion queue entries. Firstly, make Cpl queue starting
     * entry system page aligned.
     */
    queueSize = pSQI->SubQEntries * sizeof(NVMe_COMMAND);
    PtrTemp = (ULONG_PTR)((PUCHAR)pSQI->pSubQStart);
    pCQI->pCplQStart = (PVOID)(PtrTemp + queueSize);

    queueSize = pSQI->SubQEntries * sizeof(NVMe_COMPLETION_QUEUE_ENTRY);
    memset(pCQI->pCplQStart, 0, queueSize);
    pCQI->pCplQStart = PAGE_ALIGN_BUF_PTR(pCQI->pCplQStart);

    pCQI->CplQStart = NVMeGetPhysAddr( pAE, pCQI->pCplQStart );
    /* If fails on converting to physical address, return here */
    if (pCQI->CplQStart.QuadPart == 0)
        return ( STOR_STATUS_INSUFFICIENT_RESOURCES );

    return (STOR_STATUS_SUCCESS);
} /* NVMeInitCplQueue */


/*******************************************************************************
 * NVMeInitFreeQ
 *
 * @brief NVMeInitFreeQ gets called to initialize the free queue list of the
 *        specific submission queue with its associated command entries and PRP
 *        List buffers.
 *
 * @param pSQI - Pointer to the SUB_QUEUE_INFO structure.
 *
 * @return VOID
 ******************************************************************************/
static VOID NVMeInitFreeQ(
    PSUB_QUEUE_INFO pSQI,
    PNVME_DEVICE_EXTENSION pAE
)
{
    USHORT Entry;
    PCMD_ENTRY pCmdEntry = NULL;
    PCMD_INFO pCmdInfo = NULL;
    ULONG_PTR CurPRPList = 0;
    //ULONG prpListSz = 0;
#ifdef DUMB_DRIVER
    ULONG_PTR PtrTemp;
    ULONG dblBuffSz = 0;
#endif

    /* For each entry, initialize the CmdID and PRPList flields */
    CurPRPList = (ULONG_PTR)((PUCHAR)pSQI->pPRPListStart);

    for (Entry = 0; Entry < pSQI->SubQEntries; Entry++) {
        pCmdEntry = (PCMD_ENTRY)pSQI->pCmdEntry;
        pCmdEntry += Entry;
        pCmdInfo = &pCmdEntry->CmdInfo;

        /*
         * Set up CmdID and dedicated PRP Lists before adding to FreeQ
         * Use Entry to locate the starting point.
         */
        pCmdInfo->CmdID = Entry;
        pCmdInfo->pPRPList = (PVOID)(CurPRPList + pAE->PRPListSize);

        /*
         * Because PRP List can't cross page boundary, if not enough room left
         * for one list, need to move on to next page boundary.
         * NumPRPListOnepage is calculated for this purpose.
         */
        if (Entry != 0 && ((Entry % pSQI->NumPRPListOnePage) == 0))
            pCmdInfo->pPRPList = PAGE_ALIGN_BUF_PTR(pCmdInfo->pPRPList);

        /* Save the address of current list for calculating next list */
        CurPRPList = (ULONG_PTR)pCmdInfo->pPRPList;
        pCmdInfo->prpListPhyAddr = NVMeGetPhysAddr(pAE, pCmdInfo->pPRPList);

#ifdef DUMB_DRIVER
        PtrTemp = (ULONG_PTR)((PUCHAR)pSQI->pDlbBuffStartVa);
        pCmdInfo->pDblVir = (PVOID)(PtrTemp + (DUMB_DRIVER_SZ * Entry));
        pCmdInfo->dblPhy = StorPortGetPhysicalAddress(pAE,
                                                      NULL,
                                                      pCmdInfo->pDblVir,
                                                      &dblBuffSz);

        PtrTemp = (ULONG_PTR)((PUCHAR)pSQI->pDlbBuffStartListVa);
        pCmdInfo->pDblPrpListVir = (PVOID)(PtrTemp + (PAGE_SIZE * Entry));
        pCmdInfo->dblPrpListPhy = StorPortGetPhysicalAddress(pAE,
                                                      NULL,
                                                      pCmdInfo->pDblPrpListVir,
                                                      &dblBuffSz);
#endif

        InsertTailList(&pSQI->FreeQList, &pCmdEntry->ListEntry);
    }
} /* NVMeInitFreeQ */


/*******************************************************************************
 * NVMeInitCmdEntries
 *
 * @brief NVMeInitCmdEntries gets called to initialize the CMD_ENTRY structure
 *        of the specific submission queue associated with the QueueID
 *
 * @param pAE - Pointer to hardware device extension.
 * @param QueueID - Which submission queue structure to initialize
 *
 * @return ULONG
 *     STOR_STATUS_SUCCESS - If all resources are allocated properly
 *     Otherwise - If anything goes wrong
 ******************************************************************************/
static ULONG NVMeInitCmdEntries(
    PNVME_DEVICE_EXTENSION pAE,
    USHORT QueueID
)
{
    PQUEUE_INFO pQI = &pAE->QueueInfo;
    PSUB_QUEUE_INFO pSQI = pQI->pSubQueueInfo + QueueID;
    PCPL_QUEUE_INFO pCQI = pQI->pCplQueueInfo + QueueID;
    PRES_MAPPING_TBL pRMT = &pAE->ResMapTbl;
    ULONG_PTR PtrTemp = 0;

    /* Ensure the QueueID is valid via the number of active cores in system */
    if (QueueID > pRMT->NumActiveCores)
        return (STOR_STATUS_INVALID_PARAMETER);

    /* Initialize command entries and Free list */
    PtrTemp = (ULONG_PTR)((PUCHAR)pCQI->pCplQStart);
    pSQI->pCmdEntry = (PVOID) (PtrTemp + (pSQI->SubQEntries *
                                          sizeof(NVMe_COMPLETION_QUEUE_ENTRY)));

    memset(pSQI->pCmdEntry, 0, sizeof(CMD_ENTRY) * pSQI->SubQEntries);
    NVMeInitFreeQ(pSQI, pAE);

    return (STOR_STATUS_SUCCESS);
} /* NVMeInitCmdEntries */


/*******************************************************************************
 * NVMeEnableAdapter
 *
 * @brief NVMeEnableAdapter gets called to do the followings:
 *     - Program AdminQ related registers
 *     - Set EN bit of CC register as 1
 *     - Check if device is ready via RDY bit of STS register
 *     - Report the readiness of the adapter in return
 *
 * @param pAE - Pointer to hardware device extension.
 *
 * @return BOOLEAN
 *  TRUE  - if Adapter is enabled correctly
 *  FALSE - if anything goes wrong
 ******************************************************************************/
static BOOLEAN NVMeEnableAdapter(
    PNVME_DEVICE_EXTENSION pAE
)
{
    PQUEUE_INFO pQI = &pAE->QueueInfo;
    NVMe_CONTROLLER_CONFIGURATION CC;
    CC.AsUlong = 0;

    /* 
     * Disable the adapter
     */
    if (NVMeResetAdapter(pAE) != TRUE) {
        return (FALSE);
    }

    /*
     * Program Admin queue registers before enabling the adapter:
     * Admin Queue Attributes
     */
    StorPortWriteRegisterUlong(
        pAE,
        (PULONG)(&pAE->pCtrlRegister->AQA),
        (pQI->pSubQueueInfo->SubQEntries - 1) +
        ((pQI->pCplQueueInfo->CplQEntries - 1) << NVME_AQA_CQS_LSB));

    /* Admin Submission Queue Base Address (64 bit) */
    StorPortWriteRegisterUlong(
        pAE,
        (PULONG)(&pAE->pCtrlRegister->ASQ.LowPart),
        (ULONG)(pQI->pSubQueueInfo->SubQStart.LowPart));

    StorPortWriteRegisterUlong(
        pAE,
        (PULONG)(&pAE->pCtrlRegister->ASQ.HighPart),
        (ULONG)(pQI->pSubQueueInfo->SubQStart.HighPart));

    /* Admin Completion Queue Base Address (64 bit) */
    StorPortWriteRegisterUlong(
        pAE,
        (PULONG)(&pAE->pCtrlRegister->ACQ.LowPart),
        (ULONG)(pQI->pCplQueueInfo->CplQStart.LowPart));

    StorPortWriteRegisterUlong(
        pAE,
        (PULONG)(&pAE->pCtrlRegister->ACQ.HighPart),
        (ULONG)(pQI->pCplQueueInfo->CplQStart.HighPart));


    StorPortDebugPrint(INFO, "NVMeEnableAdapter:  Setting EN...\n");
    /*
     * Set up Controller Configuration Register
     */
    CC.EN = 1;
    CC.CSS = NVME_CC_NVM_CMD;
    CC.MPS = (PAGE_SIZE >> NVME_MEM_PAGE_SIZE_SHIFT);
    CC.AMS = NVME_CC_ROUND_ROBIN;
    CC.SHN = NVME_CC_SHUTDOWN_NONE;
    CC.IOSQES = NVME_CC_IOSQES;
    CC.IOCQES = NVME_CC_IOCQES;

    StorPortWriteRegisterUlong(pAE,
                               (PULONG)(&pAE->pCtrlRegister->CC),
                               CC.AsUlong);
    return TRUE;
} /* NVMeEnableAdapter */


/*******************************************************************************
 * NVMeDriverFatalError
 *
 * @brief NVMeDriverFatalError gets called to mark down the failure of state
 *        machine.
 *
 * @param pAE - Pointer to adapter device extension.
 *
 * @return VOID
 ******************************************************************************/
static VOID NVMeDriverFatalError(
    PNVME_DEVICE_EXTENSION pAE,
    ULONG ErrorNum
)
{
    ASSERT(FALSE);

    StorPortDebugPrint(ERROR, "NVMeDriverFatalError!\n");

    pAE->DriverState.DriverErrorStatus |= (ULONG64)(1 << ErrorNum);
    pAE->DriverState.NextDriverState = NVMeStateFailed;

    NVMeLogError(pAE, ErrorNum);

    return;
} /* NVMeDriverFatalError */


/*******************************************************************************
 * NVMeRunningWaitOnRDY
 *
 * @brief NVMeRunningWaitOnRDY is called to verify if the adapter is enabled and
 *        ready to process commands. It is called by NVMeRunning when the state
 *        machine is in NVMeWaitOnRDY state.
 *
 * @param pAE - Pointer to adapter device extension.
 *
 * @return VOID
 ******************************************************************************/
static VOID NVMeRunningWaitOnRDY(
    PNVME_DEVICE_EXTENSION pAE
)
{
    //NVMe_CONTROLLER_CONFIGURATION CC;
    NVMe_CONTROLLER_STATUS        CSTS;

    /*
     * Checking to see if we're enabled yet, watching the timeout value
     * we read from the controller CAP register (StateChkCount is incr
     * in uSec in this case)
     */
    if ((pAE->DriverState.StateChkCount > pAE->uSecCrtlTimeout) ||
        (pAE->DriverState.DriverErrorStatus)) {
            NVMeDriverFatalError(pAE,
                                (1 << START_STATE_RDY_FAILURE));
    } else {

        /*
         * Look for signs of life. If it's ready, set NextDriverState to proceed to
         * next state. Otherwise, wait for 1 sec only for crashdump case otherwise
         * ask Storport to call again in normal driver case.
         */
        CSTS.AsUlong =
            StorPortReadRegisterUlong(pAE, &pAE->pCtrlRegister->CSTS.AsUlong);

        if (CSTS.RDY == 1) {
            StorPortDebugPrint(INFO,"NVMeRunningWaitOnRDY: RDY has been set\n");
            pAE->DriverState.NextDriverState = NVMeWaitOnIdentifyCtrl;
            pAE->DriverState.StateChkCount = 0;
        } else {
            if (pAE->DriverState.StateChkCount == 0) {
                StorPortDebugPrint(INFO,"NVMeRunningWaitOnRDY: Waiting...\n");
            }
            pAE->DriverState.NextDriverState = NVMeWaitOnRDY;
            pAE->DriverState.StateChkCount += pAE->DriverState.CheckbackInterval;
        }
    }
    NVMeCallArbiter(pAE);
} /* NVMeRunningWaitOnRDY */


/*******************************************************************************
 * NVMeDeleteQueueCallback
 *
 * @brief NVMeDeleteQueueCallback is the callback function used to notify
 *        the caller that a queue deletion has completed
 *
 * @param pAE - Pointer to hardware device extension.
 * @param pSrbExtension - Pointer to the completion entry
 *
 * @return BOOLEAN
 *     TRUE - to indicate we completed fine
 ******************************************************************************/
static BOOLEAN NVMeDeleteQueueCallback(
    PVOID pNVMeDevExt,
    PVOID pSrbExtension
)
{
    PNVME_DEVICE_EXTENSION pAE = (PNVME_DEVICE_EXTENSION)pNVMeDevExt;
    PNVME_SRB_EXTENSION pSrbExt = (PNVME_SRB_EXTENSION)pSrbExtension;
    PNVMe_COMMAND pNVMeCmd = (PNVMe_COMMAND)(&pSrbExt->nvmeSqeUnit);
    PNVMe_COMPLETION_QUEUE_ENTRY pCplEntry = pSrbExt->pCplEntry;
    PQUEUE_INFO pQI = &pAE->QueueInfo;

    if (pNVMeCmd->CDW0.OPC == ADMIN_DELETE_IO_COMPLETION_QUEUE) {
        if (pCplEntry->DW3.SF.SC == 0) {
            PCPL_QUEUE_INFO pCQI = pQI->pCplQueueInfo + pQI->NumCplIoQCreated;
            /*
             * Because learning mode doesn't re-allocate mem, we need to
             * zero out some counters as well as all entries so nothing
             * is left stale when recreated
             */
            pCQI->CurPhaseTag = 0;
            pCQI->CplQHeadPtr = 0;
            memset(pCQI->pCplQStart,
                0,
                (pCQI->CplQEntries * sizeof(NVMe_COMPLETION_QUEUE_ENTRY)));
            pQI->NumCplIoQCreated--;
        } else {
            NVMeDriverFatalError(pAE,
                                (1 << FATAL_CPLQ_DELETE_FAILURE));
        }
    } else if (pNVMeCmd->CDW0.OPC == ADMIN_DELETE_IO_SUBMISSION_QUEUE) {
        if (pCplEntry->DW3.SF.SC == 0) {
            PSUB_QUEUE_INFO pSQI = pQI->pSubQueueInfo + pQI->NumSubIoQCreated;
            pSQI->SubQTailPtr = 0;
            pSQI->SubQHeadPtr = 0;
            /* we don't recycle SQs w/learning mode but for consistency... */
            memset(pSQI->pSubQStart,
                0,
                (pSQI->SubQEntries * sizeof(NVMe_COMMAND)));
            pQI->NumSubIoQCreated--;
        } else {
            NVMeDriverFatalError(pAE,
                                (1 << FATAL_SUBQ_DELETE_FAILURE));
        }
    }
    return TRUE;
}


/*******************************************************************************
 * NVMeSetFeaturesCompletion
 *
 * @brief NVMeSetFeaturesCompletion gets called to exam the first LBA Range
 *        entry of a given namespace. The procedure is described below:
 *
 *        Only exam the first LBA Range entry of a given namespace;
 *
 *        if (NLB == Namespace Size) {
 *            if (Type == 00b) {
 *                Change the followings via Set Features (ID#3):
 *                    Type to Filesystem;
 *                    Attributes(bit0) as 1 (can be overwritten);
 *                    Attributes(bit1) as 0 (visible);
 *                Mark ExposeNamespace as TRUE;
 *                Mark ReadOnly as FALSE;
 *            } else if (Type == 01b) {
 *                if (Attributes(bit0) != 0)
 *                    Mark ReadOnly as FALSE;
 *                else
 *                    Mark ReadOnly as TRUE;
 *
 *                if (Attributes(bit1) != 0)
 *                    Mark ExposeNamespace as FALSE;
 *                else
 *                    Mark ExposeNamespace as TRUE;
 *            } else {
 *                Mark ExposeNamespace as FALSE;
 *                Mark ReadOnly based on Attributes(bit0);
 *            }
 *        } else {
 *            Mark ExposeNamespace as FALSE;
 *            Mark ReadOnly based on Attributes(bit0);
 *        }
 *
 * @param pAE - Pointer to hardware device extension.
 * @param pNVMeCmd - Pointer to the original submission entry
 * @param pCplEntry - Pointer to the completion entry
 *
 * @return VOID
 ******************************************************************************/
static VOID NVMeSetFeaturesCompletion(
    PNVME_DEVICE_EXTENSION pAE,
    PNVMe_COMMAND pNVMeCmd,
    PNVMe_COMPLETION_QUEUE_ENTRY pCplEntry
)
{
    PQUEUE_INFO pQI = &pAE->QueueInfo;
    PADMIN_SET_FEATURES_COMMAND_LBA_RANGE_TYPE_ENTRY pLbaRangeTypeEntry = NULL;
    PNVME_LUN_EXTENSION pLunExt = NULL;
    PADMIN_SET_FEATURES_COMMAND_DW10 pSetFeaturesCDW10 = NULL;
    PADMIN_SET_FEATURES_COMMAND_LBA_RANGE_TYPE_DW11 pSetFeaturesCDW11 = NULL;
    NS_VISBILITY visibility = IGNORED;
    ULONG lunId;
    UINT8 flbas;
    UINT16 metadataSize;

    /*
     * Mark down the resulted information if succeeded. Otherwise, log the error
     * bit in case of errors and fail the state machine
     */
    pSetFeaturesCDW10 =
        (PADMIN_SET_FEATURES_COMMAND_DW10) &pNVMeCmd->CDW10;

    if (pAE->DriverState.InterruptCoalescingSet == FALSE &&
        pNVMeCmd->CDW0.OPC == ADMIN_SET_FEATURES        &&
        pSetFeaturesCDW10->FID == INTERRUPT_COALESCING ) {
        if (pCplEntry->DW3.SF.SC != 0) {
            NVMeDriverFatalError(pAE,
                                (1 << START_STATE_INT_COALESCING_FAILURE));
        } else {
            pAE->DriverState.InterruptCoalescingSet = TRUE;

            /* Reset the counter and keep tihs state to set more features */
            pAE->DriverState.StateChkCount = 0;
            pAE->DriverState.NextDriverState = NVMeWaitOnSetFeatures;
        }
    } else if (pNVMeCmd->CDW0.OPC == ADMIN_SET_FEATURES &&
               pSetFeaturesCDW10->FID == NUMBER_OF_QUEUES) {
        if (pCplEntry->DW3.SF.SC != 0) {
            NVMeDriverFatalError(pAE,
                                (1 << START_STATE_QUEUE_ALLOC_FAILURE));
        } else {
            /*
             * NCQR and NSQR are 0 based values. NumSubIoQAllocFromAdapter and
             * NumSubIoQAllocFromAdapter are 1 based values.
             */
            pQI->NumSubIoQAllocFromAdapter = GET_WORD_0(pCplEntry->DW0) + 1;
            pQI->NumCplIoQAllocFromAdapter = GET_WORD_1(pCplEntry->DW0) + 1;

            /*
             * Ensure there is the minimum number of queues between the MSI
             * granted, number of cores, and number allocated from the adapter.
             */
            if (pAE->ntldrDump == FALSE) {
                pQI->NumSubIoQAllocFromAdapter = min(pQI->NumSubIoQAllocFromAdapter,
                                                     pAE->ResMapTbl.NumActiveCores);

                pQI->NumCplIoQAllocFromAdapter = min(pQI->NumCplIoQAllocFromAdapter,
                                                     pAE->ResMapTbl.NumActiveCores);

                /* Only take the min if the number of MSI is greater than 0 */
                if (pAE->ResMapTbl.NumMsiMsgGranted > 0) {
                    pQI->NumSubIoQAllocFromAdapter = min(pQI->NumSubIoQAllocFromAdapter,
                                                         pAE->ResMapTbl.NumMsiMsgGranted);

                    pQI->NumCplIoQAllocFromAdapter = min(pQI->NumCplIoQAllocFromAdapter,
                                                         pAE->ResMapTbl.NumMsiMsgGranted);
                }
            } 

            /* Reset the counter and keep tihs state to set more features */
            pAE->DriverState.StateChkCount = 0;
            pAE->DriverState.NextDriverState = NVMeWaitOnSetFeatures;
        }
    } else if ((pAE->DriverState.TtlLbaRangeExamined <
                pAE->DriverState.IdentifyNamespaceFetched) &&
               (pSetFeaturesCDW10->FID == LBA_RANGE_TYPE)) {
        /* first check completion status code */
        if (pCplEntry->DW3.SF.SC != 0) {
           /* Support for LBA Range Type is optional,
            * handle the case that it's not supported 
            */
            if ((pCplEntry->DW3.SF.SC == INVALID_FIELD_IN_COMMAND) &&
                (pCplEntry->DW3.SF.SCT == GENERIC_COMMAND_STATUS)) {
                /*
                 * The device doesn't support the optional LBA Range Type
                 * command so assume the slot is online/visible and writable
                 */
                lunId = pAE->DriverState.VisibleNamespacesExamined;

                pLunExt = pAE->pLunExtensionTable[lunId];
                pLunExt->slotStatus = ONLINE;
                pLunExt->ReadOnly = FALSE;
                pAE->DriverState.VisibleNamespacesExamined++;
                pAE->DriverState.ConfigLbaRangeNeeded = FALSE;
                pAE->DriverState.TtlLbaRangeExamined++;

                /* Reset the counter and set next state accordingly */
                pAE->DriverState.StateChkCount = 0;
                if (pAE->DriverState.TtlLbaRangeExamined ==
                    pAE->controllerIdentifyData.NN) {
                    /* We have called identify namespace as well as get/set
                     * features for each of the NN namespaces that exist.
                     * Move on to the next state in the state machine.
                     */

                    pAE->visibleLuns = pAE->DriverState.VisibleNamespacesExamined;
                    pAE->DriverState.NextDriverState = NVMeWaitOnSetupQueues;
                } else {
                    /* We have more namespaces to identify so
                     * we'll set the state to NVMeWaitOnIdentifyNS in order
                     * to identify the remaining namespaces.
                     */
                    pAE->DriverState.NextDriverState = NVMeWaitOnIdentifyNS;
                }
            } else {
                NVMeDriverFatalError(pAE,
                                    (1 << START_STATE_LBA_RANGE_CHK_FAILURE));
            }
        } else {
            /*
             * When Get Features command completes, exam the completed data to
             * see if Set Features is required. If so, simply set
             * ConfigLbaRangeNeeded as TRUE If not, simply increase
             * TtlLbaRangeExamined and set ConfigLbaRangeNeeded as FALSE When Set
             * Features command completes, simply increase TtlLbaRangeExamined
             */
            lunId = pAE->DriverState.VisibleNamespacesExamined;

            pLbaRangeTypeEntry =
                (PADMIN_SET_FEATURES_COMMAND_LBA_RANGE_TYPE_ENTRY)
                pAE->DriverState.pDataBuffer;
            pSetFeaturesCDW11 =
                (PADMIN_SET_FEATURES_COMMAND_LBA_RANGE_TYPE_DW11) &pNVMeCmd->CDW11;

            pLunExt = pAE->pLunExtensionTable[lunId];

            if (pNVMeCmd->CDW0.OPC == ADMIN_GET_FEATURES) {
                /* driver only supports 1 LBA range type per NS (NUM is 0 based) */
                if (pSetFeaturesCDW11->NUM == 0) {

                        /*
                     *
                     * NOTE:  spec/group still working this behavior out
                     * wrt dealing with range tpyes so making this simple
                     * for now so we can update it when needed.  Currently
                     * we'll IGNORE the range TYPE entirely
                     *
                         */
                    StorPortDebugPrint(INFO,
                           "pLbaRangeTypeEntry type : 0x%lX lun id %d nsid 0x%x\n",
                           pLbaRangeTypeEntry->Type,
                           lunId,
                           pNVMeCmd->NSID);

                    visibility =
                        pLbaRangeTypeEntry->Attributes.Hidden ? HIDDEN:VISIBLE;
                    pLunExt->ReadOnly =
                            pLbaRangeTypeEntry->Attributes.Overwriteable ?
                                FALSE:TRUE;
                    /*
                     *  When the namespace is formatted using metadata, 
                     *  don't advertise it for now 
                     */
                    flbas = pLunExt->identifyData.FLBAS.SupportedCombination;
                    metadataSize = pLunExt->identifyData.LBAFx[flbas].MS;
                    if (metadataSize != 0)
                       visibility = HIDDEN;

                } else {
                    /*
                     * Don't support more than one entry per NS. Mark it IGNORED
                     */
                    visibility = IGNORED;
                }

                pAE->DriverState.ConfigLbaRangeNeeded = FALSE;
                pAE->DriverState.TtlLbaRangeExamined++;
                if (visibility == VISIBLE) {
                    pLunExt->slotStatus = ONLINE;
                    pAE->DriverState.VisibleNamespacesExamined++;
                } else {
                    StorPortDebugPrint(INFO,"NVMeSetFeaturesCompletion: FYI LnuExt at %d has been cleared (NSID not visible)\n",
                        lunId);
                    RtlZeroMemory(pLunExt, sizeof(NVME_LUN_EXTENSION));
                }

            } else if (pNVMeCmd->CDW0.OPC == ADMIN_SET_FEATURES) {

                /* TODO: set features not currently called, after its ironed out
                 * how we want to handle range types, we'll need to fill
                 * this in
                 */

                /*
                pAE->DriverState.VisibleNamespacesExamined++;
                pLunExt->ReadOnly = FALSE;
                pAE->DriverState.TtlLbaRangeExamined++;
                pAE->DriverState.ConfigLbaRangeNeeded = FALSE;
                */
            }

            /* Reset the counter and set next state accordingly */
            pAE->DriverState.StateChkCount = 0;
            if (pAE->DriverState.TtlLbaRangeExamined ==
                pAE->controllerIdentifyData.NN) {
                /* We have called identify namespace as well as get/set
                 * features for each of the NN namespaces that exist.
                 * Move on to the next state in the state machine.
                 */
                pAE->visibleLuns = pAE->DriverState.VisibleNamespacesExamined;
                pAE->DriverState.NextDriverState = NVMeWaitOnSetupQueues;
            } else {
                /* We have more namespaces to identify and get/set features
                 * for. But before we can move on to the next namespace,
                 * we'll check if we need to call set features - if not,
                 * we'll set the state to NVMeWaitOnIdentifyNS so we can
                 * continue to identify the remaining namespaces.
                 */
                if (TRUE == pAE->DriverState.ConfigLbaRangeNeeded) {
                    // We still need to issue a set features for this namespace
                pAE->DriverState.NextDriverState = NVMeWaitOnSetFeatures;
                } else {
                    // Move on to the next namespace and issue an identify
                    pAE->DriverState.NextDriverState = NVMeWaitOnIdentifyNS;
                }
            }
        }
    }
} /* NVMeSetFeaturesCompletion */


/*******************************************************************************
 * NVMePreparePRPs
 *
 * @brief NVMePreparePRPs is a helper routine to prepare PRP entries
 *        for initialization routines and ioctl requests.
 *
 * @param pAE - Pointer to hardware device extension.
 * @param pSrbExt - Pointer to Srb Extension
 * @param pBuffer - Pointer to the transferring buffer.
 * @param TxLength - Data transfer size in bytes.
 *
 * @return BOOLEAN
 *     TRUE - If the PRP conversion completed without any errors
 *     FALSE - If anything goes wrong
 ******************************************************************************/
static BOOLEAN NVMePreparePRPs(
    PNVME_DEVICE_EXTENSION pAE,
    PNVME_SRB_EXTENSION pSrbExt,
    PVOID pBuffer,
    ULONG TxLength
)
{
    PNVMe_COMMAND pSubEntry = &pSrbExt->nvmeSqeUnit;
    STOR_PHYSICAL_ADDRESS PhyAddr;
    ULONG_PTR PtrTemp = 0;
    ULONG RoomInFirstPage = 0;
    ULONG RemainLength = TxLength;
    PUINT64 pPrpList = NULL;

    if (pBuffer == NULL || TxLength == 0)
        return (FALSE);

    /* Go ahead and prepare 1st PRP entries, need at least one PRP entry */
    PhyAddr = NVMeGetPhysAddr(pAE, pBuffer);
    if (PhyAddr.QuadPart == 0)
        return (FALSE);

    pSubEntry->PRP1 = PhyAddr.QuadPart;
    pSrbExt->numberOfPrpEntries = 1;
    /*
     * Find out how much room still available in current page.
     * If it's enough, only PRP1 is needed.
     */
    RoomInFirstPage = PAGE_SIZE - (PhyAddr.QuadPart & (PAGE_SIZE - 1));
    if ( RoomInFirstPage >= TxLength )
        return (TRUE);
    else
        RemainLength -= RoomInFirstPage;

    PtrTemp = (ULONG_PTR)((PUCHAR)pBuffer);
    PtrTemp = PAGE_ALIGN_BUF_ADDR(PtrTemp);
    /*
     * With the remaining transfer size, either use PRP2 as another PRP entry or
     * a pointer to the pre-allocated PRP list
     */
    if (RemainLength > PAGE_SIZE) {
        pPrpList = &pSrbExt->prpList[0];
        pSubEntry->PRP2 = 0;
    } else {
        /* Use PRP2 as 2nd PRP entry and return */
        PhyAddr = NVMeGetPhysAddr(pAE, (PVOID)PtrTemp);
        if (PhyAddr.QuadPart == 0)
            return (FALSE);
        pSubEntry->PRP2 = PhyAddr.QuadPart;
        pSrbExt->numberOfPrpEntries++;
        return (TRUE);
    }
    /* 
     * Convert data buffer pages into PRP entries while
     * decreasing remaining transfer size and noting the # of PRP entries used
     */
    while (RemainLength) {
        PhyAddr = NVMeGetPhysAddr(pAE, (PVOID)PtrTemp);
        if (PhyAddr.QuadPart == 0)
            return (FALSE);
        *pPrpList = (UINT64) PhyAddr.QuadPart;
        pSrbExt->numberOfPrpEntries++;
        /* When remaining size is no larger than a page, it's the last entry */
        if (RemainLength <= PAGE_SIZE)
            break;
        pPrpList++;
        RemainLength -= PAGE_SIZE;
        PtrTemp += PAGE_SIZE;
    }

    return (TRUE);
}


/*******************************************************************************
 * NVMeInitCallback
 *
 * @brief NVMeInitCallback is the callback function used to notify
 *        Initialization module the completion of the requests, which are
 *        initiated by driver's initialization module itself. In addition to the
 *        NVME_DEVICE_EXTENSION, the completion entry are also passed. After
 *        examining the entry, some resulted information needs to be noted and
 *        error status needs to be reported if there is any.
 *
 * @param pAE - Pointer to hardware device extension.
 * @param pSrbExtension - Pointer to the completion entry
 *
 * @return BOOLEAN
 *     TRUE - If all resources are allocated and initialized properly
 *     FALSE - If anything goes wrong
 ******************************************************************************/
static BOOLEAN NVMeInitCallback(
    PVOID pNVMeDevExt,
    PVOID pSrbExtension
)
{
    PNVME_DEVICE_EXTENSION pAE = (PNVME_DEVICE_EXTENSION)pNVMeDevExt;
    PNVME_SRB_EXTENSION pSrbExt = (PNVME_SRB_EXTENSION)pSrbExtension;
    PNVMe_COMMAND pNVMeCmd = (PNVMe_COMMAND)(&pSrbExt->nvmeSqeUnit);
    PNVMe_COMPLETION_QUEUE_ENTRY pCplEntry = pSrbExt->pCplEntry;
    PQUEUE_INFO pQI = &pAE->QueueInfo;

    NVMe_CONTROLLER_CAPABILITIES CAP;
    CAP.AsUlonglong = 0;
    StorPortDebugPrint(INFO,"NVMeInitCallback: Driver state: %d\n", pAE->DriverState.NextDriverState);
    switch (pAE->DriverState.NextDriverState) {
        case NVMeWaitOnIdentifyCtrl:
            /*
             * Mark down Controller structure is retrieved if succeeded
             * Otherwise, log the error bit in case of errors and fail the state
             * machine.
             */
            if ((pCplEntry->DW3.SF.SC == 0) &&
                (pCplEntry->DW3.SF.SCT == 0)) {
                ULONG maxXferSize;

                /* Reset the counter and set next state */
                pAE->DriverState.NextDriverState = NVMeWaitOnIdentifyNS;
                pAE->DriverState.StateChkCount = 0;

                /* copy over the data from the init state machine temp buffer */
                StorPortCopyMemory(&pAE->controllerIdentifyData,
                                pAE->DriverState.pDataBuffer,
                                sizeof(ADMIN_IDENTIFY_CONTROLLER));

#if (NTDDI_VERSION > NTDDI_WIN7) && defined(_WIN64)
                CAP.AsUlonglong = StorPortReadRegisterUlong64(pAE,
                    (PULONG64)(&pAE->pCtrlRegister->CAP));
#else
                CAP.HighPart = StorPortReadRegisterUlong(pAE,
                    (PULONG)(&pAE->pCtrlRegister->CAP.HighPart));
                CAP.LowPart = StorPortReadRegisterUlong(pAE,
                    (PULONG)(&pAE->pCtrlRegister->CAP.LowPart));
#endif

                /*
                 * we don't discover the HW xfer limit until after we've reported it
                 * to storport so if we find out its smaller than what we'rve reported,
                 * then all we can do is fail init and log and error.  The user will
                 * have to reconfigure the regsitry and try again
                  */
                if (pAE->controllerIdentifyData.MDTS > 0) {
                    maxXferSize = (1 << pAE->controllerIdentifyData.MDTS) *
                        (1 << (12 + CAP.MPSMIN)) ;
                    if (pAE->InitInfo.MaxTxSize > maxXferSize) {
                        StorPortDebugPrint(INFO, "ERROR: Ctrl reports smaller Max Xfer Sz than INF (0x%x < 0x%x)\n",
                            maxXferSize, pAE->InitInfo.MaxTxSize);
                        NVMeDriverFatalError(pAE,
                                            (1 << START_MAX_XFER_MISMATCH_FAILURE));
                    }
                }
            } else {
                NVMeDriverFatalError(pAE,
                                    (1 << START_STATE_IDENTIFY_CTRL_FAILURE));
            }
        break;
        case NVMeWaitOnIdentifyNS:
            /*
             * Mark down Namespace structure is retrieved if succeeded
             * Otherwise, log the error bit in case of errors and
             * fail the state machine
             */
            if ((pCplEntry->DW3.SF.SC == 0) &&
                (pCplEntry->DW3.SF.SCT == 0)) {
                PNVME_LUN_EXTENSION pLunExt =
                    pAE->pLunExtensionTable[pAE->DriverState.VisibleNamespacesExamined];

                pAE->DriverState.IdentifyNamespaceFetched++;

                /* Reset the counter and set next state */
                pAE->DriverState.StateChkCount = 0;

                /* Move to the next state to set features for this namespace */
                pAE->DriverState.NextDriverState = NVMeWaitOnSetFeatures;

                /* copy over the data from the init state machine temp buffer */
                StorPortCopyMemory(&pLunExt->identifyData,
                                pAE->DriverState.pDataBuffer,
                                sizeof(ADMIN_IDENTIFY_NAMESPACE));
                /* Mark down the Namespace ID */
                pLunExt->namespaceId =
                    pAE->DriverState.IdentifyNamespaceFetched;

                /* Note next Namespace ID to fetch Namespace structure */
                pAE->DriverState.CurrentNsid++;

            } else {
                /*
                 * In case of supporting non-contiguous NSID and the inactive NSIDs, 
                 * move onto next NSID until it hits the value of NN of 
                 * Identify Controller structure.
                 */
                if ((pCplEntry->DW3.SF.SC == INVALID_NAMESPACE_OR_FORMAT) &&
                    (pCplEntry->DW3.SF.SCT == 0) &&
                    (pAE->DriverState.CurrentNsid < pAE->controllerIdentifyData.NN)) {
                    pAE->DriverState.CurrentNsid++;

                    /* Reset the counter */
                    pAE->DriverState.StateChkCount = 0;

                    /* Stay in this state to fetch next namespace structure */
                    pAE->DriverState.NextDriverState = NVMeWaitOnIdentifyNS;
                } else {
                    NVMeDriverFatalError(pAE,
                                    (1 << START_STATE_IDENTIFY_NS_FAILURE));
                }
            }
        break;
        case NVMeWaitOnSetFeatures:
            NVMeSetFeaturesCompletion(pAE, pNVMeCmd, pCplEntry);
        break;
        case NVMeWaitOnIoCQ:
            /*
             * Mark down the number of created completion queues if succeeded
             * Otherwise, log the error bit in case of errors and
             * fail the state machine
             */

            if ((pCplEntry->DW3.SF.SC == 0) &&
                (pCplEntry->DW3.SF.SCT == 0)) {
                pQI->NumCplIoQCreated++;

                /* Reset the counter and set next state */
                pAE->DriverState.StateChkCount = 0;
                if (pQI->NumCplIoQAllocated == pQI->NumCplIoQCreated) {
                    pAE->DriverState.NextDriverState = NVMeWaitOnIoSQ;
                } else {
                    pAE->DriverState.NextDriverState = NVMeWaitOnIoCQ;
                }
            } else {
                NVMeDriverFatalError(pAE,
                                    (1 << START_STATE_CPLQ_CREATE_FAILURE));
            }
        break;
        case NVMeWaitOnIoSQ:
            /*
             * Mark down the number of created submission queues if succeeded
             * Otherwise, log the error bit in case of errors and
             * fail the state machine
             */
            if ((pCplEntry->DW3.SF.SC == 0) &&
                (pCplEntry->DW3.SF.SCT == 0)) {
                PRES_MAPPING_TBL pRMT = &pAE->ResMapTbl;
                pQI->NumSubIoQCreated++;

                /* Reset the counter and set next state */
                pAE->DriverState.StateChkCount = 0;
                if (pQI->NumSubIoQAllocated == pQI->NumSubIoQCreated) {
                    /* if we've learned the cores we're done */
                    if (pAE->LearningCores < pRMT->NumActiveCores) {
                        pAE->DriverState.NextDriverState = NVMeWaitOnLearnMapping;
                    } else {
                        pAE->DriverState.NextDriverState = NVMeStartComplete;
                    }
                } else {
                    pAE->DriverState.NextDriverState = NVMeWaitOnIoSQ;
                }
            } else {
                NVMeDriverFatalError(pAE,
                                    (1 << START_STATE_SUBQ_CREATE_FAILURE));
            }
        break;
        case NVMeWaitOnLearnMapping:
            if ((pCplEntry->DW3.SF.SC == 0) &&
                (pCplEntry->DW3.SF.SCT == 0)) {
                //PRES_MAPPING_TBL pRMT = &pAE->ResMapTbl;
                ULONG totalQueuePairs;

                /*
                 * see if we still have more core/vector mapping to learn
                 * or if we're ready to redo the queues based on the new map
                 */
                pAE->DriverState.StateChkCount = 0;
                totalQueuePairs = min(pAE->QueueInfo.NumCplIoQAllocFromAdapter,
                                      pAE->QueueInfo.NumSubIoQAllocFromAdapter);
                if (pAE->LearningCores < totalQueuePairs) {
                    pAE->DriverState.NextDriverState = NVMeWaitOnLearnMapping;
                } else {
#if DBG
                    PRES_MAPPING_TBL pRMT = &pAE->ResMapTbl;
                    PCORE_TBL pCT = NULL;
                    ULONG CoreIndex;

                    /* print out the learned table */
                    StorPortDebugPrint(INFO, "Learning Complete.  Core Table:\n");
                    for (CoreIndex = 0; CoreIndex < pRMT->NumActiveCores; CoreIndex++) {
                        pCT = pRMT->pCoreTbl + CoreIndex;
                        StorPortDebugPrint(INFO,
                                           "\tCore(0x%x) MSID(0x%x) QueuePair(0x%x)\n",
                                           CoreIndex,
                                           pCT->MsiMsgID,
                                           pCT->SubQueue);

                    }

                    pAE->LearningComplete = TRUE;
#endif
                    /*
                     * Learning is done. Any additional cores not assigned to
                     * queues will be assigned in the start complete state.
                     */

                    pAE->DriverState.NextDriverState = NVMeWaitOnReSetupQueues;
                }
            } else {
                PRES_MAPPING_TBL pRMT = &pAE->ResMapTbl;

                /* possibly no NS exists at all, either way this isn't fatal */
                StorPortDebugPrint(INFO,
                    "NVMeInitCallback: WARNING: no learning possible, SC 0x%x SCT 0x%x\n",
                     pCplEntry->DW3.SF.SC, pCplEntry->DW3.SF.SCT);
                pAE->LearningCores = pRMT->NumActiveCores;
                pAE->DriverState.NextDriverState = NVMeStartComplete;
            }
        break;
        case NVMeWaitOnReSetupQueues:
            if (TRUE == NVMeDeleteQueueCallback(pAE, pSrbExt)) {
                /* if we've deleted the last CQ, we rae ready to recreate them */
                if (0 == pQI->NumCplIoQCreated) {
                    pAE->DriverState.NextDriverState = NVMeWaitOnIoCQ;
                } else {
                    pAE->DriverState.NextDriverState = NVMeWaitOnReSetupQueues;
                }
            }
        break;
        case NVMeStartComplete:
            ASSERT(pAE->ntldrDump);
        break;
        default:
            NVMeDriverFatalError(pAE,
                                (1 << START_STATE_UNKNOWN_STATE_FAILURE));
        break;
    } /* end switch */

    NVMeCallArbiter(pAE);

    return (TRUE);
} /* NVMeInitCallback */


/*******************************************************************************
 * NVMeResetController
 *
 * @brief Main routine entry for resetting the controller
 *
 * @param pHwDeviceExtension - Pointer to device extension
 * @param pSrb - Pointer to SRB
 *
 * @return BOOLEAN
 *    TRUE - DPC was successufully issued
 *    FALSE - DPC was unsuccessufully issued or recovery not possible
 ******************************************************************************/
static BOOLEAN NVMeResetController(
    __in PNVME_DEVICE_EXTENSION pAdapterExtension,
#if (NTDDI_VERSION > NTDDI_WIN7)
    __in PSTORAGE_REQUEST_BLOCK pSrb
#else
    __in PSCSI_REQUEST_BLOCK pSrb
#endif
)
{
    BOOLEAN storStatus = FALSE;

    /* In Hibernation/CrashDump mode, reset command is ignored. */
    if (pAdapterExtension->ntldrDump == TRUE) {
        return TRUE; 
    }
    
    /**
     * We only allow one recovery attempt at a time
     * if the DPC is sceduled then one has started,
     * when completed and we're ready for IOs again
     * we'll set the flag to allow recovery again.
     * Recoery runs at DPC level and grabs the startio
     * and INT spinlocks to assure no submission or
     * completion threads are in progress during reset
     */
    if (pAdapterExtension->RecoveryAttemptPossible == TRUE) {
        /* We don't want any new stoport reqeusts during reset */
        StorPortBusy(pAdapterExtension, STOR_ALL_REQUESTS);
        StorPortDebugPrint(INFO,
                       "NVMeResetController: Issue DPC.\n");
        storStatus = StorPortIssueDpc(pAdapterExtension,
                                      &pAdapterExtension->RecoveryDpc,
                                      pSrb,
                                      NULL);

        if (storStatus == TRUE) {
            pAdapterExtension->RecoveryAttemptPossible = FALSE;
            StorPortDebugPrint(INFO,
                       "NVMeResetController: Issue DPC succeeds.\n");
        }
    } else {
        DbgPrint("NVMeResetController: reset called but already pending\n");
        storStatus = TRUE;
    }

    return storStatus;
} /* NVMeResetController */


/*******************************************************************************
 * NVMeGetCplEntry
 *
 * @brief NVMeGetCplEntry gets called to retrieve one completion queue entry at
 *        a time from the specified completion queue. This routine is
 *        responsible for looking up teh embedded PhaseTag and determine if the
 *        entry is a newly completed one. If not, return
 *        STOR_STATUS_UNSUCCESSFUL. Otherwise, do the following:
 *
 *        1. Copy the completion entry to pCplEntry
 *        2. Increase the Completion Queue Head Pointer by one.
 *        3. Reset the Head Pointer to 0 and change CurPhaseTag if it's equal to
 *           the associated CplQEntries
 *        4. Program the associated Doorbell register with the Head Pointer
 *        5. Return STOR_STATUS_SUCCESS
 *
 * @param pAE - Pointer to hardware device extension.
 * @param QueueID - Which completion queue to retrieve entry from
 * @param pCplEntry - The caller prepared buffer to save the retrieved entry
 *
 * @return ULONG
 *    STOR_STATUS_SUCCESS - If there is one newly completed entry returned
 *    STOR_STATUS_UNSUCCESSFUL - If there is no newly completed entry
 *    Otherwise - If anything goes wrong
 ******************************************************************************/
static ULONG NVMeGetCplEntry(
    PNVME_DEVICE_EXTENSION pAE,
    PCPL_QUEUE_INFO pCQI,
    PVOID pCplEntry
)
{
    PQUEUE_INFO pQI = &pAE->QueueInfo;
    PNVMe_COMPLETION_QUEUE_ENTRY pCQE = NULL;

    /* Make sure the parameters are valid */
    if (pCQI->CplQueueID > pQI->NumCplIoQCreated || pCplEntry == NULL)
        return (STOR_STATUS_INVALID_PARAMETER);

    pCQE = (PNVMe_COMPLETION_QUEUE_ENTRY)pCQI->pCplQStart;
    pCQE += pCQI->CplQHeadPtr;

    /* Check Phase Tag to determine if it's a newly completed entry */
    if (pCQI->CurPhaseTag != pCQE->DW3.SF.P) {
        *(ULONG_PTR *)pCplEntry = (ULONG_PTR)pCQE;
        pCQI->CplQHeadPtr++;
        pCQI->Completions++;

        if (pCQI->CplQHeadPtr == pCQI->CplQEntries) {
            pCQI->CplQHeadPtr = 0;
            pCQI->CurPhaseTag = !pCQI->CurPhaseTag;
        }

        return STOR_STATUS_SUCCESS;
    }

    return STOR_STATUS_UNSUCCESSFUL;
} /* NVMeGetCplEntry */


/******************************************************************************
 * SntiSetScsiSenseData
 *
 * @brief Sets up the SCSI sense data in the provided SRB.
 *
 *        NOTE: The caller of this func must set the correlating SRB status
 *
 * @param pSrb - This parameter specifies the SCSI I/O request. SNTI expects
 *               that the user can access the SCSI CDB, response, and data from
 *               this pointer. For example, if there is a failure in translation
 *               resulting in sense data, then SNTI will call the appropriate
 *               internal error handling code and set the status info/data and
 *               pass the pSrb pointer as a parameter.
 * @param scsiStatus - SCSI Status to be stored in the Sense Data buffer
 * @param senseKey - Sense Key for the Sense Data
 * @param asc - Additional Sense Code (ASC)
 * @param ascq - Additional Sense Code Qualifier (ASCQ)
 *
 * @return BOOLEAN
 ******************************************************************************/
static BOOLEAN SntiSetScsiSenseData(
#if (NTDDI_VERSION > NTDDI_WIN7)
    PSTORAGE_REQUEST_BLOCK pSrb,
#else
    PSCSI_REQUEST_BLOCK pSrb,
#endif
    UCHAR scsiStatus,
    UCHAR senseKey,
    UCHAR asc,
    UCHAR ascq
)
{
    PSENSE_DATA pSenseData = NULL;
    BOOLEAN status = TRUE;
    UCHAR senseInfoBufferLength = 0;
    PVOID senseInfoBuffer = NULL;
#if (NTDDI_VERSION > NTDDI_WIN7)
    SrbSetScsiData(pSrb, NULL, NULL, &scsiStatus, NULL, NULL);
    SrbGetScsiData(pSrb, NULL, NULL, NULL, &senseInfoBuffer, &senseInfoBufferLength);
#else
    pSrb->ScsiStatus = scsiStatus;
    senseInfoBufferLength = pSrb->SenseInfoBufferLength;
    senseInfoBuffer = pSrb->SenseInfoBuffer;
#endif
    if ((scsiStatus != SCSISTAT_GOOD) &&
        (senseInfoBufferLength >= sizeof(SENSE_DATA))) {

        pSenseData = (PSENSE_DATA)senseInfoBuffer;

        memset(pSenseData, 0, senseInfoBufferLength);
        pSenseData->ErrorCode                    = FIXED_SENSE_DATA;
        pSenseData->SenseKey                     = senseKey;
        pSenseData->AdditionalSenseCode          = asc;
        pSenseData->AdditionalSenseCodeQualifier = ascq;

        pSenseData->AdditionalSenseLength = sizeof(SENSE_DATA) -
            FIELD_OFFSET(SENSE_DATA, CommandSpecificInformation);
#if (NTDDI_VERSION > NTDDI_WIN7)
        SrbSetScsiData(pSrb, NULL, NULL, NULL, NULL, sizeof(SENSE_DATA));
#else
        pSrb->SenseInfoBufferLength = sizeof(SENSE_DATA);
#endif
        pSrb->SrbStatus |= SRB_STATUS_AUTOSENSE_VALID;
    } else {
#if (NTDDI_VERSION > NTDDI_WIN7)
        SrbSetScsiData(pSrb, NULL, NULL, NULL, NULL, 0);
#else
        pSrb->SenseInfoBufferLength = 0;
#endif
        status = FALSE;
    }

   return status;
}


/******************************************************************************
 * SntiMapGenericCommandStatus
 *
 * @brief Maps the NVM Express Generic Command Status to a SCSI Status Code,
 *        sense key, and Additional Sense Code/Qualifier (ASC/ASCQ) where
 *        applicable.
 *
 * @param pSrb - This parameter specifies the SCSI I/O request. SNTI expects
 *               that the user can access the SCSI CDB, response, and data from
 *               this pointer. For example, if there is a failure in translation
 *               resulting in sense data, then SNTI will call the appropriate
 *               internal error handling code and set the status info/data and
 *               pass the pSrb pointer as a parameter.
 * @param genericCommandStatus - NVMe Generic Command Status to translate
 *
 * @return VOID
 ******************************************************************************/
static VOID SntiMapGenericCommandStatus(
#if (NTDDI_VERSION > NTDDI_WIN7)
    PSTORAGE_REQUEST_BLOCK pSrb,
#else
    PSCSI_REQUEST_BLOCK pSrb,
#endif
    UINT8 genericCommandStatus
)
{
    //PSENSE_DATA pSenseData = NULL;
    SNTI_RESPONSE_BLOCK responseData;
    //BOOLEAN status = TRUE;

    memset(&responseData, 0, sizeof(SNTI_RESPONSE_BLOCK));

    /**
     * Perform table lookup for Generic Command Status translation
     *
     * Generic Status Code Values:
     *   0x00 - 0x0B and
     *   0x80 - 0x82 (0x0C - 0xE)
     *
     * Check bit 7 to see if this is a NVM Command Set status, if so then
     * start at 0xC to index into lookup table
     */
    if ((genericCommandStatus & NVM_CMD_SET_STATUS) != NVM_CMD_SET_STATUS)
        responseData = genericCommandStatusTable[genericCommandStatus];
    else
        responseData = genericCommandStatusTable[genericCommandStatus -
                       NVM_CMD_SET_GENERIC_STATUS_OFFSET];

    SntiSetScsiSenseData(pSrb,
                         responseData.StatusCode,
                         responseData.SenseKey,
                         responseData.ASC,
                         responseData.ASCQ);

    /* Override the SRB Status */
    pSrb->SrbStatus |= responseData.SrbStatus;
} /* SntiMapGenericCommandStatus */


/******************************************************************************
 * SntiMapCommandSpecificStatus
 *
 * @brief Maps the NVM Express Command Specific Status to a SCSI Status Code,
 *        sense key, and Additional Sense Code/Qualifier (ASC/ASCQ) where
 *        applicable.
 *
 * @param pSrb - This parameter specifies the SCSI I/O request. SNTI expects
 *               that the user can access the SCSI CDB, response, and data from
 *               this pointer. For example, if there is a failure in translation
 *               resulting in sense data, then SNTI will call the appropriate
 *               internal error handling code and set the status info/data and
 *               pass the pSrb pointer as a parameter.
 * @param commandSpecificStatus - NVMe Command Specific Status to translate
 *
 * @return VOID
 ******************************************************************************/
static VOID SntiMapCommandSpecificStatus(
#if (NTDDI_VERSION > NTDDI_WIN7)
    PSTORAGE_REQUEST_BLOCK pSrb,
#else
    PSCSI_REQUEST_BLOCK pSrb,
#endif
    UINT8 commandSpecificStatus
)
{
    //PSENSE_DATA pSenseData = NULL;
    SNTI_RESPONSE_BLOCK responseData;
    //BOOLEAN status = TRUE;

    memset(&responseData, 0, sizeof(SNTI_RESPONSE_BLOCK));

    /**
     * Perform table lookup for Generic Command Status translation
     *
     * Command Specific Status Code Values:
     *   0x00 - 0x0A and
     *   0x80 (0x0B)
     *
     * Check bit 7 to see if this is a NVM Command Set status, if so then
     * start at 0xB to index into lookup table
     */
    if ((commandSpecificStatus & NVM_CMD_SET_STATUS) != NVM_CMD_SET_STATUS)
        responseData = commandSpecificStatusTable[commandSpecificStatus];
    else
        responseData = commandSpecificStatusTable[commandSpecificStatus -
                       NVM_CMD_SET_SPECIFIC_STATUS_OFFSET];

    SntiSetScsiSenseData(pSrb,
                         responseData.StatusCode,
                         responseData.SenseKey,
                         responseData.ASC,
                         responseData.ASCQ);

    /* Override the SRB Status */
    pSrb->SrbStatus |= responseData.SrbStatus;
} /* SntiMapCommandSpecificStatus */


/******************************************************************************
 * SntiMapMediaErrors
 *
 * @brief Maps the NVM Express Media Error Status to a SCSI Status Code, sense
 *        key, and Additional Sense Code/Qualifier (ASC/ASCQ) where applicable.
 *
 * @param pSrb - This parameter specifies the SCSI I/O request. SNTI expects
 *               that the user can access the SCSI CDB, response, and data from
 *               this pointer. For example, if there is a failure in translation
 *               resulting in sense data, then SNTI will call the appropriate
 *               internal error handling code and set the status info/data and
 *               pass the pSrb pointer as a parameter.
 * @param mediaError - NVMe Media Error Status to translate
 *
 * @return VOID
 ******************************************************************************/
static VOID SntiMapMediaErrors(
#if (NTDDI_VERSION > NTDDI_WIN7)
    PSTORAGE_REQUEST_BLOCK pSrb,
#else
    PSCSI_REQUEST_BLOCK pSrb,
#endif
    UINT8 mediaError
)
{
    //PSENSE_DATA pSenseData = NULL;
    SNTI_RESPONSE_BLOCK responseData;
    //BOOLEAN status = TRUE;

    memset(&responseData, 0, sizeof(SNTI_RESPONSE_BLOCK));

    /*
     * Perform table lookup for Generic Command Status translation
     *
     * Media Error Status Code Values: 0x80 - 0x86
     */
    responseData = mediaErrorTable[mediaError - NVM_MEDIA_ERROR_STATUS_OFFSET];

    SntiSetScsiSenseData(pSrb,
                         responseData.StatusCode,
                         responseData.SenseKey,
                         responseData.ASC,
                         responseData.ASCQ);

    /* Override the SRB Status */
    pSrb->SrbStatus |= responseData.SrbStatus;
} /* SntiMapMediaErrors */


/******************************************************************************
 * SntiMapCompletionStatus
 *
 * @brief Entry function to perform the mapping of the NVM Express Command
 *        Status to a SCSI Status Code, sense key, and Additional Sense
 *        Code/Qualifier (ASC/ASCQ) where applicable.
 *
 * @param pSrb - This parameter specifies the SCSI I/O request. SNTI expects
 *               that the user can access the SCSI CDB, response, and data from
 *               this pointer. For example, if there is a failure in translation
 *               resulting in sense data, then SNTI will call the appropriate
 *               internal error handling code and set the status info/data and
 *               pass the pSrb pointer as a parameter.
 *
 * @return BOOLEAN
 *     Status to indicate if the status translation was successful.
 ******************************************************************************/
static BOOLEAN SntiMapCompletionStatus(
    PNVME_SRB_EXTENSION pSrbExt
)
{
#if (NTDDI_VERSION > NTDDI_WIN7)
    PSTORAGE_REQUEST_BLOCK pSrb = pSrbExt->pSrb;
#else
    PSCSI_REQUEST_BLOCK pSrb = pSrbExt->pSrb;
#endif
    UINT8 statusCodeType = (UINT8)pSrbExt->pCplEntry->DW3.SF.SCT;
    UINT8 statusCode = (UINT8)pSrbExt->pCplEntry->DW3.SF.SC;
    BOOLEAN returnValue = TRUE;

    if (pSrb != NULL) {
        switch(statusCodeType) {
            case GENERIC_COMMAND_STATUS:
                SntiMapGenericCommandStatus(pSrb, statusCode);
            break;
            case COMMAND_SPECIFIC_ERRORS:
                SntiMapCommandSpecificStatus(pSrb, statusCode);
            break;
            case MEDIA_ERRORS:
                SntiMapMediaErrors(pSrb, statusCode);
            break;
            default:
                returnValue = FALSE;
            break;
        }

        if(pSrb->Function == SRB_FUNCTION_ABORT_COMMAND){                
            if ((pSrb->SrbStatus & SRB_STATUS_SUCCESS) 
                != SRB_STATUS_SUCCESS) {
                    pSrbExt->failedAbortCmdCnt++;
            }
            if (pSrbExt->abortedCmdCount) 
                pSrbExt->abortedCmdCount--;
            if (pSrbExt->issuedAbortCmdCnt) 
                pSrbExt->issuedAbortCmdCnt--;

            if (pSrbExt->issuedAbortCmdCnt == 0) {
                if (pSrbExt->abortedCmdCount || 
                    pSrbExt->failedAbortCmdCnt) {
                        pSrb->SrbStatus = SRB_STATUS_ERROR;
                }
                returnValue = TRUE;
            }
            else
                returnValue = FALSE;
        }

    } else {
        returnValue = FALSE;
    }
    ASSERT(returnValue == TRUE);
    return returnValue;
} /* SntiMapCompletionStatus */


/*******************************************************************************
 * NVMeCompleteCmd
 *
 * @brief NVMeCompleteCmd gets called to recover the context saved in the
 *        associated CMD_ENTRY structure with the specificed CmdID. Normally
 *        this routine is called when the caller is about to complete the
 *        request and notify StorPort.
 *
 * @param pAE - Pointer to hardware device extension.
 * @param QueueID - Which submission queue to recover the context from
 * @param CmdID - The acquired CmdID used to de-reference the CMD_ENTRY
 * @param pContext - Caller prepared buffer to save the original context
 *
 * @return VOID
 ******************************************************************************/
static VOID NVMeCompleteCmd(
    PNVME_DEVICE_EXTENSION pAE,
    USHORT QueueID,
    SHORT NewHead,
    USHORT CmdID,
    PVOID pContext
)
{
    PQUEUE_INFO pQI = &pAE->QueueInfo;
    PSUB_QUEUE_INFO pSQI = NULL;
    PCMD_ENTRY pCmdEntry = NULL;

    /* Make sure the parameters are valid */
    ASSERT((QueueID <= pQI->NumSubIoQCreated) && (pContext != NULL));

    /*
     * Identify the target submission queue/cmd entry
     * and update the head pointer for the SQ if needed
     */
    pSQI = pQI->pSubQueueInfo + QueueID;

    pCmdEntry = ((PCMD_ENTRY)pSQI->pCmdEntry) + CmdID;

    if (NewHead != NO_SQ_HEAD_CHANGE) {
        pSQI->SubQHeadPtr = NewHead;
    }

    /* Ensure the command entry had been acquired */
    ASSERT(pCmdEntry->Pending == TRUE);

    /*
     * Return the original context -- this is a pointer to srb extension
     * (sanity check first that it is not NULL)
     */
    ASSERT(pCmdEntry->Context != NULL);

    if ((pCmdEntry->Pending == FALSE) || (pCmdEntry->Context == NULL)) {
        /*
         * Something bad happened so reset the adapter and hope for the best
         */
        NVMeResetController(pAE, NULL);
        return;
    }

    *((ULONG_PTR *)pContext) = (ULONG_PTR)pCmdEntry->Context;

#ifdef DUMB_DRIVER
    /*
     * For non admin command read, need to copy from the dbl buff to the
     * SRB data buff
     */
    if ((QueueID > 0) &&
        IS_CMD_DATA_IN(((PNVME_SRB_EXTENSION)pCmdEntry->Context)->nvmeSqeUnit.CDW0.OPC)) {
        PNVME_SRB_EXTENSION pSrbExt = (PNVME_SRB_EXTENSION)pCmdEntry->Context;

        ASSERT(pSrbExt);

        StorPortCopyMemory(pSrbExt->pSrbDataVir,
                           pSrbExt->pDblVir,
                           pSrbExt->dataLen);
    }
#endif /* DUMB_DRIVER */

    /* Clear the fields of CMD_ENTRY before appending back to free list */
    pCmdEntry->Pending = FALSE;
    pCmdEntry->Context = 0;

    InsertTailList(&pSQI->FreeQList, &pCmdEntry->ListEntry);
} /* NVMeCompleteCmd */


#if DBG
BOOLEAN gResetTest = FALSE;
ULONG gResetCounter = 0;
ULONG gResetCount = 20000;
#endif
/*******************************************************************************
 * NVMeIssueCmd
 *
 * @brief NVMeIssueCmd can be called to issue one command at a time by ringing
 *        the specific doorbell register with the updated Submission Queue Tail
 *        Pointer. This routine copies the caller prepared submission entry data
 *        pointed by pTempSubEntry to next available submission entry of the
 *        specific queue before issuing the command.
 *
 * @param pAE - Pointer to hardware device extension.
 * @param QueueID - Which submission queue to issue the command
 * @param pTempSubEntry - The caller prepared Submission entry data
 *
 * @return ULONG
 *     STOR_STATUS_SUCCESS - If the command is issued successfully
 *     Otherwise - If anything goes wrong
 ******************************************************************************/
static ULONG NVMeIssueCmd(
    PNVME_DEVICE_EXTENSION pAE,
    USHORT QueueID,
    PVOID pTempSubEntry
)
{
    PQUEUE_INFO pQI = &pAE->QueueInfo;
    PSUB_QUEUE_INFO pSQI = NULL;
    //PCMD_ENTRY pCmdEntry = NULL;
    PNVMe_COMMAND pNVMeCmd = NULL;
    USHORT tempSqTail = 0;

    /* Make sure the parameters are valid */
    if (QueueID > pQI->NumSubIoQCreated || pTempSubEntry == NULL)
        return (STOR_STATUS_INVALID_PARAMETER);

    /*
     * Locate the current submission entry and
     * copy the fields from the temp buffer
     */
    pSQI = pQI->pSubQueueInfo + QueueID;

    /* First make sure FW is draining this SQ */
    tempSqTail = ((pSQI->SubQTailPtr + 1) == pSQI->SubQEntries)
        ? 0 : pSQI->SubQTailPtr + 1;

    if (tempSqTail == pSQI->SubQHeadPtr) {
#ifdef HISTORY
        TracePathSubmit(ISSUE_RETURN_BUSY, QueueID, ((PNVMe_COMMAND)pTempSubEntry)->NSID,
            ((PNVMe_COMMAND)pTempSubEntry)->CDW0, 0, 0, 0);
#endif
        return (STOR_STATUS_INSUFFICIENT_RESOURCES);
    }

    pNVMeCmd = (PNVMe_COMMAND)pSQI->pSubQStart;
    pNVMeCmd += pSQI->SubQTailPtr;

    StorPortCopyMemory((PVOID)pNVMeCmd, pTempSubEntry, sizeof(NVMe_COMMAND));

    /* Increase the tail pointer by 1 and reset it if needed */
    pSQI->SubQTailPtr = tempSqTail;

    /*
     * Track # of outstanding requests for this SQ
     */
    pSQI->Requests++;

#ifdef HISTORY
        TracePathSubmit(ISSUE, QueueID, ((PNVMe_COMMAND)pTempSubEntry)->NSID,
            ((PNVMe_COMMAND)pTempSubEntry)->CDW0, pSQI->SubQTailPtr, 0, 0);
#endif
    /* Now issue the command via Doorbell register */
    StorPortWriteRegisterUlong(pAE, pSQI->pSubTDBL, (ULONG)pSQI->SubQTailPtr);

#if DBG
    if (gResetTest && (gResetCounter++ > gResetCount)) {
        gResetCounter = 0;

        /* quick hack for testing internally driven resets */
        NVMeResetController(pAE, NULL);
        return STOR_STATUS_SUCCESS;
    }
#endif /* DBG */

    /*
     * This is for polling mode.  This code can
     * potentially be updated to implement polling for other situations
     * as well keeping in mind that it was originally implmented to account 
     * for prototype Chatham device that didn't have line INTs
     */
#if defined(ALL_POLLING)
    if (pAE->ResMapTbl.NumMsiMsgGranted == 0) {
        ULONG entryStatus = STOR_STATUS_UNSUCCESSFUL;
        PNVMe_COMPLETION_QUEUE_ENTRY pCplEntry = NULL;
        PNVME_SRB_EXTENSION pSrbExtension = NULL;
        PCPL_QUEUE_INFO pCQI = pQI->pCplQueueInfo + QueueID;
        PRES_MAPPING_TBL pRMT = &pAE->ResMapTbl;
        BOOLEAN learning;

        while (entryStatus != STOR_STATUS_SUCCESS) {
            entryStatus = NVMeGetCplEntry(pAE, pCQI, &pCplEntry);
            if (entryStatus == STOR_STATUS_SUCCESS) {

                NVMeCompleteCmd(pAE,
                    pCplEntry->DW2.SQID,
                    pCplEntry->DW2.SQHD,
                    pCplEntry->DW3.CID,
                    (PVOID)&pSrbExtension);

                if (pSrbExtension != NULL) {

                    pSrbExtension->pCplEntry = pCplEntry;
                    learning = ((pAE->LearningCores < pRMT->NumActiveCores) &&
                            (QueueID > 0)) ? TRUE : FALSE;
                    if (learning) {
                        pAE->LearningCores++;
                    }
                }

                if ((pSrbExtension->pNvmeCompletionRoutine == NULL) &&
                    (SntiMapCompletionStatus(pSrbExtension) == TRUE)) {
                    IO_StorPortNotification(RequestComplete,
                                            pAE,
                                            pSrbExtension->pSrb);

                } else if ((pSrbExtension->pNvmeCompletionRoutine(pAE,
                            (PVOID)pSrbExtension) == TRUE) &&
                            (pSrbExtension->pSrb != NULL)) {
                    IO_StorPortNotification(RequestComplete,
                                            pAE,
                                            pSrbExtension->pSrb);
                }
                StorPortWriteRegisterUlong(pAE,
                                           pCQI->pCplHDBL,
                                           (ULONG)pCQI->CplQHeadPtr);
             }
        }
    }
#endif /* POLLING */

    return STOR_STATUS_SUCCESS;
} /* NVMeIssueCmd */


/*******************************************************************************
 * NVMeGetCmdEntry
 *
 * @brief NVMeGetCmdEntry gets called to acquire an command entry for a request
 *        which needs to be processed in adapter. The returned CMD_INFO contains
 *        CmdID and the associated PRPList that might be used when necessary.
 *
 * @param pAE - Pointer to hardware device extension.
 * @param QueueID - Which submission queue to acquire Cmd ID from
 * @param Context - Depending on callers, this can be the original SRB of the
 *                  request or anything else.
 * @param pCmdInfo - Pointer to a caller prepared buffer where the acquired
 *                   command info is saved.
 *
 * @return ULONG
 *     STOR_STATUS_SUCCESS - If a CmdID is successfully acquired
 *     Otherwise - If anything goes wrong
 ******************************************************************************/
static ULONG NVMeGetCmdEntry(
    PNVME_DEVICE_EXTENSION pAE,
    USHORT QueueID,
    PVOID Context,
    PVOID pCmdInfo
)
{
    PQUEUE_INFO pQI = &pAE->QueueInfo;
    PSUB_QUEUE_INFO pSQI = NULL;
    PCMD_ENTRY pCmdEntry = NULL;
    PLIST_ENTRY pListEntry = NULL;

    if (QueueID > pQI->NumSubIoQCreated || pCmdInfo == NULL)
        return (STOR_STATUS_INVALID_PARAMETER);

    pSQI = pQI->pSubQueueInfo + QueueID;

    if (!IsListEmpty(&pSQI->FreeQList)) {
        /* Retrieve a free CMD_INFO entry for the request */
        pListEntry = RemoveHeadList(&pSQI->FreeQList);
        pCmdEntry = (PCMD_ENTRY)CONTAINING_RECORD(pListEntry, CMD_ENTRY, ListEntry);
    } else {
        StorPortDebugPrint(ERROR,
                           "NVMeGetCmdEntry: <Error> Queue#%d is full!\n",
                           QueueID);
        return (STOR_STATUS_INSUFFICIENT_RESOURCES);
    }


    /* Mark down it's used and save the original context */
    pCmdEntry->Context = Context;
    ASSERT(pCmdEntry->Pending == FALSE);

    pCmdEntry->Pending = TRUE;

    /* Return the CMD_INFO structure */
    *(ULONG_PTR *)pCmdInfo = (ULONG_PTR)(&pCmdEntry->CmdInfo);

    return (STOR_STATUS_SUCCESS);
} /* NVMeGetCmdEntry */


/*******************************************************************************
 * IoCompletionDpcRoutine
 *
 * @brief IO DPC completion routine; called by all IO completion objects
 *
 * @param pHwDeviceExtension - Pointer to device extension
 * @param pSystemArgument1 - MSI-X message Id
 *
 * @return void
 ******************************************************************************/
static VOID
IoCompletionDpcRoutine(
    IN PSTOR_DPC  pDpc,
    IN PVOID  pHwDeviceExtension,
    IN PVOID  pSystemArgument1,
    IN PVOID  pSystemArgument2
    )
{
    PNVME_DEVICE_EXTENSION pAE = (PNVME_DEVICE_EXTENSION)pHwDeviceExtension;
    ULONG MsgID = (ULONG)pSystemArgument1;
    PNVMe_COMPLETION_QUEUE_ENTRY pCplEntry = NULL;
    PNVME_SRB_EXTENSION pSrbExtension = NULL;
    //SNTI_TRANSLATION_STATUS sntiStatus = SNTI_TRANSLATION_SUCCESS;
    ULONG entryStatus = STOR_STATUS_SUCCESS;
    PMSI_MESSAGE_TBL pMMT = NULL;
    PQUEUE_INFO pQI = &pAE->QueueInfo;
    PRES_MAPPING_TBL pRMT = &pAE->ResMapTbl;
    //PSUB_QUEUE_INFO pSQI = NULL;
    PCPL_QUEUE_INFO pCQI = NULL;
    USHORT firstCheckQueue = 0;
    USHORT lastCheckQueue = 0;
    USHORT indexCheckQueue = 0;
    BOOLEAN InterruptClaimed = FALSE;
    STOR_LOCK_HANDLE DpcLockhandle = { 0 };
    STOR_LOCK_HANDLE StartLockHandle = { 0 };

    if (pDpc != NULL) {
        ASSERT(pAE->ntldrDump == FALSE);
		if (pAE->MultipleCoresToSingleQueueFlag) {
			StorPortAcquireSpinLock(pAE, StartIoLock, NULL, &StartLockHandle);
		} else {
			StorPortAcquireSpinLock(pAE, DpcLock, pDpc, &DpcLockhandle);
		}
	}
    
    /* Use the message id to find the correct entry in the MSI_MESSAGE_TBL */
    pMMT = pRMT->pMsiMsgTbl + MsgID;

    if (pAE->ntldrDump == FALSE && pMMT->Shared == FALSE) {
        /* Determine which CQ to look in based on start state */
        if (pAE->DriverState.NextDriverState == NVMeStartComplete) {

            firstCheckQueue = lastCheckQueue = pMMT->CplQueueNum;
        } else if (pAE->DriverState.NextDriverState == NVMeWaitOnLearnMapping) {

            /* while learning we setup the CT so that this is always true */
            firstCheckQueue = lastCheckQueue = (USHORT)MsgID + 1;
        }
        /* else we're init'ing so admin queue is in use */
    } else {
        /*
         * when Qs share an MSI, the we don't learn anything about core
         * mapping, etc., we just look through all of the queues all the time
         */
        firstCheckQueue = 0;
        lastCheckQueue = (USHORT)pQI->NumCplIoQCreated;
    }

    /* loop through all the queues we've decided we need to look at */
    indexCheckQueue = firstCheckQueue;
    do {
        pCQI = pQI->pCplQueueInfo + indexCheckQueue;
        //pSQI = pQI->pSubQueueInfo + indexCheckQueue;
        indexCheckQueue++;
        /* loop through each queue itself */
        do {
            entryStatus = NVMeGetCplEntry(pAE, pCQI, &pCplEntry);
            if (entryStatus == STOR_STATUS_SUCCESS) {
                /*
                 * Mask the interrupt only when first pending completed entry
                 * found.
                 */
                if ((pRMT->InterruptType == INT_TYPE_INTX) &&
                    (pAE->IntxMasked == FALSE)) {
                    StorPortWriteRegisterUlong(pAE,
                                               &pAE->pCtrlRegister->IVMS,
                                               1);

                    pAE->IntxMasked = TRUE;
                }

                InterruptClaimed = TRUE;

//#pragma prefast(suppress:6011,"This pointer is not NULL")
                NVMeCompleteCmd(pAE,
                                pCplEntry->DW2.SQID,
                                pCplEntry->DW2.SQHD,
                                pCplEntry->DW3.CID,
                                (PVOID)&pSrbExtension);

#ifdef HISTORY
                TracePathComplete(COMPPLETE_CMD, pCplEntry->DW2.SQID,
                    pCplEntry->DW3.CID, pCplEntry->DW2.SQHD,
                    pCplEntry->DW3,
                    (ULONGLONG)pSrbExtension->pNvmeCompletionRoutine,
                    0);
#endif

                if (pSrbExtension != NULL) {
                    BOOLEAN callStorportNotification = FALSE;
#if DBG
                    /* for checked builds, sanity check our learning mode */
                    PROCESSOR_NUMBER procNum;

                    if ((pAE->LearningComplete == TRUE) && (firstCheckQueue > 0)) {
                        StorPortPatchGetCurrentProcessorNumber((PVOID)pAE, &procNum);
                        if ((pSrbExtension->procNum.Group != procNum.Group) ||
                            (pSrbExtension->procNum.Number != procNum.Number))           
                            StorPortDebugPrint(INFO, 
                                "Affinity Check Failed: sub:grp(%d)core(%d), cpl:grp(%d)core(%d)\n",
                                pSrbExtension->procNum.Group, pSrbExtension->procNum.Number,
                                procNum.Group, procNum.Number);
                    }
#endif

                    pSrbExtension->pCplEntry = pCplEntry;

                    /*
                     * If we're learning and this is an IO queue then update
                     * the PCT to note which QP to start using for this core
                     */
                    if (pAE->DriverState.NextDriverState == NVMeWaitOnLearnMapping) {
                        PCORE_TBL pCT = NULL;
                        pQI = &pAE->QueueInfo;
                        pCQI = NULL;
                        PROCESSOR_NUMBER procNum;
                        ULONG coreNum = 0;
                        PPROC_GROUP_TBL pPGT = NULL;

                        StorPortPatchGetCurrentProcessorNumber((PVOID)pAE,
                                 &procNum);
                        /* Figure out the final core number with group number */
                        pPGT = pRMT->pProcGroupTbl + procNum.Group;
                        coreNum = (ULONG)(procNum.Number + pPGT->BaseProcessor);
                        /* reference appropriate tables */
                        pCT = pRMT->pCoreTbl + coreNum;
                        pMMT = pRMT->pMsiMsgTbl + MsgID;
                        pCQI = pQI->pCplQueueInfo + pCT->CplQueue;

                        /* update based on current completion info */
                        pCT->MsiMsgID = (USHORT)MsgID;
                        pCT->Learned = TRUE;
                        pCQI->MsiMsgID = pCT->MsiMsgID;
                        pMMT->CplQueueNum = pCT->CplQueue;

                        /* increment our learning counter */
                        pAE->LearningCores++;
                        StorPortDebugPrint(INFO, 
                            "Mapped#%d: core(%d) to MSI ID(%d)\n",
                                pAE->LearningCores, coreNum, MsgID);
                        /* free the read buffer for learning IO */
                        ASSERT(pSrbExtension->pDataBuffer);
                        if (NULL != pSrbExtension->pDataBuffer) {
                            StorPortFreePool((PVOID)pAE, pSrbExtension->pDataBuffer);
                            pSrbExtension->pDataBuffer = NULL;
                        }
                    }

                    if (pSrbExtension->pNvmeCompletionRoutine == NULL) {
                    /*
                         * if no comp reoutine, call only if we had a valid
                         * status translation, otherwise let it timeout if
                         * if was host based
                     */
                        callStorportNotification =
                            SntiMapCompletionStatus(pSrbExtension);
                    } else {
                        /*
                         * if we have a completion routine, call it and then
                         * complete onlt if this was a host request (srb exsits)
                         * In this case the completion routine is responsible
                         * for mapping Srb status
                         */
                        callStorportNotification =
                            pSrbExtension->pNvmeCompletionRoutine(pAE, (PVOID)pSrbExtension)
                            && (pSrbExtension->pSrb != NULL);
                    }
                    /*
                     *This is to signal to NVMeIsrMsix()and ultimately ProcessIo() in dump mode
                     *  that the Admin request has been completed.
                     */
                    if (pAE->ntldrDump && pSystemArgument1 != NULL) {
                        *(BOOLEAN*)pSystemArgument1 = TRUE;
                    }

                    /* for async calls, call storport if needed */
                    if (callStorportNotification) {
                        IO_StorPortNotification(RequestComplete,
                                                pAE,
                                                pSrbExtension->pSrb);
                    }
                } /* If there was an SRB Extension */
            } /* If a completed command was collected */
        } while (entryStatus == STOR_STATUS_SUCCESS);

        if (InterruptClaimed == TRUE) {
            /* Now update the Completion Head Pointer via Doorbell register */
            StorPortWriteRegisterUlong(pAE,
                                       pCQI->pCplHDBL,
                                       (ULONG)pCQI->CplQHeadPtr);
            InterruptClaimed = FALSE;
        }
        /*
         * If we serviced another queue on MSIX0 then we also have to check
         * the admin queue (admin queue shared with one other QP)
         */
        if ((firstCheckQueue > 0) &&
            (MsgID == 0)) {
            firstCheckQueue = lastCheckQueue = indexCheckQueue = 0;
        }
    } while (indexCheckQueue <= lastCheckQueue); /* end queue checking loop */

    /* Un-mask interrupt if it had been masked */
    if (pAE->IntxMasked == TRUE) {
        StorPortWriteRegisterUlong(pAE, &pAE->pCtrlRegister->INTMC, 1);
        pAE->IntxMasked = FALSE;
    }
    if (pDpc != NULL) {
		if (pAE->MultipleCoresToSingleQueueFlag) {
			StorPortReleaseSpinLock(pAE, &StartLockHandle);
		} else {
			StorPortReleaseSpinLock(pAE, &DpcLockhandle);
		}
	}
} /* IoCompletionDpcRoutine */


/*******************************************************************************
 * NVMeIsrMsix
 *
 * @brief MSI-X interupt routine
 *
 * @param AdapterExtension - Pointer to device extension
 * @param MsgId - MSI-X message Id to be parsed
 *
 * @return BOOLEAN
 *     TRUE - Indiciates successful completion
 *     FALSE - Unsuccessful completion or error
 ******************************************************************************/
static BOOLEAN
NVMeIsrMsix (
    __in PVOID AdapterExtension,
    __in ULONG MsgID )
{
    PNVME_DEVICE_EXTENSION        pAE = (PNVME_DEVICE_EXTENSION)AdapterExtension;
    PRES_MAPPING_TBL              pRMT = &pAE->ResMapTbl;
    PMSI_MESSAGE_TBL              pMMT = pRMT->pMsiMsgTbl;
    ULONG                         qNum = 0;
    BOOLEAN                       status = FALSE;

    if (pAE->hwResetInProg)
        return TRUE;

    if (pAE->ntldrDump == TRUE) {
    	/* status will return TRUE, if the request has been completed. */
        IoCompletionDpcRoutine(NULL, AdapterExtension, &status, 0);
        return status;
    }
    
    /*
     * For shared mode, we'll use the DPC for queue 0,
     * otherwise we'll use the DPC assoiated with the known
     * queue number
     */
    if (pMMT->Shared == FALSE) {
        pMMT = pRMT->pMsiMsgTbl + MsgID;
        qNum = pMMT->CplQueueNum;
    }

    StorPortIssueDpc(pAE,
                (PSTOR_DPC)pAE->pDpcArray + qNum,
                (PVOID)MsgID,
                NULL);

    return TRUE;
}


/*******************************************************************************
 * NVMeMapCore2Queue
 *
 * @brief NVMeMapCore2Queue is called to retrieve the associated queue ID of the
 *        given core number
 *
 * @param pAE - Pointer to hardware device extension
 * @param pPN - Pointer to PROCESSOR_NUMBER structure
 * @param pSubQueue - Pointer to buffer to save retrieved submI[QueueID]ssion queue ID
 * @param pCplQueue - Pointer to buffer to save retrieved Completion queue ID
 *
 * @return ULONG
 *     STOR_STATUS_SUCCESS - If valid number is retrieved
 *     STOR_STATUS_UNSUCCESSFUL - If anything goes wrong
 ******************************************************************************/
static ULONG NVMeMapCore2Queue(
    PNVME_DEVICE_EXTENSION pAE,
    PPROCESSOR_NUMBER pPN,
    USHORT* pSubQueue,
    USHORT* pCplQueue
)
{
    PRES_MAPPING_TBL pRMT = &pAE->ResMapTbl;
    PCORE_TBL pCT = NULL;
    PPROC_GROUP_TBL pPGT = pRMT->pProcGroupTbl + pPN->Group;
    ULONG coreNum = (ULONG)(pPN->Number + pPGT->BaseProcessor);

    if (pAE->ntldrDump == TRUE) {
        *pSubQueue = *pCplQueue = 1;
        return (STOR_STATUS_SUCCESS);
    }
    
    /* Ensure the core number is valid first */
    if (coreNum >= pRMT->NumActiveCores) {
        StorPortDebugPrint(ERROR,
            "NVMeMapCore2Queue: <Error> Invalid core number = %d.\n", coreNum);
        return (STOR_STATUS_UNSUCCESSFUL);
    }

    /* Locate the target CORE_TBL entry for the specific core number
     * indexed depending on whether we're still learning the table or not
     */
    if (pAE->LearningCores == pRMT->NumActiveCores) {
        pCT = pRMT->pCoreTbl + coreNum;
        /* Return the queue IDs */
        *pSubQueue = pCT->SubQueue;
        *pCplQueue = pCT->CplQueue;
    } else {
        *pSubQueue = (USHORT)pAE->LearningCores + 1;
        *pCplQueue = (USHORT)pAE->LearningCores + 1;
    }

    return (STOR_STATUS_SUCCESS);
} /* NVMeMapCore2Queue */


/*******************************************************************************
 * ProcessIo
 *
 * @brief Routine for processing an I/O request (both internal and externa)
 *        and setting up all the necessary info. Then, calls NVMeIssueCmd to
 *        issue the command to the controller.
 *
 * @param AdapterExtension - pointer to device extension
 * @param SrbExtension - SRB extension for this command
 * @param QueueType - type of queue (admin or I/O)
 * @param AcquireLock - if the caller needs the StartIO lock acquired or not
 *
 * @return BOOLEAN
 *     TRUE - command was processed successfully
 *     FALSE - If anything goes wrong
 ******************************************************************************/
static BOOLEAN
ProcessIo(
    __in PNVME_DEVICE_EXTENSION pAdapterExtension,
    __in PNVME_SRB_EXTENSION pSrbExtension,
    __in NVME_QUEUE_TYPE QueueType,
    __in BOOLEAN AcquireLock
)
{
    PNVMe_COMMAND pNvmeCmd;
    ULONG StorStatus;
    IO_SUBMIT_STATUS IoStatus = SUBMITTED;
    PCMD_INFO pCmdInfo = NULL;
    PROCESSOR_NUMBER ProcNumber;
    USHORT SubQueue = 0;
    USHORT CplQueue = 0;
    //PQUEUE_INFO pQI = &pAdapterExtension->QueueInfo;
    STOR_LOCK_HANDLE hStartIoLock = {0};
#ifdef PRP_DBG
    PVOID pVa = NULL;
#endif

     __try {

        if (AcquireLock == TRUE) {
            StorPortAcquireSpinLock(pAdapterExtension,
                                    StartIoLock,
                                    NULL,
                                    &hStartIoLock);
        }

    if (pAdapterExtension->ntldrDump == FALSE) {
        StorStatus = StorPortPatchGetCurrentProcessorNumber((PVOID)pAdapterExtension,
                                                       &ProcNumber);
        if (StorStatus != STOR_STATUS_SUCCESS) {
            IoStatus = NOT_SUBMITTED;
            __leave;
        }
    } else {
        memset(&ProcNumber, 0, sizeof(PROCESSOR_NUMBER));
    }

#if DBG
        /* save off the submitting core info for debug CT learning purposes */
    pSrbExtension->procNum = ProcNumber;
#endif

    /* 1 - Select Queue based on CPU */
    if (QueueType == NVME_QUEUE_TYPE_IO) {

            StorStatus =  NVMeMapCore2Queue(pAdapterExtension,
                                         &ProcNumber,
                                         &SubQueue,
                                         &CplQueue);

            if (StorStatus != STOR_STATUS_SUCCESS) {
                IoStatus = NOT_SUBMITTED;
                __leave;
            }
    } else {
        /* It's an admin queue */
            SubQueue = CplQueue = 0;
    }

    /* 2 - Choose CID for the CMD_ENTRY */
    StorStatus = NVMeGetCmdEntry(pAdapterExtension,
                                 SubQueue,
                                 (PVOID)pSrbExtension,
                                 &pCmdInfo);

    if (StorStatus != STOR_STATUS_SUCCESS) {
            IoStatus = BUSY;
            __leave;
    }

    pNvmeCmd = &pSrbExtension->nvmeSqeUnit;
//#pragma prefast(suppress:6011,"This pointer is not NULL")
    pNvmeCmd->CDW0.CID = (USHORT)pCmdInfo->CmdID;

#ifdef DUMB_DRIVER
    /*
     * For reads/writes, create PRP list in pre-allocated
     * space describing the dbl buff location... make sure that
     * we do not double buffer NVMe Flush commands.
     */
    if ((QueueType == NVME_QUEUE_TYPE_IO) &&
        (pNvmeCmd->CDW0.OPC != NVM_FLUSH)) {
        ULONG len = GET_DATA_LENGTH(pSrbExtension->pSrb);
        ULONG i = 1;
        PUINT64 pPrpList = (PUINT64)pCmdInfo->pDblPrpListVir;

        ASSERT(len <= DUMB_DRIVER_SZ);

        if (len <= (PAGE_SIZE * 2)) {
            pNvmeCmd->PRP1 = pCmdInfo->dblPhy.QuadPart;
            if (len > PAGE_SIZE) {
                pNvmeCmd->PRP2 = pCmdInfo->dblPhy.QuadPart + PAGE_SIZE;
            } else {
                pNvmeCmd->PRP2 = 0;
            }
        } else {
            pNvmeCmd->PRP1 = pCmdInfo->dblPhy.QuadPart;
            len -= PAGE_SIZE;
            pNvmeCmd->PRP2 = pCmdInfo->dblPrpListPhy.QuadPart;

            while (len > 0) {
                *pPrpList = pCmdInfo->dblPhy.QuadPart + (PAGE_SIZE * i);
                len -= PAGE_SIZE;
                pPrpList++;
                i++;
            }
        }
        /* Pre-allacted so this had better be true! */
        ASSERT(IS_SYS_PAGE_ALIGNED(pNvmeCmd->PRP1));
        ASSERT(IS_SYS_PAGE_ALIGNED(pNvmeCmd->PRP2));

        // Get Virtual address, only for read or write
        if (IS_CMD_DATA_IN(pNvmeCmd->CDW0.OPC) ||
            IS_CMD_DATA_OUT(pNvmeCmd->CDW0.OPC)) {
            /*
             * Save the dblBuff location, Srb databuff location and len
             * all in one handy location in the srb ext
             */
            StorStatus = StorPortGetSystemAddress(pAdapterExtension,
                                                  pSrbExtension->pSrb,
                                                  &pSrbExtension->pSrbDataVir);

            ASSERT(StorStatus == STOR_STATUS_SUCCESS);

            pSrbExtension->dataLen = GET_DATA_LENGTH(pSrbExtension->pSrb);
            pSrbExtension->pDblVir = pCmdInfo->pDblVir;

            /*
             * For a write, copy data to the dbl buff, read data will
             * be copied out in the ISR
             */
            if (IS_CMD_DATA_OUT(pNvmeCmd->CDW0.OPC)) {
                    StorPortCopyMemory(pSrbExtension->pDblVir,
                                   pSrbExtension->pSrbDataVir,
                                   pSrbExtension->dataLen);
            }
        }
    }
#else /* DUMB_DRIVER */
    /*
     * 3 - If a PRP list is used, copy the buildIO prepared list to the
     * preallocated memory location and update the entry not the pCmdInfo is a
     * stack var but contains a to the pre allocated mem which is what we're
     * updating.
     */
#ifndef PRP_DBG
    if (pSrbExtension->numberOfPrpEntries > 2) {
        pNvmeCmd->PRP2 = pCmdInfo->prpListPhyAddr.QuadPart;

        /*
         * Copy the PRP list pointed to by PRP2. Size of the copy is total num
         * of PRPs -1 because PRP1 is not in the PRP list pointed to by PRP2.
         */
            StorPortCopyMemory(
            (PVOID)pCmdInfo->pPRPList,
            (PVOID)&pSrbExtension->prpList[0],
            ((pSrbExtension->numberOfPrpEntries - 1) * sizeof(UINT64)));
    }
#else
        if (pSrbExtension->pSrb) {
            StorPortGetSystemAddress(pSrbExtension->pNvmeDevExt,
                              pSrbExtension->pSrb,
                              &pVa);
            StorPortDebugPrint(INFO,
                               "NVME: Process Cmd 0x%x VA 0x%x 0x%x SLBA 0x%x 0x%x for LEN 0x%x\n",
                               pNvmeCmd->CDW0.OPC,
                               (ULONGLONG)pVa >> 32, (ULONG)pVa,
                               pNvmeCmd->CDW11,
                               pNvmeCmd->CDW10,
                               GET_DATA_LENGTH(pSrbExtension->pSrb));
        }

        if (pSrbExtension->numberOfPrpEntries > 2) {
            ULONG i;
            pNvmeCmd->PRP2 = pCmdInfo->prpListPhyAddr.QuadPart;

            StorPortCopyMemory(
                (PVOID)pCmdInfo->pPRPList,
                               (PVOID)&pSrbExtension->prpList[0],
                ((pSrbExtension->numberOfPrpEntries - 1) * sizeof(UINT64)));

            StorPortDebugPrint(INFO,
                   "NVME: Process prp1 0x%x 0x%x prp2 0x%x 0x%x (list for 0x%x entries)\n",
                   pNvmeCmd->PRP1 >> 32, pNvmeCmd->PRP1,
                   pNvmeCmd->PRP2 >> 32, pNvmeCmd->PRP2,
                   (pSrbExtension->numberOfPrpEntries - 1));

            for (i=0;i<(pSrbExtension->numberOfPrpEntries - 1);i++) {
                StorPortDebugPrint(INFO,
                   "NVME: Process entry # 0x%x prp 0x%x 0x%x\n",
                   i,
                   pSrbExtension->prpList[i] >> 32, pSrbExtension->prpList[i]
                   );
            }
        } else if (pNvmeCmd->PRP1 != 0) {
                 StorPortDebugPrint(INFO,
                       "NVME: Process prp1 0x%x 0x%x prp2 0x%x 0x%x (no list)\n",
                   pNvmeCmd->PRP1 >> 32, pNvmeCmd->PRP1,
                   pNvmeCmd->PRP2 >> 32, pNvmeCmd->PRP2
                   );
        }
#endif /* PRP_DBG */
#endif /* DBL_BUFF */
#ifdef HISTORY
            TracePathSubmit(PRE_ISSUE, SubQueue, pNvmeCmd->NSID, pNvmeCmd->CDW0,
                pNvmeCmd->PRP1, pNvmeCmd->PRP2, pSrbExtension->numberOfPrpEntries);
#endif

    /* 4 - Issue the Command */
    StorStatus = NVMeIssueCmd(pAdapterExtension, SubQueue, pNvmeCmd);

    if (StorStatus != STOR_STATUS_SUCCESS) {
        NVMeCompleteCmd(pAdapterExtension,
                        SubQueue,
                        NO_SQ_HEAD_CHANGE,
                        pNvmeCmd->CDW0.CID,
                        (PVOID)pSrbExtension);
            IoStatus = BUSY;
            __leave;
    }

    /*
     * In crashdump we poll on admin command completions
     * in order to allow our init state machine to function.
     * We don't poll on IO commands as storport will poll
     * for us and call our ISR.
     */
    if ((pAdapterExtension->ntldrDump == TRUE) &&
        (QueueType == NVME_QUEUE_TYPE_ADMIN)   &&
        (pAdapterExtension->DriverState.NextDriverState != NVMeStateFailed) && 
        (pAdapterExtension->DriverState.NextDriverState != NVMeStartComplete) &&
        (StorStatus == STOR_STATUS_SUCCESS)) {
        ULONG pollCount = 0;

        while (FALSE == NVMeIsrMsix(pAdapterExtension, NVME_ADMIN_MSG_ID)) {
            NVMeCrashDelay(pAdapterExtension->DriverState.CheckbackInterval, TRUE);
            if (++pollCount > DUMP_POLL_CALLS) {
                /* a polled admin command timeout is considered fatal */
                pAdapterExtension->DriverState.DriverErrorStatus |=
                    (1 << FATAL_POLLED_ADMIN_CMD_FAILURE);
                pAdapterExtension->DriverState.NextDriverState = NVMeStateFailed;
                /*
                 * depending on whether the timer driven thread is dead or not
                 * this error may get loggged twice
                 */
                NVMeLogError(pAdapterExtension,
                    (ULONG)pAdapterExtension->DriverState.DriverErrorStatus);
                    IoStatus = NOT_SUBMITTED;
                    __leave;
                }
            }
        }

    } __finally {

        if (AcquireLock == TRUE) {
            StorPortReleaseSpinLock(pAdapterExtension, &hStartIoLock);
        }

        if (IoStatus == BUSY) {
#ifdef HISTORY
            TracePathSubmit(GETCMD_RETURN_BUSY, SubQueue,
                ((PNVMe_COMMAND)(&pSrbExtension->nvmeSqeUnit))->NSID,
                ((PNVMe_COMMAND)(&pSrbExtension->nvmeSqeUnit))->CDW0,
                0, 0, 0);
#endif
             if (pSrbExtension->pSrb != NULL) {
                pSrbExtension->pSrb->SrbStatus = SRB_STATUS_BUSY;
                IO_StorPortNotification(RequestComplete,
                                        pAdapterExtension,
                                        pSrbExtension->pSrb);
            }
        }
    }

    return (IoStatus == SUBMITTED) ? TRUE : FALSE;

} /* ProcessIo */


/*******************************************************************************
 * NVMeGetIdentifyStructures
 *
 * @brief NVMeGetIdentifyStructures gets called to retrieve Identify structures,
 *        including the Controller and Namespace information depending on
 *        NamespaceID.
 *
 * @param pAE - Pointer to hardware device extension.
 * @param NamespaceID - Specify either Controller or Namespace structure to
 *                      retrieve
 *
 * @return BOOLEAN
 *     TRUE - If the issued command completed without any errors
 *     FALSE - If anything goes wrong
 ******************************************************************************/
static BOOLEAN NVMeGetIdentifyStructures(
    PNVME_DEVICE_EXTENSION pAE,
    ULONG NamespaceID
)
{
    PNVME_SRB_EXTENSION pNVMeSrbExt =
        (PNVME_SRB_EXTENSION)pAE->DriverState.pSrbExt;
    PNVMe_COMMAND pIdentify = NULL;
    PADMIN_IDENTIFY_CONTROLLER pIdenCtrl = &pAE->controllerIdentifyData;
    PADMIN_IDENTIFY_COMMAND_DW10 pIdentifyCDW10 = NULL;

    /* Zero-out the entire SRB_EXTENSION */
    memset((PVOID)pNVMeSrbExt, 0, sizeof(NVME_SRB_EXTENSION));

    /* Populate SRB_EXTENSION fields */
    pNVMeSrbExt->pNvmeDevExt = pAE;
    pNVMeSrbExt->pNvmeCompletionRoutine = NVMeInitCallback;

    /* Populate submission entry fields */
    pIdentify = &pNVMeSrbExt->nvmeSqeUnit;
    pIdentify->CDW0.OPC = ADMIN_IDENTIFY;

    if (NamespaceID == IDEN_CONTROLLER) {
        StorPortDebugPrint(INFO,"NVMeGetIdentifyStructures: IDEN_CONTROLLER\n");
        /* Indicate it's for Controller structure */
        pIdentifyCDW10 = (PADMIN_IDENTIFY_COMMAND_DW10) &pIdentify->CDW10;
        pIdentifyCDW10->CNS = 1;

        /* Prepare PRP entries, need at least one PRP entry */
        if (NVMePreparePRPs(pAE,
                            pNVMeSrbExt,
                            (PVOID)pAE->DriverState.pDataBuffer,
                            sizeof(ADMIN_IDENTIFY_CONTROLLER)) == FALSE) {
            return (FALSE);
        }
    } else {
        //ULONG lunId;

        if ( pIdenCtrl == NULL )
            return (FALSE);

        StorPortDebugPrint(INFO,"NVMeGetIdentifyStructures: IDEN_NAMESPACE\n");
        /* Indicate it's for Namespace structure */
        pIdentifyCDW10 = (PADMIN_IDENTIFY_COMMAND_DW10) &pIdentify->CDW10;
        pIdentifyCDW10->CNS = 0;

        /* NN of Controller structure is 1-based */
        if (NamespaceID <= pIdenCtrl->NN) {
            //lunId = pAE->DriverState.VisibleNamespacesExamined;

            /* Namespace ID is 1-based. */
            pIdentify->NSID = NamespaceID;

            //StorPortDebugPrint(INFO,
            //    "NVMeGetIdentifyStructures: Get NS INFO for NSID 0x%x tgt lun 0x%x\n",
            //        NamespaceID, lunId);

            /* Prepare PRP entries, need at least one PRP entry */
            if (NVMePreparePRPs(pAE,
                                pNVMeSrbExt,
                                (PVOID)pAE->DriverState.pDataBuffer,
                                sizeof(ADMIN_IDENTIFY_NAMESPACE)) == FALSE) {
                return (FALSE);
            }
        } else {
            /* no initial namespaces defined */
            StorPortDebugPrint(INFO,
                "NVMeGetIdentifyStructures: NamespaceID <= pIdenCtrl->NN\n");
            pAE->DriverState.StateChkCount = 0;
            pAE->visibleLuns = 0;
            pAE->DriverState.NextDriverState = NVMeWaitOnSetupQueues;
            NVMeCallArbiter(pAE);
            return (TRUE);
        }
    }

    /* Now issue the command via Admin Doorbell register */
    return ProcessIo(pAE, pNVMeSrbExt, NVME_QUEUE_TYPE_ADMIN, FALSE);
} /* NVMeGetIdentifyStructures */


/*******************************************************************************
 * NVMeRunningWaitOnIdentifyCtrl
 *
 * @brief NVMeRunningWaitOnIdentifyCtrl is called to issue Identify command to
 *        retrieve Controller structures.
 *
 * @param pAE - Pointer to adapter device extension.
 *
 * @return VOID
 ******************************************************************************/
static VOID NVMeRunningWaitOnIdentifyCtrl(
    PNVME_DEVICE_EXTENSION pAE
)
{
    /*
     * Issue Identify command for the very first time If failed, fail the state
     * machine
     */
    if (NVMeGetIdentifyStructures(pAE, IDEN_CONTROLLER) == FALSE) {
        NVMeDriverFatalError(pAE,
                            (1 << START_STATE_IDENTIFY_CTRL_FAILURE));
        NVMeCallArbiter(pAE);
        return;
    }
} /* NVMeRunningWaitOnIdentifyCtrl */


/*******************************************************************************
 * NVMeRunningWaitOnIdentifyNS
 *
 * @brief NVMeRunningWaitOnIdentifyNS is called to issue Identify command to
 *        retrieve Namespace structures.
 *
 * @param pAE - Pointer to adapter device extension.
 *
 * @return VOID
 ******************************************************************************/
static VOID NVMeRunningWaitOnIdentifyNS(
    PNVME_DEVICE_EXTENSION pAE
)
{
    ULONG nsid = pAE->DriverState.CurrentNsid + 1;

    /*
     * Issue an identify command.  The completion handler will keep us at this
     * state if there are more identifies needed based on what the ctlr told us
     * If failed, fail the state machine
     *
     * Please note that NN of Controller structure is 1-based.
     */

    if (NVMeGetIdentifyStructures(pAE, nsid) == FALSE) {
        NVMeDriverFatalError(pAE,
                            (1 << START_STATE_IDENTIFY_NS_FAILURE));
        NVMeCallArbiter(pAE);
    }
} /* NVMeRunningWaitOnIdentifyNS */


/*******************************************************************************
 * NVMeSetIntCoalescing
 *
 * @brief NVMeSetIntCoalescing gets called to configure interrupt coalescing
 *        with the values fetched from Registry via Set Features command with
 *        Feature ID#8.
 *
 * @param pAE - Pointer to hardware device extension.
 *
 * @return BOOLEAN
 *     TRUE - If the issued command completed without any errors
 *     FALSE - If anything goes wrong
 ******************************************************************************/
static BOOLEAN NVMeSetIntCoalescing(
    PNVME_DEVICE_EXTENSION pAE
)
{
    PNVME_SRB_EXTENSION pNVMeSrbExt =
        (PNVME_SRB_EXTENSION)pAE->DriverState.pSrbExt;
    PNVMe_COMMAND pSetFeatures = (PNVMe_COMMAND)(&pNVMeSrbExt->nvmeSqeUnit);

    PADMIN_SET_FEATURES_COMMAND_DW10 pSetFeaturesCDW10 = NULL;
    PADMIN_SET_FEATURES_COMMAND_INTERRUPT_COALESCING_DW11
        pSetFeaturesCDW11 = NULL;

    /* Zero out the extension first */
    memset((PVOID)pNVMeSrbExt, 0, sizeof(NVME_SRB_EXTENSION));

    /* Populate SRB_EXTENSION fields */
    pNVMeSrbExt->pNvmeDevExt = pAE;
    pNVMeSrbExt->pNvmeCompletionRoutine = NVMeInitCallback;

    /* Populate submission entry fields */
    pSetFeatures->CDW0.OPC = ADMIN_SET_FEATURES;
    pSetFeaturesCDW10 = (PADMIN_SET_FEATURES_COMMAND_DW10) &pSetFeatures->CDW10;
    pSetFeaturesCDW11 = (PADMIN_SET_FEATURES_COMMAND_INTERRUPT_COALESCING_DW11)
        &pSetFeatures->CDW11;

    pSetFeaturesCDW10->FID = INTERRUPT_COALESCING;

    /* Set up the Aggregation Time and Threshold */
    pSetFeaturesCDW11->TIME = pAE->InitInfo.IntCoalescingTime;
    pSetFeaturesCDW11->THR = pAE->InitInfo.IntCoalescingEntry;

    /* Now issue the command via Admin Doorbell register */
    return ProcessIo(pAE, pNVMeSrbExt, NVME_QUEUE_TYPE_ADMIN, FALSE);
} /* NVMeSetIntCoalescing */


/*******************************************************************************
 * NVMeAllocQueueFromAdapter
 *
 * @brief NVMeAllocQueueFromAdapter gets called to allocate
 *        submission/completion queues from the adapter via Set Feature command
 *        with Feature ID#7. This routine requests the same number of
 *        submission/completion queues as the number of current active cores in
 *        the system.
 *
 * @param pAE - Pointer to hardware device extension.
 *
 * @return BOOLEAN
 *     TRUE - If the issued command completed without any errors
 *     FALSE - If anything goes wrong
 ******************************************************************************/
static BOOLEAN NVMeAllocQueueFromAdapter(
    PNVME_DEVICE_EXTENSION pAE
)
{
    PNVME_SRB_EXTENSION pNVMeSrbExt =
        (PNVME_SRB_EXTENSION)pAE->DriverState.pSrbExt;
    PNVMe_COMMAND pSetFeatures = (PNVMe_COMMAND)(&pNVMeSrbExt->nvmeSqeUnit);
    PADMIN_SET_FEATURES_COMMAND_DW10 pSetFeaturesCDW10 = NULL;
    PADMIN_SET_FEATURES_COMMAND_NUMBER_OF_QUEUES_DW11 pSetFeaturesCDW11 = NULL;

    /* Zero out the extension first */
    memset((PVOID)pNVMeSrbExt, 0, sizeof(NVME_SRB_EXTENSION));

    /* Populate SRB_EXTENSION fields */
    pNVMeSrbExt->pNvmeDevExt = pAE;
    pNVMeSrbExt->pNvmeCompletionRoutine = NVMeInitCallback;

    /* Populate submission entry fields */
    pSetFeatures->CDW0.OPC = ADMIN_SET_FEATURES;
    pSetFeaturesCDW10 = (PADMIN_SET_FEATURES_COMMAND_DW10) &pSetFeatures->CDW10;
    pSetFeaturesCDW11 = (PADMIN_SET_FEATURES_COMMAND_NUMBER_OF_QUEUES_DW11)
        &pSetFeatures->CDW11;
    pSetFeaturesCDW10->FID = NUMBER_OF_QUEUES;

    /* Set up NCSQR and NSQR, which are 0-based. */
    if (pAE->ntldrDump == TRUE) {
        /* In crashdump/hibernation case, only 1 pair of IO queue needed */
        pSetFeaturesCDW11->NCQR = 0;
        pSetFeaturesCDW11->NSQR = 0;
    } else {
        /*
         * Number of cores, number of MSI granted, and number of queues
         * supported by the controller are the variables to determine how
         * many queues to create.
         */
        // In Windows 2003 Server pAE->ResMapTbl.NumMsiMsgGranted is zero, because
        // the call to the extended storport function StorPortGetMSIInfo returns
        // STOR_STATUS_NOT_IMPLEMENTED (0xC1000002L). This leads to a wrong value
        // of 0xFFFF for NCQR and NSQR in the original code.
        if(pAE->ResMapTbl.NumMsiMsgGranted == 0)
        {
            pSetFeaturesCDW11->NCQR = 0;
            pSetFeaturesCDW11->NSQR = 0;
        }
        else
        {
            pSetFeaturesCDW11->NCQR = min(pAE->ResMapTbl.NumActiveCores,
                                          pAE->ResMapTbl.NumMsiMsgGranted) - 1;
            pSetFeaturesCDW11->NSQR = min(pAE->ResMapTbl.NumActiveCores,
                                          pAE->ResMapTbl.NumMsiMsgGranted) - 1;
        }
    }

    /* Now issue the command via Admin Doorbell register */
    return ProcessIo(pAE, pNVMeSrbExt, NVME_QUEUE_TYPE_ADMIN, FALSE);
} /* NVMeAllocQueueFromAdapter */


/*******************************************************************************
 * NVMeAccessLbaRangeEntry
 *
 * @brief NVMeAccessLbaRangeEntry gets called to query/update the first LBA
 *        Range entry of a given namespace. The procedure is described below:
 *
 *        Depending on the value of ConfigLbaRangeNeeded, if TRUE, it issues
 *        Set Features command. Otherwise, it issues Get Features commands.
 *
 *        When issuing Set Features, the namespace has:
 *          Type = Filesystem,
 *          ReadOnly = FALSE,
 *          Visible = TRUE.
 *
 *        When command completes, NVMeSetFeaturesCompletion is called to exam
 *        the LBA Range Type entries and wrap up the access.
 *
 * @param pAE - Pointer to hardware device extension.
 *
 * @return BOOLEAN
 *     TRUE - If the issued command completed without any errors
 *     FALSE - If anything goes wrong
 ******************************************************************************/
static BOOLEAN NVMeAccessLbaRangeEntry(
    PNVME_DEVICE_EXTENSION pAE
)
{
    PNVME_SRB_EXTENSION pNVMeSrbExt =
        (PNVME_SRB_EXTENSION)pAE->DriverState.pSrbExt;
    PNVMe_COMMAND pSetFeatures = (PNVMe_COMMAND)(&pNVMeSrbExt->nvmeSqeUnit);
    PADMIN_SET_FEATURES_COMMAND_DW10 pSetFeaturesCDW10 = NULL;
    PADMIN_SET_FEATURES_COMMAND_LBA_RANGE_TYPE_ENTRY pLbaRangeEntry = NULL;
    ULONG NSID = pAE->DriverState.CurrentNsid;
    //BOOLEAN Query = pAE->DriverState.ConfigLbaRangeNeeded;

    /* Fail here if Namespace ID is 0 or out of range */
    if (NSID == 0 || NSID > pAE->controllerIdentifyData.NN)
        return FALSE;

    /* Zero out the extension first */
    memset((PVOID)pNVMeSrbExt, 0, sizeof(NVME_SRB_EXTENSION));

    /* Populate SRB_EXTENSION fields */
    pNVMeSrbExt->pNvmeDevExt = pAE;
    pNVMeSrbExt->pNvmeCompletionRoutine = NVMeInitCallback;

    /* Populate submission entry fields */
    pSetFeatures->NSID = NSID;

    /*
     * Prepare the buffer for transferring LBA Range entries
     * Need to zero the buffer out first when retrieving the entries
     */
    if (pAE->DriverState.ConfigLbaRangeNeeded == TRUE) {
        pSetFeatures->CDW0.OPC = ADMIN_SET_FEATURES;

        /* Prepare new first LBA Range entry */
        pLbaRangeEntry = (PADMIN_SET_FEATURES_COMMAND_LBA_RANGE_TYPE_ENTRY)
            pAE->DriverState.pDataBuffer;
        pLbaRangeEntry->Type = LBA_TYPE_FILESYSTEM;
        pLbaRangeEntry->Attributes.Overwriteable = 1;
        pLbaRangeEntry->Attributes.Hidden = 0;
        pLbaRangeEntry->NLB = pAE->pLunExtensionTable[NSID-1]->identifyData.NSZE;
    } else {
        pSetFeatures->CDW0.OPC = ADMIN_GET_FEATURES;
        memset(pAE->DriverState.pDataBuffer, 0, PAGE_SIZE);
    }

    /* Prepare PRP entries, need at least one PRP entry */
    if (NVMePreparePRPs(pAE,
                        pNVMeSrbExt,
                        pAE->DriverState.pDataBuffer,
                        sizeof(ADMIN_SET_FEATURES_LBA_COMMAND_RANGE_TYPE_ENTRY))
                        == FALSE) {
        return (FALSE);
    }

    pSetFeaturesCDW10 = (PADMIN_SET_FEATURES_COMMAND_DW10) &pSetFeatures->CDW10;
    pSetFeaturesCDW10->FID = LBA_RANGE_TYPE;

    /* Now issue the command via Admin Doorbell register */
    return ProcessIo(pAE, pNVMeSrbExt, NVME_QUEUE_TYPE_ADMIN, FALSE);
} /* NVMeAccessLbaRangeEntry */


/*******************************************************************************
 * NVMeRunningWaitOnSetFeatures
 *
 * @brief NVMeRunningWaitOnSetFeatures is called to issue the following
 *        commands:
 *
 *        1. Set Features command (Interrupt Coalescing, Feature ID#8)
 *        2. Set Features command (Number of Queues, Feature ID#7)
 *        3. For each existing Namespace, Get Features (LBA Range Type) first.
 *           When its Type is 00b and NLB matches the size of the Namespace,
 *           isssue Set Features (LBA Range Type) to configure:
 *             a. its Type as Filesystem,
 *             b. can be overwritten, and
 *             c. to be visible
 *
 * @param pAE - Pointer to adapter device extension.
 *
 * @return VOID
 ******************************************************************************/
static VOID NVMeRunningWaitOnSetFeatures(
    PNVME_DEVICE_EXTENSION pAE
)
{
    PQUEUE_INFO pQI = &pAE->QueueInfo;

    /*
     * There are multiple steps hanlded in this state as they're all
     * grouped into 'set feature' type things.  This simplifies adding more
     * set features in the future as jst this sub-state machine needs updating
     */
    if (pAE->DriverState.InterruptCoalescingSet == FALSE) {
        if (NVMeSetIntCoalescing(pAE) == FALSE) {
            NVMeDriverFatalError(pAE,
                                (1 << START_STATE_SET_FEATURE_FAILURE));
            NVMeCallArbiter(pAE);
            return;
        }
    } else if (pQI->NumSubIoQAllocFromAdapter == 0) {
        if (NVMeAllocQueueFromAdapter(pAE) == FALSE) {
            NVMeDriverFatalError(pAE,
                                (1 << START_STATE_SET_FEATURE_FAILURE));
            NVMeCallArbiter(pAE);
            return;
        }
    } else {
        if(NVMeAccessLbaRangeEntry(pAE) == FALSE) {
            NVMeDriverFatalError(pAE,
                                (1 << START_STATE_SET_FEATURE_FAILURE));
            NVMeCallArbiter(pAE);
            return;
        }
    }
} /* NVMeRunningWaitOnSetFeatures */


/*******************************************************************************
 * NVMeAllocIoQueues
 *
 * @brief NVMeAllocIoQueues gets called to allocate IO queue(s) from system
 *        memory after determining the number of queues to allocate. In the case
 *        of failing to allocate memory, it needs to fall back to use one queue
 *        per adapter and free up the allocated buffers that is not used. The
 *        scenarios can be described in the follow pseudo codes:
 *
 *        if (Queue Number granted from adapter >= core number) {
 *            for (NUMA = 0; NUMA < NUMA node number; NUMA++) {
 *                for (Core = FirstCore; Core <= LastCore; Core++) {
 *                    Allocate queue pair for each core;
 *                    if (Succeeded) {
 *                        Note down which queue to use for each core;
 *                    } else {
 *                        if (failed on first queue allocation and NUMA == 0) {
 *                            return FALSE;
 *                        } else { //at least one queue allocated
 *                            // Fall back to one queue per adapter
 *                            Free up the allocated, not used queues;
 *                            Mark down number of queues allocated;
 *                            Note down which queue to use for each core;
 *                            return TRUE;
 *                        }
 *                    }
 *                }
 *            }
 *            Mark down number of queue pairs allocated;
 *            return TRUE;
 *        } else {
 *            // Allocate one queue pair only
 *            Allocate one queue pair;
 *            if (Succeeded) {
 *                Mark down one pair of queues allocated;
 *                Note down which queue to use for each core;
 *                return TRUE;
 *            } else {
 *                return FALSE;
 *            }
 *        }
 *
 * @param pAE - Pointer to hardware device extension.
 *
 * @return BOOLEAN
 *     TRUE - If all resources are allocated and initialized properly
 *     FALSE - If anything goes wrong
 ******************************************************************************/
static BOOLEAN NVMeAllocIoQueues(
    PNVME_DEVICE_EXTENSION pAE
)
{
    ULONG Status = STOR_STATUS_SUCCESS;
    PQUEUE_INFO pQI = &pAE->QueueInfo;
    PRES_MAPPING_TBL pRMT = &pAE->ResMapTbl;
    PNUMA_NODE_TBL pNNT = NULL;
    PSUB_QUEUE_INFO pSQI = NULL;
    //PSUB_QUEUE_INFO pSQIDest = NULL, pSQISrc = NULL;
    PCORE_TBL pCT = NULL;
    ULONG Core, Node, QEntries;
    ULONG CoreTableIndex = 0;
    USHORT QueueID = 0;
    ULONG Queue = 0;

    pQI->NumSubIoQAllocated = pQI->NumCplIoQAllocated = 0;

    if (pAE->ntldrDump == TRUE) {
        QEntries = MIN_IO_QUEUE_ENTRIES; 
        Status = NVMeAllocQueues(pAE,
                                 QueueID + 1,
                                 QEntries,
                                 0);
    
        if (Status == STOR_STATUS_SUCCESS) {
            pQI->NumSubIoQAllocated = ++pQI->NumCplIoQAllocated;
            return (TRUE);
        } else {
            return (FALSE);
        }
    } else {
        for (Node = 0; Node < pRMT->NumNumaNodes; Node++) {
            pNNT = pRMT->pNumaNodeTbl + Node;
            /* When no logical processors assigned to the node, just move on */
            if (pNNT->NumCores == 0)
                continue;

            for (Core = pNNT->FirstCoreNum; Core <= pNNT->LastCoreNum; Core++) {


                pCT = pRMT->pCoreTbl + CoreTableIndex;

                /*
                 * If there are more cores than Qs alloc'd from the adapter, just
                 * cycle through the available Qs in the core table.  Ex, if there
                 * are 2 Q's and 4 cores, the table will look like:
                 * Core  QID
                 *  0     1
                 *  1     2
                 *  3     1
                 *  4     2
                 */
                 QueueID = (QueueID + 1 > (USHORT)pQI->NumSubIoQAllocFromAdapter) ?
                            1 : QueueID + 1;

                if (pQI->NumSubIoQAllocated < QueueID)  {
 
                    QEntries = pAE->InitInfo.IoQEntries;
                    Status = NVMeAllocQueues(pAE,
                                             QueueID,
                                             QEntries,
                                             (USHORT)Node);

                    if (Status == STOR_STATUS_SUCCESS) {
   
                        pQI->NumSubIoQAllocated = ++pQI->NumCplIoQAllocated;
                    } else {
                        /*
                         * If faling on the very first queue allocation, failure
                         * case.
                         */
                        if (CoreTableIndex == pNNT->FirstCoreNum && Node == 0) {
                            return (FALSE);
                        } else {
                            /*
                             * Fall back to share the very first queue allocated.
                             * Free the other allocated queues before returning
                             * and return TRUE.
                             */
                            Queue = 0;
                            for (Core = 1; Core < pRMT->NumActiveCores; Core++) {
                                /* Need to keep first allocated IO queue for sharing */
                                Queue = Core + 1; 
                                pSQI = pQI->pSubQueueInfo + Queue;

                                if (pSQI->pQueueAlloc != NULL)
                                    StorPortPatchFreeContiguousMemorySpecifyCache(
                                        (PVOID)pAE,
                                        pSQI->pQueueAlloc,
                                        pSQI->QueueAllocSize,
                                        MmCached);
                                pSQI->pQueueAlloc = NULL;

                                if (pSQI->pPRPListAlloc != NULL)
                                    StorPortPatchFreeContiguousMemorySpecifyCache(
                                        (PVOID)pAE,
                                        pSQI->pPRPListAlloc,
                                        pSQI->PRPListAllocSize,
                                        MmCached);
                                pSQI->pPRPListAlloc = NULL;
#ifdef DUMB_DRIVER
                                if (pSQI->pDblBuffAlloc != NULL)
                                    StorPortPatchFreeContiguousMemorySpecifyCache(
                                                            (PVOID)pAE,
                                                            pSQI->pDblBuffAlloc,
                                                            pSQI->dblBuffSz,
                                                            MmCached);

                                if (pSQI->pDblBuffListAlloc != NULL)
                                    StorPortPatchFreeContiguousMemorySpecifyCache(
                                                            (PVOID)pAE,
                                                            pSQI->pDblBuffListAlloc,
                                                            pSQI->dblBuffListSz,
                                                            MmCached);

#endif
                                pCT = pRMT->pCoreTbl + Core;
                                pCT->SubQueue = pCT->CplQueue = 1;
                            }

                            pQI->NumSubIoQAllocated = pQI->NumCplIoQAllocated = 1;
        
                            pAE->MultipleCoresToSingleQueueFlag = TRUE;
                            return (TRUE);
                        } /* fall back to use only one queue */
                    } /* failure case */
                } else {
                    pAE->MultipleCoresToSingleQueueFlag = TRUE;
                }

                /* Succeeded! Mark down the number of queues allocated */
                pCT->SubQueue = pCT->CplQueue = QueueID;
                StorPortDebugPrint(INFO,
                    "NVMeAllocIoQueues: Core 0x%x ---> QueueID 0x%x\n",
                    CoreTableIndex, QueueID);
                CoreTableIndex++;
            } /* current core */
        } /* current NUMA node */

        return (TRUE);
    }
} /* NVMeAllocIoQueues */


/*******************************************************************************
 * NVMeMsiMapCores
 *
 * @brief NVMeMsiMapCores is called to setup the initial mapping for MSI or MSIX
 *        modes.  Initial mapping is just 1:1, learning will happen as each core
 *        processes an IO and new mappings will be created for optimal use.
 *
 * @param pAE - Pointer to hardware device extension.
 *
 * @return VOID
 ******************************************************************************/
static VOID NVMeMsiMapCores(
    PNVME_DEVICE_EXTENSION pAE
)
{
    UCHAR Core;
    UCHAR MaxCore;
    PRES_MAPPING_TBL pRMT = &pAE->ResMapTbl;
    PMSI_MESSAGE_TBL pMMT = NULL;
    PCORE_TBL pCT = NULL;

    MaxCore = (UCHAR)min(pAE->QueueInfo.NumSubIoQAllocFromAdapter,
                         pAE->QueueInfo.NumCplIoQAllocFromAdapter);


    /*
     * Loop thru the cores and assign granted messages in sequential manner.
     * When requests completed, based on the messagID and look up the
     * associated completion queue for just-completed entries
     */
    for (Core = 0; Core < pRMT->NumActiveCores; Core++) {
        if (Core < MaxCore) {
            /* Handle one Core Table at a time */
            pCT = pRMT->pCoreTbl + Core;

            /* Mark down the initial associated message + SQ/CQ for this core */
            pCT->MsiMsgID = pCT->CplQueue - 1;

            /*
             * On the other side, mark down the associated core number
             * for the message as well
             */
            pMMT = pRMT->pMsiMsgTbl + pCT->MsiMsgID;
            pMMT->CplQueueNum = pCT->CplQueue;

            StorPortDebugPrint(INFO,
                               "NVMeMsiMapCores: Core(0x%x)Msg#(0x%x)\n",
                               Core, pCT->MsiMsgID);        
        }
    }
} /* NVMeMsiMapCores */


/*******************************************************************************
 * NVMeCompleteResMapTbl
 *
 * @brief NVMeCompleteResMapTbl completes the resource mapping table among
 *        active cores, the granted vectors and the allocated IO queues.
 *
 * @param pAE - Pointer to hardware device extension.
 *
 * @return VOID
 ******************************************************************************/
static VOID NVMeCompleteResMapTbl(
    PNVME_DEVICE_EXTENSION pAE
)
{
    PRES_MAPPING_TBL pRMT = &pAE->ResMapTbl;

    /* The last thing to do is completing Resource Mapping Table. */
    if ( (pRMT->InterruptType == INT_TYPE_MSIX) ||
         (pRMT->InterruptType == INT_TYPE_MSI) ) {
        NVMeMsiMapCores(pAE);
    }

    /* No need to do anything more for INTx */

} /* NVMeCompleteResMapTbl */


/*******************************************************************************
 * NVMeRunningWaitOnSetupQueues
 *
 * @brief Called as part of init state machine to perform alloc of queues and
 *        setup of the resouce table
 *
 * @param pAE - Pointer to adapter device extension.
 *
 * @return VOID
 ******************************************************************************/
static VOID NVMeRunningWaitOnSetupQueues(
    PNVME_DEVICE_EXTENSION pAE
)
{
    PQUEUE_INFO pQI = &pAE->QueueInfo;
    USHORT QueueID;
    ULONG Status = STOR_STATUS_SUCCESS;

    /*
     * 1. Allocate IO queues
     * 2. Initialize IO queues
     * 3. Complete Resource Table
     *
     * If not, wait for 1 sec only for crashdump case or ask Storport to call
     * again in normal driver case
     */

    /* Allocate IO queues memory if they weren't already allocated */
    if (pAE->IoQueuesAllocated == FALSE) {
        if (NVMeAllocIoQueues( pAE ) == FALSE) {
            NVMeDriverFatalError(pAE,
                                (1 << START_STATE_QUEUE_ALLOC_FAILURE));
            NVMeCallArbiter(pAE);
            return;
        }

        pAE->IoQueuesAllocated = TRUE;
    }

    /*
     * Now we have all resources in place, complete resource mapping table now.
     */
    if (pAE->ResourceTableMapped == FALSE) {
        NVMeCompleteResMapTbl(pAE);
        pAE->ResourceTableMapped = TRUE;
    }

    /* Once we have the queue memory allocated, initialize them */
    for (QueueID = 1; QueueID <= pQI->NumSubIoQAllocated; QueueID++) {
        /* Initialize all Submission queues/entries */
        Status = NVMeInitSubQueue(pAE, QueueID);
        if (Status != STOR_STATUS_SUCCESS) {
            NVMeDriverFatalError(pAE,
                                (1 << START_STATE_QUEUE_INIT_FAILURE));
            NVMeCallArbiter(pAE);
            return;
        }
    }

    for (QueueID = 1; QueueID <= pQI->NumCplIoQAllocated; QueueID++) {
        /* Initialize all Completion queues/entries */
        Status = NVMeInitCplQueue(pAE, QueueID);
        if (Status != STOR_STATUS_SUCCESS) {
            NVMeDriverFatalError(pAE,
                                (1 << START_STATE_QUEUE_INIT_FAILURE));
            NVMeCallArbiter(pAE);
            return;
        }
    }

    for (QueueID = 1; QueueID <= pQI->NumSubIoQAllocated; QueueID++) {
        /* Initialize all CMD_ENTRY entries */
        Status = NVMeInitCmdEntries(pAE, QueueID);
        if (Status != STOR_STATUS_SUCCESS) {
            NVMeDriverFatalError(pAE,
                                (1 << START_STATE_QUEUE_INIT_FAILURE));
            NVMeCallArbiter(pAE);
            return;
        }
    }

    /*
     * Transition to NVMeWaitOnIoCQ state
     * pAE->StartState.NextStartState = NVMeWaitOnIoCQ;
     */
    ASSERT(pQI->NumCplIoQCreated == 0);
    pAE->DriverState.NextDriverState = NVMeWaitOnIoCQ;
    pAE->DriverState.StateChkCount = 0;

    NVMeCallArbiter(pAE);
} /* NVMeRunningWaitOnSetupQueues */


/*******************************************************************************
 * NVMeCreateCplQueue
 *
 * @brief NVMeCreateCplQueue gets called to create one IO completion queue at a
 *        time via issuing Create IO Completion Queue command. The smaller value
 *        of NumCplIoQAllocated of QUEUE_INFO and NumActiveCores of
 *        RES_MAPPING_TABLE decides the number of completion queues to create,
 *        which is indicated in NumCplIoQAllocated of QUEUE_INFO.
 *
 * @param pAE - Pointer to hardware device extension.
 * @param QueueID - Which completion queue to create.
 *
 * @return BOOLEAN
 *     TRUE - If the issued commands completed without any errors
 *     FALSE - If anything goes wrong
 ******************************************************************************/
static BOOLEAN NVMeCreateCplQueue(
    PNVME_DEVICE_EXTENSION pAE,
    USHORT QueueID
)
{
    PNVME_SRB_EXTENSION pNVMeSrbExt =
        (PNVME_SRB_EXTENSION)pAE->DriverState.pSrbExt;
    PQUEUE_INFO pQI = &pAE->QueueInfo;
    PNVMe_COMMAND pCreateCpl = NULL;
    PADMIN_CREATE_IO_COMPLETION_QUEUE_DW10 pCreateCplCDW10 = NULL;
    PADMIN_CREATE_IO_COMPLETION_QUEUE_DW11 pCreateCplCDW11 = NULL;
    PCPL_QUEUE_INFO pCQI = NULL;

    if (QueueID != 0 && QueueID <= pQI->NumCplIoQAllocated) {
        /* Zero-out the entire SRB_EXTENSION */
        memset((PVOID)pNVMeSrbExt, 0, sizeof(NVME_SRB_EXTENSION));

        /* Populate SRB_EXTENSION fields */
        pNVMeSrbExt->pNvmeDevExt = pAE;
        pNVMeSrbExt->pNvmeCompletionRoutine = NVMeInitCallback;

        /* Populate submission entry fields */
        pCreateCpl = (PNVMe_COMMAND)(&pNVMeSrbExt->nvmeSqeUnit);
        pCreateCplCDW10 = (PADMIN_CREATE_IO_COMPLETION_QUEUE_DW10)
                          (&pCreateCpl->CDW10);
        pCreateCplCDW11 = (PADMIN_CREATE_IO_COMPLETION_QUEUE_DW11)
                          (&pCreateCpl->CDW11);

        /* Populate submission entry fields */
        pCreateCpl->CDW0.OPC = ADMIN_CREATE_IO_COMPLETION_QUEUE;
        pCQI = pQI->pCplQueueInfo + QueueID;
        pCreateCpl->PRP1 = pCQI->CplQStart.QuadPart;
        pCreateCplCDW10->QID = QueueID;
        pCreateCplCDW10->QSIZE = pQI->NumIoQEntriesAllocated - 1;
        pCreateCplCDW11->PC = 1;
        pCreateCplCDW11->IEN = 1;
        pCreateCplCDW11->IV = pCQI->MsiMsgID;

        /* Now issue the command via Admin Doorbell register */
        return ProcessIo(pAE, pNVMeSrbExt, NVME_QUEUE_TYPE_ADMIN, FALSE);
    }

    return (FALSE);
} /* NVMeCreateCplQueue */


/*******************************************************************************
 * NVMeRunningWaitOnIoCQ
 *
 * @brief NVMeRunningWaitOnIoCQ gets called to create IO completion queues via
 *        issuing Create IO Completion Queue command(s)
 *
 * @param pAE - Pointer to adapter device extension.
 *
 * @return VOID
 ******************************************************************************/
static VOID NVMeRunningWaitOnIoCQ(
    PNVME_DEVICE_EXTENSION pAE
)
{
    PQUEUE_INFO pQI = &pAE->QueueInfo;

    /*
     * Issue Create IO Completion Queue commands when first called
     * If failed, fail the state machine
     */
    if (NVMeCreateCplQueue(pAE, (USHORT)pQI->NumCplIoQCreated + 1 ) == FALSE) {
        NVMeDriverFatalError(pAE,
                            (1 << START_STATE_CPLQ_CREATE_FAILURE));
        NVMeCallArbiter(pAE);
    }
} /* NVMeRunningWaitOnIoCQ */


/*******************************************************************************
 * NVMeCreateSubQueue
 *
 * @brief NVMeCreateSubQueue gets called to create one IO submission queue at a
 *        time via issuing Create IO Submission Queue command. The smaller value
 *        of NumSubIoQAllocated of QUEUE_INFO and NumActiveCores of
 *        RES_MAPPING_TABLE decides the number of submission queues to create,
 *        which is indicated in NumSubIoQAllocated of QUEUE_INFO.
 *
 * @param pAE - Pointer to hardware device extension.
 * @param QueueID - Which submission queue to create.
 *
 * @return BOOLEAN
 *     TRUE - If the issued commands completed without any errors
 *     FALSE - If anything goes wrong
 ******************************************************************************/
static BOOLEAN NVMeCreateSubQueue(
    PNVME_DEVICE_EXTENSION pAE,
    USHORT QueueID
)
{
    PNVME_SRB_EXTENSION pNVMeSrbExt =
        (PNVME_SRB_EXTENSION)pAE->DriverState.pSrbExt;
    PQUEUE_INFO pQI = &pAE->QueueInfo;
    PNVMe_COMMAND pCreateSub = NULL;
    PADMIN_CREATE_IO_SUBMISSION_QUEUE_DW10 pCreateSubCDW10 = NULL;
    PADMIN_CREATE_IO_SUBMISSION_QUEUE_DW11 pCreateSubCDW11 = NULL;
    PSUB_QUEUE_INFO pSQI = NULL;

    if (QueueID != 0 && QueueID <= pQI->NumCplIoQAllocated) {
        /* Zero-out the entire SRB_EXTENSION */
        memset((PVOID)pNVMeSrbExt, 0, sizeof(NVME_SRB_EXTENSION));

        /* Populate SRB_EXTENSION fields */
        pNVMeSrbExt->pNvmeDevExt = pAE;
        pNVMeSrbExt->pNvmeCompletionRoutine = NVMeInitCallback;

        /* Populate submission entry fields */
        pCreateSub = (PNVMe_COMMAND)(&pNVMeSrbExt->nvmeSqeUnit);
        pCreateSubCDW10 = (PADMIN_CREATE_IO_SUBMISSION_QUEUE_DW10)
                          (&pCreateSub->CDW10);
        pCreateSubCDW11 = (PADMIN_CREATE_IO_SUBMISSION_QUEUE_DW11)
                          (&pCreateSub->CDW11);

        /* Populate submission entry fields */
        pCreateSub->CDW0.OPC = ADMIN_CREATE_IO_SUBMISSION_QUEUE;
        pSQI = pQI->pSubQueueInfo + QueueID;
        pCreateSub->PRP1 = pSQI->SubQStart.QuadPart;
        pCreateSubCDW10->QID = QueueID;
        pCreateSubCDW10->QSIZE = pQI->NumIoQEntriesAllocated - 1;
        pCreateSubCDW11->CQID = pSQI->CplQueueID;
        pCreateSubCDW11->PC = 1;

        /* Now issue the command via Admin Doorbell register */
        return ProcessIo(pAE, pNVMeSrbExt, NVME_QUEUE_TYPE_ADMIN, FALSE);
    }

    return (FALSE);
} /* NVMeCreateSubQueue */


/*******************************************************************************
 * NVMeRunningWaitOnIoSQ
 *
 * @brief NVMeRunningWaitOnIoSQ gets called to create IO submission queues via
 *        issuing Create IO Submission Queue command(s)
 *
 * @param pAE - Pointer to adapter device extension.
 *
 * @return VOID
 ******************************************************************************/
static VOID NVMeRunningWaitOnIoSQ(
    PNVME_DEVICE_EXTENSION pAE
)
{
    PQUEUE_INFO pQI = &pAE->QueueInfo;

    /*
     * Issue Create IO Submission Queue commands when first called
     * If failed, fail the state machine
     */
    if (NVMeCreateSubQueue(pAE, (USHORT)pQI->NumSubIoQCreated + 1) == FALSE) {
        NVMeDriverFatalError(pAE,
                            (1 << START_STATE_SUBQ_CREATE_FAILURE));
        NVMeCallArbiter(pAE);
    }
} /* NVMeRunningWaitOnIoSQ */


/*******************************************************************************
 * NVMeAllocatePool
 *
 * @brief Helper routoine for non-contiguous buffer allocation.
 *        StorPortAllocatePool is called to allocate memory from non-paged pool.
 *        If succeeded, zero out the memory before returning to the caller.
 *
 * @param pAE - Pointer to hardware device extension
 * @param Size - In bytes
 *
 * @return PVOID
 *     Buffer Addr - If all resources are allocated and initialized properly
 *     NULL - If anything goes wrong
 ******************************************************************************/
static PVOID NVMeAllocatePool(
    PNVME_DEVICE_EXTENSION pAE,
    ULONG Size
)
{
    ULONG Tag = 0x654D564E;
    PVOID pBuf = NULL;
    ULONG Status = STOR_STATUS_SUCCESS;
    ULONG NewBytesAllocated;

    if (pAE->ntldrDump == FALSE) {
        /* Call StorPortAllocatePool to allocate the buffer */
        Status = StorPortAllocatePool(pAE, Size, Tag, (PVOID)&pBuf);
    } else {     
        NewBytesAllocated = pAE->DumpBufferBytesAllocated + Size;
        if (NewBytesAllocated <= DUMP_BUFFER_SIZE) {
            pBuf = pAE->DumpBuffer + pAE->DumpBufferBytesAllocated;               
            pAE->DumpBufferBytesAllocated = NewBytesAllocated;
        } else {
             StorPortDebugPrint(ERROR,
                                "Unable to allocate %d bytes at DumpBuffer offset %d.\n",
                                Size,
                                pAE->DumpBufferBytesAllocated);
        }
    }    

    /* It fails, log the error and return NULL */
    if ((Status != STOR_STATUS_SUCCESS) || (pBuf == NULL)) {
        StorPortDebugPrint(ERROR,
                           "NVMeAllocatePool:<Error> Failure, sts=0x%x\n",
                           Status );
        return NULL;
    }

    /* Zero out the buffer before return */
    memset(pBuf, 0, Size);

    return pBuf;
} /* NVMeAllocatePool */


/*******************************************************************************
 * NVMeRunningWaitOnLearnMapping
 *
 * @brief NVMeRunningWaitOnLearnMapping is one of the final steps in the init
 *        state machine and simply sends a through each queue in order to
 *        excercise the learning mode in the IO path.
 *
 * NOTE/TODO:  This only works if a namespace exists on boot.  If the ctlr
 *             has no namespace defined until later, the Qs will not be
 *             optimized until the next boot.
 *
 * @param pAE - Pointer to adapter device extension.
 *
 * @return VOID
 ******************************************************************************/
static VOID NVMeRunningWaitOnLearnMapping(
    PNVME_DEVICE_EXTENSION pAE
)
{
    PNVME_SRB_EXTENSION pNVMeSrbExt =
        (PNVME_SRB_EXTENSION)pAE->DriverState.pSrbExt;
    PNVMe_COMMAND pCmd = (PNVMe_COMMAND)(&pNVMeSrbExt->nvmeSqeUnit);
    PRES_MAPPING_TBL pRMT = &pAE->ResMapTbl;
    STOR_PHYSICAL_ADDRESS PhysAddr;
    PNVME_LUN_EXTENSION pLunExt = NULL;
    UINT32 lbaLengthPower, lbaLength;
    //PQUEUE_INFO pQI = &pAE->QueueInfo;
    UINT8 flbas;
    BOOLEAN error = FALSE;

    __try {
        pLunExt = pAE->pLunExtensionTable[0];
#ifdef DUMB_DRIVER
        {
#else
        if ((pRMT->pMsiMsgTbl->Shared == TRUE) ||
            (pRMT->InterruptType == INT_TYPE_INTX) ||
            (pRMT->InterruptType == INT_TYPE_MSI) ||
            (pAE->DriverState.IdentifyNamespaceFetched == 0) ||
            (pAE->visibleLuns == 0) ||
            (pLunExt == NULL)) {
#endif
            /*
             * go ahead and complete the init state machine now
             * but effectively disable learning as one of the above
             * conditions makes it impossible or unneccessary to learn
            */
            pAE->LearningCores = min(pAE->QueueInfo.NumCplIoQAllocFromAdapter,
                                     pAE->QueueInfo.NumSubIoQAllocFromAdapter);
            pAE->DriverState.NextDriverState = NVMeStartComplete;
            __leave;
        }

        /* Zero out the extension first */
        memset((PVOID)pNVMeSrbExt, 0, sizeof(NVME_SRB_EXTENSION));

        /* Populate SRB_EXTENSION fields */
        pNVMeSrbExt->pNvmeDevExt = pAE;
        pNVMeSrbExt->pNvmeCompletionRoutine = NVMeInitCallback;

        /* send a READ of 1 block to LBA0, NSID 0 */
        flbas = pLunExt->identifyData.FLBAS.SupportedCombination;
        lbaLengthPower = pLunExt->identifyData.LBAFx[flbas].LBADS;
        lbaLength = 1 << lbaLengthPower;

        pNVMeSrbExt->pDataBuffer = NVMeAllocatePool(pAE, lbaLength);
        if (NULL == pNVMeSrbExt->pDataBuffer) {
            /*
             * strange that we can't get a small amount of memory
             * so go ahead and complete the init state machine now
            */
            pAE->LearningCores = min(pAE->QueueInfo.NumCplIoQAllocFromAdapter,
                                     pAE->QueueInfo.NumSubIoQAllocFromAdapter);
            pAE->DriverState.NextDriverState = NVMeStartComplete;
            __leave;
        }
        PhysAddr = NVMeGetPhysAddr(pAE, pNVMeSrbExt->pDataBuffer);
        pCmd->CDW0.OPC = NVME_READ;
        pCmd->PRP1 = (ULONGLONG)PhysAddr.QuadPart;
        pCmd->NSID = pLunExt->namespaceId;

        /* Now issue the command via IO queue */
        if (FALSE == ProcessIo(pAE, pNVMeSrbExt, NVME_QUEUE_TYPE_IO, FALSE)) {
            error = TRUE;
            pAE->LearningCores = min(pAE->QueueInfo.NumCplIoQAllocFromAdapter,
                                     pAE->QueueInfo.NumSubIoQAllocFromAdapter);
            pAE->DriverState.NextDriverState = NVMeStartComplete;
            __leave;
        }

    } __finally {
        if ((error == TRUE) && (NULL != pNVMeSrbExt->pDataBuffer)) {
            StorPortFreePool((PVOID)pAE, pNVMeSrbExt->pDataBuffer);
        }
        if (pAE->DriverState.NextDriverState == NVMeStartComplete) {
            NVMeCallArbiter(pAE);
        }
    }

    return;
} /* NVMeRunningWaitOnLearnMapping */


/*******************************************************************************
 * NVMeInitSrbExtension
 *
 * @brief Helper function to initialize the SRB extension
 *
 * @param pSrbExt - Pointer to SRB extension
 * @param pDevExt - Pointer to device extension
 * @param pSrb - Pointer to SRB
 *
 * @return VOID
 ******************************************************************************/
static VOID NVMeInitSrbExtension(
    PNVME_SRB_EXTENSION pSrbExt,
    PNVME_DEVICE_EXTENSION pDevExt,
#if (NTDDI_VERSION > NTDDI_WIN7)
    PSTORAGE_REQUEST_BLOCK pSrb
#else
    PSCSI_REQUEST_BLOCK pSrb
#endif
)
{
    memset(pSrbExt, 0, sizeof(NVME_SRB_EXTENSION));

    pSrbExt->pNvmeDevExt = pDevExt;
    pSrbExt->pSrb = pSrb;

    /* Any future initializations go here... */
} /* NVMeInitSrbExtension */


/*******************************************************************************
 * NVMeDeleteSubQueues
 *
 * @brief NVMeDeleteSubQueues gets called to delete a number of IO submission
 *        queues via issuing Delete IO Submission Queue commands.
 *        For us exclusively by the init state machine
 *
 * @param pAE - Pointer to hardware device extension.
 *
 * @return BOOLEAN
 *     TRUE - If the issued commands completed without any errors
 *     FALSE - If anything goes wrong
 ******************************************************************************/
static BOOLEAN NVMeDeleteSubQueues(
    PNVME_DEVICE_EXTENSION pAE
)
{
    PNVME_SRB_EXTENSION pNVMeSrbExt =
        (PNVME_SRB_EXTENSION)pAE->DriverState.pSrbExt;
    PQUEUE_INFO pQI = &pAE->QueueInfo;
    PNVMe_COMMAND pDeleteSub = NULL;
    PADMIN_DELETE_IO_SUBMISSION_QUEUE_DW10 pDeleteSubCDW10 = NULL;
    USHORT QueueID = (USHORT)pQI->NumSubIoQCreated;

    if (QueueID > 0) {

        /* init and setup srb ext */
        NVMeInitSrbExtension(pNVMeSrbExt, pAE, NULL);
        pNVMeSrbExt->pNvmeCompletionRoutine = NVMeInitCallback;

        /* Populate submission entry fields */
        pDeleteSub = (PNVMe_COMMAND)(&pNVMeSrbExt->nvmeSqeUnit);
        pDeleteSubCDW10 = (PADMIN_DELETE_IO_SUBMISSION_QUEUE_DW10)
                          (&pDeleteSub->CDW10);
        pDeleteSub->CDW0.OPC = ADMIN_DELETE_IO_SUBMISSION_QUEUE;
        pDeleteSubCDW10->QID = QueueID;

        /* Now issue the command via Admin Doorbell register */
        if (ProcessIo(pAE, pNVMeSrbExt, NVME_QUEUE_TYPE_ADMIN, FALSE) == FALSE) {
            return (FALSE);
        }
    }
    return (TRUE);
} /* NVMeDeleteSubQueues */


/*******************************************************************************
 * NVMeDeleteCplQueues
 *
 * @brief NVMeDeleteCplQueues gets called to delete a number of IO completion
 *        queues via issuing Delete IO Completion Queue commands.
 *        For us exclusively by the init state machine
 *
 * @param pAE - Pointer to hardware device extension.
 *
 * @return BOOLEAN
 *     TRUE - If the issued commands completed without any errors
 *     FALSE - If anything goes wrong
 ******************************************************************************/
static BOOLEAN NVMeDeleteCplQueues(
    PNVME_DEVICE_EXTENSION pAE
)
{
    PNVME_SRB_EXTENSION pNVMeSrbExt =
        (PNVME_SRB_EXTENSION)pAE->DriverState.pSrbExt;
    PQUEUE_INFO pQI = &pAE->QueueInfo;
    PNVMe_COMMAND pDeleteCpl = NULL;
    PADMIN_DELETE_IO_COMPLETION_QUEUE_DW10 pDeleteCplCDW10 = NULL;
    USHORT QueueID = (USHORT)pQI->NumCplIoQCreated;

    if (QueueID > 0) {

        /* init and setup srb ext */
        NVMeInitSrbExtension(pNVMeSrbExt, pAE, NULL);
        pNVMeSrbExt->pNvmeCompletionRoutine = NVMeInitCallback;

        /* Populate submission entry fields */
        pDeleteCpl = (PNVMe_COMMAND)(&pNVMeSrbExt->nvmeSqeUnit);
        pDeleteCplCDW10 = (PADMIN_DELETE_IO_COMPLETION_QUEUE_DW10)
                          (&pDeleteCpl->CDW10);
        pDeleteCpl->CDW0.OPC = ADMIN_DELETE_IO_COMPLETION_QUEUE;
        pDeleteCplCDW10->QID = QueueID;

        /* Now issue the command via Admin Doorbell register */
        if (ProcessIo(pAE, pNVMeSrbExt, NVME_QUEUE_TYPE_ADMIN, FALSE) == FALSE) {
            return (FALSE);
        }
    }

    return (TRUE);
} /* NVMeDeleteCplQueues */


/*******************************************************************************
 * NVMeRunningWaitOnReSetupQueues
 *
 * @brief NVMeRunningWaitOnReSetupQueues gets called if learning mode decided
 *        that the queues were not correctly mapped.  It deletes all the
 *        queues and reallocates mem and recreates them based on learned
 *        mappings
 *
 * @param pAE - Pointer to adapter device extension.
 *
 * @return VOID
 ******************************************************************************/
static VOID NVMeRunningWaitOnReSetupQueues(
    PNVME_DEVICE_EXTENSION pAE
)
{
    //PRES_MAPPING_TBL pRMT = &pAE->ResMapTbl;
    PQUEUE_INFO pQI = &pAE->QueueInfo;

    /* Delete all submission queues */
    if (NVMeDeleteSubQueues(pAE) == FALSE) {
        NVMeDriverFatalError(pAE,
                            (1 << FATAL_SUBQ_DELETE_FAILURE));
        NVMeCallArbiter(pAE);
        return;
    }

    /* Delete all completion queues if we're done deleting submision queues */
    if (pQI->NumSubIoQCreated == 0) {
        if (NVMeDeleteCplQueues(pAE) == FALSE) {
            NVMeDriverFatalError(pAE,
                                (1 << FATAL_SUBQ_DELETE_FAILURE));
            NVMeCallArbiter(pAE);
            return;
        }
    }

} /* NVMeRunningWaitOnReSetupQueues */


/*******************************************************************************
 * NVMeRunning
 *
 * @brief NVMeRunning is called to dispatch the processing depending on the next
 *        state. It can be called by NVMeRunningStartAttempt or Storport to call
 *        the associated function.
 *
 * @param pAE - Pointer to adapter device extension.
 *
 * @return VOID
 ******************************************************************************/
static VOID NVMeRunning(
    PNVME_DEVICE_EXTENSION pAE
)
{

    ULONG coreCount = 0;
    USHORT queueIndex = 1; 

    /*
     * Go to the next state in the Start State Machine
     * transitions are managed either in the state handler or
     * in the completion routines and executed via DPC (except crasdump)
     * calling back into this arbiter
     */
    switch (pAE->DriverState.NextDriverState) {
        case NVMeStateFailed:
            NVMeFreeBuffers(pAE);
        break;
        case NVMeWaitOnRDY:
            NVMeRunningWaitOnRDY(pAE);
        break;
        case NVMeWaitOnIdentifyCtrl:
            NVMeRunningWaitOnIdentifyCtrl(pAE);
        break;
        case NVMeWaitOnIdentifyNS:
            NVMeRunningWaitOnIdentifyNS(pAE);
        break;
        case NVMeWaitOnSetFeatures:
            NVMeRunningWaitOnSetFeatures(pAE);
        break;
        case NVMeWaitOnSetupQueues:
            NVMeRunningWaitOnSetupQueues(pAE);
        break;
        case NVMeWaitOnIoCQ:
            NVMeRunningWaitOnIoCQ(pAE);
        break;
        case NVMeWaitOnIoSQ:
            NVMeRunningWaitOnIoSQ(pAE);
        break;
        case NVMeWaitOnLearnMapping:
            NVMeRunningWaitOnLearnMapping(pAE);
        break;
        case NVMeWaitOnReSetupQueues:
            NVMeRunningWaitOnReSetupQueues(pAE);
        break;
        case NVMeStartComplete:
            pAE->RecoveryAttemptPossible = TRUE;

            for (coreCount = 0;
                 coreCount < pAE->ResMapTbl.NumActiveCores;
                 coreCount++) {

                PCORE_TBL pCT = NULL;
                /*
                 * Assign queues to cores by Round Robin. Only the
                 * submission side needs to be set because the
                 * completion for IO submitted on these cores will
                 * go to the core associated with that queue during
                 * learning mode. For gaps in MSI vectors mapped to
                 * cores, do this for all unlearned cores.
                 */
                pCT = pAE->ResMapTbl.pCoreTbl + coreCount;

                if (!pCT->Learned) {
                    pCT->SubQueue = queueIndex;

                    //queueIndex = (queueIndex < (USHORT)pAE->LearningCores) ?
                    //                ++queueIndex : 1;
                    if(queueIndex < (USHORT)pAE->LearningCores)
                    {
                        ++queueIndex;
                    }
                    else
                    {
                        queueIndex = 1;
                    }
                }
            }

            /* Indicate learning is done with no unassigned cores */
            pAE->LearningCores = coreCount;

            if (pAE->DriverState.resetDriven) {
                /* If this was at the request of the host, complete that Srb */
                if (pAE->DriverState.pResetSrb != NULL) {
                    pAE->DriverState.pResetSrb->SrbStatus = SRB_STATUS_SUCCESS;

                    IO_StorPortNotification(RequestComplete,
                                            pAE,
                                            pAE->DriverState.pResetSrb);
                }
                pAE->DriverState.resetDriven = FALSE;
                pAE->DriverState.pResetSrb = NULL;
            }
            /* Ready again for host commands */
            StorPortDebugPrint(INFO,"NVMeRunning: StorPortReady...\n");
            StorPortReady(pAE);
        break;
        default:
        break;
    } /* end switch */
} /* NVMeRunning */


/*******************************************************************************
 * NVMeEnumMsiMessages
 *
 * @brief NVMeEnumMsiMessages retrieves the related information of granted MSI
 *        vectors, such as, address, data, and the number of granted vectors,
 *        etc...
 *
 * @param pAE - Pointer to hardware device extension.
 *
 * @return BOOLEAN
 *     TRUE - If all resources are allocated and initialized properly
 *     FALSE - If anything goes wrong
 ******************************************************************************/
static BOOLEAN NVMeEnumMsiMessages (
    PNVME_DEVICE_EXTENSION pAE
)
{
    ULONG32 MsgID;
    ULONG32 Status = STOR_STATUS_SUCCESS;
    MESSAGE_INTERRUPT_INFORMATION MII;
    PRES_MAPPING_TBL pRMT = &pAE->ResMapTbl;
    PMSI_MESSAGE_TBL pMMT = NULL;

    /* Assuming it's MSI-X by defult and find it out later */
    pRMT->InterruptType = INT_TYPE_MSIX;

    /*
     * Loop thru each MessageID by calling StorPortMSIInfo
     * to see if it is granted by OS and figure out how many
     * messages are actually granted
     */
    for (MsgID = 0; MsgID < pRMT->NumActiveCores; MsgID++) {
        pMMT = pRMT->pMsiMsgTbl + MsgID;

        memset(&MII, 0, sizeof(MESSAGE_INTERRUPT_INFORMATION));

        Status = StorPortGetMSIInfo ( pAE, MsgID, &MII );
        if (Status == STOR_STATUS_SUCCESS) {
            /* It's granted only when the IDs matched */
            if (MsgID == MII.MessageId) {
                pMMT->MsgID = MII.MessageId;
                pMMT->Addr = MII.MessageAddress;
                pMMT->Data = MII.MessageData;
            } else {
                ASSERT(FALSE);
            }
        } else {
            /* Use INTx when failing to retrieve any message information */
            if (MsgID == 0)
                pRMT->InterruptType = INT_TYPE_INTX;

            break;
        }
    }

    pRMT->NumMsiMsgGranted = MsgID;

    StorPortDebugPrint(INFO,
                       "NVMeEnumMsiMessages: Msg granted=%d\n",
                       pRMT->NumMsiMsgGranted);

    /* Is the request message number satisfied? */
    if (pRMT->NumMsiMsgGranted > 1) {
        /* If the addresses for the first 2 messages are == then its MSI */
        pMMT = pRMT->pMsiMsgTbl + 1;
        if (pMMT->Addr.QuadPart == pRMT->pMsiMsgTbl->Addr.QuadPart) {
            pRMT->InterruptType = INT_TYPE_MSI;
        }
    } else if (pRMT->NumMsiMsgGranted == 1) {
        /* Only one message granted and it is shared */
        pRMT->InterruptType = INT_TYPE_MSI;
        pRMT->pMsiMsgTbl->Shared = TRUE;
    } else {
        /* Using INTx and the interrupt is shared anyway */
        pRMT->pMsiMsgTbl->Shared = TRUE;
    }

    return (TRUE);
} /* NVMeEnumMsiMessages */


/*******************************************************************************
 * NVMeInitAdminQueues
 *
 * @brief This function initializes the admin queues (SQ/CQ pair)
 *
 * @param pAE - Pointer to device extension
 *
 * @return ULONG
 *     Returns status based upon results of init'ing the admin queues
 ******************************************************************************/
static ULONG NVMeInitAdminQueues(
    PNVME_DEVICE_EXTENSION pAE
)
{
    ULONG Status;

    /* Initialize Admin Submission queue */
    Status = NVMeInitSubQueue(pAE, 0);
    if (Status != STOR_STATUS_SUCCESS) {
        return (Status);
    }

    /* Initialize Admin Completion queue */
    Status = NVMeInitCplQueue(pAE, 0);
    if (Status != STOR_STATUS_SUCCESS) {
        return (Status);
    }

    /* Initialize Command Entries */
    Status = NVMeInitCmdEntries(pAE, 0);
    if (Status != STOR_STATUS_SUCCESS) {
        return (Status);
    }

    /*
     * Enable adapter after initializing some controller and Admin queue
     * registers. Need to determine if the adapter is ready for
     * processing commands after entering Start State Machine
     */
    if ((NVMeEnableAdapter(pAE)) == FALSE){
        return (STOR_STATUS_UNSUCCESSFUL);
    }

    return (STOR_STATUS_SUCCESS);
} /* NVMeInitAdminQueues */


/******************************************************************************
 * SntiDpcRoutine
 *
 * @brief SNTI DPC routine to feee memory that was allocated (since we cannot
 *        call the Storport API to free memory at DIRQL).
 *
 * @param pDpc - Pointer to SNTI DPC
 * @param pHwDeviceExtension - Pointer to device extension
 * @param pSystemArgument1 - Arg 1
 * @param pSystemArgument2 - Arg 2
 *
 * @return VOID
 ******************************************************************************/
static VOID SntiDpcRoutine(
    IN PSTOR_DPC  pDpc,
    IN PVOID  pHwDeviceExtension,
    IN PVOID  pSystemArgument1,
    IN PVOID  pSystemArgument2
)
{
    PNVME_SRB_EXTENSION pSrbExt = NULL;
    PNVME_DEVICE_EXTENSION pDevExt = NULL;
    UINT32 bufferSize;
    ULONG status;

    pDevExt = (PNVME_DEVICE_EXTENSION)pHwDeviceExtension;
    pSrbExt = (PNVME_SRB_EXTENSION)pSystemArgument1;
    bufferSize = (UINT32)pSystemArgument2;

    status = StorPortPatchFreeContiguousMemorySpecifyCache(pDevExt,
                                                      pSrbExt->pDataBuffer,
                                                      bufferSize,
                                                      MmCached);

    if (status != STOR_STATUS_SUCCESS) {
        ASSERT(FALSE);
    }
    pSrbExt->pDataBuffer = NULL;
} /* SntiDpcRoutine */


/*******************************************************************************
 * NVMeDetectPendingCmds
 *
 * @brief NVMeDetectPendingCmds gets called to check for commands that may still
 *        be pending. Called when the caller is about to shutdown per S3 or S4.
 *
 * @param pAE - Pointer to hardware device extension.
 * @param completeCmd - determines if deteced commands should be completed
 *
 * @return BOOLEAN
 *     TRUE if commands are detected that are still pending
 *     FALSE if no commands pending
 ******************************************************************************/
static BOOLEAN NVMeDetectPendingCmds(
    PNVME_DEVICE_EXTENSION pAE,
    BOOLEAN completeCmd
)
{
    PQUEUE_INFO pQI = &pAE->QueueInfo;
    PSUB_QUEUE_INFO pSQI = NULL;
    PCMD_ENTRY pCmdEntry = NULL;
    USHORT CmdID;
    USHORT QueueID = 0;
    PNVME_SRB_EXTENSION pSrbExtension = NULL;
    BOOLEAN retValue = FALSE;
    PNVMe_COMMAND pNVMeCmd = NULL;

    /*
     * there is a 0xD1 BSOD on shutdown/restart *with verifier on*
     * something to do with QEMU and IA emulation.  Confirmed the
     * mem in question is safe (Q mem) and this doesn't happen with
     * real HW.  So, if you use QEMU and verifier, uncomment this
     * line
     */
    /* return FALSE; */

    /* Simply return FALSE when buffer had been freed */
    if (pQI->pSubQueueInfo == NULL)
        return retValue;

    /* Search all submission queues */
    for (QueueID = 0; QueueID <= pQI->NumSubIoQCreated; QueueID++) {
        pSQI = pQI->pSubQueueInfo + QueueID;

        for (CmdID = 0; CmdID < pSQI->SubQEntries; CmdID++) {
            pCmdEntry = ((PCMD_ENTRY)pSQI->pCmdEntry) + CmdID;
            if (pCmdEntry->Pending == TRUE) {
                pSrbExtension = (PNVME_SRB_EXTENSION)pCmdEntry->Context;

                /*
                 * Since pending is set, pSrbExtension should exist
                 */
		        ASSERT(pSrbExtension != NULL);
                        
		        pNVMeCmd = &pSrbExtension->nvmeSqeUnit;

		        /*
		         * Internal cmd need to be completed
		         */
		        if (pSrbExtension->pSrb == NULL) {
			        NVMeCompleteCmd(pAE,
                                        pSQI->SubQueueID,
                                        NO_SQ_HEAD_CHANGE,
                                        pNVMeCmd->CDW0.CID,
                                        (PVOID)&pSrbExtension);
			        continue;
		        }

#ifdef HISTORY
                TraceEvent(DETECTED_PENDING_CMD,
                    QueueID,
                    pNVMeCmd->CDW0.CID,
                    pNVMeCmd->CDW0.OPC,
                    pNVMeCmd->PRP1,
                    pNVMeCmd->PRP2,
                    pNVMeCmd->NSID);
#endif

#if DBG
                DbgPrintEx(DPFLTR_STORMINIPORT_ID,
                    DPFLTR_ERROR_LEVEL,
                    "NVMeDetectPendingCmds: cmdinfo cmd id 0x%x srbExt 0x%x srb 0x%x\n",
                    pCmdEntry->CmdInfo.CmdID, pSrbExtension, pSrbExtension->pSrb);
                DbgPrintEx(DPFLTR_STORMINIPORT_ID,
                    DPFLTR_ERROR_LEVEL,
                    "\tnvme queue 0x%x OPC 0x%x\n",
                    QueueID, pNVMeCmd->CDW0.OPC);
                DbgPrintEx(DPFLTR_STORMINIPORT_ID,
                    DPFLTR_ERROR_LEVEL,
                    "\tnvme cmd id 0x%x\n",
                    pNVMeCmd->CDW0.CID);
                DbgPrintEx(DPFLTR_STORMINIPORT_ID,
                    DPFLTR_ERROR_LEVEL,
                    "\tnvme nsid 0x%x\n",
                    pNVMeCmd->NSID);
                DbgPrintEx(DPFLTR_STORMINIPORT_ID,
                    DPFLTR_ERROR_LEVEL,
                    "\tnvme prp1 0x%x 0x%x\n",
                    pNVMeCmd->PRP1 >> 32,
                    pNVMeCmd->PRP1);
                DbgPrintEx(DPFLTR_STORMINIPORT_ID,
                    DPFLTR_ERROR_LEVEL,
                    "\tnvme prp2 0x%x 0x%x\n",
                    pNVMeCmd->PRP2 >> 32,
                    pNVMeCmd->PRP2);
                DbgPrintEx(DPFLTR_STORMINIPORT_ID,
                    DPFLTR_ERROR_LEVEL,
                    "\tnvme CDW10 0x%x\n",
                    pNVMeCmd->CDW10);
                DbgPrintEx(DPFLTR_STORMINIPORT_ID,
                    DPFLTR_ERROR_LEVEL,
                    "\tnvme CDW11 0x%x\n",
                    pNVMeCmd->CDW11);
                DbgPrintEx(DPFLTR_STORMINIPORT_ID,
                    DPFLTR_ERROR_LEVEL,
                    "\tnvme CDW12 0x%x\n",
                    pNVMeCmd->CDW12);
                DbgPrintEx(DPFLTR_STORMINIPORT_ID,
                    DPFLTR_ERROR_LEVEL,
                    "\tnvme CDW13 0x%x\n",
                    pNVMeCmd->CDW13);
                DbgPrintEx(DPFLTR_STORMINIPORT_ID,
                    DPFLTR_ERROR_LEVEL,
                    "\tnvme CDW14 0x%x\n",
                    pNVMeCmd->CDW14);
                DbgPrintEx(DPFLTR_STORMINIPORT_ID,
                    DPFLTR_ERROR_LEVEL,
                    "\tnvme CDW15 0x%x\n",
                    pNVMeCmd->CDW15);
#endif

                /* don't count AER as an outstanding cmd */
                if ((pNVMeCmd->CDW0.OPC != ADMIN_ASYNCHRONOUS_EVENT_REQUEST) &&
                    (pSrbExtension->pSrb != NULL)) {
                    retValue = TRUE;
                }

                /* if requested, complete the command now */
                if (completeCmd == TRUE) {

                    NVMeCompleteCmd(pAE,
                                    pSQI->SubQueueID,
                                    NO_SQ_HEAD_CHANGE,
                                    pNVMeCmd->CDW0.CID,
                                    (PVOID)&pSrbExtension);

                    if (pSrbExtension->pSrb != NULL) {
#ifdef HISTORY
                        NVMe_COMPLETION_QUEUE_ENTRY_DWORD_3 nullEntry = {0};
                        TracePathComplete(COMPLETE_CMD_RESET,
                            pSQI->SubQueueID,
                            pNVMeCmd->CDW0.CID, 0, nullEntry,
                            (ULONGLONG)pSrbExtension->pNvmeCompletionRoutine,
                            0);
#endif
                        pSrbExtension->pSrb->SrbStatus = SRB_STATUS_BUS_RESET;
                        IO_StorPortNotification(RequestComplete,
                                                pAE,
                                                pSrbExtension->pSrb);
                    } /* has an Srb */
                } /* complete the command? */
            } /* if cmd is pending */
        } /* for cmds on the SQ */
    } /* for the SQ */

    return retValue;
} /* NVMeDetectPendingCmds */


/*******************************************************************************
 * RecoveryDpcRoutine
 *
 * @brief DPC routine for recovery and resets
 *
 * @param pDpc - Pointer to DPC
 * @param pHwDeviceExtension - Pointer to device extension
 * @param pSystemArgument1
 * @param pSystemArgument2
 *
 * @return VOID
 ******************************************************************************/
static VOID RecoveryDpcRoutine(
    IN PSTOR_DPC pDpc,
    IN PVOID pHwDeviceExtension,
    IN PVOID pSystemArgument1,
    IN PVOID pSystemArgument2
)
{
    PNVME_DEVICE_EXTENSION pAE = (PNVME_DEVICE_EXTENSION)pHwDeviceExtension;
#if (NTDDI_VERSION > NTDDI_WIN7)
    PSTORAGE_REQUEST_BLOCK pSrb = (PSTORAGE_REQUEST_BLOCK)pSystemArgument1;
#else
    PSCSI_REQUEST_BLOCK pSrb = (PSCSI_REQUEST_BLOCK)pSystemArgument1;
#endif
    
    STOR_LOCK_HANDLE startLockhandle = { 0 };
    NVMe_CONTROLLER_CONFIGURATION CC;
    CC.AsUlong = 0;

    StorPortDebugPrint(INFO, "RecoveryDpcRoutine: Entry\n");
#ifdef HISTORY
    TraceEvent(DPC_RESET,0,0,0,0, 0,0);
#endif
    /*
     * Get spinlocks in order, this assures we don't have submission or
     * completion threads happening before or during reset
     */
    StorPortAcquireSpinLock(pAE, StartIoLock, NULL, &startLockhandle);
    
    CC.AsUlong =
            StorPortReadRegisterUlong(pAE, (PULONG)(&pAE->pCtrlRegister->CC));
    
    if (CC.EN == 1) {
        StorPortDebugPrint(INFO, "RecoveryDpcRoutine:  EN already set, wait for RDY...\n");        
        /*
         * Before we transition to 0, make sure the ctrl is actually RDY
         * NOTE:  Some HW implementations may not require this wait and
         * if not then it could be removed as waiting at this IRQL is
         * not recommended.  The spec is not clear on whether we need
         * to wait for RDY to transition EN back to 0 or not.
         */
        NVMeWaitForCtrlRDY(pAE, 1);
        

        StorPortDebugPrint(INFO, "RecoveryDpcRoutine:  Clearing EN...\n");
        /* Now reset */
        CC.EN = 0;
        StorPortWriteRegisterUlong(pAE,
                                   (PULONG)(&pAE->pCtrlRegister->CC),
                                   CC.AsUlong);

        /* Need to ensure it's cleared in CSTS */
        NVMeWaitForCtrlRDY(pAE, 0);                                   
    }

    /*
     * Reset the controller; if any steps fail we just quit which
     * will leave the controller un-usable(storport queues frozen)
     * on purpose to prevent possible data corruption
     */
    if (NVMeResetAdapter(pAE) == TRUE) {
        /* 10 msec "settle" delay post reset */
        NVMeStallExecution(pAE, 10000);

        /* Complete outstanding commands on submission queues */
        StorPortNotification(ResetDetected, pAE, 0);

                /*
         * detect and complete all commands
                 */
        NVMeDetectPendingCmds(pAE, TRUE);

        /*
         * Don't need to hold this anymore, we won't accept new IOs until the
         * init state machine has completed.
         */
        StorPortReleaseSpinLock(pAE, &startLockhandle);

        /* Prepare for new commands */
        if (NVMeInitAdminQueues(pAE) == STOR_STATUS_SUCCESS) {
            /*
             * Start the state mahcine, if all goes well we'll complete the
             * reset Srb when the machine is done.
             */
            NVMeRunningStartAttempt(pAE, TRUE, pSrb);
        } /* init the admin queues */
    } else {
        StorPortReleaseSpinLock(pAE, &startLockhandle);
    }  /* reset the controller */
} /* RecoveryDpcRoutine */


/*******************************************************************************
 * NVMePassiveInitialize
 *
 * @brief NVMePassiveInitialize gets called to do the following for the
 *        Controller:
 *
 *        1. Allocate memory buffers for Admin and IO queues
 *        2. Initialize the queues
 *        3. Initialize/Enable the adapter
 *        4. Construct resource mapping table
 *        5. Issue Admin commands for the initialization
 *        6. Initialize DPCs for command completions that need to free memory
 *        7. Enter Start State machine for other initialization
 *
 * @param Context - Pointer to hardware device extension.
 *
 * @return BOOLEAN
 *     TRUE - If all resources are allocated and initialized properly
 *     FALSE - If anything goes wrong
 ******************************************************************************/
static BOOLEAN NVMePassiveInitialize(
    PVOID Context
)
{
    PNVME_DEVICE_EXTENSION pAE = Context;
    ULONG Status = STOR_STATUS_SUCCESS;
    PQUEUE_INFO pQI = &pAE->QueueInfo;
    PRES_MAPPING_TBL pRMT = &pAE->ResMapTbl;
    ULONG Lun;
    ULONG i;
    ULONG passiveTimeout;

    /* Ensure the Context is valid first */
    if (pAE == NULL)
        return (FALSE);

    /*
     * Based on the number of active cores in the system, allocate sub/cpl queue
     * info structure array first. The total number of structures should be the
     * number of active cores plus one (Admin queue).
     */
    pQI->pSubQueueInfo =
        (PSUB_QUEUE_INFO)NVMeAllocatePool(pAE, sizeof(SUB_QUEUE_INFO) *
                                          (pRMT->NumActiveCores + 1));

    if (pQI->pSubQueueInfo == NULL) {
        /* Free the allocated SUB_QUEUE_INFO structure memory */
        NVMeFreeBuffers(pAE);
        return (FALSE);
    }

    pQI->pCplQueueInfo =
        (PCPL_QUEUE_INFO)NVMeAllocatePool(pAE, sizeof(CPL_QUEUE_INFO) *
                                          (pRMT->NumActiveCores + 1));

    if (pQI->pCplQueueInfo == NULL) {
        /* Free the allocated SUB_QUEUE_INFO structure memory */
        NVMeFreeBuffers(pAE);
        return (FALSE);
    }

    /*
     * Allocate Admin queue first from NUMA node#0 by default If failed, return
     * failure.
     */
    Status = NVMeAllocQueues(pAE,
                             0,
                             pAE->InitInfo.AdQEntries,
                             0);

    if (Status != STOR_STATUS_SUCCESS) {
        /* Free the allocated SUB/CPL_QUEUE_INFO structures memory */
        NVMeFreeBuffers(pAE);
        return (FALSE);
    }

    /* Mark down the actual number of entries allocated for Admin queue */
    pQI->pSubQueueInfo->SubQEntries = pQI->NumAdQEntriesAllocated;
    pQI->pCplQueueInfo->CplQEntries = pQI->NumAdQEntriesAllocated;

    Status = NVMeInitAdminQueues(pAE);
    if (Status != STOR_STATUS_SUCCESS) {
        NVMeFreeBuffers(pAE);
        return (FALSE);
    }

    /* Allocate one SRB Extension for Start State Machine command submissions */
    pAE->DriverState.pSrbExt = NVMeAllocatePool(pAE, sizeof(NVME_SRB_EXTENSION));
    if (pAE->DriverState.pSrbExt == NULL) {
        /* Free the allocated buffers before returning */
        NVMeFreeBuffers (pAE);
        return (FALSE);
    }

    /* Allocate memory for LUN extensions */
    pAE->LunExtSize = MAX_NAMESPACES * sizeof(NVME_LUN_EXTENSION);
    pAE->pLunExtensionTable[0] =
        (PNVME_LUN_EXTENSION)NVMeAllocateMem(pAE, pAE->LunExtSize, 0);

    if (pAE->pLunExtensionTable[0] == NULL) {
        /* Free the allocated buffers before returning */
        NVMeFreeBuffers (pAE);
        return (FALSE);
    }

    /* Populate each LUN extension table with a valid address */
    for (Lun = 1; Lun < MAX_NAMESPACES; Lun++)
        pAE->pLunExtensionTable[Lun] = pAE->pLunExtensionTable[0] + Lun;

    /*
     * Allocate buffer for data transfer in Start State Machine before State
     * Machine starts
     */
    pAE->DriverState.pDataBuffer = NVMeAllocateMem(pAE, PAGE_SIZE, 0);
    if ( pAE->DriverState.pDataBuffer == NULL ) {
        /* Free the allocated buffers before returning */
        NVMeFreeBuffers(pAE);
        return (FALSE);
    }

#ifdef HISTORY
    SubmitIndex = 0;
    CompleteIndex = 0;
    EventIndex = 0;
    memset(&SubmitHistory, 0, sizeof(HISTORY_SUBMIT) * HISTORY_DEPTH);
    memset(&CompleteHistory, 0, sizeof(HISTORY_COMPLETE) * HISTORY_DEPTH);
    memset(&EventHistory, 0, sizeof(HISTORY_EVENT) * HISTORY_DEPTH);
#endif

    /* Initialize a DPC for command completions that need to free memory */
    StorPortInitializeDpc(pAE, &pAE->SntiDpc, SntiDpcRoutine);
    StorPortInitializeDpc(pAE, &pAE->RecoveryDpc, RecoveryDpcRoutine);

    /* Initialize DPC objects for IO completions */
    for (i = 0; i < pAE->NumDpc; i++) {
        StorPortInitializeDpc(pAE,
            (PSTOR_DPC)pAE->pDpcArray + i,
            IoCompletionDpcRoutine);
    }

    pAE->RecoveryAttemptPossible = FALSE;
    pAE->IoQueuesAllocated = FALSE;
    pAE->ResourceTableMapped = FALSE;
    pAE->LearningCores = 0;

    /*
     * Start off the state machine here, the following commands need to be
     * issued before initialization can be finalized:
     *
     *   Identify (Controller structure)
     *   Identify (Namespace structures)
     *   Asynchronous Event Requests (4 commands by default)
     *   Create IO Completion Queues
     *   Create IO Submission Queues
     *   Go through learning mode to match cores/vestors
     */
     NVMeRunningStartAttempt(pAE, FALSE, NULL);


     /*
      * Check timeout, if we fail to start (or recover from reset) then
      * we leave the controller in this state (NextDriverState) and we
      * won't accept any IO.  We'll also log an error.
      *
      */

     /* TO val is based on CAP registre plus a few, 5, seconds to init post RDY */
     passiveTimeout = pAE->uSecCrtlTimeout + (STORPORT_TIMER_CB_us * MICRO_TO_SEC);
     while ((pAE->DriverState.NextDriverState != NVMeStartComplete) &&
            (pAE->DriverState.NextDriverState != NVMeStateFailed)){

        /* increment 5000us (timer callback val */
        pAE->DriverState.TimeoutCounter += pAE->DriverState.CheckbackInterval;
        if (pAE->DriverState.TimeoutCounter > passiveTimeout) {
            NVMeDriverFatalError(pAE,
                                (1 << START_STATE_TIMEOUT_FAILURE));
            break;
        }
        NVMeStallExecution(pAE,STORPORT_TIMER_CB_us);
     }

     return (pAE->DriverState.NextDriverState == NVMeStartComplete) ? TRUE : FALSE;

} /* NVMePassiveInitialize */


/*******************************************************************************
 * NVMeInitialize
 *
 * @brief NVMeInitialize gets called to initialize the following resources after
 *        resetting the adpater. In normal driver loading, enable passive
 *        initialization to handle most of the it. Otherwise, initialization
 *        needs to be finished here.
 *
 *        0. Set up NUMA I/O optimizations
 *        1. Allocate non-paged system memroy queue entries, structures, etc
 *        2. Initialize queues
 *        3. Issue Admin commands to gather more adapter information
 *        4. Construct resource mapping table
 *
 * @param Context - Pointer to hardware device extension.
 *
 * @return VOID
 *     TRUE - If all resources are allocated and initialized properly
 *     FALSE - If anything goes wrong
 ******************************************************************************/
static BOOLEAN NVMeInitialize(
    PVOID Context
)
{
    PNVME_DEVICE_EXTENSION pAE = Context;
    PQUEUE_INFO pQI = &pAE->QueueInfo;
    PRES_MAPPING_TBL pRMT = &pAE->ResMapTbl;
    ULONG Status = STOR_STATUS_SUCCESS;
    USHORT QueueID;
    ULONG QEntries;
    ULONG Lun;
    NVMe_CONTROLLER_CONFIGURATION CC;
    CC.AsUlong = 0;
    PERF_CONFIGURATION_DATA perfData = {0};

    /* Ensure the Context is valid first */
    if (pAE == NULL)
        return (FALSE);

    if (pAE->ntldrDump == FALSE) {
        Status = StorPortInitializePerfOpts(pAE, TRUE, &perfData);
		// we remove the ASSERT here, because the extended storport function StorPortInitializePerfOpts
		// is not present on Windows Server 2003 and we will receive the return code STOR_STATUS_NOT_IMPLEMENTED (0xC1000002L)
        //ASSERT(STOR_STATUS_SUCCESS == Status);
        if (STOR_STATUS_SUCCESS == Status) {
            /* Allow optimization of storport DPCs */
            if (perfData.Flags & STOR_PERF_DPC_REDIRECTION) {
                perfData.Flags = STOR_PERF_DPC_REDIRECTION;
            }
            Status = StorPortInitializePerfOpts(pAE, FALSE, &perfData);
            ASSERT(STOR_STATUS_SUCCESS == Status);
        }
    }
    CC.AsUlong =
        StorPortReadRegisterUlong(pAE, (PULONG)(&pAE->pCtrlRegister->CC));

    if (CC.EN == 1) {
        //NVMe_CONTROLLER_STATUS CSTS = {0};
        //ULONG time = 0;

        StorPortDebugPrint(INFO, "NVMeInitialize:  EN already set, wait for RDY...\n");
        /*
         * Before we transition to 0, make sure the ctrl is actually RDY
         * NOTE:  Some HW implementations may not require this wait and
         * if not then it could be removed as waiting at this IRQL is
         * not recommended.  The spec is not clear on whether we need
         * to wait for RDY to transition EN back to 0 or not.
         */
        NVMeWaitForCtrlRDY(pAE, 1);

        StorPortDebugPrint(INFO, "NVMeInitialize:  Clearing EN...\n");
        /* Now reset */
        CC.EN = 0;
        StorPortWriteRegisterUlong(pAE,
                                   (PULONG)(&pAE->pCtrlRegister->CC),
                                   CC.AsUlong);

        /* Need to ensure it's cleared in CSTS */
        NVMeWaitForCtrlRDY(pAE, 0);
    }

    /*
     * NULLify all to-be-allocated buffer pointers. In failure cases we need to
     * free the buffers in NVMeFreeBuffers, it has not yet been allocated. If
     * it's NULL, nothing needs to be done.
     */
    pAE->DriverState.pSrbExt = NULL;
    pAE->pLunExtensionTable[0] = NULL;
    pAE->QueueInfo.pSubQueueInfo = NULL;
    pAE->QueueInfo.pCplQueueInfo = NULL;

    /*
     * When Crashdump/Hibernation driver is being loaded, need to complete the
     * entire initialization here. In the case of normal driver loading, enable
     * passive initialization and let NVMePassiveInitialization handle the rest
     * of the initialization
     */
    if (pAE->ntldrDump == FALSE) {
        /* Enumerate granted MSI/MSI-X messages if there is any */
        if (NVMeEnumMsiMessages(pAE) == FALSE) {
            NVMeFreeBuffers(pAE);
            return (FALSE);
        }

        /* Call StorPortPassiveInitialization to enable passive init */
        StorPortEnablePassiveInitialization(pAE, NVMePassiveInitialize);

        return (TRUE);
    } else {
        if (pAE->DumpBuffer == NULL) {
            pAE->DumpBuffer = pAE->pPCI->DumpRegion.VirtualBase;
            ASSERT(DUMP_BUFFER_SIZE == pAE->pPCI->DumpRegion.Length);
        }
        pAE->DumpBufferBytesAllocated = 0;
        
        /* Initialize members of resource mapping table first */
        pRMT->InterruptType = INT_TYPE_INTX;
        pRMT->NumActiveCores = 1;
        pAE->QueueInfo.NumCplIoQAllocFromAdapter = 1;
        pAE->QueueInfo.NumSubIoQAllocFromAdapter = 1;

        /*
         * Allocate sub/cpl queue info structure array first. The total number
         * of structures should be two, one IO queue and one Admin queue.
         */
        pQI->pSubQueueInfo =
            (PSUB_QUEUE_INFO)NVMeAllocatePool(pAE, sizeof(SUB_QUEUE_INFO) *
                                              (pRMT->NumActiveCores + 1));

        if (pQI->pSubQueueInfo == NULL) {
            NVMeFreeBuffers(pAE);
            return (FALSE);
        }

        pQI->pCplQueueInfo =
            (PCPL_QUEUE_INFO)NVMeAllocatePool(pAE, sizeof(CPL_QUEUE_INFO) *
                                              (pRMT->NumActiveCores + 1));

        if (pQI->pCplQueueInfo == NULL) {
            NVMeFreeBuffers(pAE);
            return (FALSE);
        }

        /*
         * Allocate buffers for each queue and initialize them if any failures,
         * free allocated buffers and terminate the initialization
         * unsuccessfully
         */
        for (QueueID = 0; QueueID <= pRMT->NumActiveCores; QueueID++) {
            /*
             * Based on the QueueID (0 means Admin queue, others are IO queues),
             * decide number of queue entries to allocate.  Learning mode is
             * not applicable for INTX
             */
            if (pAE->ntldrDump == FALSE) {
                QEntries = (QueueID == 0) ? pAE->InitInfo.AdQEntries:
                                            pAE->InitInfo.IoQEntries;
            } else {
                QEntries = MIN_IO_QUEUE_ENTRIES;
            }

            Status = NVMeAllocQueues(pAE,
                                     QueueID,
                                     QEntries,
                                     0);

            if (Status != STOR_STATUS_SUCCESS) {
                /* Free the allocated buffers before returning */
                NVMeFreeBuffers(pAE);
                return (FALSE);
            }

            /* Initialize Submission queue */
            Status = NVMeInitSubQueue(pAE, QueueID);
            if (Status != STOR_STATUS_SUCCESS) {
                /* Free the allocated buffers before returning */
                NVMeFreeBuffers(pAE);
                return (FALSE);
            }

            /* Initialize Completion queue */
            Status = NVMeInitCplQueue(pAE, QueueID);
            if (Status != STOR_STATUS_SUCCESS) {
                /* Free the allocated buffers before returning */
                NVMeFreeBuffers(pAE);
                return (FALSE);
            }

            /* Initialize Command Entries */
            Status = NVMeInitCmdEntries(pAE, QueueID);
            if (Status != STOR_STATUS_SUCCESS) {
                /* Free the allocated buffers before returning */
                NVMeFreeBuffers(pAE);
                return (FALSE);
            }
        }

        /* Now, conclude how many IO queue memory are allocated */
        pQI->NumSubIoQAllocated = pRMT->NumActiveCores;
        pQI->NumCplIoQAllocated = pRMT->NumActiveCores;

        /*
         * Enable adapter after initializing some controller and Admin queue
         * registers. Need to ensure the adapter is ready for processing
         * commands after entering Start State Machine.
         */
        if ((NVMeEnableAdapter(pAE)) == FALSE){
            return (FALSE);
        }

        /*
         * Allocate one SRB Extension for Start State Machine command
         * submissions
         */
        pAE->DriverState.pSrbExt =
            NVMeAllocatePool(pAE, sizeof(NVME_SRB_EXTENSION));

        if (pAE->DriverState.pSrbExt == NULL) {
            /* Free the allocated buffers before returning */
            NVMeFreeBuffers (pAE);
            return (FALSE);
        }

        /* Allocate memory for LUN extensions */
        pAE->LunExtSize = MAX_NAMESPACES * sizeof(NVME_LUN_EXTENSION);
        pAE->pLunExtensionTable[0] =
            (PNVME_LUN_EXTENSION)NVMeAllocateMem(pAE, pAE->LunExtSize, 0);
        if (pAE->pLunExtensionTable[0] == NULL) {
            /* Free the allocated buffers before returning */
            NVMeFreeBuffers(pAE);
            return (FALSE);
        }

        /* Populate each LUN extension table with valid an address */
        for (Lun = 1; Lun < MAX_NAMESPACES; Lun++)
            pAE->pLunExtensionTable[Lun] = pAE->pLunExtensionTable[0] + Lun;

        /*
         * Allocate buffer for data transfer in Start State Machine before State
         * Machine starts
         */
        pAE->DriverState.pDataBuffer = NVMeAllocateMem(pAE, PAGE_SIZE, 0);
        if (pAE->DriverState.pDataBuffer == NULL) {
            /* Free the allocated buffers before returning */
            NVMeFreeBuffers(pAE);
            return (FALSE);
        }

        /*
         * Start off the state machine here, the following commands need to be
         * issued before initialization can be finalized:
         *
         *   Identify (Controller structure)
         *   Identify (Namespace structures)
         *   Set Features (Feature ID# 7)
         *   Asynchronous Event Requests (4 commands by default)
         *   Create IO Completion Queues
         *   Create IO Submission Queues
         */
         return NVMeRunningStartAttempt(pAE, FALSE, NULL);
    }
} /* NVMeInitialize */


//------------------------------------------------------------------------------
// _StorpGetHighestNodeNumber@8
//------------------------------------------------------------------------------
static ULONG StorPortPatchGetHighestNodeNumber(PVOID HwDeviceExtension,PULONG HighestNode)
{
	if(HighestNode == NULL)
	{
		return STOR_STATUS_INVALID_PARAMETER;
	}

	// the following function call is not supported in procgrp.h
	// therefore we fake the node number to be zero
	// *HighestNode = (ULONG)KeQueryHighestNodeNumber();
	*HighestNode = 0;

	return STOR_STATUS_SUCCESS;
}


//------------------------------------------------------------------------------
// _StorpGetActiveGroupCount@8
//------------------------------------------------------------------------------
static ULONG StorPortPatchGetActiveGroupCount(PVOID HwDeviceExtension,PUSHORT NumberGroups)
{
	if(NumberGroups == NULL)
	{
		return STOR_STATUS_INVALID_PARAMETER;
	}

	*NumberGroups = 1;

	return STOR_STATUS_SUCCESS;
}


//------------------------------------------------------------------------------
// _StorpGetGroupAffinity@12
//------------------------------------------------------------------------------
static ULONG StorPortPatchGetGroupAffinity(PVOID HwDeviceExtension,USHORT GroupNumber,PKAFFINITY GroupAffinityMask)
{
	USHORT GroupCount;

	if(GroupAffinityMask == NULL)
	{
		return STOR_STATUS_INVALID_PARAMETER;
	}

	GroupCount = 1;
	if(GroupNumber > GroupCount)
	{
		return STOR_STATUS_UNSUCCESSFUL;
	}

	*GroupAffinityMask = 1;

	return STOR_STATUS_SUCCESS;
}


//------------------------------------------------------------------------------
// _StorpGetNodeAffinity@12
//------------------------------------------------------------------------------
static ULONG StorPortPatchGetNodeAffinity(PVOID HwDeviceExtension,ULONG NodeNumber,PGROUP_AFFINITY NodeAffinityMask)
{
	KAFFINITY GroupAffinityMask;

	if(NodeAffinityMask == NULL || NodeNumber > 0xFFFF)
	{
		return STOR_STATUS_INVALID_PARAMETER;
	}

	// the following function call is not supported in procgrp.h
	// we fake the node number to be zero and the node affinity mask to be the group affinity mask
	// KeQueryNodeActiveAffinity(NodeNumber,NodeAffinityMask,NULL);
	GroupAffinityMask = 1;
	memcpy(&NodeAffinityMask->Mask,&GroupAffinityMask,sizeof(KAFFINITY));
	NodeAffinityMask->Group = 0;
	NodeAffinityMask->Reserved[0] = 0;
	NodeAffinityMask->Reserved[1] = 0;
	NodeAffinityMask->Reserved[2] = 0;

	return STOR_STATUS_SUCCESS;
}


/*******************************************************************************
 * NVMeActiveProcessorCount
 *
 * @brief Helper routoine for deciding the number of active processors of an
 *        NUMA node.
 *
 * @param Mask - bitmap of the active processors
 *
 * @return USHORT
 *     The number of active processors
 ******************************************************************************/
static USHORT NVMeActiveProcessorCount(
    ULONG_PTR Mask
)
{
    USHORT Count = 0;

    /*
     * Loop thru the bits of Mask (a 32 or 64-bit value), increase the count
     * by one when Mask is non-zero after bit-wise AND operation between Mask
     * and Mask-1. This figures out the number of bits (in Mask) that are set
     * as 1.
     */
    while (Mask) {
        Count++;
        Mask &= (Mask - 1);
    }

    return Count;
} /* NVMeActiveProcessorCount */


/*******************************************************************************
 * NVMeEnumNumaCores
 *
 * @brief NVMeEnumNumaCores collects the current NUMA/CPU topology information.
 *        Confirms if NUMA is supported, how many NUMA nodes in current system
 *        before allocating memory for CORE_TBL structures
 *
 * @param pAE - Pointer to hardware device extension
 *
 * @return BOOLEAN
 *     TRUE - If all resources are allocated and initialized properly
 *     FALSE - If anything goes wrong
 ******************************************************************************/
static BOOLEAN NVMeEnumNumaCores(
    PNVME_DEVICE_EXTENSION pAE
)
{
    GROUP_AFFINITY GroupAffinity;
    USHORT Bit = 0, Grp;
    ULONG Core = 0, CoreCounter = 0;
    ULONG BaseCoreNum = 0;
    ULONG Node = 0;
    ULONG Status = 0;
    ULONG TotalCores = 0;
    USHORT MaxNumCoresInGroup = sizeof(KAFFINITY) * 8;
    PRES_MAPPING_TBL pRMT = &pAE->ResMapTbl;
    PCORE_TBL pCoreTblTemp = NULL;
    PNUMA_NODE_TBL pNNT = NULL;
    BOOLEAN FirstCoreFound = FALSE;
    KAFFINITY GrpAffinityMask = 0;
    PPROC_GROUP_TBL pProcGrpTbl = NULL;
    ULONG SysProcessorNumber = 0;

    /*
     * Decide how many NUMA nodes in current system if only one (0 returned),
     * NUMA is not supported.
     */
    StorPortPatchGetHighestNodeNumber( pAE, &Node );
    pRMT->NumNumaNodes = Node + 1;

    StorPortDebugPrint(INFO,
                       "NVMeEnumNumaCores: # of NUMA node(s) = %d.\n",
                       Node + 1);
    /*
     * Allocate buffer for NUMA_NODE_TBL structure array.
     * If fails, return FALSE
     */
    pRMT->pNumaNodeTbl = (PNUMA_NODE_TBL)
        NVMeAllocatePool(pAE, pRMT->NumNumaNodes * sizeof(NUMA_NODE_TBL));

    if (pRMT->pNumaNodeTbl == NULL)
        return (FALSE);

    /* Find out the current group topology in the system */
    Status = StorPortPatchGetActiveGroupCount(pAE, &pRMT->NumGroup);
    if (pRMT->NumGroup == 0 || Status != STOR_STATUS_SUCCESS) {
        StorPortDebugPrint(INFO, "NVMeEnumNumaCores: Failed to retrieve number of group.\n");
        return (FALSE);
    }
    /*
     * Allocate buffer for PROC_GROUP_TBL structure array.
     * If fails, return FALSE
     */
    pRMT->pProcGroupTbl = (PPROC_GROUP_TBL)
        NVMeAllocatePool(pAE, pRMT->NumGroup * sizeof(PROC_GROUP_TBL));

    if (pRMT->pProcGroupTbl == NULL)
        return (FALSE);

    memset((PVOID)pRMT->pProcGroupTbl, 0, pRMT->NumGroup * sizeof(PROC_GROUP_TBL));
    StorPortDebugPrint(INFO, "NVMeEnumNumaCores: Number of groups = %d.\n", pRMT->NumGroup);
    for (Grp = 0; Grp < pRMT->NumGroup; Grp++) {
        Status = StorPortPatchGetGroupAffinity(pAE, Grp, &GrpAffinityMask);
        if (Status != STOR_STATUS_SUCCESS) {
            StorPortDebugPrint(INFO, "NVMeEnumNumaCores: Failed to retrieve group affinity(%d).\n", Grp);
            return (FALSE);
        }
        StorPortDebugPrint(INFO, "NVMeEnumNumaCores: Group(%d) affinity mask(0x%x).\n", Grp, GrpAffinityMask);
        pProcGrpTbl = pRMT->pProcGroupTbl + Grp;
        pProcGrpTbl->GroupAffinity.Group = Grp;
        pProcGrpTbl->GroupAffinity.Mask = GrpAffinityMask;

        /* Mark down the associated first system-wise logical processor number*/
        pProcGrpTbl->BaseProcessor = SysProcessorNumber;

        /* Find out the number of logical processors of the group */
        pProcGrpTbl->NumProcessor = NVMeActiveProcessorCount(GrpAffinityMask);
        SysProcessorNumber += pProcGrpTbl->NumProcessor;
    }
    StorPortDebugPrint(INFO, "NVMeEnumNumaCores: Total %d logical processors\n", SysProcessorNumber);

    /* Based on NUMA node number, retrieve their affinity masks and counts */
    for (Node = 0; Node < pRMT->NumNumaNodes; Node++) {
        pNNT = pRMT->pNumaNodeTbl + Node;

        StorPortDebugPrint(INFO, "NVMeEnumNumaCores: NUMA Node#%d\n", Node);

        /* Retrieve the processor affinity based on NUMA nodes */
        memset((PVOID)&GroupAffinity, 0, sizeof(GROUP_AFFINITY));

        Status = StorPortPatchGetNodeAffinity(pAE, Node, &GroupAffinity);
        if (Status != STOR_STATUS_SUCCESS) {
            StorPortDebugPrint(ERROR,
                "NVMeEnumNumaCores: <Error> GetNodeAffinity fails, sts=0x%x\n",
                Status);
            return (FALSE);
        }

        StorPortDebugPrint(INFO, "Core mask is 0x%x in Group(%d)\n", 
            GroupAffinity.Mask, GroupAffinity.Group);

        /* Find out the number of active cores of the NUMA node */
        pNNT->NodeNum = Node;
        pNNT->NumCores = NVMeActiveProcessorCount(GroupAffinity.Mask);
        pRMT->NumActiveCores += pNNT->NumCores;
        StorPortCopyMemory((PVOID)&pNNT->GroupAffinity,
                           (PVOID)&GroupAffinity,
                           sizeof(GROUP_AFFINITY));
    }

    /* Allocate buffer for CORE_TBL structure array. If fails, return FALSE */
    pRMT->pCoreTbl = (PCORE_TBL)
        NVMeAllocatePool(pAE, pRMT->NumActiveCores * sizeof(CORE_TBL));

    if (pRMT->pCoreTbl == NULL)
        return (FALSE);

    /* Based on NUMA node number, populate the NUMA/Core tables */
    for (Node = 0; Node < pRMT->NumNumaNodes; Node++) {
        pNNT = pRMT->pNumaNodeTbl + Node;
        if (pNNT->NumCores == 0)
            continue;
        /* figure out the system-wise starting logical processor number for the node */
        pProcGrpTbl = pRMT->pProcGroupTbl + pNNT->GroupAffinity.Group;
        BaseCoreNum = pProcGrpTbl->BaseProcessor;

        /*
         * For each existing NUMA node, need to find out its first and last
         * associated cores in terms of system-wise processor number for
         * later reference in IO queue allocation. Initialize the as the first
         * core number of the associated group.
         */
        pNNT->FirstCoreNum = BaseCoreNum;
        pNNT->LastCoreNum = BaseCoreNum;
        FirstCoreFound = FALSE;

        /* For each core, populate CORE_TBL structure */
        for (Bit = 0; Bit < MaxNumCoresInGroup; Bit++) {
            /* Save previsou bit check result first */
            if (((pNNT->GroupAffinity.Mask >> Bit) & 1) == 1) {
                /* Mark the first core if haven't found yet, reset the counter */
                if (FirstCoreFound == FALSE) {
                    CoreCounter = 0;
                    FirstCoreFound = TRUE;
                }
                Core = BaseCoreNum + CoreCounter;
                pCoreTblTemp = pRMT->pCoreTbl + Core;
                pCoreTblTemp->NumaNode = (USHORT) Node;
                pCoreTblTemp->Group = pNNT->GroupAffinity.Group;
                
                /* Always mark the last core */
                pNNT->LastCoreNum = Core;
                CoreCounter++;
                TotalCores++;
            }
        }

        StorPortDebugPrint(INFO,
                           "There are %d cores in Node#%d.\n",
                           pNNT->NumCores, Node);
    }

    /* Double check the total core number */
    if (TotalCores > pRMT->NumActiveCores) {
        StorPortDebugPrint(ERROR,
            "NVMeEnumNumaCores: <Error> Cores number mismatch, %d, %d\n",
            TotalCores, pRMT->NumActiveCores);

        return (FALSE);
    }

    StorPortDebugPrint(INFO,
                       "The total number of CPU cores %d.\n",
                       pRMT->NumActiveCores);

    return(TRUE);
} /* NVMeEnumNumaCores */


/*******************************************************************************
 * NVMeNormalShutdown
 *
 * @brief NVMeNormalShutdown gets called to follow the normal shutdown sequence
 *        defined in NVMe specification when receiving SRB_FUNCTION_SHUTDOWN
 *        from the host:
 *
 *        1. Set state to NVMeShutdown to prevent taking any new requests
 *        2. Delete all created submisison queues
 *        3. If succeeded, delete all created completion queues (reset device)
 *        4. Set SHN to normal shutdown (01b)
 *        5. Keep reading back CSTS for 100 ms and check if SHST is 01b.
 *        6. If so or timeout, return SRB_STATUS_SUCCESS to Storport
 *
 * @param pAE - Pointer to hardware device extension.
 *
 * @return BOOLEAN
 *     TRUE - If the issued commands completed without any errors
 *     FALSE - If anything goes wrong
 ******************************************************************************/
static BOOLEAN NVMeNormalShutdown(
    PNVME_DEVICE_EXTENSION pAE
)
{
    NVMe_CONTROLLER_CONFIGURATION CC;
    NVMe_CONTROLLER_STATUS CSTS;
    ULONG PollMax = pAE->uSecCrtlTimeout / MAX_STATE_STALL_us;
    ULONG PollCount;

    /* Check for any pending cmds. */
    if (NVMeDetectPendingCmds(pAE, FALSE) == TRUE) {
        return FALSE;
    }

    /* Delete all queues */
    if (NVMeResetAdapter(pAE) != TRUE) {
        return (FALSE);
    }

    /* Need to to ensure the Controller registers are memory-mapped properly */
    if ( pAE->pCtrlRegister == FALSE ) {
        return (FALSE);
    }

    /*
     * Read Controller Configuration first before setting SHN bits of
     * Controller Configuration to normal shutdown (01b)
     */
    CC.AsUlong = StorPortReadRegisterUlong(pAE,
                                           (PULONG)(&pAE->pCtrlRegister->CC));
    CC.SHN = 1;
    /* Set SHN bits of Controller Configuration to normal shutdown (01b) */
    StorPortWriteRegisterUlong (pAE,
                                (PULONG)(&pAE->pCtrlRegister->CC),
                                CC.AsUlong);

    /* Checking if the adapter is ready for normal shutdown */
    for (PollCount = 0; PollCount < PollMax; PollCount++) {
        CSTS.AsUlong = StorPortReadRegisterUlong(pAE,
                           (PULONG)(&pAE->pCtrlRegister->CSTS.AsUlong));

        if (CSTS.SHST == 2) {
            /* Free the memory if we are doing shutdown */
            NVMeFreeBuffers(pAE);
            return (TRUE);
        }

        NVMeStallExecution(pAE, MAX_STATE_STALL_us);
    }

#if DBG
    /*
     * QEMU: Returning TRUE is a workaround as Qemu device is not returning the
     * status as shutdown complete. So this is a workaround for QEMU.
     */
    NVMeFreeBuffers(pAE);
    return (TRUE);
#else /* DBG */
    return (FALSE);
#endif /* DBG */
} /* NVMeNormalShutdown */


//------------------------------------------------------------------------------
// original NVMe OFA driver code ends here
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// our custom stuff starts here
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// NVMeFindAdapter
//
// @brief This function gets called to fill in the Port Configuration
//        Information structure that indicates more capabillites the adapter
//        supports.
//
// @param Context - Pointer to hardware device extension.
// @param Reserved1 - Unused.
// @param Reserved2 - Unused.
// @param ArgumentString - DriverParameter string.
// @param pPCI - Pointer to PORT_CONFIGURATION_INFORMATION structure.
// @param Reserved3 - Unused.
//
// @return ULONG
//     Returns status based upon results of adapter parameter acquisition.
//------------------------------------------------------------------------------
static ULONG NVMeFindAdapter(NVME_DEVICE_EXTENSION *pAE)
{
	NVMe_CONTROLLER_CAPABILITIES CAP;
	CAP.AsUlonglong = 0;
	PRES_MAPPING_TBL pRMT;

	// we set ntldrDump to FALSE, this way the driver runs as in the Windows OS
	pAE->ntldrDump = FALSE;

	// example CAP register content
	// MQES   = 0x3FFF
	// CQR    = 0x01
	// AMS    = 0x01
	// TO     = 0x3C
	// DSTRD  = 0
	// CSS    = 1
	// MPSMIN = 0x00
	// MPSMAX = 0x0F

	CAP.HighPart = StorPortReadRegisterUlong(pAE,(PULONG)(&pAE->pCtrlRegister->CAP.HighPart));
	CAP.LowPart = StorPortReadRegisterUlong(pAE,(PULONG)(&pAE->pCtrlRegister->CAP.LowPart));

	// setup ctrl timeout and stride info
	// one unit (500 milliseonds)
	pAE->uSecCrtlTimeout = (ULONG)(CAP.TO * 500);
	pAE->uSecCrtlTimeout = (pAE->uSecCrtlTimeout == 0) ? 500 : pAE->uSecCrtlTimeout;
	pAE->uSecCrtlTimeout *= 1000; // pAE->uSecCrtlTimeout = 30.000.000

	// save off the DB Stride Size
	// STORMINI: NVMeFindAdapter: Stride Size set to 0x4
	pAE->strideSz = 1 << (2 + CAP.DSTRD); // pAE->strideSz = 4

	// Pre-program with default values in case of failure in accessing Registry Defaultly, it can support up to 16 LUNs per target
	// The INF file sets this to 16.
	pAE->InitInfo.Namespaces = 16; // 0x10 (16) same as set in registry

	// Max transfer size is 128KB by default
	// The INF file sets this to 128 KB.
	pAE->InitInfo.MaxTxSize = 0x20000; // 0x20000 (131.072) same as set in registry
	pAE->PRPListSize = ((pAE->InitInfo.MaxTxSize / PAGE_SIZE) * sizeof(UINT64)); // 0x100 (256)

	// 128 entries by default for Admin queue.
	// The INF file sets this to 128.
	// this is changed after the function NVMeFetchRegistry is called
	pAE->InitInfo.AdQEntries = 128; // 0x100 (256) set to 0x80 (128) by registry later

	// 1024 entries by default for IO queues.
	// The INF file sets this to 1024.
	pAE->InitInfo.IoQEntries = 1024; // 0x400 (1024) same as set in registry

	// Interrupt coalescing by default: 8 millisecond/16 completions.
	// The INF file sets this to 0.
	// this is changed after the function NVMeFetchRegistry is called
	pAE->InitInfo.IntCoalescingTime = 0; // 0x50 (80) set to 0 by registry later
	pAE->InitInfo.IntCoalescingEntry = 0; // 0x10 (16) set to 0 by registry later

	// Access Registry and enumerate NUMA/cores topology when normal driver is
	// being loaded.
	if (pAE->ntldrDump == FALSE)
	{
		// Call NVMeFetchRegistry to retrieve all designated values
		NVMeFetchRegistry(pAE);

		// regardless of hardcoded or reg overrides, IOQ is limited by CAP
		if(pAE->InitInfo.IoQEntries > (ULONG)(CAP.MQES + 1))
		{
			StorPortDebugPrint(INFO, "IO Q size limited by HW to 0x%x\n",(pAE->pCtrlRegister->CAP.MQES + 1));
			pAE->InitInfo.IoQEntries = CAP.MQES + 1;
		}

		// update in case someone used the registry to change MaxTxSize
		pAE->PRPListSize = ((pAE->InitInfo.MaxTxSize / PAGE_SIZE) * sizeof(UINT64));

		// Get the CPU Affinity of current system and construct NUMA table,
		// including if NUMA supported, how many CPU cores, NUMA nodes, etc
		if(NVMeEnumNumaCores(pAE) == FALSE)
		{
			return SP_RETURN_NOT_FOUND;
		}

		// Allocate buffer for MSI_MESSAGE_TBL structure array. If fails, return FALSE.
		pRMT = &pAE->ResMapTbl;
		pRMT->pMsiMsgTbl = (PMSI_MESSAGE_TBL)NVMeAllocatePool(pAE,pRMT->NumActiveCores * sizeof(MSI_MESSAGE_TBL));
		if(pRMT->pMsiMsgTbl == NULL)
		{
			return SP_RETURN_NOT_FOUND;
		}

		// Allocate buffer for DPC completiong array. If fails, return FALSE.
		pAE->NumDpc = pRMT->NumActiveCores + 1;
		pAE->pDpcArray = NVMeAllocatePool(pAE,pAE->NumDpc * sizeof(STOR_DPC));
		if(pAE->pDpcArray == NULL)
		{
			return SP_RETURN_NOT_FOUND;
		}
	}

	return SP_RETURN_FOUND;
}


//------------------------------------------------------------------------------
// NVMeReadPreparePRPs
//------------------------------------------------------------------------------
static BOOLEAN NVMeReadPreparePRPs(PNVME_DEVICE_EXTENSION pAE,PNVME_SRB_EXTENSION pSrbExt,QWORD pBuffer,ULONG TxLength)
{
	PNVMe_COMMAND pSubEntry = &pSrbExt->nvmeSqeUnit;
	STOR_PHYSICAL_ADDRESS PhyAddr;
	QWORD PtrTemp = 0;
	ULONG RoomInFirstPage = 0;
	ULONG RemainLength = TxLength;
	PUINT64 pPrpList = NULL;

	if(pBuffer == 0 || TxLength == 0)
	{
		return (FALSE);
	}

	// Go ahead and prepare 1st PRP entries, need at least one PRP entry
	PhyAddr.QuadPart = pBuffer;
	if(PhyAddr.QuadPart == 0)
	{
		return (FALSE);
	}

	pSubEntry->PRP1 = PhyAddr.QuadPart;
	pSrbExt->numberOfPrpEntries = 1;

	// Find out how much room still available in current page.
	// If it's enough, only PRP1 is needed.
	RoomInFirstPage = PAGE_SIZE - (PhyAddr.QuadPart & (PAGE_SIZE - 1));
	if(RoomInFirstPage >= TxLength)
	{
		return (TRUE);
	}
	else
	{
		RemainLength -= RoomInFirstPage;
	}

	PtrTemp = pBuffer;
	PtrTemp = PAGE_ALIGN_BUF_ADDR(PtrTemp);

	// With the remaining transfer size, either use PRP2 as another PRP entry or
	// a pointer to the pre-allocated PRP list
	if(RemainLength > PAGE_SIZE)
	{
		pPrpList = &pSrbExt->prpList[0];
		pSubEntry->PRP2 = 0;
	}
	else
	{
		// Use PRP2 as 2nd PRP entry and return
		PhyAddr.QuadPart = PtrTemp;
		if(PhyAddr.QuadPart == 0)
		{
			return (FALSE);
		}
		pSubEntry->PRP2 = PhyAddr.QuadPart;
		pSrbExt->numberOfPrpEntries++;
		return (TRUE);
	}

	// Convert data buffer pages into PRP entries while
	// decreasing remaining transfer size and noting the # of PRP entries used
	while(RemainLength)
	{
		PhyAddr.QuadPart = PtrTemp;
		if(PhyAddr.QuadPart == 0)
		{
			return (FALSE);
		}
		*pPrpList = (UINT64)PhyAddr.QuadPart;
		pSrbExt->numberOfPrpEntries++;
		// When remaining size is no larger than a page, it's the last entry */
		if(RemainLength <= PAGE_SIZE)
		{
			break;
		}
		pPrpList++;
		RemainLength -= PAGE_SIZE;
		PtrTemp += PAGE_SIZE;
	}

	return (TRUE);
}


//------------------------------------------------------------------------------
// NVMe read issue command
// We use the original function NVMeIssueCmd without ringing the doorbell, to
// get a higher data transfer rate.
//------------------------------------------------------------------------------
static ULONG NVMeReadIssueCmd(PNVME_DEVICE_EXTENSION pAE,USHORT QueueID,PVOID pTempSubEntry)
{
	PQUEUE_INFO pQI = &pAE->QueueInfo;
	PSUB_QUEUE_INFO pSQI = NULL;
	PNVMe_COMMAND pNVMeCmd = NULL;
	USHORT tempSqTail = 0;

	// make sure the parameters are valid
	if(QueueID > pQI->NumSubIoQCreated || pTempSubEntry == NULL)
	{
		return STOR_STATUS_INVALID_PARAMETER;
	}

	// locate the current submission entry and copy the fields from the temp buffer
	pSQI = pQI->pSubQueueInfo + QueueID;

	// first make sure FW is draining this SQ
	tempSqTail = ((pSQI->SubQTailPtr + 1) == pSQI->SubQEntries) ? 0 : pSQI->SubQTailPtr + 1;
	if(tempSqTail == pSQI->SubQHeadPtr)
	{
		return STOR_STATUS_INSUFFICIENT_RESOURCES;
	}

	pNVMeCmd = (PNVMe_COMMAND)pSQI->pSubQStart;
	pNVMeCmd += pSQI->SubQTailPtr;

	StorPortCopyMemory((PVOID)pNVMeCmd,pTempSubEntry,sizeof(NVMe_COMMAND));

	// increase the tail pointer by 1 and reset it if needed
	pSQI->SubQTailPtr = tempSqTail;

	// track number of outstanding requests for this SQ
	pSQI->Requests++;

	return STOR_STATUS_SUCCESS;
}


//------------------------------------------------------------------------------
// NVMe read ring doorbell and wait for completion
//------------------------------------------------------------------------------
static ULONG NVMeReadRingDoorbellAndWaitForCompletion(PNVME_DEVICE_EXTENSION pAE,USHORT QueueID,ULONG numRequests)
{
	PQUEUE_INFO pQI;
	PSUB_QUEUE_INFO pSQI = NULL;
	ULONG entryStatus = STOR_STATUS_UNSUCCESSFUL;
	PNVMe_COMPLETION_QUEUE_ENTRY pCplEntry = NULL;
	PNVME_SRB_EXTENSION pSrbExtension = NULL;
	PCPL_QUEUE_INFO pCQI;

	pQI = &pAE->QueueInfo;
	pSQI = pQI->pSubQueueInfo + QueueID;

	// now issue the command via Doorbell register
	StorPortWriteRegisterUlong(pAE,pSQI->pSubTDBL,(ULONG)pSQI->SubQTailPtr);

	pCQI = pQI->pCplQueueInfo + QueueID;

	// do this for all outstanding requests
	while(numRequests--)
	{
		do
		{
			// get completion entry
			entryStatus = NVMeGetCplEntry(pAE,pCQI,&pCplEntry);
			if(entryStatus == STOR_STATUS_SUCCESS)
			{
				// complete command
				NVMeCompleteCmd(pAE,pCplEntry->DW2.SQID,pCplEntry->DW2.SQHD,pCplEntry->DW3.CID,(PVOID)&pSrbExtension);

				if(pSrbExtension != NULL)
				{
					pSrbExtension->pCplEntry = pCplEntry;
				}

				// if a completion routine was specified call it
				if(pSrbExtension->pNvmeCompletionRoutine != NULL)
				{
					pSrbExtension->pNvmeCompletionRoutine(pAE,(PVOID)pSrbExtension);
				}

				StorPortWriteRegisterUlong(pAE,pCQI->pCplHDBL,(ULONG)pCQI->CplQHeadPtr);
			}
		}while(entryStatus != STOR_STATUS_SUCCESS);
	}

	return STOR_STATUS_SUCCESS;
}


//------------------------------------------------------------------------------
// NVMe read process I/O
//------------------------------------------------------------------------------
static BOOLEAN NVMeReadProcessIo(PNVME_DEVICE_EXTENSION pAdapterExtension,PNVME_SRB_EXTENSION pSrbExtension)
{
	PNVMe_COMMAND pNvmeCmd;
	ULONG StorStatus;
	PCMD_INFO pCmdInfo = NULL;
	USHORT SubQueue = 1;

	StorStatus = NVMeGetCmdEntry(pAdapterExtension,SubQueue,(PVOID)pSrbExtension,&pCmdInfo);
	if(StorStatus != STOR_STATUS_SUCCESS)
	{
		return FALSE;
	}

	pNvmeCmd = &pSrbExtension->nvmeSqeUnit;
	pNvmeCmd->CDW0.CID = (USHORT)pCmdInfo->CmdID;

	// 3 - If a PRP list is used, copy the buildIO prepared list to the
	// preallocated memory location and update the entry not the pCmdInfo is a
	// stack var but contains a to the pre allocated mem which is what we're
	// updating.
	if(pSrbExtension->numberOfPrpEntries > 2)
	{
		pNvmeCmd->PRP2 = pCmdInfo->prpListPhyAddr.QuadPart;

		// Copy the PRP list pointed to by PRP2. Size of the copy is total num
		// of PRPs -1 because PRP1 is not in the PRP list pointed to by PRP2.
		StorPortCopyMemory((PVOID)pCmdInfo->pPRPList,(PVOID)&pSrbExtension->prpList[0],((pSrbExtension->numberOfPrpEntries - 1) * sizeof(UINT64)));
	}

	StorStatus = NVMeReadIssueCmd(pAdapterExtension,SubQueue,pNvmeCmd);
	if(StorStatus != STOR_STATUS_SUCCESS)
	{
		NVMeCompleteCmd(pAdapterExtension,SubQueue,NO_SQ_HEAD_CHANGE,pNvmeCmd->CDW0.CID,(PVOID)pSrbExtension);
		return FALSE;
	}

	return TRUE;
}


//------------------------------------------------------------------------------
// NVMe read initialization
//------------------------------------------------------------------------------
static int NVMeReadInit(PNVME_DEVICE_EXTENSION pAE)
{
	PNVME_LUN_EXTENSION pLunExt = NULL;
	UINT8 flbas;
	UINT32 lbaLengthPower;
	unsigned int i;

	pLunExt = pAE->pLunExtensionTable[0];
	if(pLunExt == NULL)
	{
		return 1;
	}

	// get LBA size from identify data, normally this is 512 bytes per sector
	flbas = pLunExt->identifyData.FLBAS.SupportedCombination;
	lbaLengthPower = pLunExt->identifyData.LBAFx[flbas].LBADS;
	pAE->lbaLength = 1 << lbaLengthPower;

	// we use a data buffer size of 128 KB
	// if the specified LBA length does not work and we have a hang at ringing the doorbell
	// the problem is our max transfer size set in the NVMeFindAdapter function
	pAE->length = 256;
	pAE->DataBufferSize = pAE->length * pAE->lbaLength;

	// do this for the number of requests we send at once (32 requests)
	for(i = 0; i < NUM_REQUESTS; i++)
	{
		// allocate SRB extension
		pAE->pNVMeSrbExt[i] = (PNVME_SRB_EXTENSION)NVMeAllocateMem(pAE,sizeof(NVME_SRB_EXTENSION),0);
		if(pAE->pNVMeSrbExt[i] == NULL)
		{
			return 2;
		}
		
		// set NVMe device extension
		pAE->pNVMeSrbExt[i]->pNvmeDevExt = pAE;

		// fill the NVMe command structure
		pAE->pCmd[i] = (PNVMe_COMMAND)(&pAE->pNVMeSrbExt[i]->nvmeSqeUnit);
		pAE->pCmd[i]->CDW0.OPC = NVME_READ;
		pAE->pCmd[i]->NSID = pLunExt->namespaceId;
	}

	return 0;
}


//------------------------------------------------------------------------------
// NVMe read uninitialization
//------------------------------------------------------------------------------
static int NVMeReadUninit(PNVME_DEVICE_EXTENSION pAE)
{
	unsigned int i;

	// do this for all requests
	for(i = 0; i < NUM_REQUESTS; i++)
	{
		if(pAE->pNVMeSrbExt[i] != NULL)
		{
			// free NVMe SRB extension structure
			aligned_free(pAE->pNVMeSrbExt[i]);
			pAE->pNVMeSrbExt[i] = NULL;
		}
	}

	return 0;
}


//------------------------------------------------------------------------------
// NVMe read
// With 1 outstanding I/O request we reach about 2 GB per second for sequential read.
// With 32 outstanding I/O requests we reach about 3,2 GB per second for sequential read.
//------------------------------------------------------------------------------
int NVMeRead(PNVME_DEVICE_EXTENSION pAE,QWORD startLBA,QWORD buf,ULONG count)
{
	ULONG numRequestBlocks;
	ULONG numRemainingSectors;
	ULONG numRemainingRequests;
	ULONG numRemainingCount;
	QWORD nextStartLBA;
	unsigned int r;
	unsigned int i;

	// calculate number of request blocks which are a multiple of 8192 (256 LBAs * 32 requests)
	numRequestBlocks = count / (pAE->length * NUM_REQUESTS);
	// calculate number of remaining sectors
	numRemainingSectors = count % (pAE->length * NUM_REQUESTS);
	// calculate number of remaining requests
	numRemainingRequests = numRemainingSectors / pAE->length;
	// calculate number of remaining LBAs
	numRemainingCount = numRemainingSectors % pAE->length;
	// set next start LBA
	nextStartLBA = startLBA;

	// do this for the request blocks which are a multiple of 32
	for(r = 0; r < numRequestBlocks * NUM_REQUESTS; r += NUM_REQUESTS)
	{
		// do this for 32 requests
		for(i = 0; i < NUM_REQUESTS; i++)
		{
			// set data buffer offset to the pre allocated buffer
			pAE->pNVMeSrbExt[i]->pDataBuffer64 = buf + r * pAE->DataBufferSize + i * pAE->DataBufferSize;
			
			// prepare the PRP list
			if(NVMeReadPreparePRPs(pAE,pAE->pNVMeSrbExt[i],pAE->pNVMeSrbExt[i]->pDataBuffer64,pAE->DataBufferSize) == FALSE)
			{
				return 1;
			}

			// fill NVMe command structure
			pAE->pCmd[i]->CDW10 = (ULONG)nextStartLBA;
			pAE->pCmd[i]->CDW11 = (ULONG)(nextStartLBA >> 32);
			pAE->pCmd[i]->CDW12 = pAE->length - 1; // 0's based
			nextStartLBA += pAE->length;

			// NVMe read process I/O
			if(NVMeReadProcessIo(pAE,pAE->pNVMeSrbExt[i]) == FALSE)
			{
				return 2;
			}
		}
		
		// NVMe read ring doorbell and wait for completion
		NVMeReadRingDoorbellAndWaitForCompletion(pAE,1,NUM_REQUESTS);
	}

	// remaining requests read
	if(numRemainingRequests > 0)
	{
		// do this for the remaining requests
		for(i = 0; i < numRemainingRequests; i++)
		{
			// set data buffer offset to the pre allocated buffer
			pAE->pNVMeSrbExt[i]->pDataBuffer64 = buf + r * pAE->DataBufferSize + i * pAE->DataBufferSize;
			
			// prepare the PRP list
			if(NVMeReadPreparePRPs(pAE,pAE->pNVMeSrbExt[i],pAE->pNVMeSrbExt[i]->pDataBuffer64,pAE->DataBufferSize) == FALSE)
			{
				return 3;
			}

			// fill NVMe command structure
			pAE->pCmd[i]->CDW10 = (ULONG)nextStartLBA;
			pAE->pCmd[i]->CDW11 = (ULONG)(nextStartLBA >> 32);
			pAE->pCmd[i]->CDW12 = pAE->length - 1; // 0's based
			nextStartLBA += pAE->length;

			// NVMe read process I/O
			if(NVMeReadProcessIo(pAE,pAE->pNVMeSrbExt[i]) == FALSE)
			{
				return 4;
			}
		}
		
		// NVMe read ring doorbell and wait for completion
		NVMeReadRingDoorbellAndWaitForCompletion(pAE,1,numRemainingRequests);

		// increment number of requests
		r += numRemainingRequests;
	}

	// read the remaining sectors
	if(numRemainingCount > 0)
	{
		// set data buffer offset to the pre allocated buffer
		pAE->pNVMeSrbExt[0]->pDataBuffer64 = buf + r * pAE->DataBufferSize;
		
		// prepare the PRP list
		if(NVMeReadPreparePRPs(pAE,pAE->pNVMeSrbExt[0],pAE->pNVMeSrbExt[0]->pDataBuffer64,numRemainingCount * pAE->lbaLength) == FALSE)
		{
			return 5;
		}

		// fill NVMe command structure
		pAE->pCmd[0]->CDW10 = (ULONG)nextStartLBA;
		pAE->pCmd[0]->CDW11 = (ULONG)(nextStartLBA >> 32);
		pAE->pCmd[0]->CDW12 = numRemainingCount - 1; // 0's based
		nextStartLBA += numRemainingCount;

		// NVMe read process I/O
		if(NVMeReadProcessIo(pAE,pAE->pNVMeSrbExt[0]) == FALSE)
		{
			return 6;
		}

		// NVMe read ring doorbell and wait for completion
		NVMeReadRingDoorbellAndWaitForCompletion(pAE,1,1);
	}

	return 0;
}


//------------------------------------------------------------------------------
// NVMe initialization
//------------------------------------------------------------------------------
int NVMeInit(NVME_DEVICE_EXTENSION **DevExt,unsigned short int *foundControllers)
{
	unsigned short int i;

	// explicit calibration by using zero delay
	// this will cause to calibrate the TSC (Time Stamp Counter) against the PIT
	// (Programmable Interval Timer)
	if(delay(0) != 0)
	{
		printf("NVMeInit Error: Can't calibrate TSC against PIT!\n");
		return 1;
	}

	// find all NVMe controllers on the system
	if(NVMeFindBaseAddress(DevExt,foundControllers) != 0)
	{
		printf("NVMeInit Error: Can't find NVMe base address!\n");
		return 2;       
	}

	// do this for all found controllers
	for(i = 0; i < *foundControllers; i++)
	{
		// find adapter
		if(NVMeFindAdapter(&(*DevExt)[i]) != SP_RETURN_FOUND)
		{
			printf("NVMeInit Error: Can't find NVMe adapter!\n");
			return 3;
		}
		
		// initialize adapter
		if(NVMeInitialize(&(*DevExt)[i]) != TRUE)
		{
			printf("NVMeInit Error: Can't initialize NVMe adapter!\n");
			return 4;
		}

		// NVMe read initialization
		if(NVMeReadInit(&(*DevExt)[i]) != 0)
		{
			printf("NVMeInit Error: Can't initialize NVMe read!\n");
			return 5;
		}
	}

	return 0;
}


//------------------------------------------------------------------------------
// NVMe uninitialization
//------------------------------------------------------------------------------
void NVMeUninit(NVME_DEVICE_EXTENSION **DevExt,unsigned short int *foundControllers)
{
	unsigned short int i;

	// do this for all controllers
	for(i = 0; i < *foundControllers; i++)
	{
		// shutdown the NVMe controller
		NVMeNormalShutdown(&(*DevExt)[i]);
		
		// NVMe read uninitialization
		NVMeReadUninit(&(*DevExt)[i]);
	}

	// free NVME_DEVICE_EXTENSION structures
	if(*DevExt != NULL)
	{
		free(*DevExt);
		*DevExt = NULL;
	}

	// set found controllers to zero
	*foundControllers = 0;
}


//------------------------------------------------------------------------------
// NVMe raw data read
//------------------------------------------------------------------------------
int NVMeRawRead(unsigned long drive,unsigned long long sector,unsigned long byte_offset,unsigned long long byte_len,unsigned long long buf,unsigned long write)
{
	unsigned char *buffer;
	ULONG count;

	// NVME_DEVICE_EXTENSION structure is not present
	// no valid NVME controller is selected
	// read blocks only from the selected BIOS drive
	if(nvmeg.DevExt == NULL || nvmeg.SelectedController == -1 || nvmeg.SelectedDrive != drive)
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
	if(nvmeg.MapActive == 1)
	{
		sector += nvmeg.StartSector;
	}

	// byte_len is a multiple of sector size and byte_offset is zero
	if(byte_len % 512 == 0 && byte_offset == 0)
	{
		// NVMe read
		if(NVMeRead(&nvmeg.DevExt[nvmeg.SelectedController],sector,buf,(ULONG)(byte_len >> 9)) != 0)
		{
			printf("NVMeRawRead Error: Can't read sectors!\n");
			return 3;
		}
	}
	else
	{
		// calculate number of sectors to read
		count = (byte_len / 512) + 1;
		// allocate memory for sector read
		buffer = (unsigned char*)aligned_malloc(count * 512,4096);

		// NVMe read
		if(NVMeRead(&nvmeg.DevExt[nvmeg.SelectedController],sector,(unsigned long long)(unsigned int)(unsigned char*)buffer,count) != 0)
		{
			// free memory
			aligned_free(buffer);
			printf("NVMeRawRead Error: Can't read sectors!\n");
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
// show all found controllers
//------------------------------------------------------------------------------
void NVMeShowAllDevices(NVME_DEVICE_EXTENSION *DevExt,unsigned short int foundControllers)
{
	unsigned short int i;

	// do this for all found controllers
	for(i = 0; i < foundControllers; i++)
	{
		// get model number with spaces
		memcpy(DevExt[i].Model,DevExt[i].controllerIdentifyData.MN,40);
		DevExt[i].Model[40] = '\0';
		// delete spaces from model string
		RemoveLeadingAndTrailingSpaces(DevExt[i].Model);

		// get firmware revision with spaces
		memcpy(DevExt[i].FirmwareRevision,DevExt[i].controllerIdentifyData.FR,8);
		DevExt[i].FirmwareRevision[8] = '\0';
		// delete spaces from firmware revision
		RemoveLeadingAndTrailingSpaces(DevExt[i].FirmwareRevision);

		// get serial number with spaces
		memcpy(DevExt[i].Serial,DevExt[i].controllerIdentifyData.SN,20);
		DevExt[i].Serial[20] = '\0';
		// delete spaces from serial number
		RemoveLeadingAndTrailingSpaces(DevExt[i].Serial);

		printf("%2.1d) NVMe Controller VendorID#%.4X, DeviceID#%.4X, Base Address#%.8X\n"
			   "    Bus#%.2X, Device#%.2X, Function#%.2X\n"
			   ,i,DevExt[i].PciVendorID,DevExt[i].PciDeviceID,DevExt[i].pCtrlRegister
			   ,DevExt[i].Bus,DevExt[i].Dev,DevExt[i].Func);

		// show connected device
		printf("    %s %s %s\n",DevExt[i].Model,DevExt[i].Serial,DevExt[i].FirmwareRevision);
	}

	printf("\n");
}


//------------------------------------------------------------------------------
// show selected controller and connected device
//------------------------------------------------------------------------------
void NVMeShowSelectedDevice(NVME_DEVICE_EXTENSION *DevExt,unsigned short int i)
{
	// get model number with spaces
	memcpy(DevExt[i].Model,DevExt[i].controllerIdentifyData.MN,40);
	DevExt[i].Model[40] = '\0';
	// delete spaces from model string
	RemoveLeadingAndTrailingSpaces(DevExt[i].Model);

	// get firmware revision with spaces
	memcpy(DevExt[i].FirmwareRevision,DevExt[i].controllerIdentifyData.FR,8);
	DevExt[i].FirmwareRevision[8] = '\0';
	// delete spaces from firmware revision
	RemoveLeadingAndTrailingSpaces(DevExt[i].FirmwareRevision);

	// get serial number with spaces
	memcpy(DevExt[i].Serial,DevExt[i].controllerIdentifyData.SN,20);
	DevExt[i].Serial[20] = '\0';
	// delete spaces from serial number
	RemoveLeadingAndTrailingSpaces(DevExt[i].Serial);

	printf("%2.1d) NVMe Controller VendorID#%.4X, DeviceID#%.4X, Base Address#%.8X\n"
		   "    Bus#%.2X, Device#%.2X, Function#%.2X\n"
		   ,i,DevExt[i].PciVendorID,DevExt[i].PciDeviceID,DevExt[i].pCtrlRegister
		   ,DevExt[i].Bus,DevExt[i].Dev,DevExt[i].Func);

	// show connected device
	printf("    %s %s %s\n",DevExt[i].Model,DevExt[i].Serial,DevExt[i].FirmwareRevision);

	printf("\n");
}


//------------------------------------------------------------------------------
// our custom stuff ends here
//------------------------------------------------------------------------------

// restore GCC options
#pragma GCC pop_options

