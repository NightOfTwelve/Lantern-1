ifeq ($(targetbase),linux)
include $(MK_DIR)/sys.mk

LIB = kal
SRCS = $(shell find src/linux_um src/common -name "*.c")

CPPFLAGS += -I$(SDK_DIR)/kal/include

include $(MK_DIR)/lib.mk

CFLAGS += -Werror -Wall
endif

ifeq ($(targetbase),vxworks)
BLD_DIR=$(SDK_DIR)/build.$(CPU).$(VER)
OBJ_DIR=$(BLD_DIR)/obj.$(BOARD)/kal
LIB_DIR=$(BLD_DIR)/lib.$(BOARD)
BIN_DIR=$(BLD_DIR)/bin.$(BOARD)

all_targets = src libkal

all: $(all_targets) FORCE
clean: $(addprefix clean_,$(all_targets)) FORCE

src: FORCE
	make -C $(SDK_DIR)/kal/src -f Makefile.vx

clean_src: FORCE
	make -C $(SDK_DIR)/kal/src -f Makefile.vx clean

libkal: FORCE
	make -f Makefile_libkal.vx

clean_libkal: FORCE
	make -f Makefile_libkal.vx clean

.PHONY: FORCE
FORCE:
endif
