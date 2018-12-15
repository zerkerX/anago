include ../file.mak
OBJ += memory_manage.o
CFLAGS += -O0 -g -DDEBUG=1 -Werror -Wall
VPATH = ..:../dozeu
$(TARGET): $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAG)

include ../rule.mak
#---- depend file ----
-include unagi.d
