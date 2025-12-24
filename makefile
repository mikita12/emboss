CC = gcc

CFLAGS = -O2 -Wall -Wextra
LDFLAGS = -lpng

TARGET = filter_png

SRC = png.c mierz.c
OBJ = png.o mierz.o

# domyślny obraz
IMG ?= input.png

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

run: $(TARGET)
	@if [ ! -f "$(IMG)" ]; then \
		echo " Image file not found: $(IMG)"; \
		echo " Use: make run IMG=your_image.png"; \
		exit 1; \
	fi
	./$(TARGET) $(IMG)

clean:
	rm -f $(TARGET) $(OBJ) out.png wyniki.txt

.PHONY: all run clean

