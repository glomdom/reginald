CC            ?= clang++
LD 			  ?= clang++
CFLAGS        := -fPIC -ffreestanding -fno-stack-protector -fno-stack-check -fshort-wchar -mno-red-zone -MMD -MP
EFI_DIR       := $(PWD)/gnu-efi
INCLUDES      := -I$(EFI_DIR)/inc
CRT0          := $(EFI_DIR)/x86_64/gnuefi/crt0-efi-x86_64.o
LDSCRIPT      := $(EFI_DIR)/gnuefi/elf_x86_64_efi.lds
LIBS          := -L$(EFI_DIR)/x86_64/lib -L$(EFI_DIR)/x86_64/gnuefi -lgnuefi -lefi
OBJCOPY_FLAGS := -j .text -j .sdata -j .data -j .rodata -j .dynamic \
                 -j .dynsym -j .rel -j .rela -j .rel.* -j .rela.* \
                 -j .reloc --target efi-app-x86_64 --subsystem=10

SRCS := $(wildcard src/*.cpp)
HDRS := $(wildcard src/*.hpp)
OBJS := $(patsubst src/%.cpp,build/%.o,$(SRCS))

COMPILE_COMMANDS := build/compile_commands.json

OVMF_DIR   := build/ovmf
IMAGE_DIR  := build/image
OVMF       := $(OVMF_DIR)/ovmf-code-x86_64.fd
RUN_IMAGE  := $(IMAGE_DIR)/EFI/BOOT/BOOTX64.EFI
EFI_TARGET := build/reginald.efi

.PHONY: all clean qemu
.SECONDARY:

all: $(EFI_TARGET) $(COMPILE_COMMANDS)

$(EFI_TARGET): build/reginald.so $(HDRS)
	@printf " %-12s %s\n" "OBJCOPY" "$<"
	@objcopy $(OBJCOPY_FLAGS) $< $@

$(COMPILE_COMMANDS): $(SRCS) $(HDRS)
	@printf "[\n" > $@
	@for src in $(SRCS); do \
	  obj=build/$$(basename $$src .cpp).o; \
	  cmd="$(CC) $(CFLAGS) $(INCLUDES) -c $$src -o $$obj"; \
	  printf "  { \"directory\": \"%s\", \"command\": \"%s\", \"file\": \"%s\" },\n" \
	    "$(shell pwd)" "$$cmd" "$$src" >> $@; \
	done
	@truncate -s -2 $@
	@printf "\n]\n" >> $@

build/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	@printf " %-12s %s\n" "CC" "$<"
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

build/reginald.so: $(OBJS)
	@printf " %-12s %s\n" "LD_SHARED" "$@"
	@$(LD) -shared -Bsymbolic -T$(LDSCRIPT) $(CRT0) $^ -o $@ $(LIBS)

qemu: build/reginald.efi $(OVMF) $(RUN_IMAGE)
	qemu-system-x86_64 -M q35 \
	  -m 256M \
	  -bios $(OVMF) \
	  -drive file=fat:rw:$(IMAGE_DIR),media=disk,format=raw

$(RUN_IMAGE): $(EFI_TARGET)
	@mkdir -p $(dir $@)
	@printf " %-12s %s\n" "LN" "$@"
	@ln -sf $(abspath $(EFI_TARGET)) $@

$(OVMF):
	@mkdir -p $(OVMF_DIR)
	@printf " %-12s %s\n" "DOWNLOAD" "$@"
	@curl -sSL -o $@ https://github.com/osdev0/edk2-ovmf-nightly/releases/latest/download/ovmf-code-x86_64.fd

clean:
	rm -rf build
	rm -f $(COMPILE_COMMANDS)

-include $(OBJS:.o=.d)