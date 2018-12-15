include ../file.mak
CFLAGS += -O2 -DDEBUG=0 -DNDEBUG -fomit-frame-pointer 
VPATH = ..
$(TARGET): $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAG)
	strip $@

include ../rule.mak
#---- depend file ----
-include unagi.d
