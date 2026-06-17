CC      ?= cc
CFLAGS  ?= -O2 -std=c11 -Wall -Wextra -Wpedantic -Wno-unused-parameter
LDLIBS  := -lm
INCLUDE := -Iinclude

BUILD    := build
LIB      := $(BUILD)/libsdr.a
TEST_BIN := $(BUILD)/test_runner
SYNTH    := $(BUILD)/synth
DEMOD    := $(BUILD)/demod

SRC := src/sdr.c
TEST_SRC := tests/main.c tests/test_complex.c tests/test_nco.c tests/test_filter.c \
            tests/test_disc.c tests/test_deemph.c tests/test_io.c tests/test_siggen.c \
            tests/test_roundtrip.c
SYNTH_SRC := tools/synth.c
DEMOD_SRC := tools/demod.c

.PHONY: all lib test tools synth demo clean

all: lib tools

lib: $(LIB)

$(LIB): $(SRC) include/sdr.h | $(BUILD)
	$(CC) $(CFLAGS) $(INCLUDE) -c $(SRC) -o $(BUILD)/sdr.o
	ar rcs $@ $(BUILD)/sdr.o

$(SYNTH): $(SYNTH_SRC) $(LIB) | $(BUILD)
	$(CC) $(CFLAGS) $(INCLUDE) $(SYNTH_SRC) $(LIB) -o $@ $(LDLIBS)

$(DEMOD): $(DEMOD_SRC) $(LIB) | $(BUILD)
	$(CC) $(CFLAGS) $(INCLUDE) $(DEMOD_SRC) $(LIB) -o $@ $(LDLIBS)

tools: $(SYNTH) $(DEMOD)

$(TEST_BIN): $(TEST_SRC) $(LIB) tests/test.h | $(BUILD)
	$(CC) $(CFLAGS) $(INCLUDE) $(TEST_SRC) $(LIB) -o $@ $(LDLIBS)

test: $(TEST_BIN)
	./$(TEST_BIN)

demo: $(SYNTH) $(DEMOD)
	@mkdir -p build/demo
	./$(SYNTH) -o build/demo/tone.iq -r 2400000 -d 25000 -a 1000 -n 240000
	./$(DEMOD) -i build/demo/tone.iq -o build/demo/tone.wav -r 2400000 -d 25000
	@echo "wrote build/demo/tone.wav"
	@echo "play with: aplay -f S16_LE -r 48000 build/demo/tone.wav"

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD)
