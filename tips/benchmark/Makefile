CUR_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

include $(CUR_DIR)/../Makefile.inc

CFLAGS += -I$(INC_DIR) -I$(LIB_DIR) -I./include
CFLAGS += -march=native -mtune=native -O3
CFLAGS += -g -ggdb3  -DDEFAULT
CFLAGS += -Wall -Winline -Wextra -Wshadow #-Werror -Wdeclaration-after-statement
CFLAGS += -Wno-attributes -Wno-unused-parameter -Wno-unused-value
CFLAGS += -Wno-unused-function -Wno-packed-not-aligned
LDFLAGS += -lpthread -lgtest -lpmem -lpmemobj -ltbb -L./lib 

DEPS_DIR  := $(CUR_DIR)/.deps
DEPCFLAGS = -MD -MF $(DEPS_DIR)/$*.d -MP

SRC_FILES = $(wildcard *.c)
CXX_SRC_FILES = $(wildcard *.cpp) 
OBJS_DIR  = $(CUR_DIR)/.objs
OBJ_FILES = $(addprefix $(OBJS_DIR)/, $(SRC_FILES:.c=.o))
OBJ_FILES += $(addprefix $(OBJS_DIR)/, $(CXX_SRC_FILES:.cpp=.o)) 
BINS = bench-tips

all: $(BINS)
	@for d in $(BINS); \
                do ( cp $$d $(BIN_DIR); \
                ); \
        done						

numa-config.h:
	$(TOOLS_DIR)/cpu-topology.py > $(CUR_DIR)/numa-config.h

bench-tips: $(OBJ_FILES) numa-config.h $(LIB_DIR)/libtips.a  
	$(CXX) -o $@ $^ $(LIB_DIR)/libtips.a $(LDFLAGS)  

$(OBJS_DIR)/%.o: %.c $(DEPS_DIR) $(OBJS_DIR)
	$(Q)$(CC) $(CFLAGS) $(DEPCFLAGS) -c -o $@ $<

$(OBJS_DIR)/%.o: %.cpp $(DEPS_DIR) $(OBJS_DIR) 
	$(Q)$(CXX) $(CFLAGS) $(DEPCFLAGS) -c -o $@ $<  

$(OBJS_DIR):
	$(Q)mkdir -p $(OBJS_DIR)

$(DEPS_DIR):
	$(Q)mkdir -p $(DEPS_DIR)

clean:
	$(Q)rm -rf bench-tips
	$(Q)rm -rf $(CUR_DIR)/.deps*
	$(Q)rm -rf $(CUR_DIR)/.objs*

.PHONY: all clean
