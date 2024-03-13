#include "array.h"
#include "display.h"
#include "mesh.h"
#include "vector.h"
#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

//  Array of triangles that should be rendered frame by frame
triangle_t *triangles_to_render = NULL;

vec3_t camera_position = {0, 0, 0};

float fov_factor = 640;

bool is_running = NULL;

int previous_frame_time = 0;

void setup(void) {
    // Initialize render mode and triangle culling method
    render_method = RENDER_WIRE;
    cull_method = CULL_BACKFACE;

    // Allocate the required bytes in memory for
    // the color buffer
    color_buffer =
        (uint32_t *)malloc(sizeof(uint32_t) * window_width * window_height);

    if (!color_buffer) {
        fprintf(stderr, "Error allocating memory "
                        "for color_buffer. \n");
    }

    color_buffer_texture =
        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                          SDL_TEXTUREACCESS_STREAMING, window_width, window_height);

    load_cube_mesh_data();
    // loads the cube values into the mesh data structure
    // load_obj_file_data("./assets/cube.obj");
    // load_obj_file_data("./assets/f22.obj");
}

void process_input(void) {
    SDL_Event event;
    SDL_PollEvent(&event);

    switch (event.type) {
    case SDL_QUIT:
        is_running = false;
        break;
    case SDL_KEYDOWN:
        switch (event.key.keysym.sym) {
        case SDLK_ESCAPE:
            is_running = false;
            break;
        case SDLK_1:
            render_method = RENDER_WIRE;
            break;
        case SDLK_2:
            render_method = RENDER_WIRE_VERTEX;
            break;
        case SDLK_3:
            render_method = RENDER_FILL_TRIANGLE;
            break;
        case SDLK_4:
            render_method = RENDER_FILL_TRIANGLE_WIRE;
            break;
        case SDLK_c:
            cull_method = CULL_BACKFACE;
            break;
        case SDLK_d:
            cull_method = CULL_NONE;
            break;
        default:
            // Handle other keys
            break;
        }
        break;
    }
}

vec2_t project(vec3_t point) {
    vec2_t projected_point = {.x = (fov_factor * point.x) / point.z,
                              .y = (fov_factor * point.y) / point.z};
    return projected_point;
}

void update(void) {
    int time_to_wait = FRAME_TARGET_TIME - (SDL_GetTicks() - previous_frame_time);

    if (time_to_wait > 0 && time_to_wait <= FRAME_TARGET_TIME) {
        SDL_Delay(time_to_wait);
    }

    previous_frame_time = SDL_GetTicks();

    triangles_to_render = NULL;

    mesh.rotation.x += 0.01;
    mesh.rotation.y += 0.0;
    mesh.rotation.z += 0.0;

    int num_faces = array_length(mesh.faces);

    //  loop all triangles faces
    for (int i = 0; i < num_faces; i++) {
        face_t mesh_face = mesh.faces[i];

        vec3_t face_vertices[3];
        face_vertices[0] = mesh.vertices[mesh_face.a - 1];
        face_vertices[1] = mesh.vertices[mesh_face.b - 1];
        face_vertices[2] = mesh.vertices[mesh_face.c - 1];

        // loop all three vertices of this current face and apply
        // transformations
        vec3_t transformed_vertices[3];
        for (int j = 0; j < 3; j++) {
            vec3_t transformed_vertex = face_vertices[j];

            transformed_vertex = vec3_rotate_x(transformed_vertex, mesh.rotation.x);
            transformed_vertex = vec3_rotate_y(transformed_vertex, mesh.rotation.y);
            transformed_vertex = vec3_rotate_z(transformed_vertex, mesh.rotation.z);

            // translate the vertex away from the camera
            transformed_vertex.z += 5;

            transformed_vertices[j] = transformed_vertex;
        }

        if (cull_method == CULL_BACKFACE) {
            // Check Backface Culling Algorithm (5)
            vec3_t vector_a = transformed_vertices[0];
            vec3_t vector_b = transformed_vertices[1];
            vec3_t vector_c = transformed_vertices[2];

            // 1. Find vectors B-A and C-A
            vec3_t vector_ab = vec3_sub(vector_b, vector_a);
            vec3_t vector_ac = vec3_sub(vector_c, vector_a);
            vec3_normalize(&vector_ab);
            vec3_normalize(&vector_ac);

            // 2. Take their cross product and find the perpendicular normal N
            vec3_t normal = vec3_cross(vector_ab, vector_ac);

            // normalize the face normal vector
            vec3_normalize(&normal);

            // 3. Find the Camera Ray vector by subtracting the Camera Position from
            // Point A
            vec3_t camera_ray = vec3_sub(camera_position, vector_a);

            // 4. Take the Dot Product between normal N and the Camera Ray
            float dot_normal_camera = vec3_dot(normal, camera_ray);

            // 5. If this dot product is less than zero, then do not display the face

            if (dot_normal_camera < 0) {
                // cull face by bypassing the rest of the function
                continue;
            }
        }

        // Loop all three vertices to perform projection
        vec2_t projected_points[3];
        for (int j = 0; j < 3; j++) {

            // project current vertex
            projected_points[j] = project(transformed_vertices[j]);

            // Scale and translate projected points to the middle of the screen.

            projected_points[j].x += (window_width / 2);
            projected_points[j].y += (window_height / 2);
        }

        // Calculate the average depth for each face based on the vertices after
        // transformation

        float avg_depth = (transformed_vertices[0].z + transformed_vertices[1].z +
                           transformed_vertices[2].z) /
                          3;

        triangle_t projected_triangle = {
            .points =
                {

                    {projected_points[0].x, projected_points[0].y},
                    {projected_points[1].x, projected_points[1].y},
                    {projected_points[2].x, projected_points[2].y}

                },
            .avg_depth = avg_depth,
            .color = mesh_face.color

        };

        //  save the projected triangle in the array of triangles to render.
        array_push(triangles_to_render, projected_triangle);
    }

    // Hacky Painters Algorithm -> Sort Array of Faces by avg_depth

    // Bubble Sort
    int num_triangles = array_length(triangles_to_render);
    for (int i = 0; i < num_triangles; i++) {
        for (int j = i; j < num_triangles; j++) {
            if (triangles_to_render[i].avg_depth <
                triangles_to_render[j].avg_depth) {
                // swap triangle positions in the array
                triangle_t temp = triangles_to_render[i];
                triangles_to_render[i] = triangles_to_render[j];
                triangles_to_render[j] = temp;
            }
        }
    }
}

void render(void) {
    draw_grid(0xFF404040);

    int num_triangles = array_length(triangles_to_render);
    // loop all projected triangles and render them
    for (int i = 0; i < num_triangles; i++) {
        triangle_t triangle = triangles_to_render[i];

        if (render_method == RENDER_WIRE_VERTEX) {
            draw_rect(triangle.points[0].x - 3, triangle.points[0].y - 3, 6, 6,
                      0xFFFFFF00);
            draw_rect(triangle.points[1].x - 3, triangle.points[1].y - 3, 6, 6,
                      0xFFFFFF00);
            draw_rect(triangle.points[2].x - 3, triangle.points[2].y - 3, 6, 6,
                      0xFFFFFF00);
        }

        if (render_method == RENDER_WIRE_VERTEX ||
            render_method == RENDER_FILL_TRIANGLE_WIRE ||
            render_method == RENDER_WIRE) {
            draw_triangle(

                triangle.points[0].x, triangle.points[0].y, triangle.points[1].x,
                triangle.points[1].y, triangle.points[2].x, triangle.points[2].y,
                0xFFFFFFFF

            );
        }

        if (render_method == RENDER_FILL_TRIANGLE ||
            render_method == RENDER_FILL_TRIANGLE_WIRE) {
            draw_filled_triangle(

                triangle.points[0].x, triangle.points[0].y, triangle.points[1].x,
                triangle.points[1].y, triangle.points[2].x, triangle.points[2].y,
                triangle.color

            );
        }
    }

    // clear the array of triangles to render every frame loop
    array_free(triangles_to_render);

    render_color_buffer();

    clear_color_buffer(0xFF000000);

    SDL_RenderPresent(renderer);
}

// Free the memory that was dynamically allocated
void free_resources(void) {
    array_free(mesh.faces);
    array_free(mesh.vertices);
}

//

int main(void) {
    is_running = initialize_window();

    // game loop
    setup();

    while (is_running) {
        process_input();
        update();
        render();
    }

    destroy_window();
    free_resources();
    return 0;
}
