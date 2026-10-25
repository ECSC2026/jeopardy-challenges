NASM ?= nasm
QEMU ?= qemu-system-i386
IMAGE := kernel.img

.PHONY: kernel run clean

kernel:
	$(NASM) -f bin kernel.asm -o kernel.bin
	@bytes=$$(wc -c < kernel.bin); \
	sectors=$$((($$bytes + 511) / 512)); \
	$(NASM) -f bin -D KERNEL_SECTORS=$$sectors bootloader.asm -o bootloader.bin; \
	cat bootloader.bin kernel.bin > $(IMAGE); \
	pad=$$((sectors * 512 - bytes)); \
	dd if=/dev/zero bs=1 count=$$pad status=none >> $(IMAGE)
	@rm -f bootloader.bin kernel.bin

run: kernel
	$(QEMU) -drive format=raw,file=$(IMAGE),if=ide -boot c -no-reboot -no-shutdown

clean:
	rm -f $(IMAGE) bootloader.bin kernel.bin
