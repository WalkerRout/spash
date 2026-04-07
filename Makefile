CC = clang
LD = clang
OBJCOPY = llvm-objcopy

BIN = spash.exe
OBJS_DIR = .objs
BIN_DIR = bin

VCPKG_DIR ?= C:/Users/walke/workspace/cpp-workspace/outside/vcpkg
PKG_CCFLAGS += -I$(VCPKG_DIR)/installed/x64-windows-static/include
PKG_LDFLAGS += -L$(VCPKG_DIR)/installed/x64-windows-static/lib -lSDL2-static -lSDL2_ttf -lfreetype -lbrotlidec -lbrotlicommon -lbz2 -llibpng16 -lzlib -lwinmm -limm32 -lversion -lsetupapi -lole32 -loleaut32 -lshell32 -lgdi32 -luser32 -ladvapi32 -lcfgmgr32

WARNINGS = -Wall -Wextra -Wpedantic -Werror -Wformat=2 -Wconversion -Wimplicit-fallthrough
CCFLAGS = $(PKG_CCFLAGS) -O3 -march=native -Iinclude/ $(WARNINGS) -fno-builtin-memcpy 
LDFLAGS = $(PKG_LDFLAGS)

HDRS = $(wildcard include/*.h)
SRCS = $(wildcard src/*.c)
ASMS = $(wildcard src/*.S)
OBJS = $(patsubst src/%.c,$(OBJS_DIR)/%.o,$(SRCS)) $(patsubst src/%.S,$(OBJS_DIR)/%.o,$(ASMS))

all: $(BIN_DIR)/$(BIN)

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

$(OBJS_DIR):
	@mkdir -p $(OBJS_DIR)

# c files
$(OBJS_DIR)/%.o: src/%.c | $(OBJS_DIR)
	$(CC) $(CCFLAGS) -c $< -o $@

# assembly files
$(OBJS_DIR)/%.o: src/%.S | $(OBJS_DIR)
	$(CC) $(CCFLAGS) -c $< -o $@

# linking elf
$(BIN_DIR)/$(BIN): $(OBJS) | $(BIN_DIR)
	$(LD) $^ $(LDFLAGS) -o $@

fmt:
	clang-format -i $(SRCS) $(HDRS)

lint:
	clang-format --dry-run --Werror $(SRCS) $(HDRS)
	clang-check --analyze --extra-arg=-Xanalyzer --extra-arg=-analyzer-output=text $(SRCS) -- $(CCFLAGS)

dump: $(BIN_DIR)/$(BIN)
	llvm-objdump -D $<

clean:
	rm -rf $(OBJS_DIR) $(BIN_DIR)

.PHONY: all fmt lint dump clean
