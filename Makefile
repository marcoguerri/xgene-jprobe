obj-m += xgene_probe.o
xgene_probe-objs := xgene.o crc32.o

LIB_DIR = /lib/modules/$(shell uname -r)/build
CFLAGS_xgene.o := -I$(LIB_DIR)/drivers/net/ethernet/apm/xgene

all:
	make -C $(LIB_DIR) M=$(PWD) modules

clean:
	make -C $(LIB_DIR) M=$(PWD) clean
