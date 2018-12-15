OBJ_HK = giveio.o reader_hongkongfc.o
OBJ_HD = head/nesheader.o head/header.o file.o
SOURCE_UNAGI = \
	*.c *.h *.mak Makefile COPYING \
	debug/debug.mak profile/profile.mak release/release.mak \
	unagi.rc unagi.ico
SOURCE_ANAGO = \
	Makefile anago.c flash_device.c progress.c reader_dummy.c \
	script_common.c script_dump.c script_flash.c squirrel_wrap.c \
	flash_device.h progress.h reader_dummy.h script_common.h  script_dump.h script_flash.h squirrel_wrap.h \
	flashcore.nut flashdevice.nut \
	anago_en.txt anago_ja.txt porting.txt
ARCHIVE_GZ = unagi_client_source.0.6.0.tar.gz
ARCHIVE_ZIP = unagi_client_windows_060.zip
TARGET_DIR = debug
TARGET_MAK = debug.mak
ifeq ($(PROFILE),1)
	TARGET_DIR = profile
	TARGET_MAK = profile.mak
endif
ifeq ($(RELEASE),1)
	TARGET_DIR = release
	TARGET_MAK = release.mak
endif

all:
	cd $(TARGET_DIR); make -f $(TARGET_MAK)
	cp $(TARGET_DIR)/unagi.exe .
clean:
	rm -f unagi.exe \
		debug/*.o debug/*.exe debug/*.d \
		profile/*.o profile/*.exe profile/*.d \
		release/*.o release/*.exe release/*.d

head/nesheader.o: nesheader.c
	$(CC) $(CFLAGS) -DHEADEROUT -I. -c -o $@ $<
head/header.o: header.c
	$(CC) $(CFLAGS) -DHEADEROUT -I. -c -o $@ $<
hk.exe: $(OBJ_HK)
	$(CC) -o $@ $(OBJ_HK)
iodel.exe: iodel.o giveio.o
	$(CC) -o $@ iodel.o giveio.o
nesheader.exe: $(OBJ_HD)
	$(CC) -o $@ $(OBJ_HD)
gz:
	cd ..; \
	tar cfz $(ARCHIVE_GZ) $(addprefix client/,$(SOURCE_UNAGI)) $(addprefix client/anago/,$(SOURCE_ANAGO))
zip:
	7za a $(ARCHIVE_ZIP) \
		unagi.exe unagi.txt iodel.exe iodel.txt COPYING ../script/syntax.txt \
		$(addprefix anago/,anago.exe *.ad *.af anago_en.txt anago_ja.txt flashcore.nut flashdevice.nut dumpcore.nut)
	cd release; 7za a ../$(ARCHIVE_ZIP) unagi.cfg
	mv $(ARCHIVE_ZIP) ..
