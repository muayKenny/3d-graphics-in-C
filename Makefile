build-osx:
	gcc -Wall -std=c99 -arch arm64 \
	    -I/opt/homebrew/include \
	    ./src/*.c \
	    -L/opt/homebrew/lib \
	    -lSDL2 -lm -o renderer

build-osx-debug:
	gcc -g -O0 -Wall -Wextra -Wshadow -Wconversion -fsanitize=address -fno-omit-frame-pointer \
	    -std=c99 -arch arm64 \
	    -I/opt/homebrew/include \
	    ./src/*.c \
	    -L/opt/homebrew/lib \
	    -lSDL2 -lm -o renderer

build-linux:
	gcc -Wall -std=c99 -D_GNU_SOURCE ./src/*.c -lSDL2 -lm -o renderer


build-linux-debug:
	gcc -g -O0 -Wall -Wextra -Wshadow -Wconversion -fsanitize=address -fno-omit-frame-pointer \
	    -std=c99 -D_GNU_SOURCE ./src/*.c -lSDL2 -lm -o renderer

run:
	./renderer

clean:
	rm renderer
