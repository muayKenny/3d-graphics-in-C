SRC = ./src/*.c
SRC_NO_UPNG = $(filter-out ./src/upng.c,$(wildcard ./src/*.c))
BUILD_DIR = build
OBJ = $(patsubst ./src/%.c,$(BUILD_DIR)/%.o,$(wildcard ./src/*.c))

build-osx: clean-obj
	mkdir -p $(BUILD_DIR)
	for src in $(SRC_NO_UPNG); do \
		gcc -Wall -std=c99 -arch arm64 -I/opt/homebrew/include -c $$src -o $(BUILD_DIR)/$$(basename $${src%.c}).o; \
	done
	gcc -Wall -Wno-unused-but-set-variable -std=c99 -arch arm64 -I/opt/homebrew/include -c ./src/upng.c -o $(BUILD_DIR)/upng.o
	gcc $(OBJ) -L/opt/homebrew/lib -lSDL2 -lm -o renderer

build-osx-debug: clean-obj
	mkdir -p $(BUILD_DIR)
	for src in $(SRC_NO_UPNG); do \
		gcc -g -O0 -Wall -Wextra -Wshadow -Wconversion -fsanitize=address -fno-omit-frame-pointer -std=c99 -arch arm64 -I/opt/homebrew/include -c $$src -o $(BUILD_DIR)/$$(basename $${src%.c}).o; \
	done
	gcc -g -O0 -Wall -Wextra -Wshadow -Wconversion -Wno-unused-but-set-variable -fsanitize=address -fno-omit-frame-pointer -std=c99 -arch arm64 -I/opt/homebrew/include -c ./src/upng.c -o $(BUILD_DIR)/upng.o
	gcc $(OBJ) -L/opt/homebrew/lib -lSDL2 -lm -fsanitize=address -o renderer

build-linux: clean-obj
	mkdir -p $(BUILD_DIR)
	for src in $(SRC_NO_UPNG); do \
		gcc -Wall -std=c99 -D_GNU_SOURCE -c $$src -o $(BUILD_DIR)/$$(basename $${src%.c}).o; \
	done
	gcc -Wall -Wno-unused-but-set-variable -std=c99 -D_GNU_SOURCE -c ./src/upng.c -o $(BUILD_DIR)/upng.o
	gcc $(OBJ) -lSDL2 -lm -o renderer

build-linux-debug: clean-obj
	mkdir -p $(BUILD_DIR)
	for src in $(SRC_NO_UPNG); do \
		gcc -g -O0 -Wall -Wextra -Wshadow -Wconversion -fsanitize=address -fno-omit-frame-pointer -std=c99 -D_GNU_SOURCE -c $$src -o $(BUILD_DIR)/$$(basename $${src%.c}).o; \
	done
	gcc -g -O0 -Wall -Wextra -Wshadow -Wconversion -Wno-unused-but-set-variable -fsanitize=address -fno-omit-frame-pointer -std=c99 -D_GNU_SOURCE -c ./src/upng.c -o $(BUILD_DIR)/upng.o
	gcc $(OBJ) -lSDL2 -lm -fsanitize=address -o renderer

run:
	./renderer

clean:
	rm -f renderer
	rm -rf $(BUILD_DIR)

clean-obj:
	rm -f $(BUILD_DIR)/*.o
