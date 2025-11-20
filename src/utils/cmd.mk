.PHONY: bochs
bochs: $(IMAGES)
	bochs -q -f ../bochs/bochsrc -unlock



QEMU:= qemu-system-i386
QEMU+= -m 32M
QEMU+= -audiodev pa,id=hda	# audio dev
QEMU+= -machine pcspk-audiodev=hda	# pc speaker dev
QEMU+= -rtc base=localtime	# set rtc to localtime
QEMU+= -drive file=$(BUILD_DIR)/master.img,if=ide,index=0,media=disk,format=raw # master hard disk
QEMU+= -drive file=$(BUILD_DIR)/slave.img,if=ide,index=1,media=disk,format=raw # slave hard disk

QEMU_DISK:=-boot c

QEMU_DEBUG:= -s -S

.PHONY: qemu
qemu: $(IMAGES)
	$(QEMU) $(QEMU_DISK)

.PHONY: qemug
qemug: $(IMAGES)
	$(QEMU) $(QEMU_DISK) $(QEMU_DEBUG)

# vmware vmdk image
$(BUILD_DIR)/master.vmdk: $(BUILD_DIR)/master.img
	qemu-img convert -O vmdk $< $@

.PHONY: vmdk
vmdk: $(BUILD_DIR)/master.vmdk