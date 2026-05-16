CC = gcc
CFLAGS = -Wall -std=c99
CFLAGS_DEBUG = -Wall -g -std=c99
LDFLAGS = -lusb-1.0

SRCDIR = .
OUTDIR = bin
TARGET = $(OUTDIR)/usbscan

SOURCES = usbscan.c
OBJECTS = $(OUTDIR)/usbscan.o

.PHONY: all clean run debug help

all: CFLAGS += -O2
all: $(TARGET)

debug: CFLAGS = $(CFLAGS_DEBUG)
debug: $(TARGET)

$(OUTDIR):
	@mkdir -p $(OUTDIR)

$(TARGET): $(OUTDIR) $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "Build completed: $(TARGET)"

$(OUTDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -rf $(OUTDIR)
	@echo "Cleaned build artifacts"

run: $(TARGET)
	@./$(TARGET)

help:
	@echo "Available targets:"
	@echo "  all   - Build the project with optimization (default)"
	@echo "  debug - Build with debug symbols (-g flag)"
	@echo "  clean - Remove build artifacts"
	@echo "  run   - Build and run the program"
	@echo "  help  - Show this message"
