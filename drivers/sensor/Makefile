#
# Makefile for the sensors drivers.
#

# Each configuration option enables a list of files.

obj-$(CONFIG_SENSORS_CORE)		+= sensors_core.o
obj-$(CONFIG_SENSORS_BMP180)		+= bmp180.o
obj-$(CONFIG_SENSORS_CM3663)		+= cm3663.o
obj-$(CONFIG_SENSORS_GP2A)		+= gp2a_proximity.o gp2a_light.o
obj-$(CONFIG_SENSORS_GP2A_ANALOG)	+= gp2a_analog.o
ifeq ($(CONFIG_TARGET_LOCALE_NA),y)
obj-$(CONFIG_SENSORS_K3G)		+= k3g_spr.o
obj-$(CONFIG_SENSORS_AK8975C)		+= ak8975_spr.o
else
obj-$(CONFIG_SENSORS_K3G)		+= k3g.o
obj-$(CONFIG_SENSORS_AK8975C)		+= ak8975.o
endif
obj-$(CONFIG_SENSORS_K3DH)		+= k3dh.o
obj-$(CONFIG_SENSORS_LSM330DLC)		+= lsm330dlc_accel.o lsm330dlc_gyro.o
obj-$(CONFIG_SENSORS_LPS331)		+= lps331ap.o
obj-$(CONFIG_SENSORS_CM36651)		+= cm36651.o
# VE_GROUP
obj-$(CONFIG_SENSORS_PAS2M110)		+= pas2m110.o
