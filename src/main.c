#include "array.h"
#include "camera.h"
#include "display.h"
#include "light.h"
#include "matrix.h"
#include "mesh.h"
#include "texture.h"
#include "triangle.h"
#include "upng.h"
#include "vector.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_pixels.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

//  Array of triangles that should be rendered frame by frame
#define MAX_NUM_TRIANGLES 10000
triangle_t triangles_to_render[MAX_NUM_TRIANGLES];
int num_triangles_to_render = 0;

mat4_t proj_matrix;

enum cull_method cull_method;
enum render_method render_method;

bool is_running = NULL;

int previous_frame_time = 0;
float delta_time = 0;

void setup(void) {
  // Initialize render mode and triangle culling method
  render_method = RENDER_WIRE;
  cull_method = CULL_BACKFACE;

  // Allocate the required bytes in memory for
  // the color buffer
  color_buffer =
      (uint32_t *)malloc(sizeof(uint32_t) * window_width * window_height);
  z_buffer = (float *)malloc(sizeof(float) * window_width * window_height);

  if (!color_buffer) {
    fprintf(stderr, "Error allocating memory "
                    "for color_buffer. \n");
  }

  color_buffer_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                                           SDL_TEXTUREACCESS_STREAMING,
                                           window_width, window_height);

  // Initialize the perspective projection matrix
  float fov = M_PI / 3.0; // the same as 180/3, or 60 degrees
  float aspect = (float)window_height / (float)window_width;
  float znear = 0.1;
  float zfar = 100.0;
  proj_matrix = mat4_make_perspective(fov, aspect, znear, zfar);

  // manually load the hardcoded texture data from the literal static array
  // mesh_texture = (uint32_t *)REDBRICK_TEXTURE;
  // texture_width = 64;
  // texture_height = 64;

  // loads the cube values into the mesh data structure
  load_obj_file_data("./assets/f22.obj");
  load_png_texture_data("./assets/f22.png");

  previous_frame_time = SDL_GetTicks();
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
    case SDLK_5:
      render_method = RENDER_TEXTURED;
      break;
    case SDLK_6:
      render_method = RENDER_TEXTURED_WIRE;
      break;
    case SDLK_x:
      cull_method = CULL_BACKFACE;
      break;
    case SDLK_z:
      cull_method = CULL_NONE;
      break;
    case SDLK_UP:
      camera.position.y += 1.0 * delta_time;
      break;
    case SDLK_DOWN:
      camera.position.y -= 1.0 * delta_time;
      break;
    case SDLK_w:
      camera.forward_velocity = vec3_mul(camera.direction, 2.0 * delta_time);
      camera.position = vec3_add(camera.position, camera.forward_velocity);
      break;
    case SDLK_s:
      camera.forward_velocity = vec3_mul(camera.direction, 2.0 * delta_time);
      camera.position = vec3_sub(camera.position, camera.forward_velocity);
      break;
    case SDLK_a:
      camera.yaw_angle += 1.0 * delta_time;
      break;
    case SDLK_d:
      camera.yaw_angle -= 1.0 * delta_time;
      break;

    default:
      // Handle other keys
      break;
    }
    break;
  }
}

void update(void) {
  int time_to_wait = FRAME_TARGET_TIME - (SDL_GetTicks() - previous_frame_time);

  if (time_to_wait > 0 && time_to_wait <= FRAME_TARGET_TIME) {
    SDL_Delay(time_to_wait);
  }

  delta_time = (SDL_GetTicks() - previous_frame_time) / 1000.0f;

  previous_frame_time = SDL_GetTicks();

  num_triangles_to_render = 0;

  // Change the mesh scale, rotation, and translation values per animation frame
  mesh.rotation.x += 0.01 * delta_time;
  mesh.rotation.y += 0.01 * delta_time;
  mesh.rotation.z += 0.01 * delta_time;
  // mesh.translation.x += 0.01;
  mesh.translation.z = 4.0f;

  vec3_t target = {0,0,1};

  mat4_t camera_yaw_rotation = mat4_make_rotation_y(camera.yaw_angle);
  camera.direction = vec3_from_vec4(mat4_mul_vec4(camera_yaw_rotation, vec4_from_vec3(target)));

  target = vec3_add(camera.position, camera.direction);

  vec3_t up_direction = {0, 1, 0};
  mat4_t view_matrix = mat4_look_at(camera.position, target, up_direction);

  // Create scale, rotation, and translation matrices that will be used to
  // multiply the mesh vertices
  mat4_t scale_matrix =
      mat4_make_scale(mesh.scale.x, mesh.scale.y, mesh.scale.z);
  mat4_t translation_matrix = mat4_make_translation(
      mesh.translation.x, mesh.translation.y, mesh.translation.z);
  mat4_t rotation_matrix_x = mat4_make_rotation_x(mesh.rotation.x);
  mat4_t rotation_matrix_y = mat4_make_rotation_y(mesh.rotation.y);
  mat4_t rotation_matrix_z = mat4_make_rotation_z(mesh.rotation.z);

  int num_faces = array_length(mesh.faces);

  //  loop all triangles faces
  for (int i = 0; i < num_faces; i++) {
    face_t mesh_face = mesh.faces[i];

    vec3_t face_vertices[3];
    face_vertices[0] = mesh.vertices[mesh_face.a];
    face_vertices[1] = mesh.vertices[mesh_face.b];
    face_vertices[2] = mesh.vertices[mesh_face.c];

    // loop all three vertices of this current face and apply
    // transformations
    vec4_t transformed_vertices[3];

    // Loop all three vertices of this current face and apply transformations
    for (int j = 0; j < 3; j++) {
      vec4_t transformed_vertex = vec4_from_vec3(face_vertices[j]);

      // create a World Matrix combining scale, rotation, and translation to
      // place the vector in the "world"

      mat4_t world_matrix = mat4_identity();

      //  order matters: First scale, then rotate, then translate
      // [T]*[R]*[S]*v
      //
      world_matrix = mat4_mul_mat4(scale_matrix, world_matrix);
      world_matrix = mat4_mul_mat4(rotation_matrix_x, world_matrix);
      world_matrix = mat4_mul_mat4(rotation_matrix_y, world_matrix);
      world_matrix = mat4_mul_mat4(rotation_matrix_z, world_matrix);
      world_matrix = mat4_mul_mat4(translation_matrix, world_matrix);

      transformed_vertex = mat4_mul_vec4(world_matrix, transformed_vertex);

      // multiply the view matrix by the vector to transform the scene to
      // camera space
      transformed_vertex = mat4_mul_vec4(view_matrix, transformed_vertex);

      // Save transformed vertex in the array of transformed vertices
      transformed_vertices[j] = transformed_vertex;
    }

    // Check Backface Culling Algorithm (5)
    vec3_t vector_a = vec3_from_vec4(transformed_vertices[0]);
    vec3_t vector_b = vec3_from_vec4(transformed_vertices[1]);
    vec3_t vector_c = vec3_from_vec4(transformed_vertices[2]);

    // 1. Find vectors B-A and C-A
    vec3_t vector_ab = vec3_sub(vector_b, vector_a);
    vec3_t vector_ac = vec3_sub(vector_c, vector_a);
    vec3_normalize(&vector_ab);
    vec3_normalize(&vector_ac);

    // 2. Take their cross product and find the perpendicular normal N
    vec3_t normal = vec3_cross(vector_ab, vector_ac);

    // normalize the face normal vector
    vec3_normalize(&normal);

    vec3_t origin = {0, 0, 0};
    vec3_t camera_ray = vec3_sub(origin, vector_a);

    // 4. Take the Dot Product between normal N and the Camera Ray
    float dot_normal_camera = vec3_dot(normal, camera_ray);

    // 5. If this dot product is less than zero, then do not display the face
    if (cull_method == CULL_BACKFACE) {
      if (dot_normal_camera < 0) {
        // cull face by bypassing the rest of the function
        continue;
      }
    }

    // Loop all three vertices to perform projection
    vec4_t projected_points[3];
    for (int j = 0; j < 3; j++) {

      // project current vertex
      projected_points[j] =
          mat4_mul_vec4_project(proj_matrix, transformed_vertices[j]);

      // scale into the view
      projected_points[j].x *= (window_width / 2.0);
      projected_points[j].y *= (window_height / 2.0);

      // Invert the Y values because our obj comes with it's Y Values flipped
      projected_points[j].y *= -1;

      // translate projected points to the middle of the screen.
      projected_points[j].x += (window_width / 2.0);
      projected_points[j].y += (window_height / 2.0);
    }

    // Calculate the shade intensity based on how aligned is the face normal and
    // the light ray
    float light_intensity_factor = -vec3_dot(normal, light.direction);

    uint32_t triangle_color =
        light_apply_intensity(mesh_face.color, light_intensity_factor);

    triangle_t projected_triangle = {
        .points =
            {

                {projected_points[0].x, projected_points[0].y,
                 projected_points[0].z, projected_points[0].w},
                {projected_points[1].x, projected_points[1].y,
                 projected_points[1].z, projected_points[1].w},
                {projected_points[2].x, projected_points[2].y,
                 projected_points[2].z, projected_points[2].w}

            },
        .texcoords = {{mesh_face.a_uv.u, mesh_face.a_uv.v},
                      {mesh_face.b_uv.u, mesh_face.b_uv.v},
                      {mesh_face.c_uv.u, mesh_face.c_uv.v}},
        .color = triangle_color

    };

    //  save the projected triangle in the array of triangles to render.
    if (num_triangles_to_render < MAX_NUM_TRIANGLES) {
      triangles_to_render[num_triangles_to_render] = projected_triangle;
      num_triangles_to_render++;
    }
  }
}

void render(void) {
  draw_grid(0xFF404040);

  // loop all projected triangles and render them
  for (int i = 0; i < num_triangles_to_render; i++) {
    triangle_t triangle = triangles_to_render[i];

    if (render_method == RENDER_WIRE_VERTEX) {
      draw_rect(triangle.points[0].x - 3, triangle.points[0].y - 3, 6, 6,
                0xFFFFFF00);
      draw_rect(triangle.points[1].x - 3, triangle.points[1].y - 3, 6, 6,
                0xFFFFFF00);
      draw_rect(triangle.points[2].x - 3, triangle.points[2].y - 3, 6, 6,
                0xFFFFFF00);
    }

    if (render_method == RENDER_TEXTURED ||
        render_method == RENDER_TEXTURED_WIRE) {
      draw_textured_triangle(
          triangle.points[0].x, triangle.points[0].y, triangle.points[0].z,
          triangle.points[0].w, triangle.texcoords[0].u,
          triangle.texcoords[0].v, // vertex A
          triangle.points[1].x, triangle.points[1].y, triangle.points[1].z,
          triangle.points[1].w, triangle.texcoords[1].u,
          triangle.texcoords[1].v, // vertex B
          triangle.points[2].x, triangle.points[2].y, triangle.points[2].z,
          triangle.points[2].w, triangle.texcoords[2].u,
          triangle.texcoords[2].v, // vertex C
          mesh_texture);
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

          triangle.points[0].x, triangle.points[0].y, triangle.points[0].z,
          triangle.points[0].w, triangle.points[1].x, triangle.points[1].y,
          triangle.points[1].z, triangle.points[1].w, triangle.points[2].x,
          triangle.points[2].y, triangle.points[2].z, triangle.points[2].w,
          triangle.color

      );
    }
  }

  render_color_buffer();

  clear_color_buffer(0xFF000000);
  clear_z_buffer();

  SDL_RenderPresent(renderer);
}

// Free the memory that was dynamically allocated
void free_resources(void) {
  array_free(mesh.faces);
  array_free(mesh.vertices);
  free(color_buffer);
  free(z_buffer);
  upng_free(png_texture);
}

//

int main(void) {
  is_running = initialize_window();

  if (!is_running) {
    return 1;
  }

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
