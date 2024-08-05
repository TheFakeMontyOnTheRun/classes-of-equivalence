CXXFLAGS=-I/usr/X11R7/include
LDFLAGS=-L/opt/local/lib -L/usr/X11R7/lib -lXt -lXaw -lX11
TARGET=level-editor-x11
OBJ=mainwindow.o canvas.o loadmap.o savemap.o editor.o

$(TARGET): $(OBJ)
	$(CC) -o$(TARGET) $(OBJ) $(LDFLAGS)

all: $(TARGET)


clean:
	rm -rf $(OBJ)
	rm -rf $(TARGET)
