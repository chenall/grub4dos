
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

#pragma once

#ifndef _INC_NVME
#define _INC_NVME

// the following lines enables debugging
//#define NVME_DEBUG

// C:\WinDDK\7600.16385.1\inc\api\sdkddkver.h
#define NTDDI_WINXP                         0x05010000
#define NTDDI_WIN7                          0x06010000

// custom
#define NTDDI_VERSION NTDDI_WINXP
#define DBG 0

// custom
#define ULONG unsigned long int
#define WORD unsigned short int
#define UCHAR unsigned char
#define VOID void
#define USHORT unsigned short int
#define ULONG unsigned long int
#define ULONGLONG unsigned long long int
#define CHAR char
#define SHORT short int
#define QWORD unsigned long long int
#define PQWORD unsigned long long int*
#define __int64 long long

// C:\WinDDK\7600.16385.1\inc\api\intsafe.h
//typedef char                CHAR;
typedef signed char         INT8;
//typedef unsigned char       UCHAR;
typedef unsigned char       UINT8;
//typedef unsigned char       BYTE;
//typedef short               SHORT;
typedef signed short        INT16;
//typedef unsigned short      USHORT;
typedef unsigned short      UINT16;
//typedef unsigned short      WORD;
typedef int                 INT;
typedef signed int          INT32;
typedef unsigned int        UINT;
typedef unsigned int        UINT32;
typedef long                LONG;
//typedef unsigned long       ULONG;
//typedef unsigned long       DWORD;
typedef __int64             LONGLONG;
//typedef __int64             LONG64;
//typedef signed __int64      INT64;
//typedef unsigned __int64    ULONGLONG;
//typedef unsigned __int64    DWORDLONG;
//typedef unsigned __int64    ULONG64;
//typedef unsigned __int64    DWORD64;
//typedef unsigned __int64    UINT64;

// C:\WinDDK\7600.16385.1\inc\api\sspi.h
//typedef unsigned __int64 QWORD;
// C:\WinDDK\7600.16385.1\inc\api\ntdef.h
typedef UCHAR *PUCHAR;
// C:\WinDDK\7600.16385.1\inc\api\ntdef.h
//typedef void VOID;
// C:\WinDDK\7600.16385.1\inc\api\ntdef.h
typedef void *PVOID;
// C:\WinDDK\7600.16385.1\inc\api\WINDEF.H
//typedef int                 BOOL;
// C:\WinDDK\7600.16385.1\inc\api\WINDEF.H
#define FALSE               0
// C:\WinDDK\7600.16385.1\inc\api\WINDEF.H
#define TRUE                1
// C:\WinDDK\7600.16385.1\inc\api\ntdef.h
typedef ULONG *PULONG;
// C:\WinDDK\7600.16385.1\inc\api\ntdef.h
typedef UCHAR BOOLEAN;
// C:\WinDDK\7600.16385.1\inc\api\basetsd.h
typedef __int64 LONG64,*PLONG64;
// C:\WinDDK\7600.16385.1\inc\api\basetsd.h
typedef unsigned __int64 ULONG64,*PULONG64;
// C:\WinDDK\7600.16385.1\inc\api\basetsd.h
typedef unsigned __int64 UINT64,*PUINT64;
// C:\WinDDK\7600.16385.1\inc\api\intsafe.h
typedef unsigned long  ULONG_PTR;
// C:\WinDDK\7600.16385.1\inc\api\ntdef.h
typedef short CSHORT;
// C:\WinDDK\7600.16385.1\inc\api\basetsd.h
//typedef unsigned int        UINT32, *PUINT32;
// C:\WinDDK\7600.16385.1\inc\api\ntdef.h
typedef CHAR *PCHAR, *LPCH, *PCH;
// C:\WinDDK\7600.16385.1\inc\api\basetsd.h
typedef long LONG_PTR, *PLONG_PTR;
// C:\WinDDK\7600.16385.1\inc\api\basetsd.h
typedef unsigned int ULONG32, *PULONG32;
// C:\WinDDK\7600.16385.1\inc\api\ntdef.h
typedef USHORT *PUSHORT;

// C:\WinDDK\7600.16385.1\inc\api\ntdef.h
// Doubly linked list structure. Can be used as either a list head, or as link words.
typedef struct _LIST_ENTRY
{
	struct _LIST_ENTRY *Flink;
	struct _LIST_ENTRY *Blink;
}LIST_ENTRY,*PLIST_ENTRY;

// C:\WinDDK\7600.16385.1\inc\api\ntdef.h
typedef union _LARGE_INTEGER {
    struct {
        ULONG LowPart;
        LONG HighPart;
    };
    LONGLONG QuadPart;
} LARGE_INTEGER;

// C:\WinDDK\7600.16385.1\inc\api\ntdef.h
typedef LARGE_INTEGER PHYSICAL_ADDRESS,*PPHYSICAL_ADDRESS;
// C:\WinDDK\7600.16385.1\inc\api\basetsd.h
typedef ULONG_PTR KAFFINITY;

// C:\WinDDK\7600.16385.1\inc\api\ntdef.h
typedef struct _GROUP_AFFINITY
{
	KAFFINITY Mask;
	WORD Group;
	WORD Reserved[3];
}GROUP_AFFINITY,*PGROUP_AFFINITY;

// C:\WinDDK\7600.16385.1\inc\ddk\storport.h
typedef struct _DPC_BUFFER {
    CSHORT Type;
    UCHAR Number;
    UCHAR Importance;
    struct {
        PVOID F;
        PVOID B;
    };
    PVOID DeferredRoutine;
    PVOID DeferredContext;
    PVOID SystemArgument1;
    PVOID SystemArgument2;
    PVOID DpcData;
} DPC_BUFFER;

// C:\WinDDK\7600.16385.1\inc\ddk\storport.h
typedef struct _STOR_DPC {
    DPC_BUFFER Dpc;
    ULONG_PTR Lock;
} STOR_DPC, *PSTOR_DPC;

// C:\WinDDK\7600.16385.1\inc\ddk\storport.h
typedef
VOID
(*PHW_DPC_ROUTINE)(
    PSTOR_DPC Dpc,
    PVOID HwDeviceExtension,
    PVOID SystemArgument1,
    PVOID SystemArgument2
    );

// C:\WinDDK\7600.16385.1\inc\ddk\storport.h
// SCSI I/O Request Block
typedef struct _SCSI_REQUEST_BLOCK {
    USHORT Length;                  // offset 0
    UCHAR Function;                 // offset 2
    UCHAR SrbStatus;                // offset 3
    UCHAR ScsiStatus;               // offset 4
    UCHAR PathId;                   // offset 5
    UCHAR TargetId;                 // offset 6
    UCHAR Lun;                      // offset 7
    UCHAR QueueTag;                 // offset 8
    UCHAR QueueAction;              // offset 9
    UCHAR CdbLength;                // offset a
    UCHAR SenseInfoBufferLength;    // offset b
    ULONG SrbFlags;                 // offset c
    ULONG DataTransferLength;       // offset 10
    ULONG TimeOutValue;             // offset 14
    PVOID DataBuffer;               // offset 18
    PVOID SenseInfoBuffer;          // offset 1c
    struct _SCSI_REQUEST_BLOCK *NextSrb; // offset 20
    PVOID OriginalRequest;          // offset 24
    PVOID SrbExtension;             // offset 28
    union {
        ULONG InternalStatus;       // offset 2c
        ULONG QueueSortKey;         // offset 2c
        ULONG LinkTimeoutValue;     // offset 2c
    };

#if defined(_WIN64)

    //
    // Force PVOID alignment of Cdb
    //

    ULONG Reserved;

#endif

    UCHAR Cdb[16];                  // offset 30
} SCSI_REQUEST_BLOCK, *PSCSI_REQUEST_BLOCK;

// C:\WinDDK\7600.16385.1\inc\ddk\storport.h
#define STOR_STATUS_SUCCESS                     (0x00000000L)
#define STOR_STATUS_UNSUCCESSFUL                (0xC1000001L)
#define STOR_STATUS_NOT_IMPLEMENTED             (0xC1000002L)
#define STOR_STATUS_INSUFFICIENT_RESOURCES      (0xC1000003L)
#define STOR_STATUS_BUFFER_TOO_SMALL            (0xC1000004L)
#define STOR_STATUS_ACCESS_DENIED               (0xC1000005L)
#define STOR_STATUS_INVALID_PARAMETER           (0xC1000006L)
#define STOR_STATUS_INVALID_DEVICE_REQUEST      (0xC1000007L)
#define STOR_STATUS_INVALID_IRQL                (0xC1000008L)
#define STOR_STATUS_INVALID_DEVICE_STATE        (0xC1000009L)
#define STOR_STATUS_INVALID_BUFFER_SIZE         (0xC100000AL)
#define STOR_STATUS_UNSUPPORTED_VERSION         (0xC100000BL)

// C:\WinDDK\7600.16385.1\inc\ddk\storport.h
typedef PHYSICAL_ADDRESS STOR_PHYSICAL_ADDRESS, *PSTOR_PHYSICAL_ADDRESS;

// C:\WinDDK\7600.16385.1\inc\api\ntdef.h
typedef struct _PROCESSOR_NUMBER {
  USHORT  Group;
  UCHAR  Number;
  UCHAR  Reserved;
} PROCESSOR_NUMBER, *PPROCESSOR_NUMBER;

// C:\WinDDK\7600.16385.1\inc\api\ntdef.h
// Calculate the address of the base of the structure given its type, and an
// address of a field within the structure.
#define CONTAINING_RECORD(address, type, field) ((type *)( \
                                                  (PCHAR)(address) - \
                                                  (ULONG_PTR)(&((type *)0)->field)))

// C:\WinDDK\7600.16385.1\inc\ddk\storport.h
#define SRB_STATUS_SUCCESS                  0x01
#define SRB_STATUS_ABORTED                  0x02
#define SRB_STATUS_ERROR                    0x04

// C:\WinDDK\7600.16385.1\inc\ddk\storport.h
// SCSI bus status codes.
#define SCSISTAT_GOOD                  0x00
#define SCSISTAT_CHECK_CONDITION       0x02
#define SCSISTAT_CONDITION_MET         0x04
#define SCSISTAT_BUSY                  0x08
#define SCSISTAT_INTERMEDIATE          0x10
#define SCSISTAT_INTERMEDIATE_COND_MET 0x14
#define SCSISTAT_RESERVATION_CONFLICT  0x18
#define SCSISTAT_COMMAND_TERMINATED    0x22
#define SCSISTAT_QUEUE_FULL            0x28

// C:\WinDDK\7600.16385.1\inc\ddk\storport.h
// Sense codes
#define SCSI_SENSE_NO_SENSE         0x00
#define SCSI_SENSE_RECOVERED_ERROR  0x01
#define SCSI_SENSE_NOT_READY        0x02
#define SCSI_SENSE_MEDIUM_ERROR     0x03
#define SCSI_SENSE_HARDWARE_ERROR   0x04
#define SCSI_SENSE_ILLEGAL_REQUEST  0x05
#define SCSI_SENSE_UNIT_ATTENTION   0x06
#define SCSI_SENSE_DATA_PROTECT     0x07
#define SCSI_SENSE_BLANK_CHECK      0x08
#define SCSI_SENSE_UNIQUE           0x09
#define SCSI_SENSE_COPY_ABORTED     0x0A
#define SCSI_SENSE_ABORTED_COMMAND  0x0B
#define SCSI_SENSE_EQUAL            0x0C
#define SCSI_SENSE_VOL_OVERFLOW     0x0D
#define SCSI_SENSE_MISCOMPARE       0x0E
#define SCSI_SENSE_RESERVED         0x0F

// C:\WinDDK\7600.16385.1\inc\ddk\storport.h
// Additional Sense codes
#define SCSI_ADSENSE_NO_SENSE                              0x00
#define SCSI_ADSENSE_NO_SEEK_COMPLETE                      0x02
#define SCSI_ADSENSE_LUN_NOT_READY                         0x04
#define SCSI_ADSENSE_LUN_COMMUNICATION                     0x08
#define SCSI_ADSENSE_WRITE_ERROR                           0x0C
#define SCSI_ADSENSE_TRACK_ERROR                           0x14
#define SCSI_ADSENSE_SEEK_ERROR                            0x15
#define SCSI_ADSENSE_REC_DATA_NOECC                        0x17
#define SCSI_ADSENSE_REC_DATA_ECC                          0x18
#define SCSI_ADSENSE_PARAMETER_LIST_LENGTH                 0x1A
#define SCSI_ADSENSE_ILLEGAL_COMMAND                       0x20
#define SCSI_ADSENSE_ILLEGAL_BLOCK                         0x21
#define SCSI_ADSENSE_INVALID_CDB                           0x24
#define SCSI_ADSENSE_INVALID_LUN                           0x25
#define SCSI_ADSENSE_INVALID_FIELD_PARAMETER_LIST          0x26
#define SCSI_ADSENSE_WRITE_PROTECT                         0x27
#define SCSI_ADSENSE_MEDIUM_CHANGED                        0x28
#define SCSI_ADSENSE_BUS_RESET                             0x29
#define SCSI_ADSENSE_PARAMETERS_CHANGED                    0x2A
#define SCSI_ADSENSE_INSUFFICIENT_TIME_FOR_OPERATION       0x2E
#define SCSI_ADSENSE_INVALID_MEDIA                         0x30
#define SCSI_ADSENSE_NO_MEDIA_IN_DEVICE                    0x3a
#define SCSI_ADSENSE_POSITION_ERROR                        0x3b
#define SCSI_ADSENSE_OPERATING_CONDITIONS_CHANGED          0x3f
#define SCSI_ADSENSE_OPERATOR_REQUEST                      0x5a // see below
#define SCSI_ADSENSE_FAILURE_PREDICTION_THRESHOLD_EXCEEDED 0x5d
#define SCSI_ADSENSE_ILLEGAL_MODE_FOR_THIS_TRACK           0x64
#define SCSI_ADSENSE_COPY_PROTECTION_FAILURE               0x6f
#define SCSI_ADSENSE_POWER_CALIBRATION_ERROR               0x73
#define SCSI_ADSENSE_VENDOR_UNIQUE                         0x80 // and higher
#define SCSI_ADSENSE_MUSIC_AREA                            0xA0
#define SCSI_ADSENSE_DATA_AREA                             0xA1
#define SCSI_ADSENSE_VOLUME_OVERFLOW                       0xA7

// C:\WinDDK\7600.16385.1\inc\ddk\wdm.h
typedef enum _KINTERRUPT_MODE {
    LevelSensitive,
    Latched
} KINTERRUPT_MODE;

// C:\WinDDK\7600.16385.1\inc\ddk\storport.h
typedef struct _MESSAGE_INTERRUPT_INFORMATION {
    ULONG MessageId;
    ULONG MessageData;
    STOR_PHYSICAL_ADDRESS MessageAddress;
    ULONG InterruptVector;
    ULONG InterruptLevel;
    KINTERRUPT_MODE InterruptMode;
} MESSAGE_INTERRUPT_INFORMATION, *PMESSAGE_INTERRUPT_INFORMATION;

// C:\WinDDK\7600.16385.1\inc\api\ntdef.h
typedef UCHAR KIRQL;

// C:\WinDDK\7600.16385.1\inc\ddk\storport.h
typedef enum _STOR_SPINLOCK {
    DpcLock = 1,
    StartIoLock,
    InterruptLock
} STOR_SPINLOCK;

// C:\WinDDK\7600.16385.1\inc\ddk\storport.h
typedef struct _STOR_LOCK_HANDLE {
    STOR_SPINLOCK Lock;
    struct {
        struct {
            PVOID Next;
            PVOID Lock;
        } LockQueue;
        KIRQL OldIrql;
    } Context;
} STOR_LOCK_HANDLE, *PSTOR_LOCK_HANDLE;

// C:\WinDDK\7600.16385.1\inc\ddk\storport.h
// Notification Event Types
typedef enum _SCSI_NOTIFICATION_TYPE {
    RequestComplete,
    NextRequest,
    NextLuRequest,
    ResetDetected,
    _obsolete1,             // STORPORT: CallDisableInterrupts has been removed
    _obsolete2,             // STORPORT: CallEnableInterrupts has been removed
    RequestTimerCall,
    BusChangeDetected,
    WMIEvent,
    WMIReregister,
    LinkUp,
    LinkDown,
    QueryTickCount,
    BufferOverrunDetected,
    TraceNotification,
    GetExtendedFunctionTable,

    EnablePassiveInitialization = 0x1000,
    InitializeDpc,
    IssueDpc,
    AcquireSpinLock,
    ReleaseSpinLock

} SCSI_NOTIFICATION_TYPE, *PSCSI_NOTIFICATION_TYPE;

// C:\WinDDK\7600.16385.1\inc\ddk\ntintsafe.h
typedef ULONG_PTR SIZE_T;

// C:\WinDDK\7600.16385.1\inc\ddk\wdm.h
typedef enum _MEMORY_CACHING_TYPE {
    MmNonCached = FALSE,
    MmCached = TRUE,
    MmWriteCombined = 2,
    MmHardwareCoherentCached,
    MmNonCachedUnordered,       // IA64
    MmUSWCCached,
    MmMaximumCacheType
} MEMORY_CACHING_TYPE;

// C:\WinDDK\7600.16385.1\inc\ddk\wdm.h
typedef ULONG NODE_REQUIREMENT;

// C:\WinDDK\7600.16385.1\inc\ddk\wdm.h
#define MM_ANY_NODE_OK 0x80000000

#define StorPortCopyMemory memcpy

// C:\WinDDK\7600.16385.1\inc\ddk\wdm.h
#define RtlZeroMemory(Destination,Length) memset((Destination),0,(Length))

#define ALL_POLLING

// C:\WinDDK\7600.16385.1\inc\ddk\wdm.h
// Define the I/O bus interface types.
typedef enum _INTERFACE_TYPE {
    InterfaceTypeUndefined = -1,
    Internal,
    Isa,
    Eisa,
    MicroChannel,
    TurboChannel,
    PCIBus,
    VMEBus,
    NuBus,
    PCMCIABus,
    CBus,
    MPIBus,
    MPSABus,
    ProcessorInternal,
    InternalPowerBus,
    PNPISABus,
    PNPBus,
    Vmcs,
    MaximumInterfaceType
}INTERFACE_TYPE, *PINTERFACE_TYPE;

// C:\WinDDK\7600.16385.1\inc\ddk\wdm.h
// Define the DMA transfer widths.
typedef enum _DMA_WIDTH {
    Width8Bits,
    Width16Bits,
    Width32Bits,
    MaximumDmaWidth
}DMA_WIDTH, *PDMA_WIDTH;

// C:\WinDDK\7600.16385.1\inc\ddk\wdm.h
// Define DMA transfer speeds.
typedef enum _DMA_SPEED {
    Compatible,
    TypeA,
    TypeB,
    TypeC,
    TypeF,
    MaximumDmaSpeed
}DMA_SPEED, *PDMA_SPEED;

// C:\WinDDK\7600.16385.1\inc\ddk\storport.h
typedef struct _ACCESS_RANGE {
    STOR_PHYSICAL_ADDRESS RangeStart;
    ULONG RangeLength;
    BOOLEAN RangeInMemory;
} ACCESS_RANGE, *PACCESS_RANGE;

// C:\WinDDK\7600.16385.1\inc\ddk\miniport.h
typedef char CCHAR;

// C:\WinDDK\7600.16385.1\inc\ddk\storport.h
typedef enum _STOR_SYNCHRONIZATION_MODEL {
    StorSynchronizeHalfDuplex,
    StorSynchronizeFullDuplex
} STOR_SYNCHRONIZATION_MODEL;

// C:\WinDDK\7600.16385.1\inc\ddk\storport.h
typedef
BOOLEAN
(*PHW_MESSAGE_SIGNALED_INTERRUPT_ROUTINE) (
    PVOID HwDeviceExtension,
    ULONG MessageId
    );

// C:\WinDDK\7600.16385.1\inc\ddk\storport.h
typedef enum _INTERRUPT_SYNCHRONIZATION_MODE {
    InterruptSupportNone,
    InterruptSynchronizeAll,
    InterruptSynchronizePerMessage
} INTERRUPT_SYNCHRONIZATION_MODE;

// C:\WinDDK\7600.16385.1\inc\ddk\storport.h
// _MEMORY_REGION represents a region of physical contiguous memory.
// Generally, this is used for DMA common buffer regions.
typedef struct _MEMORY_REGION {

    //
    // Beginning virtual address of the region.
    //

    PUCHAR VirtualBase;

    //
    // Beginning physical address of the region.
    //

    PHYSICAL_ADDRESS PhysicalBase;

    //
    // Length of the region
    //
    //

    ULONG Length;

} MEMORY_REGION, *PMEMORY_REGION;

// C:\WinDDK\7600.16385.1\inc\ddk\storport.h
typedef struct _PORT_CONFIGURATION_INFORMATION {

    //
    // Length of port configuation information strucuture.
    //

    ULONG Length;

    //
    // IO bus number (0 for machines that have only 1 IO bus
    //

    ULONG SystemIoBusNumber;

    //
    // EISA, MCA or ISA
    //

    INTERFACE_TYPE  AdapterInterfaceType;

    //
    // Interrupt request level for device
    //

    ULONG BusInterruptLevel;

    //
    // Bus interrupt vector used with hardware buses which use as vector as
    // well as level, such as internal buses.
    //

    ULONG BusInterruptVector;

    //
    // Interrupt mode (level-sensitive or edge-triggered)
    //

    KINTERRUPT_MODE InterruptMode;

    //
    // Maximum number of bytes that can be transferred in a single SRB
    //

    ULONG MaximumTransferLength;

    //
    // Number of contiguous blocks of physical memory
    //

    ULONG NumberOfPhysicalBreaks;

    //
    // DMA channel for devices using system DMA
    //

    ULONG DmaChannel;
    ULONG DmaPort;
    DMA_WIDTH DmaWidth;
    DMA_SPEED DmaSpeed;

    //
    // Alignment masked required by the adapter for data transfers.
    //

    ULONG AlignmentMask;

    //
    // Number of access range elements which have been allocated.
    //

    ULONG NumberOfAccessRanges;

    //
    // Pointer to array of access range elements.
    //

    ACCESS_RANGE (*AccessRanges)[];

    //
    // Reserved field.
    //

    PVOID Reserved;

    //
    // Number of SCSI buses attached to the adapter.
    //

    UCHAR NumberOfBuses;

    //
    // SCSI bus ID for adapter
    //

    CCHAR InitiatorBusId[8];

    //
    // Indicates that the adapter does scatter/gather
    //

    BOOLEAN ScatterGather;

    //
    // Indicates that the adapter is a bus master
    //

    BOOLEAN Master;

    //
    // Host caches data or state.
    //

    BOOLEAN CachesData;

    //
    // Host adapter scans down for bios devices.
    //

    BOOLEAN AdapterScansDown;

    //
    // Primary at disk address (0x1F0) claimed.
    //

    BOOLEAN AtdiskPrimaryClaimed;

    //
    // Secondary at disk address (0x170) claimed.
    //

    BOOLEAN AtdiskSecondaryClaimed;

    //
    // The master uses 32-bit DMA addresses.
    //

    BOOLEAN Dma32BitAddresses;

    //
    // Use Demand Mode DMA rather than Single Request.
    //

    BOOLEAN DemandMode;

    //
    // Data buffers must be mapped into virtual address space.
    //

    UCHAR MapBuffers;

    //
    // The driver will need to tranlate virtual to physical addresses.
    //

    BOOLEAN NeedPhysicalAddresses;

    //
    // Supports tagged queuing
    //

    BOOLEAN TaggedQueuing;

    //
    // Supports auto request sense.
    //

    BOOLEAN AutoRequestSense;

    //
    // Supports multiple requests per logical unit.
    //

    BOOLEAN MultipleRequestPerLu;

    //
    // Support receive event function.
    //

    BOOLEAN ReceiveEvent;

    //
    // Indicates the real-mode driver has initialized the card.
    //

    BOOLEAN RealModeInitialized;

    //
    // Indicate that the miniport will not touch the data buffers directly.
    //

    BOOLEAN BufferAccessScsiPortControlled;

    //
    // Indicator for wide scsi.
    //

    UCHAR   MaximumNumberOfTargets;

    //
    // Ensure quadword alignment.
    //

    UCHAR   ReservedUchars[2];

    //
    // Adapter slot number
    //

    ULONG SlotNumber;

    //
    // Interrupt information for a second IRQ.
    //

    ULONG BusInterruptLevel2;
    ULONG BusInterruptVector2;
    KINTERRUPT_MODE InterruptMode2;

    //
    // DMA information for a second channel.
    //

    ULONG DmaChannel2;
    ULONG DmaPort2;
    DMA_WIDTH DmaWidth2;
    DMA_SPEED DmaSpeed2;

    //
    // Fields added to allow for the miniport
    // to update these sizes based on requirements
    // for large transfers ( > 64K);
    //

    ULONG DeviceExtensionSize;
    ULONG SpecificLuExtensionSize;
    ULONG SrbExtensionSize;

    //
    // Used to determine whether the system and/or the miniport support
    // 64-bit physical addresses.  See SCSI_DMA64_* flags below.
    //

    UCHAR  Dma64BitAddresses;        /* New */

    //
    // Indicates that the miniport can accept a SRB_FUNCTION_RESET_DEVICE
    // to clear all requests to a particular LUN.
    //

    BOOLEAN ResetTargetSupported;       /* New */

    //
    // Indicates that the miniport can support more than 8 logical units per
    // target (maximum LUN number is one less than this field).
    //

    UCHAR MaximumNumberOfLogicalUnits;  /* New */

    //
    // Supports WMI?
    //

    BOOLEAN WmiDataProvider;

    //
    // STORPORT synchronization model, either half or full duplex
    // depending on whether the driver supports async-with-interrupt
    // model or not.
    //

    STOR_SYNCHRONIZATION_MODEL SynchronizationModel;    // STORPORT New

    PHW_MESSAGE_SIGNALED_INTERRUPT_ROUTINE HwMSInterruptRoutine;

    INTERRUPT_SYNCHRONIZATION_MODE InterruptSynchronizationMode;

    MEMORY_REGION DumpRegion;

    ULONG         RequestedDumpBufferSize;

    BOOLEAN       VirtualDevice;

    ULONG         ExtendedFlags1;

    ULONG         MaxNumberOfIO;


} PORT_CONFIGURATION_INFORMATION, *PPORT_CONFIGURATION_INFORMATION;

// C:\WinDDK\7600.16385.1\inc\ddk\storport.h
typedef enum {
    StorPowerActionNone = 0,
    StorPowerActionReserved,
    StorPowerActionSleep,
    StorPowerActionHibernate,
    StorPowerActionShutdown,
    StorPowerActionShutdownReset,
    StorPowerActionShutdownOff,
    StorPowerActionWarmEject
} STOR_POWER_ACTION, *PSTOR_POWER_ACTION;

// C:\WinDDK\7600.16385.1\inc\ddk\storport.h
#define SRB_STATUS_BUSY                     0x05

// C:\WinDDK\7600.16385.1\inc\ddk\storport.h
typedef struct _PERF_CONFIGURATION_DATA {
    ULONG Version;
    ULONG Size;
    ULONG Flags;
    ULONG ConcurrentChannels;
    ULONG FirstRedirectionMessageNumber, LastRedirectionMessageNumber;
    ULONG DeviceNode;
    ULONG Reserved;
    PGROUP_AFFINITY MessageTargets;
} PERF_CONFIGURATION_DATA, *PPERF_CONFIGURATION_DATA;

// C:\WinDDK\7600.16385.1\inc\ddk\storport.h
#define STOR_PERF_DPC_REDIRECTION 0x00000001

// C:\WinDDK\7600.16385.1\inc\ddk\storport.h
typedef 
BOOLEAN
(*PHW_PASSIVE_INITIALIZE_ROUTINE)(
    PVOID DeviceExtension
    );

// C:\WinDDK\7600.16385.1\inc\ddk\storport.h
#define SRB_STATUS_BUS_RESET                0x0E

// C:\WinDDK\7600.16385.1\inc\ddk\storport.h
#define SP_RETURN_NOT_FOUND     0
#define SP_RETURN_FOUND         1

// C:\WinDDK\7600.16385.1\inc\api\basetsd.h
typedef KAFFINITY *PKAFFINITY;

// C:\WinDDK\7600.16385.1\inc\ddk\miniport.h
#define min(a,b) (((a) < (b)) ? (a) : (b))
// C:\WinDDK\7600.16385.1\inc\ddk\miniport.h
#define ASSERT( exp )
// C:\WinDDK\7600.16385.1\inc\ddk\storport.h
typedef struct _SENSE_DATA {
    UCHAR ErrorCode:7;
    UCHAR Valid:1;
    UCHAR SegmentNumber;
    UCHAR SenseKey:4;
    UCHAR Reserved:1;
    UCHAR IncorrectLength:1;
    UCHAR EndOfMedia:1;
    UCHAR FileMark:1;
    UCHAR Information[4];
    UCHAR AdditionalSenseLength;
    UCHAR CommandSpecificInformation[4];
    UCHAR AdditionalSenseCode;
    UCHAR AdditionalSenseCodeQualifier;
    UCHAR FieldReplaceableUnitCode;
    UCHAR SenseKeySpecific[3];
} SENSE_DATA, *PSENSE_DATA;
// C:\WinDDK\7600.16385.1\inc\ddk\wdm.h
// Calculate the byte offset of a field in a structure of type type.
#define FIELD_OFFSET(type, field)    ((LONG)(LONG_PTR)&(((type *)0)->field))
// C:\WinDDK\7600.16385.1\inc\ddk\storport.h
#define SRB_STATUS_AUTOSENSE_VALID          0x80
// C:\WinDDK\7600.16385.1\inc\ddk\storport.h
#define SRB_FUNCTION_ABORT_COMMAND          0x10

// custom
// a page has 4.096 bytes
#define PAGE_SIZE 0x1000
// custom
// if NVME_DEBUG is defined
#ifdef NVME_DEBUG
	#define StorPortDebugPrint(DebugPrintLevel,...) printf(__VA_ARGS__)
	#define DbgPrint(...) printf(__VA_ARGS__)
#else
	#define StorPortDebugPrint(DebugPrintLevel,...) ;
	#define DbgPrint(...) ;
#endif
// custom
#define __try ;
// custom
#define __leave goto cleanup
// custom
#define __finally cleanup:
// custom
#define __in
// custom
#define IN
// custom
#define NUM_REQUESTS 32


//------------------------------------------------------------------------------
// our custom stuff ends here
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// original NVMe OFA driver code starts here
//------------------------------------------------------------------------------


// nvmeStd.h
#define STORPORT_TIMER_CB_us        5000 /* .005 seconds */
// nvmeStd.h
#define MAX_STATE_STALL_us          STORPORT_TIMER_CB_us
// nvmeSntiTypes.h
#define SCSISTAT_TASK_ABORTED                      0x40
// nvmeStd.h
#define IO_StorPortNotification StorPortNotification
// nvmeStd.h
#define MIN_IO_QUEUE_ENTRIES        2
// nvmeStd.h
#define IDEN_CONTROLLER 0
// nvmeStd.h
#define STOR_ALL_REQUESTS (0xFFFFFFFF)
// nvmeStd.h
#define DUMP_BUFFER_SIZE (5 * 64 * 1024)
// nvmeStd.h
/* Align buffer pointer to next system page boundary */
#define PAGE_ALIGN_BUF_PTR(pBuf)                           \
    ((((ULONG_PTR)((PUCHAR)pBuf)) & (PAGE_SIZE-1)) == 0) ? \
    (PVOID)(pBuf) :                                        \
    (PVOID)((((ULONG_PTR)((PUCHAR)pBuf)) + PAGE_SIZE) & ~(PAGE_SIZE-1))

// nvmeReg.h
/* Various NVME register fields */
#define NVME_AQA_CQS_LSB            16
#define NVME_MEM_PAGE_SIZE_SHIFT    13 /* When MPS is 0, means 4KB */
#define NVME_CC_NVM_CMD (0)
#define NVME_CC_SHUTDOWN_NONE (0)
#define NVME_CC_ROUND_ROBIN (0)
#define NVME_CC_IOSQES (6)
#define NVME_CC_IOCQES (4)

// nvmeSntiTypes.h
#define FIXED_SENSE_DATA                           0x70
#define SCSI_ADSENSE_PERIPHERAL_DEV_WRITE_FAULT    0x03
#define SCSI_ADSENSE_LOG_BLOCK_GUARD_CHECK_FAILED  0x10
#define SCSI_ADSENSE_LOG_BLOCK_APPTAG_CHECK_FAILED 0x10
#define SCSI_ADSENSE_LOG_BLOCK_REFTAG_CHECK_FAILED 0x10
#define SCSI_ADSENSE_UNRECOVERED_READ_ERROR        0x11
#define SCSI_ADSENSE_MISCOMPARE_DURING_VERIFY      0x1D
#define SCSI_ADSENSE_ACCESS_DENIED_INVALID_LUN_ID  0x20
#define SCSI_ADSENSE_INTERNAL_TARGET_FAILURE       0x44
#define SCSI_ADSENSE_FORMAT_COMMAND_FAILED         0x31
#define SCSI_SENSEQ_INVALID_LUN_ID                 0x09
#define SCSI_SENSEQ_FORMAT_COMMAND_FAILED          0x01
#define SCSI_SENSEQ_LOG_BLOCK_GUARD_CHECK_FAILED   0x01
#define SCSI_SENSEQ_LOG_BLOCK_APPTAG_CHECK_FAILED  0x02
#define SCSI_SENSEQ_LOG_BLOCK_REFTAG_CHECK_FAILED  0x03
#define SCSI_SENSEQ_ACCESS_DENIED_INVALID_LUN_ID   0x09
#define NVM_CMD_SET_STATUS                         0x80
#define NVM_CMD_SET_GENERIC_STATUS_OFFSET          0x74
#define NVM_CMD_SET_SPECIFIC_STATUS_OFFSET         0x75
#define NVM_MEDIA_ERROR_STATUS_OFFSET              0x80
#define NVME_READ                                  0x02

// nvmeReg.h
/* Table 3.1.1 */
typedef union _NVMe_CONTROLLER_CAPABILITIES
{
    struct
    {
        /* 
         * [Maximum Queue Entries Supported] This field indicates the maximum 
         * individual queue size that the controller supports. This value 
         * applies to each of the I/O Submission Queues and I/O Completion 
         * Queues that software may create. This is a 0抯 based value. The 
         * minimum value is 1h, indicating two entries.
         */
        USHORT MQES; 

        /* 
         * [Contiguous Queues Required] This field is set to ?1? if the 
         * controller requires that I/O Submission and I/O Completion Queues are 
         * required to be physically contiguous. This field is cleared to ?0? 
         * if the controller supports I/O Submission and I/O Completion Queues 
         * that are not physically contiguous. If this field is set to ?1?, then 
         * the Physically Contiguous bit (CDW11.PC) in the Create I/O Submission 
         * Queue and Create I/O Completion Queue commands shall be set to ?1?.
         */
        UCHAR  CQR       :1; 

        /* 
         * [Arbitration Mechanism Supported] This field is bit significant and 
         * indicates the optional arbitration mechanisms supported by the 
         * controller. If a bit is set to ?1?, then the corresponding 
         * arbitration mechanism is supported by the controller. The round robin 
         * arbitration mechanism is not listed since all controllers shall 
         * support this arbitration mechanism. Refer to section 4.7 for 
         * arbitration details.  Bit 17 == Weighted Round Robin + Urgent. Bit 
         * 18 == Vendor Specific.
         */
        UCHAR  AMS       :2; 

        /* Bits 19-23 */
        UCHAR  Reserved1 :5;

        /* 
         * [Timeout] This is the worst case time that host software shall wait 
         * for the controller to become ready (CSTS.RDY set to ?1?) after a 
         * power-on or reset event (CC.EN is set to ?1? by software). This worst 
         * case time may be experienced after an unclean shutdown; typical times 
         * are expected to be much shorter. This field is in 500 millisecond 
         * units.
         */
        UCHAR TO;

        /* Bits 32-35 */
        USHORT DSTRD :4;

        /* Bit 36 */
        USHORT Reserved2 :1;

        /* 
         * [Command Sets Supported] This field indicates the command set(s) that 
         * the controller supports. A minimum of one command set shall be 
         * supported. The field is bit significant. If a bit is set to ?1?, then 
         * the corresponding command set is supported. If a bit is cleared to 
         * ?0?, then the corresponding command set is not supported. Bit 37 == 
         * NVM Command Set. Bits 38-44 Reserved.
         */
        USHORT CSS :8; // NVMe1.0E 

        /* Bits 45-47 */
        USHORT Reserved3 :3; // NVMe1.0E

        /* 
         * [Memory Page Size Minimum] This field indicates the minimum host 
         * memory page size that the controller supports. The minimum memory 
         * page size is (2 ^ (12 + MPSMIN)). The host shall not configure a 
         * memory page size in CC.MPS that is smaller than this value.
         */
        UCHAR  MPSMIN    :4; 

        /* 
         * [Memory Page Size Maximum] This field indicates the maximum host 
         * memory page size that the controller supports. The maximum memory 
         * page size is (2 ^ (12 + MPSMAX)). The host shall not configure a 
         * memory page size in CC.MPS that is larger than this value.
         */
        UCHAR  MPSMAX    :4; 
        UCHAR   Reserved4;
    };

    struct
    {
        ULONG LowPart;
        ULONG HighPart;
    };

    ULONGLONG AsUlonglong;
}  NVMe_CONTROLLER_CAPABILITIES, *PNVMe_CONTROLLER_CAPABILITIES;

// nvmeReg.h
/* Table 3.1.2 */
typedef struct _NVMe_VERSION
{
    /* [Minor Version Number] Indicates the minor version is ?0?. */
    USHORT MNR;

    /* [Major Version Number] Indicates the major version is ?1?. */
    USHORT MJR;
} NVMe_VERSION, *PNVMe_VERSION;

// nvmeReg.h
/* Table 3.1.5 */
typedef union _NVMe_CONTROLLER_CONFIGURATION
{
    struct
    {
        /* 
         * [Enable] When set to ?1?, then the controller shall process commands 
         * based on Submission Queue Tail doorbell writes. When cleared to ?0?, 
         * then the controller shall not process commands nor submit completion 
         * entries to Completion Queues. When this field transitions from ?1? to 
         * ?0?, the controller is reset (referred to as a Controller Reset). The 
         * reset deletes all I/O Submission Queues and I/O Completion Queues 
         * created, resets the Admin Submission and Completion Queues, and 
         * brings the hardware to an idle state. The reset does not affect PCI 
         * Express registers nor the Admin Queue registers (AQA, ASQ, or ACQ). 
         * All other controller registers defined in this section are reset. The 
         * controller shall ensure that there is no data loss for commands that 
         * have been completed to the host as part of the reset operation. Refer 
         * to section 7.3 for reset details. When this field is cleared to ?0?, 
         * the CSTS.RDY bit is cleared to ?0? by the controller. When this field 
         * is set to ?1?, the controller sets CSTS.RDY to ?1? when it is ready 
         * to process commands. The Admin Queue registers (AQA, ASQ, and ACQ) 
         * shall only be modified when EN is cleared to ?0?.
         */
        ULONG EN         :1; 

        /* Bits 1-3 */
        ULONG Reserved1  :3;

        /* 
         * [Command Set Selected] This field specifies the command set that is 
         * selected for use for the I/O Submission Queues. Software shall only 
         * select a supported command set, as indicated in CAP.CSS. The command 
         * set shall only be changed when the controller is disabled (CC.EN is 
         * cleared to ?0?). The command set selected shall be used for all I/O 
         * Submission Queues. Value 000b == NVM command set.  Values 001b-111b 
         * Reserved.
         */
        ULONG CSS        :3; 

        /* 
         * [Memory Page Size] This field indicates the host memory page size. 
         * The memory page size is (2 ^ (12 + MPS)). Thus, the minimum host 
         * memory page size is 4KB and the maximum host memory page size is 
         * 128MB. The value set by host software shall be a supported value as 
         * indicated by the CAP.MPSMAX and CAP.MPSMIN fields. This field 
         * describes the value used for PRP entry size.
         */
        ULONG MPS        :4; 

        /* 
         * [Arbitration Mechanism Selected] This field selects the arbitration 
         * mechanism to be used. This value shall only be changed when EN is 
         * cleared to ?0?. Software shall only set this field to supported 
         * arbitration mechanisms indicated in CAP.AMS. Value 000b == Round 
         * Robin. Value 001b == Weighted Round Robin + Urgent.  010b-110b == 
         * Reserved. 111b == Vendor Specific.
         */
        ULONG AMS        :3; 

        /* 
         * [Shutdown Notification] This field is used to initiate shutdown 
         * processing when a shutdown is occurring, i.e., a power down condition 
         * is expected. For a normal shutdown notification, it is expected that 
         * the controller is given time to process the shutdown notification. 
         * For an abrupt shutdown notification, the host may not wait for 
         * shutdown processing to complete before power is lost. The shutdown 
         * notification values are defined as: Value 00b == No notification; no 
         * effect. Value 01b == Normal shutdown notification. Value 10b == 
         * Abrupt shutdown notification.  Value 11b == Reserved. Shutdown 
         * notification should be issued by host software prior to any power 
         * down condition and prior to any change of the PCI power management 
         * state. It is recommended that shutdown notification also be sent 
         * prior to a warm reboot. To determine when shutdown processing is 
         * complete, refer to CSTS.SHST. Refer to section 7.6.2 for additional 
         * shutdown processing details.
         */
        ULONG SHN        :2; 

        /* 
         * [I/O Submission Queue Entry Size] This field defines the I/O 
         * Submission Queue entry size that is used for the selected I/O Command 
         * Set. The required and maximum values for this field are specified in 
         * the Identify Controller data structure for each I/O Command Set. The 
         * value is in bytes and is specified as a power of two (2^n).
         */
        ULONG IOSQES     :4; 

        /* 
         * [I/O Completion Queue Entry Size] This field defines the I/O 
         * Completion Queue entry size that is used for the selected I/O Command 
         * Set. The required and maximum values for this field are specified in 
         * the Identify Controller data structure for each I/O Command Set. The 
         * value is in bytes and is specified as a power of two (2^n).
         */
        ULONG IOCQES     :4; 

        /* Bits 24-31 */
        ULONG Reserved2  :8;
    };

    ULONG AsUlong;
} NVMe_CONTROLLER_CONFIGURATION, *PNVMe_CONTROLLER_CONFIGURATION;

// nvmeReg.h
/* Table 3.1.6 */
typedef union _NVMe_CONTROLLER_STATUS
{
    struct
    {
        /* 
         * [Ready] This field is set to ?1? when the controller is ready to 
         * process commands after CC.EN is set to ?1?. This field shall be 
         * cleared to ?0? when CC.EN is cleared to ?0?. Commands shall not be 
         * issued to the controller until this field is set to ?1? after the 
         * CC.EN bit is set to ?1?. Failure to follow this requirement produces 
         * undefined results. Software shall wait a minimum of CAP.TO seconds 
         * for this field to be set to ?1? after CC.EN transitions from ?0? to 
         * ?1?.
         */
        ULONG RDY        :1; 

        /* 
         * [Controller Fatal Status] Indicates that a fatal controller error 
         * occurred that could not be communicated in the appropriate Completion 
         * Queue. Refer to section 9.5.
         */
        ULONG CFS        :1; 

        /* 
         * [Shutdown Status] This field indicates the status of shutdown 
         * processing that is initiated by the host setting the CC.SHN field 
         * appropriately. The shutdown status values are defined as: Value 00b 
         * == Normal operation (no shutdown has been requested). Value 01b == 
         * Shutdown processing occurring. Value 10b == Shutdown processing 
         * complete. Value 11b == Reserved.
         */
        ULONG SHST       :2; 

        /* Bits 4-31 */
        ULONG Reserved   :28;
    };

    ULONG AsUlong;
} NVMe_CONTROLLER_STATUS, *PNVMe_CONTROLLER_STATUS;

// nvmeReg.h
/* Table 3.1.7 */
typedef union _NVMe_ADMIN_QUEUE_ATTRIBUTES
{
    struct
    {
        /* 
         * [Admin Submission Queue Size] Defines the size of the Admin 
         * Submission Queue in entries. Refer to section 4.1.3. The minimum size 
         * of the Admin Submission Queue is two entries. The maximum size of the 
         * Admin Submission Queue is 4096 entries. This is a 0抯 based value.
         */
        ULONG ASQS       :12; 

        /* Bits 12-15 */
        ULONG Reserved1  :4;

        /* 
         * [Admin Completion Queue Size] Defines the size of the Admin 
         * Completion Queue in entries. Refer to section 4.1.3. The minimum size 
         * of the Admin Completion Queue is two entries. The maximum size of the 
         * Admin Completion Queue is 4096 entries. This is a 0抯 based value.
         */
        ULONG ACQS       :12; 

        /* Bits 28-31 */
        ULONG Reserved2  :4;
    };

    ULONG AsUlong;
} NVMe_ADMIN_QUEUE_ATTRIBUTES, *PNVMe_ADMIN_QUEUE_ATTRIBUTES;

// nvmeReg.h
/* Table 3.1.8 */
typedef union _NVMe_SUBMISSION_QUEUE_BASE
{
    struct
    {
        /* Bits 0-11 */
        ULONGLONG Reserved   :12;

        /* 
         * [Admin Submission Queue Base] Indicates the 64-bit physical address 
         * for the Admin Submission Queue. This address shall be memory page 
         * aligned (based on the value in CC.MPS). All Admin commands, including 
         * creation of additional Submission Queues and Completions Queues shall 
         * be submitted to this queue. For the definition of Submission Queues, 
         * refer to section 4.1.
         */
		//ULONGLONG ASQB:52;
		// in Open Watcom we get the following error message on the above bit field:
		// implementation restriction: cannot use __int64 as bit-field base type
		// Therefore we have to use two ULONG bit fields like follows.
		ULONGLONG ASQBL:20;
		ULONGLONG ASQBH:32; 
    };

    ULONGLONG AsUlonglong;

    struct
    {
        ULONG LowPart;
        ULONG HighPart;
    };
} NVMe_SUBMISSION_QUEUE_BASE, *PNVMe_SUBMISSION_QUEUE_BASE;

// nvmeReg.h
/* Table 3.1.9 */
typedef union _NVMe_COMPLETION_QUEUE_BASE
{
    struct
    {
        /* Bits 0-11 */
        ULONGLONG Reserved   :12;

        /* 
         * [Admin Completion Queue Base] Indicates the 64-bit physical address 
         * for the Admin Completion Queue. This address shall be memory page 
         * aligned (based on the value in CC.MPS). All completion entries for 
         * the commands submitted to the Admin Submission Queue shall be posted 
         * to this Completion Queue. This queue is always associated with 
         * interrupt vector 0. For the definition of Completion Queues, refer to 
         * section 4.1.
         */
		//ULONGLONG ACQB:52;
		// in Open Watcom we get the following error message on the above bit field:
		// implementation restriction: cannot use __int64 as bit-field base type
		// Therefore we have to use two ULONG bit fields like follows.
		ULONGLONG ACQBL:20;
		ULONGLONG ACQBH:32; 
    };

    ULONGLONG AsUlonglong;

    struct
    {
        ULONG LowPart;
        ULONG HighPart;
    };
} NVMe_COMPLETION_QUEUE_BASE, *PNVMe_COMPLETION_QUEUE_BASE;

// nvmeReg.h
/* Table 3.1.11 */
typedef union _NVMe_QUEUE_Y_DOORBELL
{
    struct
    {
        USHORT  QHT;

        /* Bits 16-31 */
        USHORT  Reserved;
    };

    ULONG AsUlong;
} NVMe_QUEUE_Y_DOORBELL,
  *PNVMe_QUEUE_Y_DOORBELL;

// nvmeReg.h
/* Table 3.1 */
typedef struct _NVMe_CONTROLLER_REGISTERS
{
    NVMe_CONTROLLER_CAPABILITIES  CAP;
    NVMe_VERSION                  VS;

    /* 
     * [Interrupt Vector Mask Set] This field is bit significant. If a ?1? is
     * written to a bit, then the corresponding interrupt vector is masked. 
     * Writing a ?0? to a bit has no effect. When read, this field returns the 
     * current interrupt mask value. If a bit has a value of a ?1?, then the 
     * corresponding interrupt vector is masked. If a bit has a value of ?0?, 
     * then the corresponding interrupt vector is not masked.
     */
    ULONG                         IVMS; 

    /* 
     * [Interrupt Vector Mask Clear] This field is bit significant. If a ?1? is 
     * written to a bit, then the corresponding interrupt vector is unmasked.
     * Writing a ?0? to a bit has no effect. When read, this field returns the
     * current interrupt mask value. If a bit has a value of a ?1?, then the 
     * corresponding interrupt vector is masked, If a bit has a value of ?0?, 
     * then the corresponding interrupt vector is not masked.
     */
    ULONG                         INTMC; 

    NVMe_CONTROLLER_CONFIGURATION CC;
    ULONG                         Reserved1;
    NVMe_CONTROLLER_STATUS        CSTS;
    ULONG                         Reserved2;
    NVMe_ADMIN_QUEUE_ATTRIBUTES   AQA;
    NVMe_SUBMISSION_QUEUE_BASE    ASQ;
    NVMe_COMPLETION_QUEUE_BASE    ACQ;

    /* Bytes 0x38 - 0xEFF */
    ULONG                         Reserved3[0x3B2];

    /* Bytes 0xF00 - 0xFFF */
    ULONG                         CommandSetSpecific[0x40];

    /* variable sized array limited by the BAR size */
    NVMe_QUEUE_Y_DOORBELL           IODB[1];
} NVMe_CONTROLLER_REGISTERS, *PNVMe_CONTROLLER_REGISTERS;

// nvmeStd.h
/* Debugging Printing Levels */
enum
{
    INFO = 0,
    WARNING,
    ERROR,
    TRACE
};

// nvmeStd.h
/* Enabled Interrupt Type */
typedef enum _INT_TYPE
{
    INT_TYPE_NONE = 0,
    INT_TYPE_INTX = 1,
    INT_TYPE_MSI = 2,
    INT_TYPE_MSIX = 4
} INT_TYPE;

// nvmeStd.h
/* Queue Type */
typedef enum _NVME_QUEUE_TYPE
{
    NVME_QUEUE_TYPE_ADMIN = 0,
    NVME_QUEUE_TYPE_IO = 1
} NVME_QUEUE_TYPE;

#define NO_SQ_HEAD_CHANGE (-1)

// nvmeStd.h
/* Used when decoding the LBA Range tpye after get features on LBA type */
typedef enum _NS_VISBILITY
{
    VISIBLE = 0,
    HIDDEN,
    IGNORED
} NS_VISBILITY;

// nvmeStd.h
/*
 * Adapter Error Status; can occur during init state machine
 * or otherwise as the init state machine mechanism is used
 * to effectively halt the controller at any time by changing
 * the state to shutdown and flagging the error with this enum
 *
 * Errors in the state machine start with START_STATE_ otherweise
 * they should start with FATAL_
 *
 * Each bit is used for error situation indicators
 */
enum
{
    START_STATE_RDY_FAILURE = 0,
    START_STATE_INT_COALESCING_FAILURE,
    START_STATE_QUEUE_ALLOC_FAILURE,
    START_STATE_QUEUE_INIT_FAILURE,
    START_STATE_SET_FEATURE_FAILURE,
    START_STATE_LBA_RANGE_CHK_FAILURE,
    START_STATE_IDENTIFY_CTRL_FAILURE,
    START_STATE_IDENTIFY_NS_FAILURE,
    START_STATE_SUBQ_CREATE_FAILURE,
    START_STATE_CPLQ_CREATE_FAILURE,
    FATAL_SUBQ_DELETE_FAILURE,
    FATAL_CPLQ_DELETE_FAILURE,
    START_STATE_LEARNING_FAILURE,
    START_STATE_AER_FAILURE,
    FATAL_POLLED_ADMIN_CMD_FAILURE,
    START_STATE_UNKNOWN_STATE_FAILURE,
    START_MAX_XFER_MISMATCH_FAILURE,
    START_STATE_TIMEOUT_FAILURE = 31
};

// nvmeStd.h
/* Start State Machine states */
enum
{
    NVMeWaitOnRDY = 0x20,
    NVMeWaitOnIdentifyCtrl,
    NVMeWaitOnIdentifyNS,
    NVMeWaitOnSetFeatures,
    NVMeWaitOnSetupQueues,
    NVMeWaitOnAER,
    NVMeWaitOnIoCQ,
    NVMeWaitOnIoSQ,
    NVMeWaitOnLearnMapping,
    NVMeWaitOnReSetupQueues,
    NVMeStartComplete = 0x88,
    NVMeShutdown,
    NVMeStateFailed = 0xFF
};

// nvmeStd.h
/*******************************************************************************
 * Format NVM command specific structure.
 ******************************************************************************/
typedef struct _FORMAT_NVM_INFO
{
    /* Current state after receiving Format NVM request */
    ULONG               State;

    /* LunId corresponding to the visible namespace that is being formatted */
    ULONG               TargetLun;

    /* NSID of the next NS to fetch identify structure for */
    ULONG               NextNs;

    /* LunId corresponding to NextNs */
    ULONG               NextLun;

    /* Indicates if we're formatting one namespace or all namespaces */
    BOOLEAN             FormatAllNamespaces;

    /*
     * Flag to indicate calling StorPortNotification with BusChangeDetected
     * is required to add back the formatted namespace(s)
     */
    BOOLEAN             AddNamespaceNeeded;
} FORMAT_NVM_INFO, *PFORMAT_NVM_INFO;

// nvmeStd.h
/*******************************************************************************
 * Start State Machine structure.
 ******************************************************************************/
typedef struct _START_STATE
{
    /* Starting status, including error status */
    ULONG64 DriverErrorStatus;

    /* Interval in us for Storport to check back */
    ULONG CheckbackInterval;

    /* Next state to proceed */
    UCHAR NextDriverState;

    /* Overall state machine timeout counter */
    UINT32 TimeoutCounter;

    /* State Polling counter */
    ULONG StateChkCount;

    /* Dedicated SRB extension for command issues */
    PVOID pSrbExt;

    /* Used for data transfer during start state */
    PVOID pDataBuffer;

    /* If this is because of a reset */
    BOOLEAN resetDriven;

    /* If its a host driven reset we need the SRB */
#if (NTDDI_VERSION > NTDDI_WIN7)
    PSTORAGE_REQUEST_BLOCK pResetSrb;
#else
    PSCSI_REQUEST_BLOCK pResetSrb;
#endif

    /*
     * After adapter had completed the Identify commands,
     * the callback function is invoked to examine the completion results.
     * If no error:
     * For Namespace structures, IdentifyNamespaceFetched is increased by 1.
     * START_STATE_IDENTIFY_CTRL/NS_FAILURE bit of DriverErrorStatus is set to 1
     * in the case of errors
     */
    ULONG IdentifyNamespaceFetched;

    /* Indicates the Interrupt Coalescing configured when TRUE */
    BOOLEAN InterruptCoalescingSet;

    /*
     * Indicates Set Featurs commands is required to configure the current
     * Namespace when it's LBA Range Type is 00b and NLB matches the size of
     * the Namespace.
     */
    BOOLEAN ConfigLbaRangeNeeded;

    /*
     * For each existing Namespace, driver needs to exam its LBA Range
     * information to determine:
     *
     *   1. if it's right type to support?
     *   2. if it's a ready-only drive when supported?
     *   3. if it needs to be reported to host when supported?
     */
    ULONG TtlLbaRangeExamined;

    /* Number of Asynchronous Event Requests commands issued */
    UCHAR NumAERsIssued;

    /* Number of exposed namespaces found at init */
    ULONG VisibleNamespacesExamined;

    /* The NSID of the current device for the init state */
    ULONG CurrentNsid;

} START_STATE, *PSTART_STATE;

// nvmeStd.h
/*******************************************************************************
 * Registry Initial Information data structure.
 ******************************************************************************/
typedef struct _INIT_INFO
{
    /* Supported number of namespaces */
    ULONG Namespaces;

    /* Max transfer size via one cmd entry, 128 KB by default */
    ULONG MaxTxSize;

    /* Supported number of entries for Admin queue */
    ULONG AdQEntries;

    /* Supported number of entries for IO queues */
    ULONG IoQEntries;

    /* Aggregation time in 100 us, 0 means disabled */
    ULONG IntCoalescingTime;

    /* Aggregation entries per interrupt vector */
    ULONG IntCoalescingEntry;

} INIT_INFO, *PINIT_INFO;

// nvmeStd.h
/*******************************************************************************
 * Command Entry/Information data structure.
 ******************************************************************************/
typedef struct _CMD_INFO
{
    /* Command ID for the used Submission Queue entry */
    ULONG CmdID;

    /* Dedicated PRPList(in virtual address) of the cmd entry */
    PVOID pPRPList;

    STOR_PHYSICAL_ADDRESS prpListPhyAddr;

#ifdef DUMB_DRIVER
    /* this cmd's dbl buffer physical address */
    STOR_PHYSICAL_ADDRESS dblPhy;

    /* this cmd's dbl buffer virtual address */
    PVOID pDblVir;

    STOR_PHYSICAL_ADDRESS dblPrpListPhy; // this commands PRP list
    PVOID pDblPrpListVir; // this commands PRP list
#endif
} CMD_INFO, *PCMD_INFO;

// nvmeStd.h
typedef struct _CMD_ENTRY
{
    /* The doubly-linked list entry */
    LIST_ENTRY ListEntry;

    /* TRUE means it抯 been acquired, FALSE means a free entry */
    BOOLEAN Pending;

    /*
     * Any context callers wish to save,
     *
     * e.g., the original SRB associated with the request
     */
    PVOID Context;

    /*
     * Command Information structure returned to callers when a command ID is
     * successfully acquired
     */
    CMD_INFO CmdInfo;
} CMD_ENTRY, *PCMD_ENTRY;

// nvmeStd.h
/*******************************************************************************
 * Submission Queue Information data structure.
 ******************************************************************************/
typedef struct _SUB_QUEUE_INFO
{
    /* Allocated buffers via StorPortPatchAllocateContiguousMemorySpecifyCacheNode */

    /* Starting virtual addr of the allocated buffer for queue entries */
    PVOID pQueueAlloc;

    /* Byte size of the allocated buffer for queue entries */
    ULONG QueueAllocSize;

    /* Starting virtual addr of the allocated buffer for PRP Lists */
    PVOID pPRPListAlloc;

    /* Byte size of the allocated buffer for PRP Lists */
    ULONG PRPListAllocSize;

    /* Submission Queue */

    /* Submission queue ID, 0 based. Admin queue ID is 0 */
    USHORT SubQueueID;

    /* Reported number of queue entries when creating the queue */
    USHORT SubQEntries;

    /* Current number of free entries in the submission queue */
    USHORT FreeSubQEntries;

    /* Current tail pointer to submission queue */
    USHORT SubQTailPtr;

    /* Current head pointer to submission queue fetched from cpl queue entry */
    USHORT SubQHeadPtr;

    /* Associated doorbell register to ring for submissions */
    PULONG pSubTDBL;

    /* The associated completion queue ID */
    USHORT CplQueueID;

    /* Starting virtual addr of submission Queue (system memory page aligned) */
    PVOID pSubQStart;

    /* Starting physical address of submission queue */
    STOR_PHYSICAL_ADDRESS SubQStart;

    /* Linked-list contains SQ free entries */
    LIST_ENTRY FreeQList;

    /* Indicates the submission is shared among active cores in the system */
    BOOLEAN Shared;

    /* Command Entries */

    /* Starting virtual addr of all command entries */
    PVOID pCmdEntry;

    /* PRP Lists */

    /* Starting virtual addr of PRP Lists (system memory page aligned) */
    PVOID pPRPListStart;

    /* Starting physical address of PRP Lists */
    STOR_PHYSICAL_ADDRESS PRPListStart;

    /* Number of PRP lists one system page can accommodate */
    ULONG NumPRPListOnePage;

    /* Statistics */

    /* Current accumulated, issued requests */
    LONG64 Requests;

#ifdef DUMB_DRIVER
    PVOID pDblBuffAlloc;
    ULONG dblBuffSz;
    PVOID pDlbBuffStartVa;

    PVOID pDblBuffListAlloc;
    ULONG dblBuffListSz;
    PVOID pDlbBuffStartListVa;
#endif
} SUB_QUEUE_INFO, *PSUB_QUEUE_INFO;

// nvmeStd.h
/*******************************************************************************
 * Completiond Queue Information data structure.
 ******************************************************************************/
typedef struct _CPL_QUEUE_INFO
{
    /* Completion Queue */

    /* Associated Completion queue ID, 0 based. Admin queue ID is 0 */
    USHORT CplQueueID;

    /* Reported number of queue entries when creating the queue */
    USHORT CplQEntries;

    /* Used to decide if newly completed entry, either 0 or 1 */
    USHORT CurPhaseTag:1;

    /* Reserved */
    USHORT Reserved:15;

    /* Current head pointer to completion queue entries */
    USHORT CplQHeadPtr;

    /* Starting virtual addr of completion Queue (system memory page aligned) */
    PVOID pCplQStart;

    /* Associated doorbell register to ring for completions */
    PULONG pCplHDBL;

    /* Starting physical address of completion queue */
    STOR_PHYSICAL_ADDRESS CplQStart;

    /* Associated MSI Message used to interrupts, which is zero-based */
    USHORT MsiMsgID;

    /* Indicates the completion is shared among active cores in the system */
    BOOLEAN Shared;

    /* Statistics */

    /* Current accumulated, completed requests */
    ULONG64 Completions;
} CPL_QUEUE_INFO, *PCPL_QUEUE_INFO;

// nvmeStd.h
/*******************************************************************************
 * Main Queue Information data structure.
 ******************************************************************************/
typedef struct _QUEUE_INFO
{
    /*
     * Number of IO queues allocated from adapter via Set Feature command.
     * After adapter completes the Set Features request with Feature ID = 7
     * the callback function is invoked to examine the completion results.
     * If no error, NumSubIoQAllocFromAdapter and NumCplIoQAllocFromAdapter
     * are set to the allocated number, respectively.
     * In the error case, START_STATE_QUEUE_ALLOC_FAILURE is set as 1,
     * NumSubIoQAllocFromAdapter and NumCplIoQAllocFromAdapter remain zero.
     */
    ULONG NumSubIoQAllocFromAdapter; /* Number of submission queues allocated */
    ULONG NumCplIoQAllocFromAdapter; /* Number of completion queues allocated */

    /*
     * Allocated buffers via
     * StorPortPatchAllocateContiguousMemorySpecifyCacheNode function
     * Number of queues to allocate from system memory depends on:
     *
     *   1. NumSubIoQAllocFromAdapter and NumCplIoQAllocFromAdapter
     *   2. Number of active cores and NUMA nodes in the system
     *
     * The Number of queues can only be:
     *
     *   1. The number of active cores
     *   2. The number of NUMA nodes (all cores of a given NUMA node
     *      share one queue)
     *   3. One, i.e., all cores share the queue
     */
    ULONG NumSubIoQAllocated; /* Number of IO submission queues allocated */
    ULONG NumCplIoQAllocated; /* Number of IO completion queues allocated */

    /*
     * They start with the number of entries fetched from Registry.
     * When failing in the middle of allocating buffers,
     * they store the reduced number of queue entries and are used in
     * later allocations.
     */
    USHORT NumIoQEntriesAllocated; /* Number of IO queue entries allocated */
    USHORT NumAdQEntriesAllocated; /* Number of Admin queue entries allocated */

    /*
     * Number of queues created via Create IO Submission/Completion Queue
     * commands. When all succeeds, they are equal to NumSubIoQAllocated
     * and NumCplIoQAllocated in QUEUE_INFO.
     */
    ULONG NumSubIoQCreated; /* Number of submission queues created */
    ULONG NumCplIoQCreated; /* Number of completion queues created */

    /* Array of Submission Queue Info structures */
    PSUB_QUEUE_INFO pSubQueueInfo; /* Pointing to the first allocated element */

    /* Array of Completion Queue Info structures */
    PCPL_QUEUE_INFO pCplQueueInfo; /* Pointing to the first allocated element */
} QUEUE_INFO, *PQUEUE_INFO;

// nvmeStd.h
/*******************************************************************************
 * Processor Group Table Data Structure
 ******************************************************************************/
typedef struct _PROC_GROUP_TBL
{
    /* Its associated first system-wise logical processor number*/
    ULONG BaseProcessor;

    /* Number of associated cores */
    ULONG NumProcessor;

    /* Its associated group number */
    GROUP_AFFINITY GroupAffinity;

} PROC_GROUP_TBL, *PPROC_GROUP_TBL;

// nvmeStd.h
/*******************************************************************************
 * NUMA Node Table Data Structure
 ******************************************************************************/
typedef struct _NUMA_NODE_TBL
{
    /* The NUMA node number */
    ULONG NodeNum;

    /* Its associated first core number (system-wise) */
    ULONG FirstCoreNum;

    /* Its associated last core number (system-wise) */
    ULONG LastCoreNum;

    /* Number of associated cores */
    ULONG NumCores;

    /* Its associated group number */
    GROUP_AFFINITY GroupAffinity;

} NUMA_NODE_TBL, *PNUMA_NODE_TBL;

// nvmeStd.h
/*******************************************************************************
 * CPU Core Table Data Structure
 ******************************************************************************/
typedef struct _CORE_TBL
{
    /* Its associated NUMA node */
    USHORT NumaNode;

    /* Its processor group number */
    USHORT Group;

    /* The vector associated with this core's QP */
    USHORT MsiMsgID;

    /* The associated queue pair info for this core */
    USHORT SubQueue;
    USHORT CplQueue;
    ULONG  Learned;
} CORE_TBL, *PCORE_TBL;

// nvmeStd.h
/*******************************************************************************
 * MSI/MSI-X Message Table Data Structure
 ******************************************************************************/
typedef struct _MSI_MESSAGE_TBL
{
    /* The MSI message number granted from the system */
    ULONG MsgID;

    /* Physical address associated with the message */
    ULONG Data;

    /* Data associated with the message */
    STOR_PHYSICAL_ADDRESS Addr;

    /*
     * The associated completion queue number. When the message is shared
     * (Shared==TRUE), all queues need to be checked for completion entries
     */
    USHORT CplQueueNum;

    /* Indicates MSI message is shared by multiple completion queues */
    BOOLEAN Shared;

} MSI_MESSAGE_TBL, *PMSI_MESSAGE_TBL;

// nvmeStd.h
/*******************************************************************************
 * Resource Mapping Table Data Structure
 ******************************************************************************/
typedef struct _RES_MAPPING_TBL
{
    /* INTx(1), MSI(2), MSI-X(4) */
    ULONG InterruptType;

    /* Number of active cores in current system */
    ULONG NumActiveCores;

    /* Array of Core Tables, each active core has its own table allocated */
    PCORE_TBL pCoreTbl; /* Pointer to the first table */

    /* Number of MSI/MSI-X vectors granted from system */
    ULONG NumMsiMsgGranted;

    /*
     * Array of sorted MSI/MSI-X message table, each granted message has its
     * own table allocated. The sorting is based on the Destination
     * Field(bit[19..12]) of the MSI/MSI-X Address when it's physical mode for
     * interrupt routing without redirection.
     */
    PMSI_MESSAGE_TBL pMsiMsgTbl; /* Pointer to the first table */

    /* Number of NUMA nodes in system */
    ULONG NumNumaNodes;

    /* Array of NUMA affinity table, retrieved via StorPortPatchGetNodeAffinity */
    PNUMA_NODE_TBL pNumaNodeTbl; /* Topology of NUMA nodes/associated cores */

    /* Number of logical processor groups in current system */
    USHORT NumGroup;
    /* Topology of logical processor group */
    PPROC_GROUP_TBL pProcGroupTbl;
} RES_MAPPING_TBL, *PRES_MAPPING_TBL;

// nvmeStd.h
/* LUN Extension */
typedef enum _LUN_SLOT_STATUS
{
    FREE,
    ONLINE,
    OFFLINE
} LUN_SLOT_STATUS;

// nvmeStd.h
typedef enum _LUN_OFFLINE_REASON
{
    NOT_OFFLINE,
    FORMAT_IN_PROGRESS
    // Add more as needed
} LUN_OFFLINE_REASON;

// nvme.h
#define NUM_LBAF (16)

// nvme.h
/* Section 4.2, Figure 6 */
typedef struct _NVMe_COMMAND_DWORD_0
{
    /* [Opcode] This field indicates the opcode of the command to be executed */
    UCHAR    OPC;

    /*
     * [Fused Operation] In a fused operation, a complex command is created by
     * "fusing? together two simpler commands. Refer to section 6.1. This field
     * indicates whether this command is part of a fused operation and if so,
     * which command it is in the sequence. Value 00b Normal Operation, Value
     * 01b == Fused operation, first command, Value 10b == Fused operation,
     * second command, Value 11b == Reserved.
     */
    UCHAR    FUSE           :2;
    UCHAR    Reserved       :6;

    /*
     * [Command Identifier] This field indicates a unique identifier for the
     * command when combined with the Submission Queue identifier.
     */
    USHORT   CID;
} NVMe_COMMAND_DWORD_0, *PNVMe_COMMAND_DWORD_0;

// nvme.h
/*
 * Section 4.2, Figure 7
 */
typedef struct _NVMe_COMMAND
{
    /*
     * [Command Dword 0] This field is common to all commands and is defined
     * in Figure 6.
     */
    NVMe_COMMAND_DWORD_0    CDW0;

     /*
      * [Namespace Identifier] This field indicates the namespace that this
      * command applies to. If the namespace is not used for the command, then
      * this field shall be cleared to 0h. If a command shall be applied to all
      * namespaces on the device, then this value shall be set to FFFFFFFFh.
      */
    ULONG                   NSID;

    /* DWORD 2, 3 */
    ULONGLONG               Reserved;

    /*
     * [Metadata Pointer] This field contains the address of a contiguous
     * physical buffer of metadata. This field is only used if metadata is not
     * interleaved with the LBA data, as specified in the Format NVM command.
     * This field shall be Dword aligned.
     */
    ULONGLONG               MPTR;

    /* [PRP Entry 1] This field contains the first PRP entry for the command. */
    ULONGLONG               PRP1;

    /*
     * [PRP Entry 2] This field contains the second PRP entry for the command.
     * If the data transfer spans more than two memory pages, then this field is
     * a PRP List pointer.
     */
    ULONGLONG               PRP2;

    /* [Command Dword 10] This field is command specific Dword 10. */
    union {
        ULONG               CDW10;
        /*
         * Defined in Admin and NVM Vendor Specific Command format.
         * Number of DWORDs in PRP, data transfer (in Figure 8).
         */
        ULONG               NDP;
    };

    /* [Command Dword 11] This field is command specific Dword 11. */
    union {
        ULONG               CDW11;
        /*
         * Defined in Admin and NVM Vendor Specific Command format.
         * Number of DWORDs in MPTR, Metadata transfer (in Figure 8).
         */
        ULONG               NDM;
    };

    /* [Command Dword 12] This field is command specific Dword 12. */
    ULONG                   CDW12;

    /* [Command Dword 13] This field is command specific Dword 13. */
    ULONG                   CDW13;

    /* [Command Dword 14] This field is command specific Dword 14. */
    ULONG                   CDW14;

    /* [Command Dword 15] This field is command specific Dword 15. */
    ULONG                   CDW15;
} NVMe_COMMAND, *PNVMe_COMMAND;

// nvme.h
/* Section 4.5, Figure 12 */
typedef struct _NVMe_COMPLETION_QUEUE_ENTRY_DWORD_2
{
    /*
     * [SQ Head Pointer] Indicates the current Submission Queue Head pointer for
     * the Submission Queue indicated in the SQ Identifier field. This is used
     * to indicate to the host Submission Queue entries that have been consumed
     * and may be re-used for new entries. Note: The value returned is the value
     * of the SQ Head pointer when the completion entry was created. By the time
     * software consumes the completion entry, the controller may have an SQ
     * Head pointer that has advanced beyond the value indicated.
     */
    USHORT  SQHD;

    /*
     * [SQ Identifier] Indicates the Submission Queue that the associated
     * command was issued to. This field is used by software when more than one
     * Submission Queue shares a single Completion Queue to uniquely determine
     * the command completed in combination with the Command Identifier (CID).
     */
    USHORT  SQID;
} NVMe_COMPLETION_QUEUE_ENTRY_DWORD_2, *PNVMe_COMPLETION_QUEUE_ENTRY_DWORD_2;

// nvme.h
/* Section 4.5, Figure 13 */
typedef struct _NVMe_COMPLETION_QUEUE_ENTRY_DWORD_3
{
    /*
     * [Command Identifier] Indicates the identifier of the command that is
     * being completed. This identifier is assigned by host software when the
     * command is submitted to the Submission Queue. The combination of the SQ
     * Identifier and Command Identifier uniquely identifies the command that is
     * being completed. The maximum number of requests outstanding at one time
     * is 64K for an I/O Submission Queue and 4K for the Admin Submission Queue.
     */
    USHORT  CID;

    /*
     * [Status Field] Indicates status for the command that is being completed.
     * Refer to section 4.5.1.
     */
    struct
    {
        /*
         * [Phase Tag] Identifies whether a Completion Queue entry is new. The
         * Phase Tag values for all Completion Queue entries shall be
         * initialized to ?0? by host software prior to setting CC.EN to ?1?.
         * When the controller places an entry in the Completion Queue, it shall
         * invert the phase tag to enable host software to discriminate a new
         * entry. Specifically, for the first set of completion queue entries
         * after CC.EN is set to ?1? all Phase Tags are set to ?1? when they are
         * posted. For the second set of completion queue entries, when the
         * controller has wrapped around to the top of the Completion Queue, all
         * Phase Tags are cleared to ?0? when they are posted. The value of the
         * Phase Tag is inverted each pass through the Completion Queue.
         */
        USHORT  P        :1;

        /* Section 4.5, Figure 14 */

        /*
         * [Status Code] Indicates a status code identifying any error or status
         * information for the command indicated.
         */
        USHORT  SC       :8;

        /*
         * [Status Code Type] Indicates teh status code type of the completion
         * entry. This indicates the type of status the controller is returning.
         */
        USHORT  SCT      :3;
        USHORT  Reserved :2;

        /*
         * [More] If set to ?1?, there is more status information for this
         * command as part of the Error Information log that may be retrieved
         * with the Get Log Page command. If cleared to ?0?, there is no
         * additional status information for this command. Refer to section
         * 5.10.1.1.
         */
        USHORT  M        :1;

        /*
         * [Do Not Retry] If set to ?1?, indicates that if the same command is
         * re-issued it is expected to fail. If cleared to ?0?, indicates that
         * the same command may succeed if retried. If a command is aborted due
         * to time limited error recovery (refer to section 5.12.1.5), this
         * field should be cleared to ?0?.
         */
        USHORT  DNR      :1;
    } SF;
} NVMe_COMPLETION_QUEUE_ENTRY_DWORD_3, *PNVMe_COMPLETION_QUEUE_ENTRY_DWORD_3;

// nvme.h
/* Section 4.5, Figure 11 */
typedef struct _NVMe_COMPLETION_QUEUE_ENTRY
{
    ULONG                               DW0;
    ULONG                               Reserved;
    NVMe_COMPLETION_QUEUE_ENTRY_DWORD_2 DW2;
    NVMe_COMPLETION_QUEUE_ENTRY_DWORD_3 DW3;
} NVMe_COMPLETION_QUEUE_ENTRY, *PNVMe_COMPLETION_QUEUE_ENTRY;

// nvme.h
/*Status Code Type (SCT), Section 4.5.1.1, Figure 15 */
#define GENERIC_COMMAND_STATUS                          0
#define COMMAND_SPECIFIC_ERRORS                         1
#define MEDIA_ERRORS                                    2

// nvme.h
/* An invalid field specified in the command parameters. */
#define INVALID_FIELD_IN_COMMAND                        0x2

// nvme.h
/*
 * Identify - LBA Format Data Structure, NVM Command Set Specific
 *
 * Section 5.11, Figure 68
 */
typedef struct _ADMIN_IDENTIFY_FORMAT_DATA
{
    /*
     * [Metadata Size] This field indicates the number of metadata bytes
     * provided per LBA based on the LBA Size indicated. The namespace may
     * support the metadata being transferred as part of an extended data LBA or
     * as part of a separate contiguous buffer. If end-to-end data protection is
     * enabled, then the first eight bytes or last eight bytes of the metadata
     * is the protection information.
     */
    USHORT  MS;

    /*
     * [LBA Data Size] This field indicates the LBA data size supported. The
     * value is reported in terms of a power of two (2^n). A value smaller than
     * 9 (i.e. 512 bytes) is not supported. If the value reported is 0h then the
     * LBA format is not supported / used.
     */
    UCHAR   LBADS;

    /*
     * [Relative Performance] This field indicates the relative performance of
     * the LBA format indicated relative to other LBA formats supported by the
     * controller. Depending on the size of the LBA and associated metadata,
     * there may be performance implications. The performance analysis is based
     * on better performance on a queue depth 32 with 4KB read workload. The
     * meanings of the values indicated are included in the following table.
     * Value 00b == Best performance. Value 01b == Better performance. Value 10b
     * == Good performance. Value 11b == Degraded performance.
     */
    UCHAR   RP       :2;
    UCHAR   Reserved :6;
} ADMIN_IDENTIFY_FORMAT_DATA, *PADMIN_IDENTIFY_FORMAT_DATA;

// nvme.h
#define NUM_LBAF (16)

// nvme.h
/* Identify Namespace Data Structure, Section 5.11, Figure 67 */
typedef struct _ADMIN_IDENTIFY_NAMESPACE
{
    /*
     * [Namespace Size] This field indicates the total size of the namespace in
     * logical blocks. A namespace of size n consists of LBA 0 through (n - 1).
     * The number of logical blocks is based on the formatted LBA size. This
     * field is undefined prior to the namespace being formatted. Note: The
     * creation of the namespace(s) and initial format operation are outside the
     * scope of this specification.
     */
    ULONGLONG                   NSZE;

    /*
     * [Namespace Capacity] This field indicates the maximum number of logical
     * blocks that may be allocated in the namespace at any point in time. The
     * number of logical blocks is based on the formatted LBA size. This field
     * is undefined prior to the namespace being formatted. This field is used
     * in the case of thin provisioning and reports a value that is smaller than
     * or equal to the Namespace Size. Spare LBAs are not reported as part of
     * this field. A value of 0h for the Namespace Capacity indicates that the
     * namespace is not available for use. A logical block is allocated when it
     * is written with a Write or Write Uncorrectable command. A logical block
     * may be deallocated using the Dataset Management command.
     */
    ULONGLONG                   NCAP;

    /*
     * [Namespace Utilization] This field indicates the current number of
     * logical blocks allocated in the namespace. This field is smaller than or
     * equal to the Namespace Capacity. The number of logical blocks is based on
     * the formatted LBA size. When using the NVM command set: A logical block
     * is allocated when it is written with a Write or Write Uncorrectable
     * command. A logical block may be deallocated using the Dataset Management
     * command.
     */
    ULONGLONG                   NUSE;

    /* [Namespace Features] This field defines features of the namespace. */
    struct
    {
        /*
         * Bit 0 if set to ?1? indicates that the namespace supports thin
         * provisioning. Specifically, the Namespace Capacity reported may be
         * less than the Namespace Size. When this feature is supported and the
         * Dataset Management command is supported then deallocating LBAs shall
         * be reflected in the Namespace Utilization field. Bit 0 if cleared to
         * ?0? indicates that thin provisioning is not supported and the
         * Namespace Size and Namespace Capacity fields report the same value.
         */
        UCHAR   SupportsThinProvisioning    :1;
        UCHAR   Reserved                    :7;
    } NSFEAT;

    /*
     * [Number of LBA Formats] This field defines the number of supported LBA
     * size and metadata size combinations supported by the namespace. LBA
     * formats shall be allocated in order (starting with 0) and packed
     * sequentially. This is a 0抯 based value. The maximum number of LBA
     * formats that may be indicated as supported is 16. The supported LBA
     * formats are indicated in bytes 128 ? 191 in this data structure. The
     * metadata may be either transferred as part of the LBA (creating an
     * extended LBA which is a larger LBA size that is exposed to the
     * application) or it may be transferred as a separate contiguous buffer of
     * data. The metadata shall not be split between the LBA and a separate
     * metadata buffer. It is recommended that software and controllers
     * transition to an LBA size that is 4KB or larger for ECC efficiency at the
     * controller. If providing metadata, it is recommended that at least 8
     * bytes are provided per logical block to enable use with end-to-end data
     * protection, refer to section 8.2.
     */
    UCHAR NLBAF;

    /*
     * [Formatted LBA Size] This field indicates the LBA size & metadata size
     * combination that the namespace has been formatted with.
     */
    struct
    {
        /*
         * Bits 3:0 indicates one of the 16 supported combinations indicated in
         * this data structure. This is a 0抯 based value.
         */
        UCHAR   SupportedCombination        :4;

        /*
         * Bit 4 if set to ?1? indicates that the metadata is transferred at the
         * end of the data LBA, creating an extended data LBA. Bit 4 if cleared
         * to 0? indicates that all of the metadata for a command is transferred
         * as a separate contiguous buffer of data.
         */
        UCHAR   SupportsMetadataAtEndOfLBA  :1;
        UCHAR   Reserved                    :3;
    } FLBAS;

    /*
     * [Metadata Capabilities] This field indicates the capabilities for
     * metadata.
     */
    struct
    {
        /*
         * Bit 0 if set to ?1? indicates that the namespace supports the
         * metadata being transferred as part of an extended data LBA.
         * Specifically, the metadata is transferred as part of the data PRP
         * Lists. Bit 0 if cleared to ?0? indicates that the namespace does not
         * support the metadata being transferred as part of an extended data
         * LBA.
         */
        UCHAR   SupportsMetadataAsPartOfLBA :1;

        /*
         * Bit 1 if set to ?1? indicates the namespace supports the metadata
         * being transferred as part of a separate buffer that is specified in
         * the Metadata Pointer. Bit 1 if cleared to ?0? indicates that the
         * controller does not support the metadata being transferred as part of
         * a separate buffer.
         */
        UCHAR   SupportsMetadataAsSeperate  :1;
        UCHAR   Reserved                    :6;
    } MC;

    /*
     * [End-to-end Data Protection Capabilities] This field indicates the
     * capabilities for the end-to-end data protection feature. Multiple bits
     * may be set in this field. Refer to section 8.3.
     */
    struct {
        /*
         * Bit 0 if set to ?1? indicates that the namespace supports Protection
         * Information Type 1. Bit 0 if cleared to ?0? indicates that the
         * namespace does not support Protection Information Type 1.
         */
        UCHAR   SupportsProtectionType1     :1;

        /*
         * Bit 1 if set to ?1? indicates that the namespace supports Protection
         * Information Type 2. Bit 1 if cleared to ?0? indicates that the
         * namespace does not support Protection Information Type 2.
         */
        UCHAR   SupportsProtectionType2     :1;

        /*
         * Bit 2 if set to ?1? indicates that the namespace supports Protection
         * Information Type 3. Bit 2 if cleared to ?0? indicates that the
         * namespace does not support Protection Information Type 3.
         */
        UCHAR   SupportsProtectionType3     :1;

        /*
         * Bit 3 if set to ?1? indicates that the namespace supports protection
         * information transferred as the first eight bytes of metadata. Bit 3
         * if cleared to ?0? indicates that the namespace does not support
         * protection information transferred as the first eight bytes of
         * metadata.
         */
        UCHAR   SupportsProtectionFirst8    :1;

        /*
         * Bit 4 if set to ?1? indicates that the namespace supports protection
         * information transferred as the last eight bytes of metadata. Bit 4 if
         * cleared to ?0? indicates that the namespace does not support
         * protection information transferred as the last eight bytes of
         * metadata.
         */
        UCHAR   SupportsProtectionLast8     :1;
        UCHAR   Reserved                    :3;
    } DPC;

    /*
     * [End-to-end Data Protection Type Settings] This field indicates the Type
     * settings for the end-to-end data protection feature. Refer to section
     * 8.3.
     */
    struct
    {
        /*
         * Bits 2:0 indicate whether Protection Information is enabled and the
         * type of Protection Information enabled. The values for this field
         * have the following meanings: Value 000b == Protection information is
         * not enabled. Value 001b == Protection information is enabled, Type 1.
         * Value 010b == Protection information is enabled, Type 2. Value 011b
         * == Protection information is enabled, Type 3. Value 100b-111b ==
         * Reserved.
         */
        UCHAR   ProtectionEnabled           :3;

        /*
         * Bit 3 if set to ?1? indicates that the protection information, if
         * enabled, is transferred as the first eight bytes of metadata. Bit 3
         * if cleared to 0? indicates that the protection information, if
         * enabled, is transferred as the last eight bytes of metadata.
         */
        UCHAR   ProtectionInFirst8          :1;
        UCHAR   Reserved                    :4;
    } DPS;

    UCHAR                       Reserved1[98];

    /*
     * [LBA Format x Support] This field indicates the LBA format x that is
     * supported by the controller. The LBA format field is defined in Figure
     * 68.
     */
    ADMIN_IDENTIFY_FORMAT_DATA  LBAFx[NUM_LBAF];
    UCHAR                       Reserved2[192];

    /*
     * [Vendor Specific] This range of bytes is allocated for vendor specific
     * usage.
     */
    UCHAR                       VS[3712];
} ADMIN_IDENTIFY_NAMESPACE, *PADMIN_IDENTIFY_NAMESPACE;

// nvmeStd.h
typedef struct _nvme_lun_extension
{
    ADMIN_IDENTIFY_NAMESPACE     identifyData;
    UINT32                       namespaceId;
    BOOLEAN                      ReadOnly;
    LUN_SLOT_STATUS              slotStatus;
    LUN_OFFLINE_REASON           offlineReason;
} NVME_LUN_EXTENSION, *PNVME_LUN_EXTENSION;

// nvmeStd.h
#define MAX_NAMESPACES              16

// nvme.h
/* Identify Command, Section 5.11, Figure 64, Opcode 0x06 */
typedef struct _ADMIN_IDENTIFY_COMMAND_DW10
{
    /*
     * [Controller or Namespace Structure] If set to ?1?, then the Identify
     * Controller data structure is returned to the host. If cleared to ?0?,
     * then the Identify Namespace data structure is returned to the host for
     * the namespace specified in the command header.
     */
    ULONG   CNS      :1;
    ULONG   Reserved :31;
} ADMIN_IDENTIFY_COMMAND_DW10, *PADMIN_IDENTIFY_COMMAND_DW10;

// nvme.h
/* Identify - Power State Descriptor Data Structure, Section 5.11, Figure 66 */
typedef struct _ADMIN_IDENTIFY_POWER_STATE_DESCRIPTOR
{
    /*
     * [Maximum Power] This field indicates the maximum power consumed by the
     * NVM subsystem in this power state. The power in Watts is equal to the
     * value in this field multiplied by 0.01.
     */
    USHORT  MP;
    USHORT  Reserved1;

    /*
     * [Entry Latency] This field indicates the maximum entry latency in
     * microseconds associated with entering this power state.
     */
    ULONG   ENLAT;

    /*
     * [Exit Latency] This field indicates the maximum exit latency in
     * microseconds associated with entering this power state.
     */
    ULONG   EXLAT;

    /*
     * [Relative Read Throughput] This field indicates the relative read
     * throughput associated with this power state. The value in this field
     * shall be less than the number of supported power states (e.g., if the
     * controller supports 16 power states, then valid values are 0 through 15).
     * A lower value means higher read throughput.
     */
    UCHAR   RRT       :5;
    UCHAR   Reserved2 :3;

    /*
     * [Relative Read Latency] This field indicates the relative read latency
     * associated with this power state. The value in this field shall be less
     * than the number of supported power states (e.g., if the controller
     * supports 16 power states, then valid values are 0 through 15). A lower
     * value means lower read latency.
     */
    UCHAR   RRL       :5;
    UCHAR   Reserved3 :3;

    /*
     * [Relative Write Throughput] This field indicates the relative write
     * throughput associated with this power state. The value in this field
     * shall be less than the number of supported power states (e.g., if the
     * controller supports 16 power states, then valid values are 0 through 15).
     * A lower value means higher write throughput.
     */
    UCHAR   RWT       :5;
    UCHAR   Reserved4 :3;

    /*
     * Relative Write Latency] This field indicates the relative write latency
     * associated with this power state. The value in this field shall be less
     * than the number of supported power states (e.g., if the controller
     * supports 16 power states, then valid values are 0 through 15). A lower
     * value means lower write latency.
     */
    UCHAR   RWL       :5;
    UCHAR   Reserved5 :3;
    UCHAR   Reserved6[16];
} ADMIN_IDENTIFY_POWER_STATE_DESCRIPTOR,
  *PADMIN_IDENTIFY_POWER_STATE_DESCRIPTOR;

// nvme.h
/* Identify Controller Data Structure, Section 5.11, Figure 65 */
typedef struct _ADMIN_IDENTIFY_CONTROLLER
{
    /* Controller Capabiliites and Features */

    /*
     * [PCI Vendor ID] Contains the company vendor identifier that is assigned
     * by the PCI SIG. This is the same value as reported in the ID register in
     * section 2.1.1.
     */
    USHORT  VID;

    /*
     * [PCI Subsystem Vendor ID] Contains the company vendor identifier that is
     * assigned by the PCI SIG for the subsystem. This is the same value as
     * reported in the SS register in section 2.1.17.
     */
    USHORT  SSVID;

    /*
     * [Serial Number] Contains the serial number for the NVM subsystem that is
     * assigned by the vendor as an ASCII string. Refer to section 7.7 for
     * unique identifier requirements
     */
    UCHAR   SN[20];

    /*
     * [Model Number] Contains the model number for the NVM subsystem that is
     * assigned by the vendor as an ASCII string. Refer to section 7.7 for
     * unique identifier requirements.
     */
    UCHAR   MN[40];

    /*
     * [Firmware Revision] Contains the currently active firmware revision for
     * the NVM subsystem. This is the same revision information that may be
     * retrieved with the Get Log Page command, refer to section 5.10.1.3. See
     * section 1.8 for ASCII string requirements.
     */
    UCHAR   FR[8];

    /*
     * [Recommended Arbitration Burst] This is the recommended Arbitration Burst
     * size. Refer to section 4.7.
     */
    UCHAR   RAB;

    /*
     * IEEE OUI Identifier (IEEE): Contains the Organization Unique Identifier (OUI) for
     * the controller vendor. The OUI shall be a valid IEEE/RAC
     * ( assigned identifier that may be registered at
     * http://standards.ieee.org/develop/regauth/oui/public.html.
     * and Multi-Interface Capabilities
    */
    UCHAR IEEE[3];
    UCHAR MIC;
    /*
     *  Maximum Data Transfer Size(MDTS)
    */
    UCHAR MDTS;

    UCHAR Reserved1[178];

    /* Admin Command Set Attributes */

    /*
     * [Optional Admin Command Support] This field indicates the optional Admin
     * commands supported by the controller. Refer to section 5.
     */
    struct
    {
        /*
         * Bit 0 if set to ?1? then the controller supports the Security Send
         * and Security Receive commands. If cleared to ?0? then the controller
         * does not support the Security Send and Security Receive commands.
         */
        USHORT  SupportsSecuritySendSecurityReceive     :1;

        /*
         * Bit 1 if set to ?1? then the controller supports the Format NVM
         * command. If cleared to ?0? then the controller does not support the
         * Format NVM command.
         */
        USHORT  SupportsFormatNVM                       :1;

        /*
         * Bit 2 if set to ?1? then the controller supports the Firmware
         * Activate and Firmware Download commands. If cleared to ?0? then the
         * controller does not support the Firmware Activate and Firmware
         * Download commands.
         */
        USHORT  SupportsFirmwareActivateFirmwareDownload:1;
        USHORT  Reserved                                :13;
    } OACS;

    /*
     * [Abort Command Limit] This field is used to convey the maximum number of
     * concurrently outstanding Abort commands supported by the controller (see
     * section 5.1). This is a 0抯 based value. It is recommended that
     * implementations support a minimum of four Abort commands outstanding
     * simultaneously.
     */
    UCHAR ACL;

    /*
     * [Asynchronous Event Request Limit] This field is used to convey the
     * maximum number of concurrently outstanding Asynchronous Event Request
     * commands supported by the controller (see section 5.2). This is a 0抯
     * based value. It is recommended that implementations support a minimum of
     * four Asynchronous Event Request Limit commands oustanding simultaneously.
     */
    UCHAR   UAERL;

    /*
     * [Firmware Updates] This field indicates capabilities regarding firmware
     * updates. Refer to section 8.1 for more information on the firmware update
     * process.
     */
    struct
    {
        /*
         * Bit 0 if set to ?1? indicates that the first firmware slot (slot 1)
         * is read only. If cleared to ?0? then the first firmware slot (slot 1)
         * is read/write. Implementations may choose to have a baseline read
         * only firmware image.
         */
        UCHAR   FirstFirmwareSlotReadOnly               :1;

        /*
         * Bits 3:1 indicate the number of firmware slots that the device
         * supports. This field shall specify a value between one and seven,
         * indicating that at least one firmware slot is supported and up to
         * seven maximum. This corresponds to firmware slots 1 through 7.
         */
        UCHAR   SupportedNumberOfFirmwareSlots          :3;
        UCHAR   Reserved                                :4;
    } FRMW;

    /*
     * [Log Page Attributes] This field indicates optional attributes for log
     * pages that are accessed via the Get Log Page command.
     */
    struct
    {
        /*
         * Bit 0 if set to ?1? then the controller supports the SMART / Health
         * information log page on a per namespace basis. If cleared to ?0? then
         * the controller does not support the SMART / Health information log
         * page on a per namespace basis; the log page returned is global for
         * all namespaces.
         */
        UCHAR   SupportsSMART_HealthInformationLogPage  :1;
        UCHAR   Reserved                                :7;
    } LPA;

    /*
     * [Error Log Page Entries] This field indicates the number of Error
     * Information log entries that are stored by the controller. This field is
     * a 0抯 based value.
     */
    UCHAR   ELPE;

    /*
     * [Number of Power States Support] This field indicates the number of
     * NVMHCI power states supported by the controller. This is a 0抯 based
     * value. Refer to section 8.4. Power states are numbered sequentially
     * starting at power state 0. A controller shall support at least one power
     * state (i.e., power state 0) and may support up to 31 additional power
     * states (i.e., up to 32 total).
     */
    UCHAR NPSS;
    /*
     * Admin Vendor Specific Command Configuration (AVSCC): This field indicates
     * the configuration settings for admin vendor specific command handling.
     */
    UCHAR   AVSCC          :1;
    UCHAR   Reserved_AVSCC :7;
    UCHAR   Reserved2[247];
    /* NVM Command Set Attributes */

    /*
     * [Submission Queue Entry Size] This field defines the required and maximum
     * Submission Queue entry size when using the NVM Command Set.
     */
    struct
    {
        /*
         * Bits 3:0 define the required Submission Queue Entry size when using
         * the NVM Command Set. This is the minimum entry size that may be used.
         * The value is in bytes and is reported as a power of two (2^n). The
         * required value shall be 6, corresponding to 64.
         */
        UCHAR   RequiredSubmissionQueueEntrySize        :4;

        /*
         * Bits 7:4 define the maximum Submission Queue entry size when using
         * the NVM Command Set. This value is larger than or equal to the
         * required SQ entry size. The value is in bytes and is reported as a
         * power of two (2^n).
         */
        UCHAR   MaximumSubmissionQueueEntrySize         :4;
    } SQES;

    /*
     * [Completion Queue Entry Size] This field defines the required and maximum
     * Completion Queue entry size when using the NVM Command Set.
     */
    struct
    {
        /*
         * Bits 3:0 define the required Completion Queue entry size when using
         * the NVM Command Set. This is the minimum entry size that may be used.
         * The value is in bytes and is reported as a power of two (2^n). The
         * required value shall be 4, corresponding to 16.
         */
        UCHAR   RequiredCompletionQueueEntrySize        :4;

        /*
         * Bits 7:4 define the maximum Completion Queue entry size when using
         * the NVM Command Set. This value is larger than or equal to the
         * required CQ entry size. The value is in bytes and is reported as a
         * power of two (2^n).
         */
        UCHAR   MaximumCompletionQueueEntrySize         :4;
    } CQES;

    UCHAR   Reserved3[2];

    /*
     * [Number of Namespaces] This field defines the number of valid namespaces
     * present for the controller.  Namespaces shall be allocated in order
     * (starting with 1) and packed sequentially.
     */
    ULONG   NN;

    /*
     * [Optional NVM Command Support] This field indicates the optional NVM
     * commands supported by the controller. Refer to section 6.
     */
    struct
    {
        /*
         * Bit 0 if set to ?1? then the controller supports the Compare command.
         * If cleared to ?0? then the controller does not support the Compare
         * command.
         */
        USHORT  SupportsCompare                         :1;

        /*
         * Bit 1 if set to ?1? then the controller supports the Write
         * Uncorrectable command. If cleared to ?0? then the controller does not
         * support the Write Uncorrectable command.
         */
        USHORT  SupportsWriteUncorrectable              :1;

        /*
         * Bit 2 if set to ?1? then the controller supports the Dataset
         * Management command. If cleared to ?0? then the controller does not
         * support the Dataset Management command.
         */
        USHORT  SupportsDataSetManagement               :1;
        USHORT  Reserved                                :13;
    } ONCS;

    /*
     * [Fused Operation Support] This field indicates the fused operations that
     * the controller supports. Refer to section 6.1.
     */
    struct
    {
        /*
         * Bit 0 if set to ?1? then the controller supports the Compare and
         * Write fused operation. If cleared to ?0? then the controller does not
         * support the Compare and Write fused operation. Compare shall be the
         * first command in the sequence.
         */
        USHORT  SupportsCompare_Write                   :1;
        USHORT  Reserved                                :15;
    } FUSES;

    /*
     * [Format NVM Attributes] This field indicates attributes for the Format
     * NVM command.
     */
    struct
    {
        /*
         * Bit 0 indicates whether the format operation applies to all
         * namespaces or is specific to a particular namespace. If set to ?1?,
         * then all namespaces shall be configured with the same attributes and
         * a format of any namespace results in a format of all namespaces. If
         * cleared to ?0?, then the controller supports format on a per
         * namespace basis.
         */
        UCHAR   FormatAppliesToAllNamespaces            :1;

        /*
         * Bit 1 indicates whether secure erase functionality applies to all
         * namespaces or is specific to a particular namespace. If set to?1?,
         * then a secure erase of a particular namespace as part of a format
         * results in a secure erase of all namespaces. If cleared to ?0?, then
         * a secure erase as part of a format is performed on a per namespace
         * basis.
         */
        UCHAR   SecureEraseAppliesToAllNamespaces       :1;

        /*
         * Bit 2 indicates whether cryptographic erase is supported as part of
         * the secure erase functionality. If set to ?1?, then cryptographic
         * erase is supported. If cleared to ?0?, then cryptographic erase is
         * not supported.
         */
        UCHAR   SupportsCryptographicErase              :1;
        UCHAR   Reserved                                :5;
    } FNA;

    /*
     * [Volatile Write Cache] This field indicates attributes related to the
     * presence of a volatile write cache in the implementation.
     */
    struct
    {
        /*
         * Bit 0 if set to ?1? indicates that a volatile write cache is present.
         * If cleared to ?0?, a volatile write cache is not present. If a
         * volatile write cache is present, then the host may issue Flush
         * commands and control whether it is enabled with Set Features
         * specifying the Volatile Write Cache feature identifier. If a volatile
         * write cache is not present, the host shall not issue Flush commands
         * nor Set Features or Get Features with the Volatile Write Cache
         * identifier.
         */
        UCHAR   Present               :1;
        UCHAR   Reserved                                :7;
    } VWC;

    /*
     * [Atomic Write Unit Normal] This field indicates the atomic write size for
     * the controller during normal operation. This field is specified in
     * logical blocks and is a 0抯 based value. If a write is issued of this
     * size or less, the host is guaranteed that the write is atomic to the
     * NVM with respect to other read or write operations. A value of FFh
     * indicates all commands are atomic as this is the largest command size. It
     * is recommended that implementations support a minimum of 128KB
     * (appropriately scaled based on LBA size).
     */
    USHORT  AWUN;

    /*
     * [Atomic Write Unit Power Fail] This field indicates the atomic write size
     * for the controller during a power fail condition. This field is specified
     * in logical blocks and is a 0抯 based value. If a write is issued of this
     * size or less, the host is guaranteed that the write is atomic to the NVM
     * with respect to other read or write operations.
     */
    USHORT  AWUPF;
    /*
     * NVM Vendor Specific Command Configuration (NVSCC): This field indicates
     * the configuration settings for NVM vendor specific command handling.
     */
    UCHAR   NVSCC          :1;
    UCHAR   Reserved_NVSCC :7;
    UCHAR   Reserved4[173];
    /* I/O Command Set Attributes */
    UCHAR   Reserved5[1344];

    /* Power State Descriptors */

    /*
     * [Power State x Descriptor] This field indicates the characteristics of
     * power state x. The format of this field is defined in Figure 66.
     */
    ADMIN_IDENTIFY_POWER_STATE_DESCRIPTOR PSDx[32];

    /* Vendor Specific */

    /*
     * [Vendor Specific] This range of bytes is allocated for vendor specific
     * usage.
     */
    UCHAR   VS[1024];
} ADMIN_IDENTIFY_CONTROLLER, *PADMIN_IDENTIFY_CONTROLLER;

// nvme.h
/* The namespace or the format of that namespace is invalid. */
#define INVALID_NAMESPACE_OR_FORMAT                     0xB

// nvme.h
/*
 * LBA Range Type - Entry
 *
 * Section 5.12.1.3, Figure 77, Feature Identifier 03h
 */
typedef struct _ADMIN_SET_FEATURES_COMMAND_LBA_RANGE_TYPE_ENTRY
{
    /*
     * [Type] Identifies the Type of the LBA range. The Types are listed below.
     * Value 00h == Reserved.  Value 01h == Filesystem. Value 02h == RAID. Value
     * 03h == Cache. Value 04h == Page/swap file.  Value 05h-7Fh == Reserved.
     * Value 80h-FFh == Vendor Specific.
     */
    UCHAR       Type;

    /* Identifies attributes of the LBA range. Each bit defines an attribute. */
    struct
    {
        /*
         * If set to ?1?, the LBA range may be overwritten. If cleared to ?0?,
         * the area should not be overwritten.
         */
        UCHAR   Overwriteable   :1;

        /*
         * If set to ?1?, the LBA range should be hidden from the OS / EFI /
         * BIOS. If cleared to ?0?, the area should be visible to the OS / EFI
         * / BIOS.
         */
        UCHAR   Hidden          :1;
        UCHAR   Reserved        :6;
    } Attributes;

    UCHAR       Reserved1[14];

    /*
     * [Starting LBA] This field indicates the 64-bit address of the first LBA
     * that is part of this LBA range.
     */
    ULONGLONG   SLBA;

    /*
     * [Number of Logical Blocks] This field indicates the number of logical
     * blocks that are part of this LBA range. This is a 0抯 based value.
     */
    ULONGLONG   NLB;

    /*
     * [Unique Identifier] This field is a global unique identifier that
     * uniquely identifies the type of this LBA range. Well known Types may be
     * defined and are published on the NVMHCI website.
     */
    UCHAR       GUID[16];
    UCHAR       Reserved2[16];
} ADMIN_SET_FEATURES_LBA_COMMAND_RANGE_TYPE_ENTRY,
  *PADMIN_SET_FEATURES_COMMAND_LBA_RANGE_TYPE_ENTRY;

// nvme.h
/* Set Features Command, Section 5.12, Figure 71, Opcode 0x0A */
typedef struct _ADMIN_SET_FEATURES_COMMAND_DW10
{
    /*
     * [Feature Identifier] his field indicates the identifier of the Feature
     * that attributes are being specified for.
     */
    ULONG   FID      :8;
    ULONG   Reserved :24;
} ADMIN_SET_FEATURES_COMMAND_DW10, *PADMIN_SET_FEATURES_COMMAND_DW10;

// nvme.h
/* LBA Range Type, Section 5.12.1.3, Figure 76, Feature Identifier 02h */
typedef struct _ADMIN_SET_FEATURES_COMMAND_LBA_RANGE_TYPE_DW11
{
    /*
     * [Number of LBA Ranges] This field indicates the number of LBA ranges
     * specified in this command. This is a 0抯 based value.
     */
    ULONG   NUM      :6;
    ULONG   Reserved :26;
} ADMIN_SET_FEATURES_COMMAND_LBA_RANGE_TYPE_DW11,
  *PADMIN_SET_FEATURES_COMMAND_LBA_RANGE_TYPE_DW11;

// nvmeStd.h
/* Callback function prototype for internal request completions */
typedef BOOLEAN (*PNVME_COMPLETION_ROUTINE) (PVOID param1, PVOID param2);
// nvmeStd.h
#define MAX_TX_SIZE                 (1024*1024)
// nvmeStd.h
#define PAGE_SIZE_IN_4KB            0x1000
// nvmeStd.h
#define PAGE_SIZE_IN_DWORDS         PAGE_SIZE_IN_4KB / 4
// nvmeStd.h
#define MODE_SNS_MAX_BUF_SIZE       256
// declare empty structure definition so that it may be referenced by routines before they are defined
typedef struct _nvme_srb_extension *PNVME_SRB_EXTENSION;

// nvmeStd.h
/* NVMe Miniport Device Extension */
typedef struct _NVME_DEVICE_EXTENSION
{
	//---------------------------
	// custom fields start here
	//---------------------------
	// PCI Bus Number
	USHORT Bus;
	// PCI Device Number
	USHORT Dev;
	// PCI Function Number
	USHORT Func;
	// PCI Vendor ID
	USHORT PciVendorID;
	// PCI Device ID
	USHORT PciDeviceID;
	// model of connected device
	CHAR Model[41];
	// firmware revision of connected device
	CHAR FirmwareRevision[9];
	// serial of connected device
	CHAR Serial[21];
	// NVMeRead initialization variables
	UINT32 lbaLength;
	UINT16 length;
	ULONG DataBufferSize;
	PNVME_SRB_EXTENSION pNVMeSrbExt[NUM_REQUESTS];
	PNVMe_COMMAND pCmd[NUM_REQUESTS];
	//---------------------------
	// custom fields end here
	//---------------------------

    /* Controller register base address */
    PNVMe_CONTROLLER_REGISTERS  pCtrlRegister;

    /* PCI configuration information for the controller */ 
    PPORT_CONFIGURATION_INFORMATION pPCI;

    /* NVMe queue information structure */
    QUEUE_INFO                  QueueInfo;
    BOOLEAN                     IoQueuesAllocated;

    /* Byte size of each PRP list, (MaxTx/PAGE_SIZE)*8 */
    ULONG                       PRPListSize;

    /* Resource Mapping Table */
    RES_MAPPING_TBL             ResMapTbl;
    BOOLEAN                     ResourceTableMapped;

    /* The initial values fetched from Registry */
    INIT_INFO                   InitInfo;

    /* Used to read the PCI config space */
    ULONG                       SystemIoBusNumber;

    /* Bus, Device, Function */
    ULONG                       SlotNumber;

    /* Tables */
    ULONG                       LunExtSize;

    /* Reference by LUN Id and current number of visible NSs */
    PNVME_LUN_EXTENSION         pLunExtensionTable[MAX_NAMESPACES];
    ULONG                       visibleLuns;

    /* Controller Identify Data */
    ADMIN_IDENTIFY_CONTROLLER   controllerIdentifyData;

    /* General Device Extension info */

    /* State Machine structure */
    START_STATE                 DriverState;

    /* Used to determine if we are in NT context */
    BOOLEAN                     InNTContext;

    /* Used to determine if we are in crashdump mode */
    BOOLEAN                     ntldrDump;

    /* The bytes of memory have been allocated from the dump/hibernation buffer*/
    ULONG                       DumpBufferBytesAllocated;
    /* The memory buffer in dump/hibernation mode. */
    PUCHAR                      DumpBuffer;

    /* saved a few calc'd values based on CAP fields */
    ULONG                       uSecCrtlTimeout;
    ULONG                       strideSz;

    /* DPCs needed for SNTI, AER, and error recovery */
    STOR_DPC                    SntiDpc;
    STOR_DPC                    AerDpc;
    STOR_DPC                    RecoveryDpc;
    BOOLEAN                     RecoveryAttemptPossible;

    /* IO Completion DPC Array Info */
    PVOID                       pDpcArray;
    ULONG                       NumDpc;

    /* INTx interrupt mask flag */
    BOOLEAN                     IntxMasked;
    BOOLEAN                     ShutdownInProgress;

    /* state of shutdown */
    STOR_POWER_ACTION           PowerAction;

    /* Format NVM State Machine information */
    FORMAT_NVM_INFO             FormatNvmInfo;

    /* counter used to determine in learning the vector/core table */
    ULONG                       LearningCores;

    /* Flag to indicate multiple cores are sharing a single queue */
    BOOLEAN                     MultipleCoresToSingleQueueFlag;

    /* Flag to indicate hardReset is in progress */
    BOOLEAN                     hwResetInProg;

#if DBG
    /* part of debug code to sanity check learning */
    BOOLEAN                     LearningComplete;
#endif


} NVME_DEVICE_EXTENSION, *PNVME_DEVICE_EXTENSION;

// nvmeStd.h
/* SRB Extension */
typedef struct _nvme_srb_extension
{
    /* General SRB Extension info */

    /* Pointer back to miniport adpater extension*/
    PNVME_DEVICE_EXTENSION       pNvmeDevExt;

    /* Pointer back to SRB - NULL if internal I/O */
#if (NTDDI_VERSION > NTDDI_WIN7)
    PSTORAGE_REQUEST_BLOCK       pSrb;
#else
    PSCSI_REQUEST_BLOCK          pSrb;
#endif

    /* Is this an ADMIN command or an NVM command */
    BOOLEAN                      forAdminQueue;

    /* NVMe Specific data */

    /* Submission queue entry */
    NVMe_COMMAND  nvmeSqeUnit;

    /* Completion Entry Data */
    PNVMe_COMPLETION_QUEUE_ENTRY pCplEntry;

    /* Callback completion routine, if needed */
    PNVME_COMPLETION_ROUTINE     pNvmeCompletionRoutine;

    /*
     * dsmBuffer adds a DWORD-aligned 2K buffer that -- when combined with 
     * the prpList which immediately follows it -- serves as a 4K area that is 
     * used to house DSM range definitions when issuing DSM commands in 
     * processing SCSI UNMAP requests. 
     * 
     * THIS MUST REMAIN IMMEDIATELY IN FRONT OF prpList IN ORDER TO ACHIEVE 4K 
     * OF BUFFER SPACE (OTHERWISE, IF SRB EXTENSION SIZE IS NOT A CONCERN, IT 
     * COULD SIMPLY BE DEFINED AS PAGE_SIZE_IN_DWORDS TO ELIMINATE DEPENDANCY)
     */

#define PRP_LIST_SIZE            sizeof(UINT64) * (MAX_TX_SIZE / PAGE_SIZE)

    UINT32                       dsmBuffer[PAGE_SIZE_IN_DWORDS -
                                        PRP_LIST_SIZE /sizeof(UINT32)];
    /* Temp PRP List */
    UINT64                       prpList[MAX_TX_SIZE / PAGE_SIZE];
    UINT32                       numberOfPrpEntries;

    /* Data buffer pointer for internally allocated memory */
    PVOID                        pDataBuffer;
    QWORD                        pDataBuffer64;

    /* Child/Parent pointers for child I/O's needed when holes in SGL's */
    PVOID                        pChildIo;
    PVOID                        pParentIo;

    /* 
	 * Temporary buffer to prepare the modesense data before copying into 
	 * pSrb->DataBuffer
	 */
	UCHAR                        modeSenseBuf[MODE_SNS_MAX_BUF_SIZE];
    ULONG                        abortedCmdCount;
    ULONG                        issuedAbortCmdCnt;
    ULONG                        failedAbortCmdCnt;
    BOOLEAN                      cmdGotAbortedFlag;

#if DBG
    /* used for debug learning the vector/core mappings */
    PROCESSOR_NUMBER             procNum;
#endif

#ifdef DUMB_DRIVER
    PVOID pDblVir;     // this cmd's dbl buffer virtual address
    PVOID pSrbDataVir; // this cmd's SRB databuffer virtual address
    ULONG dataLen;     // dbl buff data length
#endif
} NVME_SRB_EXTENSION;

// nvme.h
#define ADMIN_SET_FEATURES                              0x09
// nvme.h
#define INTERRUPT_COALESCING                0x08
// nvme.h
#define NUMBER_OF_QUEUES                    0x07
// nvmeStd.h
#define GET_WORD_0(Value)  (Value & 0xFFFF)
// nvmeStd.h
#define GET_WORD_1(Value)  ((Value & 0xFFFF0000) >> 16)

// nvme.h
/*
 * Get Features - Features Identifiers
 *
 * Section 5.9, Figure 53, Figure 54; Section 5.12.1, Figure 72, Figure 73
 */
#define ARBITRATION                         0x01
#define POWER_MANAGEMENT                    0x02
#define LBA_RANGE_TYPE                      0x03
#define TEMPERATURE_THRESHOLD               0x04
#define ERROR_RECOVERY                      0x05
#define VOLATILE_WRITE_CACHE                0x06
#define NUMBER_OF_QUEUES                    0x07
#define INTERRUPT_COALESCING                0x08
#define INTERRUPT_VECTOR_CONFIGURATION      0x09
#define WRITE_ATOMICITY                     0x0A
#define ASYNCHRONOUS_EVENT_CONFIGURATION    0x0B
#define SOFTWARE_PROGRESS_MARKER            0x80

// nvme.h
/*Opcodes for Admin Commands, Section 5, Figure 24 */
#define ADMIN_DELETE_IO_SUBMISSION_QUEUE                0x00
#define ADMIN_CREATE_IO_SUBMISSION_QUEUE                0x01
#define ADMIN_GET_LOG_PAGE                              0x02
#define ADMIN_DELETE_IO_COMPLETION_QUEUE                0x04
#define ADMIN_CREATE_IO_COMPLETION_QUEUE                0x05
#define ADMIN_IDENTIFY                                  0x06
#define ADMIN_ABORT                                     0x08
#define ADMIN_SET_FEATURES                              0x09
#define ADMIN_GET_FEATURES                              0x0A
#define ADMIN_ASYNCHRONOUS_EVENT_REQUEST                0x0C
#define ADMIN_FIRMWARE_ACTIVATE                         0x10
#define ADMIN_FIRMWARE_IMAGE_DOWNLOAD                   0x11

// nvme.h
/*
 * NVMe NVM Command Set
 *
 * Opcodes for NVM Commands, Section 6, Figure 98
 *    and FUSE methods
 */
#define NVM_READ                            0x02

// nvmeStd.h
/* Align buffer address to next system page boundary */
#define PAGE_ALIGN_BUF_ADDR(Address) \
    ((Address + PAGE_SIZE) & ~(PAGE_SIZE-1))

// nvmeIo.h
typedef enum _IO_SUBMIT_STATUS
{
    NOT_SUBMITTED = 0,
    SUBMITTED,
    BUSY
} IO_SUBMIT_STATUS;

// nvmeStd.h
#define NVME_ADMIN_MSG_ID           0

// nvmeSnti.h
/*******************************************************************************
 * SNTI_TRANSLATION_STATUS
 *
 * The SNTI_TRANSLATION_STATUS enumeration defines all possible status codes
 * for a translation sequence. 
 ******************************************************************************/
typedef enum _snti_translation_status
{
    SNTI_TRANSLATION_SUCCESS = 0,     /* Translation occurred w/o error */
    SNTI_COMMAND_COMPLETED,           /* Command completed in xlation phase */
    SNTI_SEQUENCE_IN_PROGRESS,        /* Command sequence still in progress */
    SNTI_SEQUENCE_COMPLETED,          /* Command sequence completed */
    SNTI_SEQUENCE_ERROR,              /* Error in command sequence */
    SNTI_FAILURE_CHECK_RESPONSE_DATA, /* Check SCSI status, device error */
    SNTI_UNSUPPORTED_SCSI_REQUEST,    /* Unsupported SCSI opcode */
    SNTI_UNSUPPORTED_SCSI_TM_REQUEST, /* Unsupported SCSI TM opcode */
    SNTI_INVALID_SCSI_REQUEST_PARM,   /* An invalid SCSI request parameter */
    SNTI_INVALID_SCSI_PATH_ID,        /* An invalid SCSI path id */
    SNTI_INVALID_SCSI_TARGET_ID,      /* An invalid SCSI target id */
    SNTI_UNRECOVERABLE_ERROR,         /* Unrecoverable error */
    SNTI_RESERVED                     /* Reserved for future use */
     
    /* TBD: Add additional codes as necessary */

} SNTI_TRANSLATION_STATUS;

// nvmeSnti.h
/******************************************************************************
 * SNTI_RESPONSE_BLOCK
 ******************************************************************************/
typedef struct _snti_response_block
{
    UINT8 SrbStatus;
    UINT8 StatusCode;
    UINT8 SenseKey;
    UINT8 ASC;
    UINT8 ASCQ;
} SNTI_RESPONSE_BLOCK, *PSNTI_RESPONSE_BLOCK;

// nvmeStd.h
#define DUMP_POLL_CALLS             3

// nvme.h
/* Interrupt Coalescing, Section 5.12.1.8, Figure 83, Feature Identifier 08h */
typedef struct _ADMIN_SET_FEATURES_COMMAND_INTERRUPT_COALESCING_DW11
{

    /*
     * [Aggregation Threshold] Specifies the desired minimum number of
     * completion queue entries to aggregate per interrupt vector before
     * signaling an interrupt to the host; the default value is 0h. This is a
     * 0抯 based value.
     */
    ULONG   THR     :8;

    /*
     * [Aggregation Time] Specifies the recommended maximum time in 100
     * microsecond increments that a controller may delay an interrupt due to
     * interrupt coalescing. A value of 0h corresponds to no delay (i.e.,
     * disabling this capability). The controller may apply this time per
     * interrupt vector or across all interrupt vectors.
     */
    ULONG   TIME     :8;
    ULONG   Reserved :16;
} ADMIN_SET_FEATURES_COMMAND_INTERRUPT_COALESCING_DW11,
  *PADMIN_SET_FEATURES_COMMAND_INTERRUPT_COALESCING_DW11;

// nvme.h
/* Number of Queues, Section 5.12.1.7, Figure 81, Feature Identifier 07h */
typedef struct _ADMIN_SET_FEATURES_COMMAND_NUMBER_OF_QUEUES_DW11
{
    /*
     * [Number of I/O Completion Queues Requested] Indicates the number of I/O
     * Completion Queues requested by software. This number does not include the
     * Admin Completion Queue. A minimum of one shall be requested, reflecting
     * that the minimum support is for one I/O Completion Queue. This is a 0抯
     * based value.
     */
    ULONG   NCQR     :16;

    /*
     * [Number of I/O Submission Queues Requested] Indicates the number of I/O
     * Submission Queues requested by software. This number does not include the
     * Admin Submission Queue. A minimum of one shall be requested, reflecting
     * that the minimum support is for one I/O Submission Queue. This is a 0抯
     * based value.
     */
    ULONG   NSQR     :16;
} ADMIN_SET_FEATURES_COMMAND_NUMBER_OF_QUEUES_DW11,
  *PADMIN_SET_FEATURES_COMMAND_NUMBER_OF_QUEUES_DW11;

// nvmeStd.h
#define LBA_TYPE_FILESYSTEM 1

// nvme.h
/* Create I/O Completion Queue Command, Section 5.3, Figure 33, Opcode 0x05 */
typedef struct _ADMIN_CREATE_IO_COMPLETION_QUEUE_DW10
{
    /*
     * [Queue Identifier] This field indicates the identifier to assign to the
     * Completion Queue to be created. This identifier corresponds to the
     * Completion Queue Head Doorbell used for this command (i.e., the value y).
     * This value shall not exceed the value reported in the Number of Queues
     * feature for I/O Completion Queues.
     */
    USHORT  QID;

    /*
     * [Queue Size] This field indicates the size of the Completion Queue to be
     * created. Refer to section 4.1.3. This is a 0抯 based value.
     */
    USHORT  QSIZE;
} ADMIN_CREATE_IO_COMPLETION_QUEUE_DW10,
  *PADMIN_CREATE_IO_COMPLETION_QUEUE_DW10;

// nvme.h
/* Create I/O Completion Queue Command, Section 5.3, Figure 34 */
typedef struct _ADMIN_CREATE_IO_COMPLETION_QUEUE_DW11
{
    /*
     * [Physically Contiguous] If set to ?1?, then the Completion Queue is
     * physically contiguous and PRP Entry 1 (PRP1) is the address of a
     * contiguous physical buffer. If cleared to ?0?, then the Completion Queue
     * is not physically contiguous and PRP Entry 1 (PRP1) is a PRP List
     * pointer.
     */
    ULONG   PC       :1;

    /*
     * [Interrupts Enabled] If set to ?1?, then interrupts are enabled for this
     * Completion Queue. If cleared to ?0?, then interrupts are disabled for
     * this Completion Queue.
     */
    ULONG   IEN      :1;
    ULONG   Reserved :14;

    /*
     * [Interrupt Vector] This field indicates interrupt vector to use for this
     * Completion Queue. This corresponds to the MSI-X or multiple message MSI
     * vector to use. If using single message MSI or pin-based interrupts, then
     * this field shall be cleared to 0h. In MSI-X, a maximum of 2K vectors are
     * used. This value shall not be set to a value greater than the number of
     * messages the controller supports (refer to MSICAP.MC.MME or
     * MSIXCAP.MXC.TS).
     */
    ULONG   IV       :16;
} ADMIN_CREATE_IO_COMPLETION_QUEUE_DW11,
  *PADMIN_CREATE_IO_COMPLETION_QUEUE_DW11;

// nvme.h
/* Create I/O Submission Queue Command, Section 5.4, Figure 37, Opcode 0x01 */
typedef struct _ADMIN_CREATE_IO_SUBMISSION_QUEUE_DW10
{
    /*
     * [Queue Identifier] This field indicates the identifier to assign to the
     * Submission Queue to be created. This identifier corresponds to the
     * Submission Queue Tail Doorbell used for this command (i.e., the value y).
     * This value shall not exceed the value reported in the Number of Queues
     * feature for I/O Submission Queues.
     */
    USHORT  QID;

    /*
     * [Queue Size] This field indicates the size of the Submission Queue to be
     * created. Refer to section 4.1.3. This is a 0抯 based value.
     */
    USHORT  QSIZE;
} ADMIN_CREATE_IO_SUBMISSION_QUEUE_DW10,
  *PADMIN_CREATE_IO_SUBMISSION_QUEUE_DW10;

// nvme.h
/* Create I/O Submission Queue Command, Section 5.4, Figure 38 */
typedef struct _ADMIN_CREATE_IO_SUBMISSION_QUEUE_DW11
{
    /*
     * [Physically Contiguous] If set to ?1?, then the Submission Queue is
     * physically contiguous and PRP Entry 1 (PRP1) is the address of a
     * contiguous physical buffer. If cleared to ?0?, then the Submission Queue
     * is not physically contiguous and PRP Entry 1 (PRP1) is a PRP List
     * pointer.
     */
    ULONG   PC      :1;

    /*
     * [Queue Priority] This field indicates the priority service class to use
     * for commands within this Submission Queue. This field is only used when
     * the weighted round robin with an urgent priority service class is the
     * arbitration mechanism is selected. Refer to section 4.7. Value 00b ==
     * Urgent, Value 01b == High, Value 10b == Medium, Value 11b == Low.
     */
    ULONG   QPRIO   :2;
    ULONG   Reserved:13;

    /*
     * [Completion Queue Identifier] This field indicates the identifier of the
     * Completion Queue to utilize for any command completions entries
     * associated with this Submission Queue. The value of 0h (Admin Completion
     * Queue) shall not be specified.
     */
    ULONG   CQID    :16;
} ADMIN_CREATE_IO_SUBMISSION_QUEUE_DW11,
  *PADMIN_CREATE_IO_SUBMISSION_QUEUE_DW11;

// nvme.h
/* Delete I/O Submission Queue Command, Section 5.6, Figure 42, Opcode 0x00 */
typedef struct _ADMIN_DELETE_IO_SUBMISSION_QUEUE_DW10
{
    /*
     * [Queue Identifier] This field indicates the identifier of the Submission
     * Queue to be deleted. The value of 0h (Admin Submission Queue) shall not
     * be specified.
     */
    USHORT  QID;
    USHORT  Reserved;
} ADMIN_DELETE_IO_SUBMISSION_QUEUE_DW10,
  *PADMIN_DELETE_IO_SUBMISSION_QUEUE_DW10;

// nvme.h
/* Delete I/O Completion Queue Command, Section 5.5, Figure 40, Opcode 0x04 */
typedef struct _ADMIN_DELETE_IO_COMPLETION_QUEUE_DW10
{
    /*
     * [Queue Identifier] This field indicates the identifier of the Completion
     * Queue to be deleted. The value of 0h (Admin Completion Queue) shall not
     * be specified.
     */
    USHORT  QID;
    USHORT  Reserved;
} ADMIN_DELETE_IO_COMPLETION_QUEUE_DW10,
  *PADMIN_DELETE_IO_COMPLETION_QUEUE_DW10;

// nvme.h
/* Read Command, Section 6.8, Figure 120, Opcode 0x02 */
typedef struct _NVM_READ_COMMAND_DW12
{
    /*
     * [Number of Logical Blocks] This field indicates the number of logical
     * blocks to be read.  This is a 0抯 based value.
     */
    USHORT  NLB;
    USHORT  Reserved    :10;

    /*
     * [Protection Information Field] Specifies the protection information
     * action and check field, as defined in Figure 100.
     */
    USHORT  PRINFO      :4;

    /*
     * [Force Unit Access] This field indicates that the data read shall be
     * returned from non-volatile media.  There is no implied ordering with
     * other commands.
     */
    USHORT  FUA         :1;

    /*
     * [Limited Retry] If set to ?1?, the controller should apply limited retry
     * efforts.  If cleared to ?0?, the controller should apply all available
     * error recovery means to return the data to the host.
     */
    USHORT  LR          :1;
} NVM_READ_COMMAND_DW12, *PNVM_READ_COMMAND_DW12;

// nvme.h
/* Read Command, Section 6.8, Figure 121 */
typedef struct _NVM_READ_COMMAND_DW13
{
    /*
     * [Dataset Management] This field indicates attributes for the dataset that
     * the LBA(s) being read are associated with.
     */
    struct
    {
        /*
         * Value 0000b == No frequency information provided. Value 0001b ==
         * Typical number of reads and writes expected for this LBA range. Value
         * 0010b == Infrequent writes and infrequent reads to the LBA range
         * indicated. Value 0011b == Infrequent writes and frequent reads to the
         * LBA range indicated. Value 0100b == Frequent writes and infrequent
         * reads to the LBA range indicated. Value 0101b == Frequent writes and
         * frequent reads to the LBA range indicated. Value 0110b == One time
         * read. E.g. command is due to virus scan, backup, file copy, or
         * archive. Value 0111b == Speculative read.  The command is part of a
         * prefetch operation. Value 1000b == The LBA range is going to be
         * overwritten in the near future. Value 1001b ? 1111b == Reserved.
         */
        UCHAR   AccessFrequency     :4;

        /*
         * Value 00b == None. No latency information provided. Value 01b ==
         * Idle. Longer latency acceptable. Value == 10b Normal. Typical
         * latency. Value 11b == Low. Smallest possible latency.
         */
        UCHAR   AccessLatency       :2;

        /*
         * If set to ?1?, then this command is part of a sequential read that
         * includes multiple Read commands.  If cleared to ?0?, then no
         * information on sequential access is provided.
         */
        UCHAR   SequentialRequest   :1;

        /*
         * If set to ?1?, then data is not compressible for the logical blocks
         * indicated.  If cleared to ?0?, then no information on compression is
         * provided.
         */
        UCHAR   Incompressible      :1;
    } DSM;

    UCHAR   Reserved[3];
} NVM_READ_COMMAND_DW13, *PNVM_READ_COMMAND_DW13;

// nvme.h
/* Read Command, Section 6.8, Figure 123 */
typedef struct _NVM_READ_COMMAND_DW15
{
    /*
     * [Expected Logical Block Application Tag] This field indicates the
     * Application Tag expected value.  This field is only used if the namespace
     * is formatted to use end-to-end protection information.  Refer to section
     * 8.2.
     */
    USHORT  ELBAT;

    /*
     * [Expected Logical Block Application Tag Mask] This field indicates the
     * Application Tag Mask expected value.  This field is only used if the
     * namespace is formatted to use end-to-end protection information.  Refer
     * to section 8.2.
     */
    USHORT  ELBATM;
} NVM_READ_COMMAND_DW15, *PNVM_READ_COMMAND_DW15;

// nvmeSnti.h
/*******************************************************************************
 * SNTI_STATUS
 *
 * The SNTI_STATUS enumeration defines all possible "internal" status codes
 * for a translation sequence.
 ******************************************************************************/
typedef enum _snti_status
{
    SNTI_SUCCESS = 0,
    SNTI_FAILURE,
    SNTI_INVALID_REQUEST,
    SNTI_INVALID_PARAMETER,
    SNTI_INVALID_PATH_TARGET_ID,
    SNTI_NO_MEMORY,
    SNTI_COMPLETE_CMD
    /* TBD: Add fields as necessary */

} SNTI_STATUS;

// nvmeStd.h
#define MICRO_TO_SEC                1000


//------------------------------------------------------------------------------
// original NVMe OFA driver code ends here
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// our custom stuff starts here
//------------------------------------------------------------------------------


//--------------------------------------------------------------------------------
// structure for the global NVMe information
//--------------------------------------------------------------------------------
typedef struct _NVME_GLOBALS
{
	// number of found controllers and number of the following NVME_DEVICE_EXTENSION structures
	USHORT FoundControllers;
	// pointer to 1st NVME_DEVICE_EXTENSION structure
	NVME_DEVICE_EXTENSION *DevExt;
	// selected BIOS drive number
	SHORT SelectedDrive;
	// selected NVMe controller
	SHORT SelectedController;
	// show all found controllers and connected devices
	USHORT ShowAll;
	// show selected controller and connected device
	USHORT ShowSelected;
	// uninitialize all controllers
	USHORT Uninit;
	// start sector for drive mapping
	ULONGLONG StartSector;
	// drive mapping is active
	USHORT MapActive;
}NVME_GLOBALS,*PNVME_GLOBALS;


//--------------------------------------------------------------------------------
// functions that can be called from extern
//--------------------------------------------------------------------------------
int NVMeRead(PNVME_DEVICE_EXTENSION pAE,QWORD startLBA,QWORD buf,ULONG count);
int NVMeInit(NVME_DEVICE_EXTENSION **DevExt,unsigned short int *foundControllers);
void NVMeUninit(NVME_DEVICE_EXTENSION **DevExt,unsigned short int *foundControllers);
int NVMeRawRead(unsigned long drive,unsigned long long sector,unsigned long byte_offset,unsigned long long byte_len,unsigned long long buf,unsigned long write);
void NVMeShowAllDevices(NVME_DEVICE_EXTENSION *DevExt,unsigned short int foundControllers);
void NVMeShowSelectedDevice(NVME_DEVICE_EXTENSION *DevExt,unsigned short int i);


//--------------------------------------------------------------------------------
// global structure that is used from extern
//--------------------------------------------------------------------------------
extern NVME_GLOBALS nvmeg;

#endif // _INC_NVME


//------------------------------------------------------------------------------
// our custom stuff ends here
//------------------------------------------------------------------------------

// restore GCC options
#pragma GCC pop_options

