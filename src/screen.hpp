#pragma once

#include <efi.h>
#include <efilib.h>

namespace reginald {

class Screen {
 public:
  explicit Screen(EFI_SYSTEM_TABLE* systemTable) : _systemTable(systemTable) {}

  void clear() const {
    uefi_call_wrapper(
      (void*)_systemTable->ConOut->ClearScreen,
      1,
      _systemTable->ConOut
    );
  }

  void printf(const CHAR16* fmt, ...) const {
    va_list args;
    va_start(args, fmt);
    vprint(fmt, args, false);
    va_end(args);
  }

  void printfln(const CHAR16* fmt = L"", ...) const {
    va_list args;
    va_start(args, fmt);
    vprint(fmt, args, true);
    va_end(args);
  }

 private:
  EFI_SYSTEM_TABLE* const _systemTable;

  void vprint(const CHAR16* fmt, va_list args, bool newline) const {
    CHAR16 buffer[512];

    VSPrint(buffer, sizeof(buffer) / sizeof(CHAR16), fmt, args);

    if (newline) {
      size_t len = StrLen(buffer);

      if (len + 2 < sizeof(buffer) / sizeof(CHAR16)) {
        buffer[len] = L'\n';
        buffer[len + 1] = L'\r';
        buffer[len + 2] = L'\0';
      }
    }

    uefi_call_wrapper(
      (void*)_systemTable->ConOut->OutputString,
      2,
      _systemTable->ConOut,
      buffer
    );
  }
};

}  // namespace reginald