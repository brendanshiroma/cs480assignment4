CC      = gcc
CFLAGS  = -g3 -Wall -Wextra -std=gnu11
LDFLAGS = -pthread

TARGET  = carassembly
SRCS    = main.c shared.c producer.c consumer.c finalline.c log.c
OBJS    = $(SRCS:.c=.o)
HDRS    = car_assembly.h log.h shared.h producer.h consumer.h finalline.h

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -pthread -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
