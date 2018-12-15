crc32.o: ../crc32.c ../type.h ../crc32.h ../crctable.h
file.o: ../file.c ../memory_manage.h ../file.h ../type.h
flashmemory.o: ../flashmemory.c ../type.h ../header.h ../flashmemory.h
giveio.o: ../giveio.c ../giveio.h
header.o: ../header.c ../memory_manage.h ../type.h ../file.h ../crc32.h \
 ../config.h ../reader_master.h ../header.h ../flashmemory.h
iodel.o: ../iodel.c ../giveio.h
memory_manage.o: ../memory_manage.c ../memory_manage.h
reader_hongkongfc.o: ../reader_hongkongfc.c ../type.h ../paralellport.h \
 ../giveio.h ../reader_master.h ../reader_hongkongfc.h \
 ../hard_hongkongfc.h
reader_kazzo.o: ../reader_kazzo.c ../memory_manage.h ../reader_master.h \
 ../type.h ../usb_device.h ../reader_kazzo.h
reader_master.o: ../reader_master.c ../giveio.h ../reader_master.h \
 ../type.h ../reader_onajimi.h ../reader_hongkongfc.h ../reader_kazzo.h
reader_onajimi.o: ../reader_onajimi.c ../type.h ../paralellport.h \
 ../giveio.h ../hard_onajimi.h ../reader_master.h ../reader_onajimi.h
script_engine.o: ../script_engine.c ../memory_manage.h ../type.h \
 ../file.h ../reader_master.h ../textutil.h ../config.h ../header.h \
 ../flashmemory.h ../script_syntax.h ../script.h
script_syntax.o: ../script_syntax.c ../type.h ../textutil.h ../script.h \
 ../script_syntax.h ../config.h ../reader_master.h ../header.h \
 ../flashmemory.h ../syntax_data.h
textutil.o: ../textutil.c ../type.h ../textutil.h
unagi.o: ../unagi.c ../memory_manage.h ../type.h ../reader_master.h \
 ../giveio.h ../file.h ../script.h ../header.h ../flashmemory.h \
 ../textutil.h ../config.h ../version.h
usb_device.o: ../usb_device.c ../usb_device.h
