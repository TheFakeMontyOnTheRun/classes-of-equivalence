LDFLAGS=-L/opt/local/lib -lXt -lXaw -lX11
TARGET=level-editor
OBJ=mainwindow.o canvas.o loadmap.o savemap.o editor.o


$(TARGET): $(OBJ)
	$(CC) -o$(TARGET) $(OBJ) $(LDFLAGS)

all: $(TARGET)


clean:
	rm -rf $(OBJ)
	rm -rf $(TARGET)
