# Builds the awk2c runtime support library: libawk_runtime.a
CC      ?= cc
CFLAGS  ?= -std=gnu23 -O2 -Wall -Wextra
AR      ?= ar
# Where objects and the archive land. Override it to keep a build variant (the
# sanitizer build in tests/run.py) out of the source tree.
OBJDIR  ?= .

SRCS = awk_value.c awk_array.c awk_record.c awk_runtime.c awk_regex.c awk_builtins.c
OBJS = $(addprefix $(OBJDIR)/,$(SRCS:.c=.o))
LIB  = $(OBJDIR)/libawk_runtime.a

.PHONY: all clean
all: $(LIB)

$(LIB): $(OBJS)
	$(AR) rcs $@ $(OBJS)

$(OBJDIR)/%.o: %.c awk_runtime.h awk_internal.h | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -f $(OBJS) $(LIB)
