#
# Files that need to be bundled with the building system
#

MAINDIRS += arch drivers include Documentation firmware sound

# Media drivers
TARDIR += drivers/media/
TARDIR += drivers/staging/cx25821/
TARDIR += drivers/staging/go7007/
TARDIR += drivers/staging/tm6000/
TARFILES += sound/pci/bt87x.c
TARFILES += sound/i2c/other/tea575x-tuner.c

# Includes
TARDIR += include/media/
TARDIR += include/linux/dvb/
TARFILES += include/linux/usb/video.h
TARFILES += include/linux/i2c-id.h
TARFILES += include/linux/videodev.h
TARFILES += include/linux/videotext.h
TARFILES += include/linux/video_decoder.h
TARFILES += include/linux/ivtv.h
TARFILES += include/linux/mmc/sdio_ids.h
TARFILES += include/linux/ivtvfb.h
TARFILES += include/linux/videodev2.h
TARFILES += include/linux/meye.h
TARFILES += include/sound/tea575x-tuner.h
TARFILES += include/sound/aci.h
TARFILES += include/asm-arm/arch-pxa/camera.h
TARFILES += include/asm-arm/arch-pxa/pxa-regs.h
TARFILES += arch/arm/plat-mxc/include/mach/mx1_camera.h
TARFILES += arch/arm/plat-mxc/include/mach/mx3_camera.h
TARFILES += arch/arm/plat-mxc/include/mach/memory.h

# Documentation
TARDIR += Documentation/dvb/
TARDIR += Documentation/video4linux/
TARDIR += Documentation/DocBook/dvb/
TARDIR += Documentation/DocBook/v4l/
TARFILES += Documentation/DocBook/media.tmpl
TARFILES += Documentation/DocBook/media-entities.tmpl
TARFILES += Documentation/DocBook/media-indices.tmpl
TARFILES += Documentation/DocBook/Makefile

# Firmware
TARFILES += firmware/av7110/bootcode.bin.ihex
TARFILES += firmware/av7110/Boot.S
TARFILES += firmware/cpia2/stv0672_vp4.bin.ihex
TARFILES += firmware/ihex2fw.c
TARFILES += firmware/vicam/firmware.H16
TARFILES += firmware/dabusb/bitstream.bin.ihex
TARFILES += firmware/dabusb/firmware.HEX
TARFILES += firmware/ttusb-budget/dspbootcode.bin.ihex

## Miscelaneous arch-dependent drivers required for compilation
## Probably, we can just get rid of them
#TARFILES += arch/arm/mach-mx3/clock.c
#TARFILES += arch/arm/mach-pxa/pcm990-baseboard.c
#TARFILES += arch/arm/mach-pxa/devices.c
#TARFILES += arch/arm/mach-mx1/devices.c
#TARFILES += arch/arm/mach-mx1/mx1_camera_fiq.S
#TARFILES += arch/arm/mach-mx1/ksym_mx1.c
#TARFILES += arch/arm/mach-mx1/Makefile
#TARFILES += arch/sh/boards/mach-kfr2r09/setup.c
#TARFILES += arch/sh/boards/mach-migor/setup.c
#TARFILES += arch/sh/boards/mach-ap325rxa/setup.c

default: help

help:
	@echo "Use: make tar DIR=<directory>"
	@echo "          untar|clean|distclean"

ifeq ($(DIR),)
tar:
	@echo  Use: make tar DIR=<directory>
	@exit -1
else
tar:
	tar cvfj $(PWD)/linux-media.tar.bz2 -C $(DIR) $(TARFILES) $(TARDIR)
endif

untar:
	tar xvfj $(PWD)/linux-media.tar.bz2

clean:
	-rm -rf $(MAINDIRS)

distclean: clean
	-rm linux-media.tar.bz2
