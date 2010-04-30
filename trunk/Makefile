# $Id: Makefile,v 1.8 2009/02/24 00:10:11 itix Exp $

#ROOTPATH=../../../
ROOTPATH=./
include $(ROOTPATH)Makefile.global

CFLAGS+=-Iinclude -DAROS_ALMOST_COMPATIBLE -D__INLINED_METHODS -DSYSTEM_PRIVATE -D__MORPHOS_SHAREDLIBS -O3 -finline-functions -mcpu=750 -mmultiple -mstring

include makefile.objs

CATALOGS =
TARGET = ScreenRecorder

CATCOMP    = catcomp
NEWCATCOMP = $(ROOTPATH)morphoswb/development/tools/catcomp/catcomp.pl

all: $(TARGET) catalogs

$(TARGET): $(OBJS)
	$(LINKECHO)
	$(LINKPREFIX)$(CC) -noixemul -nostartfiles $(OBJS) -o $@.db
	$(LINKPREFIX)$(STRIP) $(STRIPFLAGS) $@.db -o $@
	$(LINKPREFIX)chmod u+x $@

locale/screenrecorder.ct: locale/screenrecorder.cd
	@$(CATCOMP) locale/screenrecorder.cd CTFILE $@

locale.h: locale/screenrecorder.ct
	@$(CATCOMP) locale/screenrecorder.cd CFILE locale.c NOBLOCK NOCODE
	@$(CATCOMP) locale/screenrecorder.cd CFILE locale.h NOSTRINGS NOARRAY NOBLOCK NOCODE

locale: locale.h locale/screenrecorder.cd locale/screenrecorder.ct
	@

catalogs: $(patsubst %,locale/%.catalog,$(CATALOGS))

%.catalog: %.ct locale/screenrecorder.cd
	@echo "Generating $@ ..."
	@$(NEWCATCOMP) -u -cd locale/screenrecorder.cd -ct $< -o $@

clean:
	rm -f $(TARGET) $(TARGET).db $(TARGET).s $(OBJS) locale.h locale.c locale/*.catalog

dump:
	objdump --disassemble-all --reloc $(TARGET).db >$(TARGET).s

install: all
	@mkdir -p "/sys/Applications/LCD Monitor Test" "/sys/Applications/LCD Monitor Test/images"
	cp LCDMonitorTest "/sys/Applications/LCD Monitor Test/LCD Monitor Test"
	cp images/LCDTest.png "/sys/Applications/LCD Monitor Test/LCD Monitor Test.info"
	cp images/splash_*.png "/sys/Applications/LCD Monitor Test/images"
	cp images/topbar_icon.png "/sys/Applications/LCD Monitor Test/images"
	cp docs/ReadMe "/sys/Applications/LCD Monitor Test"

install-iso: all
	@mkdir -p "$(ISOPATH)Applications/LCD Monitor Test" "$(ISOPATH)Applications/LCD Monitor Test/images"
	cp LCDMonitorTest "$(ISOPATH)Applications/LCD Monitor Test/LCD Monitor Test"
	cp images/LCDTest.png "$(ISOPATH)Applications/LCD Monitor Test/LCD Monitor Test.info"
	cp images/splash_*.png "$(ISOPATH)Applications/LCD Monitor Test/images"
	cp images/topbar_icon.png "$(ISOPATH)Applications/LCD Monitor Test/images"
	cp docs/ReadMe "$(ISOPATH)Applications/LCD Monitor Test"

dep:
	echo >Makefile.dep
	$(CC) -MM $(CFLAGS) `ls *.c` >>Makefile.dep

include Makefile.dep
