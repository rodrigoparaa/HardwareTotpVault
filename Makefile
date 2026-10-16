TARGET = TotpVault

OBJS = main.o \
       core/crypto_sha.o \
       core/crypto_hmac.o \
       core/crypto_chacha20.o \
       core/base32.o \
       core/otp.o \
       core/vault.o \
       core/input_logic.o \
       core/app_state.o \
       platform/psp_rtc.o \
       platform/psp_fs.o \
       platform/psp_input.o \
       platform/psp_power.o \
       ui/graphics.o \
       ui/font_7seg.o \
       ui/font_8x8.o \
       ui/theme_engine.o \
       ui/screens.o

INCDIR = core platform ui
CFLAGS = -O2 -G0 -Wall -Wextra -std=c99 -DPSP=1
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =
LIBS = -lpspgu -lpsprtc -lpsppower

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Hardware TOTP Vault (Air-Gapped)

PSPSDK = $(shell psp-config --pspsdk-path 2>/dev/null)
ifneq ($(PSPSDK),)
include $(PSPSDK)/lib/build.mak
else
all:
	@echo "PSPSDK not found in current PATH."
	@echo "Run: source ./activate.sh"
	@echo "Or compile in Docker: docker build -t psp-totp-builder . && docker run --rm -v \"\$$PWD\":/src psp-totp-builder"
endif

HOST_CC ?= cc
HOST_CFLAGS = -O2 -Wall -Wextra -std=c99 -I. -Icore -Iui
CORE_SRCS = core/crypto_sha.c core/crypto_hmac.c core/crypto_chacha20.c core/base32.c core/otp.c core/vault.c core/input_logic.c core/app_state.c ui/theme_engine.c

.PHONY: test cli
test:
	$(HOST_CC) $(HOST_CFLAGS) tests/test_core.c $(CORE_SRCS) -o tests/test_core
	./tests/test_core

cli:
	$(HOST_CC) $(HOST_CFLAGS) tools/vault_cli.c core/crypto_sha.c core/crypto_hmac.c core/crypto_chacha20.c core/base32.c core/otp.c core/vault.c -o tools/vault_cli

