CC ?= cc

.PHONY: all clean

all: uxn11

uxn11: uxn11-src.zip
	rm -rf .build
	unzip -q $< -d .build
	$(CC) .build/uxn11/etc/alt/uxncli11.c -lutil -o $@

clean:
	rm -rf .build uxn11
