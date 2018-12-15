include ../file.mak
CFLAGS += -O2 -pg -DDEBUG=0
VPATH = ..
$(TARGET): $(OBJ)
	$(CC) -pg -o $@ $(OBJ)

include ../rule.mak
#---- depend file ----
-include unagi.d
