CC      := arm-linux-gcc
CFLAGS  := -march=armv4 -mabi=apcs-gnu -msoft-float -O2 -Wall -Wextra
LDFLAGS := -s

BIN     := retrocomputing
OBJS    := main.o check.o
DEPS    := $(OBJS:%.o=%.d)

.PHONY: clean

$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@

-include $(DEPS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -MMD -MF $*.d -o $@

%.o: %.S
	$(CC) $(CFLAGS) -c $< -MMD -MF $*.d -o $@

clean:
	$(RM) $(OBJS) $(DEPS) $(BIN)
