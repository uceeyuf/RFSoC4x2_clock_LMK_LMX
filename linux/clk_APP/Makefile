SRC = setclk.c
TARGET = build/setclk

all: $(TARGET)

$(TARGET): $(SRC)
	mkdir -p build
	$(CC) $(SRC) -o $(TARGET)

clean:
	rm -rf build

.PHONY: all clean
