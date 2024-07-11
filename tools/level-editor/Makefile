LDFLAGS=-lXt -lXaw -lX11
TARGET=level-editor
OBJ=mainwindow.o


$(TARGET): $(OBJ)
	$(CC) -o$(TARGET) $(OBJ) $(LDFLAGS)

all: $(TARGET)


clean:
	rm -rf $(OBJ)
	rm -rf $(TARGET)
