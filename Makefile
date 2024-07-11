CC=gcc

CFLAGS += -std=c11
CFLAGS += -Wall -Wpedantic
CFLAGS += -g

LDFLAGS += -g

OBJECTS := fakecom.o

TARGET := fakecom

.PHONY: all
all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) -o $@ $(LDFLAGS) $^

$(OBJECTS): %.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJECTS)
