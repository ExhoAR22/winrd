#include <wdm.h>

#include "Common.h"
#include "MajorFunctions.h"

PDEVICE_OBJECT DiskDevice = NULL;
UNICODE_STRING SymlinkName;

void DriverUnload(PDRIVER_OBJECT DriverObject) {
	UNREFERENCED_PARAMETER(DriverObject);
	TRACE("Unloading...");
	IoDeleteSymbolicLink(&SymlinkName);
	IoDeleteDevice(DiskDevice);
	FreeStorage();
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	UNREFERENCED_PARAMETER(RegistryPath);
	DriverObject->DriverUnload = DriverUnload;

	TRACE("Initializing...");

	NTSTATUS Status = STATUS_SUCCESS;
	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
		DriverObject->MajorFunction[i] = MajorFunctionGenericSuccess;
	}

	DriverObject->MajorFunction[IRP_MJ_READ] = MajorFunctionRead;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = MajorFUnctionWrite;
	DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] = MajorFunctionQueryVolumeInformation;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = MajorFunctionIoctl;

	CHECK(AllocateStorage());

	UNICODE_STRING DiskDeviceName;
	RtlInitUnicodeString(&DiskDeviceName, L"\\Device\\WinRD");
	CHECK(IoCreateDevice(DriverObject, 0, &DiskDeviceName, FILE_DEVICE_DISK, 0, FALSE, &DiskDevice));

	DiskDevice->Flags |= DO_BUFFERED_IO;
	DiskDevice->Flags &= ~DO_DEVICE_INITIALIZING;

	RtlInitUnicodeString(&SymlinkName, L"\\DosDevices\\X:");
	CHECK(IoCreateSymbolicLink(&SymlinkName, &DiskDeviceName));

Cleanup:
	return Status;
}