#include <raylib.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include "mo_colors.h"

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 180
#define TILE_WIDTH 16
#define TILE_HEIGHT 24
#define TILE_ORIGIN ((Vector2){TILE_WIDTH / 2.f, TILE_HEIGHT / 2.f})
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX_FRAMES 4
#define MAX_ACTORS 128

typedef struct {
  Texture2D interface;
  Texture2D terrain;
  Texture2D monsters;
  Texture2D avatar;
  Texture2D fx_general;
} dc_Tilesets;

typedef struct {
  Texture2D skeleton_textures[MAX_FRAMES];
  Rectangle skeleton_rects[MAX_FRAMES];
  unsigned int skeleton_frames;
  Texture2D slice_textures[MAX_FRAMES];
  Rectangle slice_rects[MAX_FRAMES];
  unsigned int slice_frames;
} dc_Frames;

typedef struct {
  bool door_north;
  bool door_south;
  bool door_east;
  bool door_west;
} dc_Room;

typedef struct {
  Texture2D* textures;
  Rectangle* sources;
  Vector2 position;
  Vector2 velocity;
  float time_per_frame;
  float time_until_next_frame;
  unsigned int current_frame;
  bool has_shadow;
  Vector2 shadow_offset;
  unsigned int frame_count;
  // bool free_on_anim_comp;
} dc_Actor;

double dc_get_vector_length(Vector2 v) {
  return v.x * v.x + v.y * v.y; 
}

Vector2 dc_normalize_vector(Vector2 v) {
  double length = dc_get_vector_length(v);
  return (Vector2){v.x / length, v.y / length};
}

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
    DrawTexturePro(tilesets.interface, (Rectangle){160, 120, 16, 24}, (Rectangle){15 + TILE_WIDTH*i, 15, 16, 24}, (Vector2){0, 0}, 0.f, WHITE);
    DrawTexturePro(tilesets.interface, (Rectangle){176, 120, 16, 24}, (Rectangle){15 + TILE_WIDTH*i, 14, 16, 24}, (Vector2){0, 0}, 0.f, RED);
  }
  for(int i = 0; i < half_hearts; i++) {
    DrawTexturePro(tilesets.interface, (Rectangle){144, 120, 16, 24}, (Rectangle){15 + TILE_WIDTH*i + full_hearts*TILE_WIDTH, 15, 16, 24}, (Vector2){0, 0}, 0.f, RED);
    DrawTexturePro(tilesets.interface, (Rectangle){160, 120, 16, 24}, (Rectangle){15 + TILE_WIDTH*i + full_hearts*TILE_WIDTH, 15, 16, 24}, (Vector2){0, 0}, 0.f, WHITE);
  }
  for(int i = 0; i < empty_hearts; i++) {
    DrawTexturePro(tilesets.interface, (Rectangle){160, 120, 16, 24}, (Rectangle){15 + TILE_WIDTH*i + full_hearts*TILE_WIDTH + half_hearts*TILE_WIDTH, 15, 16, 24}, (Vector2){0, 0}, 0.f, WHITE);
  }
}

//void dc_draw_player_targeting(dc_Tilesets tilesets, dc_Actor player, Camera2D cam) {
void dc_draw_player_targeting(dc_Tilesets tilesets, dc_Actor* player) {
  // Vector2 mouse_pos = GetWorldToScreen2D(GetMousePosition(), cam);
  Vector2 mouse_pos = GetMousePosition();
  mouse_pos.x = mouse_pos.x * (SCREEN_WIDTH / (float)GetScreenWidth());
  mouse_pos.y = mouse_pos.y * (SCREEN_HEIGHT / (float)GetScreenHeight());
  // Vector2 dir_to_mouse = dc_normalize_vector((Vector2){mouse_pos.x - player.position.x, mouse_pos.y - player.position.y});
  double angle_to_mouse = atan2(mouse_pos.y - player->position.y, mouse_pos.x - player->position.x) * 180 / PI + 135;
  DrawTexturePro(tilesets.fx_general, (Rectangle){12 * TILE_WIDTH, 0, TILE_WIDTH, TILE_HEIGHT}, (Rectangle){player->position.x, player->position.y, TILE_WIDTH, TILE_HEIGHT}, (Vector2){15, 16}, angle_to_mouse, WHITE);
}

void dc_Actor_update(dc_Actor* const actor_ptr, float dt) {
  actor_ptr->time_until_next_frame -= dt;
  if(actor_ptr->time_until_next_frame <= 0) {
    if(actor_ptr->current_frame+1 >= actor_ptr->frame_count) {
      actor_ptr->current_frame = 0;
    } else {
      actor_ptr->current_frame++;
    }
  actor_ptr->time_until_next_frame += actor_ptr->time_per_frame;
  }
}

void dc_Actor_draw(dc_Actor* actor) {
  if(actor->has_shadow) DrawEllipse(actor->position.x + actor->shadow_offset.x, actor->position.y + actor->shadow_offset.y, TILE_WIDTH / 2.f, 2, GRAY);
  Rectangle dest = {actor->position.x, actor->position.y, TILE_WIDTH, TILE_HEIGHT};
  DrawTexturePro(actor->textures[actor->current_frame], actor->sources[actor->current_frame], dest, TILE_ORIGIN, 0.f, WHITE);
}

dc_Actor* dc_Actor_create_player(dc_Frames* frame_data) {
  dc_Actor* player = malloc(sizeof(dc_Actor));
  player->textures = frame_data->skeleton_textures;
  player->sources = frame_data->skeleton_rects;
  player->position = (Vector2){100, 100};
  player->velocity = (Vector2){0};
  player->time_per_frame = 0.5f;
  player->time_until_next_frame = 0.5f;
  player->current_frame = 0;
  player->has_shadow = true;
  player->shadow_offset = (Vector2){0, TILE_HEIGHT * 0.3};
  player->frame_count = frame_data->skeleton_frames;

  return player;
}

dc_Actor* dc_Actor_create_slice(dc_Frames* frame_data) {
  dc_Actor* slice = malloc(sizeof(dc_Actor));
  slice->textures = frame_data->slice_textures;
  slice->sources = frame_data->slice_rects;
  slice->position = (Vector2){100, 100};
  slice->velocity = (Vector2){0};
  slice->time_per_frame = 0.1f;
  slice->time_until_next_frame = 0.1f;
  slice->current_frame = 0;
  slice->has_shadow = false;
  // slice->shadow_offset = (Vector2){0, TILE_HEIGHT * 0.3};
  slice->frame_count = frame_data->slice_frames;

  return slice;
}

Vector2 dc_get_player_input_vector() {
  Vector2 p_input_vec = (Vector2){0};
  if(IsKeyDown(KEY_A)) {
    p_input_vec.x -= 1;
  }
  if(IsKeyDown(KEY_D)) {
    p_input_vec.x += 1;
  }
  if(IsKeyDown(KEY_W)) {
    p_input_vec.y -= 1;
  }
  if(IsKeyDown(KEY_S)) {
    p_input_vec.y += 1;
  }
  return dc_get_vector_length(p_input_vec) == 0 ? (Vector2){0} : dc_normalize_vector(p_input_vec);
}

int main(int argc, char** argv) {
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "SHITHEAD WIZARD");
  SetWindowState(FLAG_WINDOW_RESIZABLE);

  Texture2D f_tex = LoadTexture("./gfx/frame.png");
  Rectangle f_rect = {0, 0, f_tex.width, f_tex.height};

  dc_Tilesets tilesets = {.interface = LoadTexture("./gfx/oryx/Interface.png"), .terrain = LoadTexture("./gfx/oryx/Terrain.png"), .monsters = LoadTexture("./gfx/oryx/Monsters.png"), .avatar = LoadTexture("./gfx/oryx/Avatar.png"), .fx_general = LoadTexture("./gfx/oryx/FX_General.png")};

  RenderTexture2D r_target = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
  //SetTextureFilter(r_target.texture, TEXTURE_FILTER_POINT);
  Rectangle r_target_rect = {0, 0, r_target.texture.width, -r_target.texture.height};

  Font font = LoadFontEx("./gfx/Perfect DOS VGA 437.ttf", 16.f*4, NULL, 0);
  //SetTextureFilter(font.texture, TEXTURE_FILTER_POINT);

  unsigned int number_of_rooms = 10;
  dc_Room** rooms = malloc(sizeof(dc_Room*) * number_of_rooms);
  for(unsigned int i = 0; i < number_of_rooms; i++) {
    rooms[i] = NULL;
  }
  rooms[0] = malloc(sizeof(dc_Room));
  *rooms[0] = (dc_Room){0};
  rooms[0]->door_north = true;
  rooms[0]->door_west = true;

  dc_Frames frame_data = {
    .skeleton_textures = {tilesets.monsters, tilesets.monsters},
    .skeleton_rects = {(Rectangle){1 * TILE_WIDTH, 12 * TILE_HEIGHT, TILE_WIDTH, TILE_HEIGHT}, (Rectangle){1 * TILE_WIDTH, 13 * TILE_HEIGHT, TILE_WIDTH, TILE_HEIGHT}},
    .skeleton_frames = 2,
    .slice_textures = {tilesets.fx_general, tilesets.fx_general, tilesets.fx_general},
    .slice_rects = {(Rectangle){12 * TILE_WIDTH, 0, TILE_WIDTH, TILE_HEIGHT}, (Rectangle){13 * TILE_WIDTH, 0, TILE_WIDTH, TILE_HEIGHT}, (Rectangle){14 * TILE_WIDTH, 0, TILE_WIDTH, TILE_HEIGHT}},
    .slice_frames = 3
  };

  dc_Actor* player = dc_Actor_create_player(&frame_data);

  dc_Actor* actors[MAX_ACTORS] = {player};
  actors[1] = dc_Actor_create_slice(&frame_data);
  for(int a = 2; a < MAX_ACTORS; a++) {
    actors[a] = NULL;
  }

  Camera2D cam = {(Vector2){0}, (Vector2){0}, 0.f, 1.f};

  while(!WindowShouldClose()) {
    float dt = MIN(GetFrameTime(), 1000.f/15.f); // cap how slow the game can run because i'm not doing interpolation for your commodore 64
    Vector2 p_input_v = dc_get_player_input_vector();
    player->position.x += p_input_v.x * dt * 100;
    player->position.y += p_input_v.y * dt * 100;

    if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
      for(int a = 0; a < MAX_ACTORS; a++) {
        if(actors[a] != NULL) continue;

      }
    }

    for(int a = 0; a < MAX_ACTORS; a++) {
      if(actors[a] != NULL) dc_Actor_update(actors[a], dt);
    }

    BeginDrawing();
      BeginTextureMode(r_target);
      ClearBackground(BLACK);

      BeginMode2D(cam);
      dc_Room_draw(tilesets, *rooms[0]);

      for(int a = 0; a < MAX_ACTORS; a++) {
        if(actors[a] != NULL) dc_Actor_draw(actors[a]);
      }
      EndMode2D();

      DrawTexturePro(f_tex, f_rect, f_rect, (Vector2){0}, 0.f, WHITE);

      dc_draw_player_health(tilesets, 3, 10);
      dc_draw_player_targeting(tilesets, player);

      DrawTextEx(font, "Monsters Remaining ?/?", (Vector2){100, 20}, 16.f, 0.1f, WHITE);
      // SetTextureFilter

      EndTextureMode();
      Rectangle r_window_rect = {0, 0, GetScreenWidth(), GetScreenHeight()};
      DrawTexturePro(r_target.texture, r_target_rect, r_window_rect, (Vector2){0, 0}, 0.f, WHITE);
    EndDrawing();
  }

  CloseWindow();
  UnloadTexture(f_tex);
  UnloadTexture(tilesets.interface);
  UnloadTexture(tilesets.terrain);
  UnloadTexture(tilesets.monsters);
  UnloadTexture(tilesets.avatar);
  UnloadTexture(tilesets.fx_general);
  for(unsigned int i = 0; i < number_of_rooms; i++) {
    if(rooms[i] != NULL) free(rooms[i]);
  }
  free(rooms);

  for(unsigned int a = 0; a < MAX_ACTORS; a++) {
    if(actors[a] != NULL) free(actors[a]);
  }

  UnloadFont(font);

  return 0;
}
