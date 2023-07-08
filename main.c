#include <raylib.h>
#include <stdlib.h>
#include <stdbool.h>
#include "mo_colors.h"

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 180
#define TILE_WIDTH 16
#define TILE_HEIGHT 24

typedef struct {
  Texture2D interface;
  Texture2D terrain;
} dc_Tilesets;

typedef struct {
  bool door_north;
  bool door_south;
  bool door_east;
  bool door_west;
} dc_Room;

void dc_Room_draw(dc_Tilesets tilesets, dc_Room room) {
  static const unsigned int room_width = 17;
  static const unsigned int room_height = 4;
  static const unsigned int horiz_center = room_width / 2;
  static const unsigned int vert_center = room_height / 2;

  // TODO: remove magic numbers for vert_wall_rect, horiz_wall_rect, closed_door_rect, offset for drawing stuff

  // north wall
  for(unsigned int i = 0; i < room_width; i++) {
    if(i == horiz_center && room.door_north) {
      DrawTexturePro(tilesets.terrain, (Rectangle){7 * TILE_WIDTH, 2 * TILE_HEIGHT, TILE_WIDTH, TILE_HEIGHT}, (Rectangle){TILE_WIDTH * 1.5 + TILE_WIDTH * i, TILE_HEIGHT * 2, TILE_WIDTH, TILE_HEIGHT}, (Vector2){0, 0}, 0.f, WHITE);
    } else {
      DrawTexturePro(tilesets.terrain, (Rectangle){0, 3 * TILE_HEIGHT, TILE_WIDTH, TILE_HEIGHT}, (Rectangle){TILE_WIDTH * 1.5 + TILE_WIDTH * i, TILE_HEIGHT * 2, TILE_WIDTH, TILE_HEIGHT}, (Vector2){0, 0}, 0.f, BRICKRED);
    }
  }

  // south wall
  for(unsigned int i = 0; i < room_width+2; i++) {
    if(i == horiz_center && room.door_south) {
      DrawTexturePro(tilesets.terrain, (Rectangle){7 * TILE_WIDTH, 2 * TILE_HEIGHT, TILE_WIDTH, TILE_HEIGHT}, (Rectangle){TILE_WIDTH * 0.5 + TILE_WIDTH * i, TILE_HEIGHT * 6, TILE_WIDTH, TILE_HEIGHT}, (Vector2){0, 0}, 0.f, WHITE);
    } else {
      DrawTexturePro(tilesets.terrain, (Rectangle){0, 3 * TILE_HEIGHT, TILE_WIDTH, TILE_HEIGHT}, (Rectangle){TILE_WIDTH * 0.5 + TILE_WIDTH * i, TILE_HEIGHT * 6, TILE_WIDTH, TILE_HEIGHT}, (Vector2){0, 0}, 0.f, BRICKRED);
    }
  }

  // west wall
  for(unsigned int i = 0; i < room_height; i++) {
    if(i == vert_center && room.door_west) {
      DrawTexturePro(tilesets.terrain, (Rectangle){7 * TILE_WIDTH, 2 * TILE_HEIGHT, TILE_WIDTH, TILE_HEIGHT}, (Rectangle){TILE_WIDTH * 0.5, TILE_HEIGHT * i + TILE_HEIGHT * 2, TILE_WIDTH, TILE_HEIGHT}, (Vector2){0, 0}, 0.f, WHITE);
    } else {
      DrawTexturePro(tilesets.terrain, (Rectangle){0, 2 * TILE_HEIGHT, TILE_WIDTH, TILE_HEIGHT}, (Rectangle){TILE_WIDTH * 0.5, TILE_HEIGHT * i + TILE_HEIGHT * 2, TILE_WIDTH, TILE_HEIGHT}, (Vector2){0, 0}, 0.f, BRICKRED);
    }
  }

  // east wall
  for(unsigned int i = 0; i < room_height; i++) {
    if(i == vert_center && room.door_east) {
      DrawTexturePro(tilesets.terrain, (Rectangle){7 * TILE_WIDTH, 2 * TILE_HEIGHT, TILE_WIDTH, TILE_HEIGHT}, (Rectangle){TILE_WIDTH * room_width + TILE_WIDTH * 1.5, TILE_HEIGHT * i + TILE_HEIGHT * 2, TILE_WIDTH, TILE_HEIGHT}, (Vector2){0, 0}, 0.f, WHITE);
    } else {
      DrawTexturePro(tilesets.terrain, (Rectangle){0 * TILE_WIDTH, 2 * TILE_HEIGHT, TILE_WIDTH, TILE_HEIGHT}, (Rectangle){TILE_WIDTH * room_width + TILE_WIDTH * 1.5, TILE_HEIGHT * i + TILE_HEIGHT * 2, TILE_WIDTH, TILE_HEIGHT}, (Vector2){0, 0}, 0.f, BRICKRED);
    }
  }
}

void dc_draw_player_health(dc_Tilesets tilesets, int hp, int hp_max) {
  unsigned int full_hearts = hp / 2;
  unsigned int half_hearts = hp % 2;
  unsigned int empty_hearts = hp_max / 2 - full_hearts - half_hearts;
  for(int i = 0; i < full_hearts; i++) {
    DrawTexturePro(tilesets.interface, (Rectangle){160, 120, 16, 24}, (Rectangle){15 + 24*i, 15, 16, 24}, (Vector2){0, 0}, 0.f, WHITE);
    DrawTexturePro(tilesets.interface, (Rectangle){176, 120, 16, 24}, (Rectangle){15 + 24*i, 14, 16, 24}, (Vector2){0, 0}, 0.f, RED);
  }
  for(int i = 0; i < half_hearts; i++) {
    DrawTexturePro(tilesets.interface, (Rectangle){144, 120, 16, 24}, (Rectangle){15 + 24*i + full_hearts*24, 15, 16, 24}, (Vector2){0, 0}, 0.f, RED);
    DrawTexturePro(tilesets.interface, (Rectangle){160, 120, 16, 24}, (Rectangle){15 + 24*i + full_hearts*24, 15, 16, 24}, (Vector2){0, 0}, 0.f, WHITE);
  }
  for(int i = 0; i < empty_hearts; i++) {
    DrawTexturePro(tilesets.interface, (Rectangle){160, 120, 16, 24}, (Rectangle){15 + 24*i + full_hearts*24 + half_hearts*24, 15, 16, 24}, (Vector2){0, 0}, 0.f, WHITE);
  }
}

int main(int argc, char** argv) {
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "SHITHEAD WIZARD");
  SetWindowState(FLAG_WINDOW_RESIZABLE);

  Texture2D f_tex = LoadTexture("./gfx/frame.png");
  Rectangle f_rect = {0, 0, f_tex.width, f_tex.height};

  dc_Tilesets tilesets = {.interface = LoadTexture("./gfx/oryx/Interface.png"), .terrain = LoadTexture("./gfx/oryx/Terrain.png")};

  RenderTexture2D r_target = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
  Rectangle r_target_rect = {0, 0, r_target.texture.width, -r_target.texture.height};

  unsigned int number_of_rooms = 10;
  dc_Room** rooms = malloc(sizeof(dc_Room*) * number_of_rooms);
  for(unsigned int i = 0; i < number_of_rooms; i++) {
    rooms[i] = NULL;
  }
  rooms[0] = malloc(sizeof(dc_Room));
  *rooms[0] = (dc_Room){0};
  rooms[0]->door_north = true;
  rooms[0]->door_west = true;

  while(!WindowShouldClose()) {
    BeginDrawing();
      BeginTextureMode(r_target);
      ClearBackground(BLACK);

      dc_Room_draw(tilesets, *rooms[0]);

      DrawTexturePro(f_tex, f_rect, f_rect, (Vector2){0}, 0.f, WHITE);

      dc_draw_player_health(tilesets, 3, 10);

      EndTextureMode();
      Rectangle r_window_rect = {0, 0, GetScreenWidth(), GetScreenHeight()};
      DrawTexturePro(r_target.texture, r_target_rect, r_window_rect, (Vector2){0, 0}, 0.f, WHITE);
    EndDrawing();
  }

  CloseWindow();
  UnloadTexture(f_tex);
  UnloadTexture(tilesets.interface);
  UnloadTexture(tilesets.terrain);
  for(unsigned int i = 0; i < number_of_rooms; i++) {
    if(rooms[i] != NULL) free(rooms[i]);
  }
  free(rooms);

  return 0;
}
