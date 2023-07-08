HEADERS = mo_colors.h
OBJECTS = main.o
CC = gcc
ifeq ($(OS), Windows_NT)
	FLAGS = -lraylib -lopengl32 -lgdi32 -lwinmm -std=c99 -Wall -Wpedantic
else
	FLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -std=c99 -Wall -Wpedantic
endif

ifneq ($(BUILD), release)
	FLAGS += -g
endif

default: rpg

main.o: main.c $(HEADERS)
	$(CC) -c main.c -o main.o $(FLAGS)

rpg: $(OBJECTS)
	$(CC) $(OBJECTS) -o rpg $(FLAGS)

run:
	./rpg

clean:
	-rm -f main.o
	-rm -f rpg
