#
# Files that need to be bundled with the building system
#

MAINDIRS += Documentation drivers firmware include sound

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
TARFILES += include/linux/ivtv.h
TARFILES += include/linux/mmc/sdio_ids.h
TARFILES += include/linux/ivtvfb.h
TARFILES += include/linux/videodev2.h
TARFILES += include/linux/meye.h
TARFILES += include/sound/tea575x-tuner.h
TARFILES += include/sound/aci.h

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

TODAY_TAR := linux-media-`date "+%Y-%m-%d"`.tar
LATEST_TAR := http://linuxtv.org/downloads/drivers/linux-media-LATEST.tar.bz2

default: help

help:
	@echo "Use: make tar DIR=<directory>"
	@echo "          untar|clean|distclean"

todaytar:
	@if [ "$(DIR)" == "" ]; then echo "make $@ DIR=<version>"; exit -1; fi
	-rm $(PWD)/$(TODAY_TAR).bz2
	tar cf $(PWD)/$(TODAY_TAR) -C $(DIR) $(TARFILES)
	for i in $(TARDIR); do \
		if [ "`echo $$i|grep Documentation`" == "" ]; then \
			dir="`(cd $(DIR); find $$i -type f -name *.[ch])`"; \
			dir="$$dir `(cd $(DIR); find $$i -type f -name Makefile)`"; \
			dir="$$dir `(cd $(DIR); find $$i -type f -name Kconfig)`"; \
			tar rvf $(PWD)/$(TODAY_TAR) -C $(DIR) $$dir; \
		else \
			tar rvf $(PWD)/$(TODAY_TAR) -C $(DIR) $$i; \
		fi; done; bzip2 $(PWD)/$(TODAY_TAR)

tar:
	@if [ "$(DIR)" == "" ]; then echo "make $@ DIR=<version>"; exit -1; fi
	-rm $(PWD)/linux-media.tar.bz2
	tar cf $(PWD)/linux-media.tar -C $(DIR) $(TARFILES)
	for i in $(TARDIR); do \
		if [ "`echo $$i|grep Documentation`" == "" ]; then \
			dir="`(cd $(DIR); find $$i -type f -name *.[ch])`"; \
			dir="$$dir `(cd $(DIR); find $$i -type f -name Makefile)`"; \
			dir="$$dir `(cd $(DIR); find $$i -type f -name Kconfig)`"; \
			tar rvf $(PWD)/linux-media.tar -C $(DIR) $$dir; \
		else \
			tar rvf $(PWD)/linux-media.tar -C $(DIR) $$i; \
		fi; done; bzip2 $(PWD)/linux-media.tar

untar: linux-media.tar.bz2
	tar xfj linux-media.tar.bz2

clean:
	-rm -rf $(MAINDIRS)

dir: clean
	@if [ "$(DIR)" == "" ]; then echo "make $@ DIR=<version>"; exit -1; fi
	for i in $(TARFILES); do \
		install -D $(DIR)/$$i $$i; \
	done
	for i in $(TARDIR); do \
		if [ "`echo $$i|grep Documentation`" == "" ]; then \
			dir="`(cd $(DIR); find $$i -type f -name *.[ch])`"; \
			dir="$$dir `(cd $(DIR); find $$i -type f -name Makefile)`"; \
			dir="$$dir `(cd $(DIR); find $$i -type f -name Kconfig)`"; \
			for j in $$dir; do \
				install -D $(DIR)/$$j $$j; \
			done; \
		else \
			install -d $$i; \
			cp -ar $(DIR)/$$i $$i; \
		fi; done

distclean: clean
	-rm linux-media.tar.bz2

apply_patches apply-patches:
	@if [ "$(VER)" == "" ]; then echo "make $@ VER=<version>"; exit -1; fi
	if [ -e ../backports/$(VER)/series ]; then \
	for i in `cat ../backports/$(VER)/series`; do patch -t -p1 -i ../backports/$(VER)/$$i; done \
	fi

unapply_patches unapply-patches:
	@if [ "$(VER)" == "" ]; then echo "make $@ VER=<version>"; exit -1; fi
	if [ -e ../backports/$(VER)/series ]; then \
	for i in `cat ../backports/$(VER)/series`; do patch -R -p1 -i ../backports/$(VER)/$$i; done \
	fi

download:
	wget $(LATEST_TAR) -O linux-media.tar.bz2

