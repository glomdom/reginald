#include <efi.h>
#include <efilib.h>

#include "screen.hpp"

const CHAR16* GetMemoryTypeName(UINT32 type) {
  switch (type) {
    case EfiConventionalMemory:
      return L"EfiConventionalMemory";
    case EfiLoaderCode:
      return L"EfiLoaderCode";
    case EfiLoaderData:
      return L"EfiLoaderData";
    // case EfiBootServicesCode:       return L"EfiBootServicesCode";
    // case EfiBootServicesData:       return L"EfiBootServicesData";
    case EfiACPIReclaimMemory:
      return L"EfiACPIReclaimMemory";
    case EfiACPIMemoryNVS:
      return L"EfiACPIMemoryNVS";
    case EfiRuntimeServicesCode:
      return L"EfiRuntimeServicesCode";
    case EfiRuntimeServicesData:
      return L"EfiRuntimeServicesData";

    default:
      return NULL;  // skip others
  }
}

extern "C" EFI_STATUS EFIAPI
efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable) {
  InitializeLib(ImageHandle, SystemTable);

  reginald::Screen screen(SystemTable);
  screen.clear();
  screen.printfln(L"reginald bootloader v%d.%d", 69, 420);
  screen.printfln();
  screen.printfln(L"i am now going to ep1c haxx0r ur data");

  EFI_MEMORY_DESCRIPTOR* memMap = nullptr;
  UINTN memMapSize = 0;
  UINTN mapKey;
  UINTN descriptorSize;
  UINT32 descriptorVersion;

  EFI_STATUS status;

  status = uefi_call_wrapper(
    (void*)SystemTable->BootServices->GetMemoryMap,
    5,
    &memMapSize,
    memMap,
    &mapKey,
    &descriptorSize,
    &descriptorVersion
  );

  memMapSize += 2 * descriptorSize;
  status = uefi_call_wrapper(
    (void*)SystemTable->BootServices->AllocatePool,
    3,
    EfiLoaderData,
    memMapSize,
    (void**)&memMap
  );

  if (EFI_ERROR(status)) {
    screen.printfln(L"failed to allocate memory map buffer: %r\n", status);

    return status;
  }

  status = uefi_call_wrapper(
    (void*)SystemTable->BootServices->GetMemoryMap,
    5,
    &memMapSize,
    memMap,
    &mapKey,
    &descriptorSize,
    &descriptorVersion
  );

  if (EFI_ERROR(status)) {
    screen.printfln(L"GetMemoryMap failed: %r\n", status);

    return status;
  }

  UINTN numEntries = memMapSize / descriptorSize;
  for (UINTN i = 0; i < numEntries; i++) {
    EFI_MEMORY_DESCRIPTOR* desc =
      (EFI_MEMORY_DESCRIPTOR*)((UINT8*)memMap + (i * descriptorSize));

    const CHAR16* typeName = GetMemoryTypeName(desc->Type);

    if (typeName) {
      Print(
        L"%-24s Start: %016lx Pages: %08lx Attr: 0x%lx\n",
        typeName,
        desc->PhysicalStart,
        desc->NumberOfPages,
        desc->Attribute
      );
    }
  }

  uefi_call_wrapper((void*)SystemTable->BootServices->FreePool, 1, memMap);

  while (1) {
    __asm__ __volatile__("hlt");
  }

  return EFI_SUCCESS;
}