
LIBS = \
	-l:libcertdb.a \
	-l:libcerthi.a \
	-l:libcryptohi.a \
	-l:libfreebl_static.a \
	-l:libgcm.a \
	-l:libghash-aes-x86_c_lib.a \
	-l:libhw-acc-crypto-avx.a \
	-l:libhw-acc-crypto-avx2.a \
	-l:libintel-gcm-wrap_c_lib.a \
	-l:libsha-x86_c_lib.a \
	-l:libnspr4.a \
	-l:libnss_static.a \
	-l:libnssb.a \
	-l:libnssdev.a \
	-l:libnsspki.a \
	-l:libnssutil.a \
	-l:libplc4.a \
	-l:libpk11wrap_static.a \
	-l:libpkixcertsel.a \
	-l:libpkixchecker.a \
	-l:libpkixcrlsel.a \
	-l:libpkixmodule.a \
	-l:libpkixparams.a \
	-l:libpkixpki.a \
	-l:libpkixresults.a \
	-l:libpkixstore.a \
	-l:libpkixsystem.a \
	-l:libpkixtop.a \
	-l:libpkixutil.a \
	-l:libplds4.a \
	-l:libsoftokn_static.a \
	-l:libsqlite.a \
	-l:libssl.a

ARGS = -Wall -std=c11 -Wl,--gc-sections -s -O2 \
	-Inss-3.125/dist/Release/include/nspr \
	-Inss-3.125/dist/public \
	-Lnss-3.125/dist/Release/lib \
	-Wl,--start-group $(LIBS) -Wl,--end-group

.PHONY: all clean

all: client client_debug

client: client.c
	gcc -o $@ $< $(ARGS)

client_debug: client.c
	gcc -o $@ $< $(ARGS) -DDEBUG

clean:
	rm -f client
