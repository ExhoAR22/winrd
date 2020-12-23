#pragma once
#include <wdm.h>

NTSTATUS AllocateStorage();
void FreeStorage();

NTSTATUS MajorFunctionGenericSuccess(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS MajorFunctionRead(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS MajorFUnctionWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS MajorFunctionQueryVolumeInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS MajorFunctionIoctl(PDEVICE_OBJECT DeviceObject, PIRP Irp);