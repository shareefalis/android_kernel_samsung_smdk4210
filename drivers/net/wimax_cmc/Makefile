EXTRA_CFLAGS		+= -DDEBUG

#obj-$(CONFIG_WIMAX_CMC7XX)	+= control.o hardware.o send.o receive.o
#obj-$(CONFIG_WIMAX_CMC7XX)	+= wimax_sdio.o download.o

obj-$(CONFIG_WIMAX_CMC)		+=cmc7xx_sdio.o

cmc7xx_sdio-y	:= control.o hardware.o receive.o wimax_sdio.o download.o send.o
