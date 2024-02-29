#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include "display.h"

bool is_running = NULL;

void setup(void){
	// Allocate the required bytes in memory for the color buffer
	color_buffer = (uint32_t*) malloc(sizeof(uint32_t) * window_width * window_height);
	
	if (!color_buffer) {
		fprintf(stderr, "Error allocating memory for color_buffer. \n");
	}

	color_buffer_texture = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		window_width,
		window_height
	);
}

void process_input(void){
	SDL_Event event;
	SDL_PollEvent(&event);

	switch(event.type) {
		case SDL_QUIT:
			is_running = false;
			break;
		case SDL_KEYDOWN:
			if(event.key.keysym.sym == SDLK_ESCAPE)
				is_running = false;
			break;
	}
}

void update(void){

}

void render(void){
	SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
	SDL_RenderClear(renderer);

	render_color_buffer();
	clear_color_buffer(0xFF000000);
	draw_grid(0xFF404040);
	// draw_rect(600,50, 250,150, 0xFFFFFF00);
	// draw_pixel(300,307, 0xFFFFFF00);
	SDL_RenderPresent(renderer);
}

int main(void) {
	is_running = initialize_window();

	// game loop
	setup();

	while(is_running){
		process_input();
		update();
		render();
	}

	destroy_window();
	return 0;
}
