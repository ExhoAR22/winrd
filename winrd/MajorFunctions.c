#include "MajorFunctions.h"
#include "Common.h"

#include <ntdddisk.h>
#include <mountmgr.h>
#include <mountdev.h>

const UINT64 DISK_SIZE_IN_BYTES = 1024 * 1024 * 128;
const ULONG SECTOR_SIZE = 512;

UINT8* Storage = NULL;

NTSTATUS AllocateStorage() {
    Storage = ExAllocatePoolWithTag(PagedPool, DISK_SIZE_IN_BYTES, 'RAMD');
    if (!Storage) {
        return STATUS_UNSUCCESSFUL;
    } else {
        return STATUS_SUCCESS;
    }
}

void FreeStorage() {
    ExFreePool(Storage);
}

NTSTATUS MajorFunctionGenericSuccess(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    UNREFERENCED_PARAMETER(DeviceObject);
    PIO_STACK_LOCATION CurrentIoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    TRACE("Generic success for major function %u", (ULONG)CurrentIoStackLocation->MajorFunction);
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS MajorFunctionRead(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    UNREFERENCED_PARAMETER(DeviceObject);
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION CurrentIoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    if (!Irp->AssociatedIrp.SystemBuffer) {
        void* Buffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
        RtlCopyMemory(Buffer, (VOID*)(Storage + CurrentIoStackLocation->Parameters.Read.ByteOffset.QuadPart), CurrentIoStackLocation->Parameters.Read.Length);
    } else {
        RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer, (VOID*)(Storage + CurrentIoStackLocation->Parameters.Read.ByteOffset.QuadPart), CurrentIoStackLocation->Parameters.Read.Length);
    }
    Irp->IoStatus.Information = CurrentIoStackLocation->Parameters.Read.Length;
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

NTSTATUS MajorFUnctionWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    UNREFERENCED_PARAMETER(DeviceObject);
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION CurrentIoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    if (!Irp->AssociatedIrp.SystemBuffer) {
        void* Buffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
        RtlCopyMemory((VOID*)(Storage + CurrentIoStackLocation->Parameters.Write.ByteOffset.QuadPart), Buffer, CurrentIoStackLocation->Parameters.Write.Length);
    }
    else {
        RtlCopyMemory((VOID*)(Storage + CurrentIoStackLocation->Parameters.Write.ByteOffset.QuadPart), Irp->AssociatedIrp.SystemBuffer, CurrentIoStackLocation->Parameters.Write.Length);
    }
    Irp->IoStatus.Information = CurrentIoStackLocation->Parameters.Write.Length;
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

NTSTATUS MajorFunctionQueryVolumeInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);
    TRACE("QueryVolumeInformation");
	Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_INVALID_DEVICE_REQUEST;
}

NTSTATUS MajorFunctionIoctl(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    UNREFERENCED_PARAMETER(DeviceObject);
    PIO_STACK_LOCATION CurrentIoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    switch (CurrentIoStackLocation->Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_DISK_GET_DRIVE_LAYOUT:
        if (CurrentIoStackLocation->Parameters.DeviceIoControl.OutputBufferLength < sizeof(DRIVE_LAYOUT_INFORMATION)) {
            Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
            Irp->IoStatus.Information = 0;
        } else {
            PDRIVE_LAYOUT_INFORMATION OutputBuffer = (PDRIVE_LAYOUT_INFORMATION)Irp->AssociatedIrp.SystemBuffer;
            OutputBuffer->PartitionCount = 1;
            OutputBuffer->Signature = 0;
            OutputBuffer->PartitionEntry->PartitionType = PARTITION_ENTRY_UNUSED;
            OutputBuffer->PartitionEntry->BootIndicator = FALSE;
            OutputBuffer->PartitionEntry->RecognizedPartition = TRUE;
            OutputBuffer->PartitionEntry->RewritePartition = FALSE;
            OutputBuffer->PartitionEntry->StartingOffset.QuadPart = 0;
            OutputBuffer->PartitionEntry->PartitionLength.QuadPart = DISK_SIZE_IN_BYTES;
            OutputBuffer->PartitionEntry->HiddenSectors = 1L;
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = sizeof(PARTITION_INFORMATION);
        }
        break;
    case IOCTL_DISK_IS_WRITABLE:
    case IOCTL_DISK_MEDIA_REMOVAL:
    case IOCTL_STORAGE_MEDIA_REMOVAL:
    case IOCTL_DISK_CHECK_VERIFY:
    case IOCTL_STORAGE_CHECK_VERIFY:
    case IOCTL_STORAGE_CHECK_VERIFY2:
        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;
        break;
    case IOCTL_DISK_GET_DRIVE_GEOMETRY:
        if (CurrentIoStackLocation->Parameters.DeviceIoControl.OutputBufferLength < sizeof(DISK_GEOMETRY)) {
            Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
            Irp->IoStatus.Information = 0;
            break;
        }

        PDISK_GEOMETRY DiskGeometry = (PDISK_GEOMETRY)Irp->AssociatedIrp.SystemBuffer;
        DiskGeometry->Cylinders.QuadPart = DISK_SIZE_IN_BYTES / SECTOR_SIZE / 0x20 / 0x80;
        DiskGeometry->MediaType = FixedMedia;
        DiskGeometry->TracksPerCylinder = 0x80;
        DiskGeometry->SectorsPerTrack = 0x20;
        DiskGeometry->BytesPerSector = SECTOR_SIZE;
        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = sizeof(DISK_GEOMETRY);
        break;
    case IOCTL_DISK_GET_LENGTH_INFO:
        if (CurrentIoStackLocation->Parameters.DeviceIoControl.OutputBufferLength < sizeof(GET_LENGTH_INFORMATION)) {
            Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
            Irp->IoStatus.Information = 0;
            break;
        }

        PGET_LENGTH_INFORMATION GetLengthInfo = (PGET_LENGTH_INFORMATION)Irp->AssociatedIrp.SystemBuffer;
        GetLengthInfo->Length.QuadPart = DISK_SIZE_IN_BYTES;
        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = sizeof(GET_LENGTH_INFORMATION);
        break;
    case IOCTL_DISK_GET_PARTITION_INFO:
        if (CurrentIoStackLocation->Parameters.DeviceIoControl.OutputBufferLength < sizeof(PARTITION_INFORMATION)) {
            Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
            Irp->IoStatus.Information = 0;
            break;
        }

        PPARTITION_INFORMATION PartitionInfo = (PPARTITION_INFORMATION)Irp->AssociatedIrp.SystemBuffer;
        PartitionInfo->StartingOffset.QuadPart = 0;
        PartitionInfo->PartitionLength.QuadPart = DISK_SIZE_IN_BYTES;
        PartitionInfo->HiddenSectors = 0;
        PartitionInfo->PartitionNumber = 0;
        PartitionInfo->PartitionType = 0;
        PartitionInfo->BootIndicator = FALSE;
        PartitionInfo->RecognizedPartition = TRUE;
        PartitionInfo->RewritePartition = FALSE;
        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = sizeof(PARTITION_INFORMATION);
        break;
    case IOCTL_DISK_GET_PARTITION_INFO_EX:
        if (CurrentIoStackLocation->Parameters.DeviceIoControl.OutputBufferLength < sizeof(PARTITION_INFORMATION_EX)) {
            Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
            Irp->IoStatus.Information = 0;
            break;
        }

        PPARTITION_INFORMATION_EX PartitionInfoEx = (PPARTITION_INFORMATION_EX)Irp->AssociatedIrp.SystemBuffer;
        PartitionInfoEx->PartitionStyle = PARTITION_STYLE_MBR;
        PartitionInfoEx->StartingOffset.QuadPart = 0;
        PartitionInfoEx->PartitionLength.QuadPart = DISK_SIZE_IN_BYTES;
        PartitionInfoEx->PartitionNumber = 0;
        PartitionInfoEx->RewritePartition = FALSE;
        PartitionInfoEx->Mbr.PartitionType = 0;
        PartitionInfoEx->Mbr.BootIndicator = FALSE;
        PartitionInfoEx->Mbr.RecognizedPartition = TRUE;
        PartitionInfoEx->Mbr.HiddenSectors = 0;
        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = sizeof(PARTITION_INFORMATION_EX);
        break;
    case IOCTL_DISK_SET_PARTITION_INFO:
        if (CurrentIoStackLocation->Parameters.DeviceIoControl.InputBufferLength < sizeof(SET_PARTITION_INFORMATION)) {
            Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
            Irp->IoStatus.Information = 0;
            break;
        }

        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;
        break;
    case IOCTL_DISK_VERIFY:
        if (CurrentIoStackLocation->Parameters.DeviceIoControl.InputBufferLength < sizeof(VERIFY_INFORMATION)) {
            Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
            Irp->IoStatus.Information = 0;
            break;
        }

        PVERIFY_INFORMATION VerifyInfo = (PVERIFY_INFORMATION)Irp->AssociatedIrp.SystemBuffer;
        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = VerifyInfo->Length;
        break;
    case IOCTL_MOUNTDEV_QUERY_DEVICE_NAME:
        if (CurrentIoStackLocation->Parameters.DeviceIoControl.OutputBufferLength < sizeof(MOUNTDEV_NAME)) {
            Irp->IoStatus.Information = sizeof(MOUNTDEV_NAME);
            Irp->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
        } else {
            PMOUNTDEV_NAME MountedDeviceName = (PMOUNTDEV_NAME)Irp->AssociatedIrp.SystemBuffer;
            UNICODE_STRING DeviceName;
            RtlInitUnicodeString(&DeviceName, L"\\Device\\WinRD");

            MountedDeviceName->NameLength = DeviceName.Length;
            ULONG OutputLength = sizeof(USHORT) + DeviceName.Length;
            if (CurrentIoStackLocation->Parameters.DeviceIoControl.OutputBufferLength < OutputLength)
            {
                Irp->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
                Irp->IoStatus.Information = sizeof(MOUNTDEV_NAME);
                break;
            }

            RtlCopyMemory(MountedDeviceName->Name, DeviceName.Buffer, DeviceName.Length);
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = OutputLength;
        }
        break;
    case IOCTL_MOUNTDEV_QUERY_UNIQUE_ID:
        if (CurrentIoStackLocation->Parameters.DeviceIoControl.OutputBufferLength < sizeof(MOUNTDEV_UNIQUE_ID)) {
            Irp->IoStatus.Information = sizeof(MOUNTDEV_UNIQUE_ID);
            Irp->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
        } else {
            PMOUNTDEV_UNIQUE_ID MountedDeviceId = (PMOUNTDEV_UNIQUE_ID)Irp->AssociatedIrp.SystemBuffer;
            UNICODE_STRING UniqueId;
            RtlInitUnicodeString(&UniqueId, L"\\Device\\WinRD");

            MountedDeviceId->UniqueIdLength = UniqueId.Length;
            ULONG OutputLength = sizeof(USHORT) + UniqueId.Length;
            if (CurrentIoStackLocation->Parameters.DeviceIoControl.OutputBufferLength < OutputLength) {
                Irp->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
                Irp->IoStatus.Information = sizeof(MOUNTDEV_UNIQUE_ID);
                break;
            }

            RtlCopyMemory(MountedDeviceId->UniqueId, UniqueId.Buffer, UniqueId.Length);
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = OutputLength;
        }
        break;
    case IOCTL_MOUNTDEV_QUERY_STABLE_GUID:
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Information = 0;
        break;
    case IOCTL_STORAGE_GET_HOTPLUG_INFO:
        if (CurrentIoStackLocation->Parameters.DeviceIoControl.OutputBufferLength < sizeof(STORAGE_HOTPLUG_INFO)) {
            Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        PSTORAGE_HOTPLUG_INFO HotplugInfo = (PSTORAGE_HOTPLUG_INFO)Irp->AssociatedIrp.SystemBuffer;
        RtlZeroMemory(HotplugInfo, sizeof(STORAGE_HOTPLUG_INFO));
        HotplugInfo->Size = sizeof(STORAGE_HOTPLUG_INFO);
        HotplugInfo->MediaRemovable = 1;
        Irp->IoStatus.Information = sizeof(STORAGE_HOTPLUG_INFO);
        Irp->IoStatus.Status = STATUS_SUCCESS;
        break;
    default:
        TRACE("Unknown Ioctl code %x", CurrentIoStackLocation->Parameters.DeviceIoControl.IoControlCode);
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Information = 0;
    }

    NTSTATUS Status = Irp->IoStatus.Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}