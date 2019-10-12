ifneq ("$(wildcard ./config.mk)", "")
include config.mk
endif

ifeq ($(CFLAG),)
  SRC_ROOT = $(pwd)/src
endif

ifeq ($(LOCKMANAGEMENT_LIB_NAME),)
	LOCKMANAGEMENT_LIB_NAME = liblockMang.a
endif

ifeq ($(ERROR_MSG_MODE), yes)
	CFLAG = -DERROR_MSG_MODE
endif

ifeq ($(DEBUG_MODE), yes)
CFLAG += -DDEBUG_MODE
endif

ifeq ($(SYSLOG), yes)
CFLAG += -DSYSLOG
endif

LOCKMANG_FILE = src/lock_mang.c
LOCK_DATA_FILE = src/lock_keynode.c

LOCKMANG_LIB_FILE = $(LOCKMANG_FILE) $(LOCK_DATA_FILE)

INCLUDE_DIR = \
-I./inc/ \
-I./h/

all:
	$(CC) -c $(LOCKMANG_LIB_FILE) $(CFLAG) $(INCLUDE_DIR)
	ar -r $(LOCKMANAGEMENT_LIB_NAME) *.o
	mv $(LOCKMANAGEMENT_LIB_NAME) ./lib/
	rm -rf *.o

clean:
	rm -rf *.o ./lib/$(LOCKMANAGEMENT_LIB_NAME)

distclean: clean
	rm -rf lkmtest

test:
	$(CC) -c $(LOCKMANG_LIB_FILE) lkm_test.c $(CFLAG) \
$(INCLUDE_DIR) $(DEBUG_FLAG) -DCONSOLE_DEBUG
	$(CC) -g *.o -lcrypto -o lkmtest -lpthread
	rm -rf *.o
