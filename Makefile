BUILD_DIR = bin
SOURCES	  = $(wildcard *.c)
TOOLS     = $(basename $(SOURCES))
BINARIES  = $(foreach T, $(TOOLS), $(BUILD_DIR)/$T)

.PHONY: clean

all: $(BINARIES)

${BUILD_DIR}/%: %.c
	@mkdir -p bin
	$(CC) -Wall $^ -o $@

clean:
	rm -r bin
