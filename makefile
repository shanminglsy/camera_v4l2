TOP_DIR := $(shell pwd)
APP = $(TOP_DIR)/out/camera_h264encode

CC = gcc
CFLAGS = -g 
LIBS = -lpthread -lx264 -lm 
DEP_LIBS = -L$(TOP_DIR)/lib
HEADER =
OBJS = video.o h264encoder.o

hhi:  $(OBJS)
	$(CC) -g -o $(APP) $(OBJS) $(LIBS) $(DEP_LIBS) 

#clean:
#	rm -f *.o a.out $(APP) core *~

