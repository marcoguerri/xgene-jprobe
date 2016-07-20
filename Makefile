obj-m += xgene_probe.o
xgene_probe-objs := xgene.o libcrc/libcrc.a

LIBCRC_DIR = $(PWD)/libcrc
KERNEL_HEADERS = /lib/modules/$(shell uname -r)/build

CFLAGS_xgene.o := -I$(KERNEL_HEADERS)/drivers/net/ethernet/apm/xgene
LDFLAGS_xgene_probe.o += -L$(LIBCRC_DIR)/libcrc

all:
	make -C $(KERNEL_HEADERS) M=$(PWD) modules

$(LIBCRC_DIR)/libcrc.a: 
	# Not relying on Kbuild, need to define include paths manually via EXTRA_CFLAGS.
	# isystem is evaluated after -I and before default system include paths

	make -C $(LIBCRC_DIR) libcrc.a \
	EXTRA_CFLAGS="-D__KERNEL__ -isystem $(KERNEL_HEADERS)/include -isystem $(KERNEL_HEADERS)/arch/$(ARCH)/include"

clean:
	make -C libcrc $@
	make -C $(KERNEL_HEADERS) M=$(PWD) clean
