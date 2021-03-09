.PYONY:  

TARGET_NAME = world4utau
TARGET_TYPE = bin
EXE = resampler.exe # 临时用来兼容OpenUtau

CUR_DIR  = $(shell pwd)

OUT_DIR = $(CUR_DIR)/bin
SRC_DIR = $(CUR_DIR)/src
OBJ_DIR = $(CUR_DIR)/obj

SRCS = $(foreach dir,$(SRC_DIR),$(wildcard $(dir)/*.c))
OBJS = $(filter-out world4utau.o, $(notdir $(patsubst %.c, %.o, $(SRCS))))
OBJS_WITH_DIR = $(addprefix $(OBJ_DIR)/,$(OBJS))

TARGET = $(OUT_DIR)/$(TARGET_NAME)
INCLUDES = -I$(SRC_DIR)
CFLAGS = -O3 #-DFFTW3 #-D_DEBUG #-g -Wall
LIBS = -lfftw3 -lm

CC = gcc

build: setup ${TARGET_NAME}
	cp ${TARGET} ${EXE}

setup:
	mkdir -p $(OBJ_DIR)
	mkdir -p ${OUT_DIR}

$(TARGET_NAME):$(OBJS)
	$(CC) $(OBJS_WITH_DIR) $(SRC_DIR)/$(TARGET_NAME).c -o $(TARGET) $(CFLAGS) $(LIBS)

$(OBJS): %.o : $(SRC_DIR)/%.c
	$(info $@ $<)
	$(CC) -o $(OBJ_DIR)/$@ -c $< $(CFLAGS)

tool: build
	$(CC) $(OBJS_WITH_DIR) tool/main.c -o $(OUT_DIR)/tool $(INCLUDES) $(CFLAGS) $(LIBS)

clean:
	@rm -rf obj/*.o
	@rm -rf bin/*
	@rm $(EXE)