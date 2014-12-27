#include <ntddk.h>
#include "stdio.h"
#include "hwinterfacedrv.h"


NTSTATUS hwinterfaceDeviceControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
VOID hwinterfaceUnload(IN PDRIVER_OBJECT DriverObject);
NTSTATUS UnmapPhysicalMemory(HANDLE PhysicalMemoryHandle, PVOID pPhysMemLin);
NTSTATUS MapPhysicalMemoryToLinearSpace(PVOID pPhysAddress,
                                        SIZE_T PhysMemSizeInBytes,
                                        PVOID *ppPhysMemLin,
                                        HANDLE *pPhysicalMemoryHandle);


NTSTATUS hwinterfaceCreateDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{


    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
    PDEVICE_OBJECT deviceObject;
    NTSTATUS status;
#ifdef _AMD64_
    WCHAR NameBuffer[] = L"\\Device\\inpoutx64";
    WCHAR DOSNameBuffer[] = L"\\DosDevices\\inpoutx64";
#else
    WCHAR NameBuffer[] = L"\\Device\\inpout32";
    WCHAR DOSNameBuffer[] = L"\\DosDevices\\inpout32";
#endif
    UNICODE_STRING uniNameString, uniDOSString;

    RtlInitUnicodeString(&uniNameString, NameBuffer);
    RtlInitUnicodeString(&uniDOSString, DOSNameBuffer);

    status = IoCreateDevice(DriverObject, 
                            0,
                            &uniNameString,
                            FILE_DEVICE_UNKNOWN,
                            0, 
                            FALSE, 
                            &deviceObject);

    if(!NT_SUCCESS(status))
        return status;

    status = IoCreateSymbolicLink (&uniDOSString, &uniNameString);

    if (!NT_SUCCESS(status))
        return status;

    DriverObject->MajorFunction[IRP_MJ_CREATE] = hwinterfaceCreateDispatch;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = hwinterfaceDeviceControl;
    DriverObject->DriverUnload = hwinterfaceUnload;

    return STATUS_SUCCESS;
}

NTSTATUS hwinterfaceDeviceControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP pIrp)
{
    PIO_STACK_LOCATION  stkloc;
    NTSTATUS            ntStatus = STATUS_SUCCESS; 
	struct              tagPhys32Struct Phys32Struct;
	
    PUCHAR              cData;
	PUSHORT             sData;
	PULONG				lData;
	PUSHORT             address;

	ULONG               inBuffersize;   
    ULONG               outBuffersize;  
    ULONG               inBuf;         
    PVOID               CtrlBuff;

    stkloc = IoGetCurrentIrpStackLocation( pIrp );
    inBuffersize = stkloc->Parameters.DeviceIoControl.InputBufferLength;
    outBuffersize = stkloc->Parameters.DeviceIoControl.OutputBufferLength;

    CtrlBuff    = pIrp->AssociatedIrp.SystemBuffer;
    cData  = (PUCHAR) CtrlBuff;
	sData  = (PUSHORT) CtrlBuff;
	lData  = (PULONG) CtrlBuff;
    address = (PUSHORT) CtrlBuff;

    switch ( stkloc->Parameters.DeviceIoControl.IoControlCode )
     {
  		case IOCTL_READ_PORT_UCHAR:
            if ((inBuffersize >= 2) && (outBuffersize >= 1)) 
			{
				UCHAR value;
				value = READ_PORT_UCHAR((PUCHAR)address[0]);
				cData[0] = value;
            } 
			else 
			{	
				ntStatus = STATUS_BUFFER_TOO_SMALL;
			}
			pIrp->IoStatus.Information = sizeof(UCHAR); 
            ntStatus = STATUS_SUCCESS;

            break;

		case IOCTL_WRITE_PORT_UCHAR:
            if (inBuffersize >= 3) 
			{
				WRITE_PORT_UCHAR((PUCHAR)address[0], cData[2]);	//Byte 0,1=Address Byte 2=Value
				pIrp->IoStatus.Information = 10;
            } 
			else 
			{
				ntStatus = STATUS_BUFFER_TOO_SMALL;
				pIrp->IoStatus.Information = 0; 
				ntStatus = STATUS_SUCCESS;
			}
            break;
		case IOCTL_READ_PORT_USHORT:
            if ((inBuffersize >= 2) && (outBuffersize >= 2)) 
			{
                USHORT value;
				value = READ_PORT_USHORT((PUSHORT)address[0]);
				sData[0] = value;
            } 
			else 
			{	
				ntStatus = STATUS_BUFFER_TOO_SMALL;
			}
			pIrp->IoStatus.Information = sizeof(USHORT); 
            ntStatus = STATUS_SUCCESS;
			break;
		case IOCTL_WRITE_PORT_USHORT:
			if (inBuffersize >= 4) 
			{
                WRITE_PORT_USHORT((PUSHORT)address[0], sData[1]); //Short 0=Address Short 1=Value
				pIrp->IoStatus.Information = 10;
            } 
			else 
			{
				ntStatus = STATUS_BUFFER_TOO_SMALL;
				pIrp->IoStatus.Information = 0; 
				ntStatus = STATUS_SUCCESS;
			}
			break;
		case IOCTL_READ_PORT_ULONG:
			if ((inBuffersize >= 4) && (outBuffersize >= 4)) 
			{
				ULONG value;
				value = READ_PORT_ULONG((PULONG)address[0]);
				lData[0] = value;
            } 
			else 
			{	
				ntStatus = STATUS_BUFFER_TOO_SMALL;
			}
			pIrp->IoStatus.Information = sizeof(ULONG); 
            ntStatus = STATUS_SUCCESS;
			break;
		case IOCTL_WRITE_PORT_ULONG:
            if (inBuffersize >= 8) 
			{
				WRITE_PORT_ULONG(&(lData[0]), lData[1]); //Short 0=Address long 1=Value
				pIrp->IoStatus.Information = 10;
            } 
			else 
			{
				ntStatus = STATUS_BUFFER_TOO_SMALL;
				pIrp->IoStatus.Information = 0; 
				ntStatus = STATUS_SUCCESS;
			}
			break;

		case IOCTL_WINIO_MAPPHYSTOLIN:
			if (inBuffersize)
			{
				memcpy (&Phys32Struct, CtrlBuff, inBuffersize);
	            ntStatus = MapPhysicalMemoryToLinearSpace(Phys32Struct.pvPhysAddress,
		                                                  Phys32Struct.dwPhysMemSizeInBytes,
			                                              &Phys32Struct.pvPhysMemLin,
				                                          &Phys32Struct.PhysicalMemoryHandle);

				if (NT_SUCCESS(ntStatus))
				{
				  memcpy (CtrlBuff, &Phys32Struct, inBuffersize);
				  pIrp->IoStatus.Information = inBuffersize;
				}

				pIrp->IoStatus.Status = ntStatus;
			}
			else
				pIrp->IoStatus.Status = STATUS_INVALID_PARAMETER;
			break;

        case IOCTL_WINIO_UNMAPPHYSADDR:
			if (inBuffersize)
			{
				memcpy (&Phys32Struct, CtrlBuff, inBuffersize);
				ntStatus = UnmapPhysicalMemory(Phys32Struct.PhysicalMemoryHandle, Phys32Struct.pvPhysMemLin);
				pIrp->IoStatus.Status = ntStatus;
			}
			else
				pIrp->IoStatus.Status = STATUS_INVALID_PARAMETER;
			break;

		default:
            ntStatus = STATUS_UNSUCCESSFUL;
            pIrp->IoStatus.Information = 0;
            break;

    }
    pIrp->IoStatus.Status = ntStatus;
    IoCompleteRequest( pIrp, IO_NO_INCREMENT );
    return ntStatus;
}

VOID hwinterfaceUnload(IN PDRIVER_OBJECT DriverObject)
{
#ifdef _AMD64_
    WCHAR DOSNameBuffer[] = L"\\DosDevices\\inpoutx64";
#else
    WCHAR DOSNameBuffer[] = L"\\DosDevices\\inpout32";
#endif
    UNICODE_STRING uniDOSString;

    RtlInitUnicodeString(&uniDOSString, DOSNameBuffer);
    IoDeleteSymbolicLink (&uniDOSString);
    IoDeleteDevice(DriverObject->DeviceObject);
}

NTSTATUS MapPhysicalMemoryToLinearSpace(PVOID pPhysAddress,
                                        SIZE_T PhysMemSizeInBytes,
                                        PVOID *ppPhysMemLin,
                                        HANDLE *pPhysicalMemoryHandle)
{
  UNICODE_STRING     PhysicalMemoryUnicodeString;
  PVOID              PhysicalMemorySection = NULL;
  OBJECT_ATTRIBUTES  ObjectAttributes;
  PHYSICAL_ADDRESS   ViewBase;
  NTSTATUS           ntStatus;
  PHYSICAL_ADDRESS   pStartPhysAddress;
  PHYSICAL_ADDRESS   pEndPhysAddress;
  PHYSICAL_ADDRESS   MappingLength;
  BOOLEAN            Result1, Result2;
  ULONG              IsIOSpace;
  unsigned char     *pbPhysMemLin = NULL;

  RtlInitUnicodeString (&PhysicalMemoryUnicodeString, L"\\Device\\PhysicalMemory");
  InitializeObjectAttributes (&ObjectAttributes,
                              &PhysicalMemoryUnicodeString,
                              OBJ_CASE_INSENSITIVE,
                              (HANDLE) NULL,
                              (PSECURITY_DESCRIPTOR) NULL);

  *pPhysicalMemoryHandle = NULL;
  ntStatus = ZwOpenSection (pPhysicalMemoryHandle,
                            SECTION_ALL_ACCESS,
                            &ObjectAttributes);

  if (NT_SUCCESS(ntStatus))
  {
    ntStatus = ObReferenceObjectByHandle (*pPhysicalMemoryHandle,
                                          SECTION_ALL_ACCESS,
                                          (POBJECT_TYPE) NULL,
                                          KernelMode,
                                          &PhysicalMemorySection,
                                          (POBJECT_HANDLE_INFORMATION) NULL);

    if (NT_SUCCESS(ntStatus))
    {
      pStartPhysAddress.QuadPart = (ULONGLONG)pPhysAddress;
      pEndPhysAddress.QuadPart = pStartPhysAddress.QuadPart + (LONGLONG)PhysMemSizeInBytes;
      IsIOSpace = 0;
      Result1 = HalTranslateBusAddress (1, 0, pStartPhysAddress, &IsIOSpace, &pStartPhysAddress);
      IsIOSpace = 0;
      Result2 = HalTranslateBusAddress (1, 0, pEndPhysAddress, &IsIOSpace, &pEndPhysAddress);

	  if (Result1 && Result2)
      {

        MappingLength = RtlLargeIntegerSubtract (pEndPhysAddress, pStartPhysAddress);

        if (MappingLength.LowPart)
        {
          // Let ZwMapViewOfSection pick a linear address
#ifdef _AMD64_
		  PhysMemSizeInBytes = MappingLength.QuadPart;
#else
		  PhysMemSizeInBytes = MappingLength.LowPart;
#endif
          ViewBase = pStartPhysAddress;
          ntStatus = ZwMapViewOfSection (*pPhysicalMemoryHandle,
                                         (HANDLE) -1,
                                         &pbPhysMemLin,
                                         0L,
                                         PhysMemSizeInBytes,
                                         &ViewBase,
                                         &PhysMemSizeInBytes,
                                         ViewShare,
                                         0,
                                         PAGE_READWRITE | PAGE_NOCACHE);

          if (NT_SUCCESS(ntStatus))
          {
            pbPhysMemLin += (ULONG)pStartPhysAddress.LowPart - (ULONG)ViewBase.LowPart;
            *ppPhysMemLin = pbPhysMemLin;
          }  
        }
      }
    }
  }
    
  if (!NT_SUCCESS(ntStatus))
    ZwClose(*pPhysicalMemoryHandle);
  
  return ntStatus;
}


NTSTATUS UnmapPhysicalMemory(HANDLE PhysicalMemoryHandle, PVOID pPhysMemLin)
{
  NTSTATUS ntStatus;
  ntStatus = ZwUnmapViewOfSection((HANDLE)-1, pPhysMemLin);
  ZwClose(PhysicalMemoryHandle);
  return ntStatus;
}