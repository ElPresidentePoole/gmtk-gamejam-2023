#include <raylib.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include "mo_colors.h"

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 180
#define TILE_WIDTH 16.f
#define TILE_HEIGHT 24.f
#define TILE_ORIGIN ((Vector2){TILE_WIDTH / 2.f, TILE_HEIGHT / 2.f})
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX_FRAMES 4
#define MAX_ACTORS 128
#define IFRAME_DURATION 1.f
#define IFRAME_FLASH_SPEED 4.f // N times a second
#define FLOOR_WIDTH 5
#define FLOOR_HEIGHT 5
#define ROOMS_LENGTH (FLOOR_WIDTH * FLOOR_HEIGHT)

#define COL_LAYER_PLAYER 1 // 0b01
#define COL_LAYER_ENEMY 2 //  0b10

typedef struct {
  Texture2D interface;
  Texture2D terrain;
  Texture2D monsters;
  Texture2D avatar;
  Texture2D fx_general;

  Texture2D zach;
} dc_Tilesets;

typedef struct {
  Sound door_open;
} dc_Sounds;

typedef struct {
  Texture2D skeleton_textures[MAX_FRAMES];
  Rectangle skeleton_rects[MAX_FRAMES];
  unsigned int skeleton_frames;
  Texture2D slice_textures[MAX_FRAMES];
  Rectangle slice_rects[MAX_FRAMES];
  unsigned int slice_frames;
  Texture2D dwarf_textures[MAX_FRAMES];
  Rectangle dwarf_rects[MAX_FRAMES];
  unsigned int dwarf_frames;
} dc_Frames;

typedef struct {
  bool door_north;
  bool door_south;
  bool door_east;
  bool door_west;
  unsigned int remaining_monsters;
  bool doors_opened;
} dc_Room;

typedef struct dc_Actor_s {
  Texture2D* textures;
  Rectangle* sources;
  Color color;
  Vector2 position;
  Vector2 velocity;
  Vector2 origin;
  float rotation;
  float time_per_frame;
  float time_until_next_frame;
  unsigned int current_frame;
  bool has_shadow;
  Vector2 shadow_offset;
  unsigned int frame_count;
  bool free_on_anim_comp;
  bool should_be_freed;
  unsigned int collision_layer;
  unsigned int collision_mask;
  int collision_damage;
  float iframe_time_remaining;
  int hp;
  int hp_max;
  void (*ai)(struct dc_Actor_s* self, struct dc_Actor_s* player);
} dc_Actor;

float dc_clampf(float n, float min, float max) {
  return min > n ? min : max < n ? max : n;
}

Vector2 dc_get_screen_scaling_percent(void) {
  return (Vector2){SCREEN_WIDTH / (float)GetScreenWidth(), SCREEN_HEIGHT / (float)GetScreenHeight()};
}

double dc_get_vector_length(Vector2 v) {
  return v.x * v.x + v.y * v.y; 
}

Vector2 dc_normalize_vector(Vector2 v) {
  double length = dc_get_vector_length(v);
  return (Vector2){v.x / length, v.y / length};
}

Vector2 dc_get_direction_to(Vector2 from, Vector2 to) {
  float dx = to.x - from.x;
  float dy = to.y - from.y;
  float rot = atan2(dy, dx);
  return (Vector2){cos(rot), sin(rot)};
}

void dc_Room_draw(dc_Tilesets tilesets, dc_Room* const room) {
  static const unsigned int room_width = 17;
  static const unsigned int room_height = 4;
  static const unsigned int horiz_center = room_width / 2;
  static const unsigned int vert_center = room_height / 2;

  Rectangle hori_wall_rect = (Rectangle){0, 7 * TILE_HEIGHT, TILE_WIDTH, TILE_HEIGHT};
  Rectangle vert_wall_rect = (Rectangle){0, 7 * TILE_HEIGHT, TILE_WIDTH, TILE_HEIGHT};
  Rectangle closed_door_rect = (Rectangle){2 * TILE_WIDTH, 7 * TILE_HEIGHT, TILE_WIDTH, TILE_HEIGHT};
  Rectangle opened_door_rect = (Rectangle){1 * TILE_WIDTH, 7 * TILE_HEIGHT, TILE_WIDTH, TILE_HEIGHT};

  // TODO: remove magic numbers for vert_wall_rect, horiz_wall_rect, closed_door_rect, offset for drawing stuff

  // north wall
  for(unsigned int i = 0; i < room_width; i++) {
    if(i == horiz_center && room->door_north) {
      if(room->doors_opened) DrawTexturePro(tilesets.zach, opened_door_rect, (Rectangle){TILE_WIDTH * 1.5 + TILE_WIDTH * i, TILE_HEIGHT * 2, TILE_WIDTH, TILE_HEIGHT}, (Vector2){0, 0}, 0.f, WHITE);
      else DrawTexturePro(tilesets.zach, closed_door_rect, (Rectangle){TILE_WIDTH * 1.5 + TILE_WIDTH * i, TILE_HEIGHT * 2, TILE_WIDTH, TILE_HEIGHT}, (Vector2){0, 0}, 0.f, WHITE);
    } else {
      DrawTexturePro(tilesets.zach, hori_wall_rect, (Rectangle){TILE_WIDTH * 1.5 + TILE_WIDTH * i, TILE_HEIGHT * 2, TILE_WIDTH, TILE_HEIGHT}, (Vector2){0, 0}, 0.f, BRICKRED);
    }
  }

  // south wall
  for(unsigned int i = 0; i < room_width+2; i++) {
    if(i == horiz_center && room->door_south) {
      if(room->doors_opened) DrawTexturePro(tilesets.zach, opened_door_rect, (Rectangle){TILE_WIDTH * 0.5 + TILE_WIDTH * i, TILE_HEIGHT * 6, TILE_WIDTH, TILE_HEIGHT}, (Vector2){0, 0}, 0.f, WHITE);
      else DrawTexturePro(tilesets.zach, closed_door_rect, (Rectangle){TILE_WIDTH * 0.5 + TILE_WIDTH * i, TILE_HEIGHT * 6, TILE_WIDTH, TILE_HEIGHT}, (Vector2){0, 0}, 0.f, WHITE);
    } else {
      DrawTexturePro(tilesets.zach, hori_wall_rect, (Rectangle){TILE_WIDTH * 0.5 + TILE_WIDTH * i, TILE_HEIGHT * 6, TILE_WIDTH, TILE_HEIGHT}, (Vector2){0, 0}, 0.f, BRICKRED);
    }
  }

  // west wall
  for(unsigned int i = 0; i < room_height; i++) {
    if(i == vert_center && room->door_west) {
      if(room->doors_opened) DrawTexturePro(tilesets.zach, opened_door_rect, (Rectangle){TILE_WIDTH * 0.5, TILE_HEIGHT * i + TILE_HEIGHT * 2, TILE_WIDTH, TILE_HEIGHT}, (Vector2){0, 0}, 0.f, WHITE);
      else DrawTexturePro(tilesets.zach, closed_door_rect, (Rectangle){TILE_WIDTH * 0.5, TILE_HEIGHT * i + TILE_HEIGHT * 2, TILE_WIDTH, TILE_HEIGHT}, (Vector2){0, 0}, 0.f, WHITE);
    } else {
      DrawTexturePro(tilesets.zach, vert_wall_rect, (Rectangle){TILE_WIDTH * 0.5, TILE_HEIGHT * i + TILE_HEIGHT * 2, TILE_WIDTH, TILE_HEIGHT}, (Vector2){0, 0}, 0.f, BRICKRED);
    }
  }

  // east wall
  for(unsigned int i = 0; i < room_height; i++) {
    if(i == vert_center && room->door_east) {
      if(room->doors_opened) DrawTexturePro(tilesets.zach, opened_door_rect, (Rectangle){TILE_WIDTH * room_width + TILE_WIDTH * 1.5, TILE_HEIGHT * i + TILE_HEIGHT * 2, TILE_WIDTH, TILE_HEIGHT}, (Vector2){0, 0}, 0.f, WHITE);
      else DrawTexturePro(tilesets.zach, closed_door_rect, (Rectangle){TILE_WIDTH * room_width + TILE_WIDTH * 1.5, TILE_HEIGHT * i + TILE_HEIGHT * 2, TILE_WIDTH, TILE_HEIGHT}, (Vector2){0, 0}, 0.f, WHITE);
    } else {
      DrawTexturePro(tilesets.zach, vert_wall_rect, (Rectangle){TILE_WIDTH * room_width + TILE_WIDTH * 1.5, TILE_HEIGHT * i + TILE_HEIGHT * 2, TILE_WIDTH, TILE_HEIGHT}, (Vector2){0, 0}, 0.f, BRICKRED);
    }
  }
}

void dc_draw_player_health(dc_Tilesets tilesets, int hp, int hp_max) {
  unsigned int full_hearts = hp / 2;
  unsigned int half_hearts = hp % 2;
  unsigned int empty_hearts = full_hearts + half_hearts - hp_max / 2;
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
  Vector2 screen_scale = dc_get_screen_scaling_percent();
  mouse_pos.x = mouse_pos.x * screen_scale.x;
  mouse_pos.y = mouse_pos.y * screen_scale.y;
  // Vector2 dir_to_mouse = dc_normalize_vector((Vector2){mouse_pos.x - player.position.x, mouse_pos.y - player.position.y});
  double angle_to_mouse = atan2(mouse_pos.y - player->position.y, mouse_pos.x - player->position.x) * RAD2DEG + 135;
  DrawTexturePro(tilesets.fx_general, (Rectangle){12 * TILE_WIDTH, 0, TILE_WIDTH, TILE_HEIGHT}, (Rectangle){player->position.x, player->position.y, TILE_WIDTH, TILE_HEIGHT}, (Vector2){15, 16}, angle_to_mouse, TBLUE);
}

void dc_ai_bat(dc_Actor* const self, dc_Actor* const player) {
  if(player == NULL) return;
  if(self->iframe_time_remaining > 0) return;
  static const int BAT_SPEED = 50;

  self->velocity = dc_get_direction_to(self->position, player->position);
  self->velocity.x *= BAT_SPEED;
  self->velocity.y *= BAT_SPEED;
}

void dc_Actor_update(dc_Actor* const actor_ptr, float dt) {
  // update frames/anims
  actor_ptr->time_until_next_frame -= dt;
  if(actor_ptr->time_until_next_frame <= 0) {
    if(actor_ptr->current_frame+1 >= actor_ptr->frame_count) {
      actor_ptr->current_frame = 0;
      if(actor_ptr->free_on_anim_comp) actor_ptr->should_be_freed = true;
    } else {
      actor_ptr->current_frame++;
    }
    actor_ptr->time_until_next_frame += actor_ptr->time_per_frame;
  }

  if(actor_ptr->iframe_time_remaining > 0) actor_ptr->iframe_time_remaining -= dt;

  // update position based on velocity
  actor_ptr->position.x += actor_ptr->velocity.x * dt;
  actor_ptr->position.y += actor_ptr->velocity.y * dt;
}

void dc_Actor_draw(dc_Actor* actor) {
  if(actor->has_shadow) DrawEllipse(actor->position.x + actor->shadow_offset.x, actor->position.y + actor->shadow_offset.y, TILE_WIDTH / 2.f, 2, GRAY);
  Rectangle dest = {actor->position.x, actor->position.y, TILE_WIDTH, TILE_HEIGHT};
  Color c = actor->iframe_time_remaining > 0 ? (Color){255u * sin(GetTime() * IFRAME_FLASH_SPEED), 255u * sin(GetTime() * IFRAME_FLASH_SPEED), 255u * sin(GetTime() * IFRAME_FLASH_SPEED), 255u} : actor->color;
  DrawTexturePro(actor->textures[actor->current_frame], actor->sources[actor->current_frame], dest, actor->origin, actor->rotation, c);
}

dc_Actor* dc_Actor_create_bat(dc_Frames* frame_data, Vector2 pos) {
  dc_Actor* bat = malloc(sizeof(dc_Actor));
  bat->textures = frame_data->dwarf_textures;
  bat->sources = frame_data->dwarf_rects;
  bat->color = WHITE;
  bat->rotation = 0.f;
  bat->position = pos;
  bat->velocity = (Vector2){0};
  bat->origin = (Vector2){11, 17};
  bat->time_per_frame = 0.5f;
  bat->time_until_next_frame = 0.5f;
  bat->current_frame = 0;
  bat->has_shadow = true;
  bat->shadow_offset = (Vector2){0, TILE_HEIGHT * 0.3};
  bat->frame_count = frame_data->dwarf_frames;
  bat->free_on_anim_comp = false;
  bat->should_be_freed = false;
  bat->collision_layer = COL_LAYER_ENEMY;
  bat->collision_mask = COL_LAYER_PLAYER;
  bat->collision_damage = 1;
  bat->iframe_time_remaining = 0;
  bat->hp = 3;
  bat->hp_max = 3;
  bat->ai = dc_ai_bat;

  return bat;
}

dc_Actor* dc_Actor_create_player(dc_Frames* frame_data) {
  dc_Actor* player = malloc(sizeof(dc_Actor));
  player->textures = frame_data->skeleton_textures;
  player->sources = frame_data->skeleton_rects;
  player->color = WHITE;
  player->rotation = 0.f;
  player->position = (Vector2){100, 100};
  player->origin = (Vector2){10, 13};
  player->velocity = (Vector2){0};
  player->time_per_frame = 0.5f;
  player->time_until_next_frame = 0.5f;
  player->current_frame = 0;
  player->has_shadow = true;
  player->shadow_offset = (Vector2){1, TILE_HEIGHT * 0.475};
  player->frame_count = frame_data->skeleton_frames;
  player->free_on_anim_comp = false;
  player->should_be_freed = false;
  player->collision_layer = COL_LAYER_PLAYER;
  player->collision_mask = 0;
  player->collision_damage = 0;
  player->iframe_time_remaining = 0;
  player->hp = 6;
  player->hp_max = 6;
  player->ai = NULL;

  return player;
}

dc_Actor* dc_Actor_create_player_slice(dc_Frames* frame_data, Vector2 pos, float rot) {
  dc_Actor* slice = malloc(sizeof(dc_Actor));
  slice->textures = frame_data->slice_textures;
  slice->sources = frame_data->slice_rects;
  slice->color = WHITE;
  slice->rotation = rot;
  // slice->position = (Vector2){pos.x - 16 * cos(rot * DEG2RAD), pos.y - 16 * sin(rot * DEG2RAD)};
  slice->position = pos;
  //slice->position.x -= 16 * cos(rot * DEG2RAD);
  //slice->position.y -= 16 * sin(rot * DEG2RAD);
  // slice->origin = (Vector2){15, 16};
  // slice->origin = (Vector2){TILE_WIDTH/2, TILE_HEIGHT/2};
  slice->origin = (Vector2){TILE_WIDTH/2, TILE_HEIGHT/2};
  slice->velocity = (Vector2){0};
  slice->time_per_frame = 0.1f;
  slice->time_until_next_frame = 0.1f;
  slice->current_frame = 0;
  slice->has_shadow = false;
  // slice->shadow_offset = (Vector2){0, TILE_HEIGHT * 0.3};
  slice->frame_count = frame_data->slice_frames;
  slice->free_on_anim_comp = true;
  slice->should_be_freed = false;
  slice->collision_layer = 0;
  slice->collision_mask = COL_LAYER_ENEMY;
  slice->collision_damage = 1;
  slice->iframe_time_remaining = 0;
  slice->hp = 1; // not like it matters; this has no layer so it can't be hit!
  slice->hp_max = 1;
  slice->ai = NULL;

  return slice;
}

void dc_Actor_handle_collisions(dc_Actor** actors) { // const?
  for(int us_idx = 0; us_idx < MAX_ACTORS; us_idx++) {
    for(int them_idx = 0; them_idx < MAX_ACTORS; them_idx++) {
      dc_Actor* us = actors[us_idx];
      dc_Actor* them = actors[them_idx];
      // if(us == NULL || them == NULL || us->iframe_time_remaining > 0 || them->iframe_time_remaining > 0) continue;
      if(us == NULL || them == NULL) continue;
      if(us == them) continue;
      if(us->iframe_time_remaining > 0 || them->iframe_time_remaining > 0) continue;
      if(!CheckCollisionCircles(us->position, TILE_WIDTH/2.f, them->position, TILE_WIDTH/2.f)) continue;
      if(us->collision_mask & them->collision_layer) {
        them->hp -= us->collision_damage;
        Vector2 v = dc_get_direction_to(us->position, them->position);
        v.x *= 20;
        v.y *= 20;
        them->velocity = v;
        if(them->hp <= 0) {
          them->should_be_freed = true;
        } else them->iframe_time_remaining = IFRAME_DURATION;
      }
    }
  }
}

Vector2 dc_get_player_input_vector(void) {
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

void dc_Room_generate(dc_Room** rooms, unsigned int old_room_x, unsigned int old_room_y, unsigned int new_room_x, unsigned int new_room_y) {
  unsigned int old_room_idx = old_room_x + old_room_x * FLOOR_WIDTH;
  unsigned int new_room_idx = new_room_x + new_room_y * FLOOR_WIDTH;

  if(rooms[new_room_idx] != NULL) return; // room already exists
  
  rooms[new_room_idx] = malloc(sizeof(dc_Room));
  *rooms[new_room_idx] = (dc_Room){0};

  // guarantee a door based on our last room
  if(old_room_x < new_room_x) {
    rooms[new_room_idx]->door_west = true;
  } else if(old_room_x > new_room_x) {
    rooms[new_room_idx]->door_east = true;
  } else if(old_room_y < new_room_y) {
    rooms[new_room_idx]->door_north = true;
  } else if(old_room_y > new_room_y) {
    rooms[new_room_idx]->door_south = true;
  }

  if(!rooms[new_room_idx]->door_west && new_room_x-1 > 0) {
    // rooms[new_room_idx]->door_west = rand() % 2 == 0;
    rooms[new_room_idx]->door_west = true;
  } else if(!rooms[new_room_idx]->door_east && new_room_x+1 < FLOOR_WIDTH) {
    // rooms[new_room_idx]->door_east = rand() % 2 == 0;
    rooms[new_room_idx]->door_east = true;
  } else if(!rooms[new_room_idx]->door_south && new_room_y+1 < FLOOR_HEIGHT) {
    // rooms[new_room_idx]->door_south = rand() % 2 == 0;
    rooms[new_room_idx]->door_south = true;
  } else if(!rooms[new_room_idx]->door_south && new_room_y-1 > 0) {
    // rooms[new_room_idx]->door_north = rand() % 2 == 0;
    rooms[new_room_idx]->door_north = true;
  }

  rooms[new_room_idx]->remaining_monsters = 1 + rand() % 4;
}

void dc_spawn_actor(dc_Frames* frame_data, dc_Actor** actors, unsigned int new_fella_count) {
  Vector2 spawn_points[] = {(Vector2){50, 50}, (Vector2){250, 50}, (Vector2){50, 250}, (Vector2){250, 250}};

  for(int e = 0; e < new_fella_count; e++) {
    for(int a = 0; a < MAX_ACTORS; a++) {
      if(actors[a] != NULL) continue;
      actors[a] = dc_Actor_create_bat(frame_data, spawn_points[e]);
      break;
    }
  }
}

int main(int argc, char** argv) {
  // srand(time(NULL));
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "REVENGE OF THE LICH");
  SetWindowState(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_MAXIMIZED);

  Image w_icon = LoadImage("./gfx/gmtk_icon.png");
  SetWindowIcon(w_icon);

  InitAudioDevice();

  Texture2D f_tex = LoadTexture("./gfx/frame.png");
  Rectangle f_rect = {0, 0, f_tex.width, f_tex.height};

  dc_Tilesets tilesets = {.interface = LoadTexture("./gfx/oryx/Interface.png"), .terrain = LoadTexture("./gfx/oryx/Terrain.png"), .monsters = LoadTexture("./gfx/oryx/Monsters.png"), .avatar = LoadTexture("./gfx/oryx/Avatar.png"), .fx_general = LoadTexture("./gfx/oryx/FX_General.png"), .zach = LoadTexture("./gfx/gmtk_spritesheet.png")};

  RenderTexture2D r_target = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
  //SetTextureFilter(r_target.texture, TEXTURE_FILTER_POINT);
  Rectangle r_target_rect = {0, 0, r_target.texture.width, -r_target.texture.height};

  Font font = LoadFontEx("./gfx/Perfect DOS VGA 437.ttf", 16.f*4, NULL, 0);
  //SetTextureFilter(font.texture, TEXTURE_FILTER_POINT);

  unsigned int number_of_rooms = ROOMS_LENGTH;
  unsigned int current_room = 3 + 3 * FLOOR_WIDTH;
  dc_Room** rooms = malloc(sizeof(dc_Room*) * number_of_rooms);
  for(unsigned int i = 0; i < number_of_rooms; i++) {
    rooms[i] = NULL;
  }
  rooms[current_room] = malloc(sizeof(dc_Room));
  *rooms[current_room] = (dc_Room){0};
  {
    unsigned int fucking_door = rand() % 4;
    if(fucking_door == 0) rooms[current_room]->door_north = true;
    else if(fucking_door == 1) rooms[current_room]->door_west = true;
    else if(fucking_door == 2) rooms[current_room]->door_east = true;
    else if(fucking_door == 3) rooms[current_room]->door_south = true;
  }
  rooms[current_room]->remaining_monsters = 1;

  dc_Frames frame_data = {
    .skeleton_textures = {tilesets.zach, tilesets.zach},
    .skeleton_rects = {(Rectangle){1 * TILE_WIDTH, 6 * TILE_HEIGHT, TILE_WIDTH, TILE_HEIGHT}, (Rectangle){2 * TILE_WIDTH, 6 * TILE_HEIGHT, TILE_WIDTH, TILE_HEIGHT}},
    .skeleton_frames = 2,
    .slice_textures = {tilesets.fx_general, tilesets.fx_general, tilesets.fx_general},
    .slice_rects = {(Rectangle){12 * TILE_WIDTH, 0, TILE_WIDTH, TILE_HEIGHT}, (Rectangle){13 * TILE_WIDTH, 0, TILE_WIDTH, TILE_HEIGHT}, (Rectangle){14 * TILE_WIDTH, 0, TILE_WIDTH, TILE_HEIGHT}},
    .slice_frames = 3,
    .dwarf_textures = {tilesets.zach, tilesets.zach},
    .dwarf_rects = {(Rectangle){1 * TILE_WIDTH, 4 * TILE_HEIGHT, TILE_WIDTH, TILE_HEIGHT}, (Rectangle){2 * TILE_WIDTH, 4 * TILE_HEIGHT, TILE_WIDTH, TILE_HEIGHT}},
    .dwarf_frames = 2
  };

  dc_Sounds sounds = {
    .door_open = LoadSound("./sfx/door_open.wav")
  };

  dc_Actor* player = dc_Actor_create_player(&frame_data);

  dc_Actor* actors[MAX_ACTORS] = {player};
  actors[1] = dc_Actor_create_bat(&frame_data, (Vector2){250, 250});
  for(int a = 2; a < MAX_ACTORS; a++) {
    actors[a] = NULL;
  }

  Camera2D cam = {(Vector2){0}, (Vector2){0}, 0.f, 1.f};

  while(!WindowShouldClose()) {
    float dt = MIN(GetFrameTime(), 1000.f/15.f); // cap how slow the game can run because i'm not doing interpolation for your commodore 64
    
    if(player != NULL) {
      Vector2 p_input_v = dc_get_player_input_vector();
      player->velocity.x = p_input_v.x * 100;
      player->velocity.y = p_input_v.y * 100;
      if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        for(int a = 0; a < MAX_ACTORS; a++) {
          if(actors[a] != NULL) continue;
          Vector2 mouse_pos = GetMousePosition();
          Vector2 screen_scaling = dc_get_screen_scaling_percent();
          float dx = mouse_pos.x * screen_scaling.x - player->position.x;
          float dy = mouse_pos.y * screen_scaling.y - player->position.y;
          float rot = atan2(dy, dx);
          static const int slice_distance = 12;
          Vector2 slice_pos = (Vector2){player->position.x + slice_distance * cos(rot), player->position.y + slice_distance * sin(rot)};
          actors[a] = dc_Actor_create_player_slice(&frame_data, slice_pos, atan2(dy, dx) * RAD2DEG + 135);
          break;
        }
      }
    }

    for(int a = 0; a < MAX_ACTORS; a++) {
      if(actors[a] != NULL && actors[a]->ai != NULL) actors[a]->ai(actors[a], player);
    }

    for(int a = 0; a < MAX_ACTORS; a++) {
      if(actors[a] != NULL) dc_Actor_update(actors[a], dt);
    }

    for(int a = 0; a < MAX_ACTORS; a++) {
      if(actors[a] == NULL) continue;
      if(actors[a] == player && rooms[current_room]->doors_opened) {
        // XXX: magic number bullshit
        Rectangle north_door_hitbox = (Rectangle){TILE_WIDTH * 9.5, TILE_HEIGHT * 1.8, TILE_WIDTH, TILE_HEIGHT};
        Rectangle north_wall_hitbox = (Rectangle){TILE_WIDTH * 1.5, TILE_HEIGHT * 1.8, TILE_WIDTH * 17, TILE_HEIGHT};
        Rectangle south_door_hitbox = (Rectangle){TILE_WIDTH * 8.5, TILE_HEIGHT * 5.6, TILE_WIDTH, TILE_HEIGHT}; // 8.5 and not 9.5?
        Rectangle south_wall_hitbox = (Rectangle){TILE_WIDTH * 1.5, TILE_HEIGHT * 5.6, TILE_WIDTH * 17, TILE_HEIGHT};
        Rectangle west_wall_hitbox = (Rectangle){TILE_WIDTH * 0.5, TILE_HEIGHT * 2, TILE_WIDTH, TILE_HEIGHT * 5};
        Rectangle west_door_hitbox = (Rectangle){TILE_WIDTH * 0.5, TILE_HEIGHT * 4, TILE_WIDTH, TILE_HEIGHT};
        Rectangle east_wall_hitbox = (Rectangle){TILE_WIDTH * 18.5, TILE_HEIGHT * 2, TILE_WIDTH, TILE_HEIGHT * 5};
        Rectangle east_door_hitbox = (Rectangle){TILE_WIDTH * 18.5, TILE_HEIGHT * 4, TILE_WIDTH, TILE_HEIGHT};

        // my beautiful codebase is getting worse as the hour draws near
        unsigned int old_room_x = current_room % FLOOR_WIDTH;
        unsigned int old_room_y = current_room / FLOOR_WIDTH;
        if(rooms[current_room]->door_north && CheckCollisionPointRec(player->position, north_door_hitbox)) {
          unsigned int new_room_x = old_room_x;
          unsigned int new_room_y = old_room_y-1;
          dc_Room_generate(rooms, old_room_x, old_room_y, new_room_x, new_room_y);
          current_room = new_room_x + new_room_y * FLOOR_WIDTH;
          player->position.y = TILE_HEIGHT * 2;
          // we don't check for errors at all, and will probably just fail to spawn em if we some how max out our actors array /shrug
          dc_spawn_actor(&frame_data, actors, rooms[current_room]->remaining_monsters);
          break;
        } else if(rooms[current_room]->door_south && CheckCollisionPointRec(player->position, south_door_hitbox)) {
          unsigned int new_room_x = old_room_x;
          unsigned int new_room_y = old_room_y+1;
          dc_Room_generate(rooms, old_room_x, old_room_y, new_room_x, new_room_y);
          current_room = new_room_x + new_room_y * FLOOR_WIDTH;
          player->position.y = TILE_HEIGHT * 5;
          dc_spawn_actor(&frame_data, actors, rooms[current_room]->remaining_monsters);
          break;
        } else if(rooms[current_room]->door_west && CheckCollisionPointRec(player->position, west_door_hitbox)) {
          unsigned int new_room_x = old_room_x-1;
          unsigned int new_room_y = old_room_y;
          dc_Room_generate(rooms, old_room_x, old_room_y, new_room_x, new_room_y);
          current_room = new_room_x + new_room_y * FLOOR_WIDTH;
          player->position.x = TILE_WIDTH * 18;
          dc_spawn_actor(&frame_data, actors, rooms[current_room]->remaining_monsters);
          break;
        } else if(rooms[current_room]->door_east && CheckCollisionPointRec(player->position, east_door_hitbox)) {
          unsigned int new_room_x = old_room_x+1;
          unsigned int new_room_y = old_room_y;
          dc_Room_generate(rooms, old_room_x, old_room_y, new_room_x, new_room_y);
          current_room = new_room_x + new_room_y * FLOOR_WIDTH;
          player->position.x = TILE_WIDTH;
          dc_spawn_actor(&frame_data, actors, rooms[current_room]->remaining_monsters);
          break;
        }

        // nah get out of that there wall
        if(CheckCollisionPointRec(player->position, north_wall_hitbox)) {
          actors[a]->position.x -= actors[a]->velocity.x * dt;
          actors[a]->position.y -= actors[a]->velocity.y * dt;
        } else if(CheckCollisionPointRec(player->position, south_wall_hitbox)) {
          actors[a]->position.x -= actors[a]->velocity.x * dt;
          actors[a]->position.y -= actors[a]->velocity.y * dt;
        } else if(CheckCollisionPointRec(player->position, west_wall_hitbox)) {
          actors[a]->position.x -= actors[a]->velocity.x * dt;
          actors[a]->position.y -= actors[a]->velocity.y * dt;
        } else if(CheckCollisionPointRec(player->position, east_wall_hitbox)) {
          actors[a]->position.x -= actors[a]->velocity.x * dt;
          actors[a]->position.y -= actors[a]->velocity.y * dt;
        }
      } else {
        actors[a]->position.x = dc_clampf(actors[a]->position.x, TILE_WIDTH * 2, TILE_WIDTH * 18);
        actors[a]->position.y = dc_clampf(actors[a]->position.y, TILE_HEIGHT * 2.8, TILE_HEIGHT * 5.6);
      }
    }

    for(int a = 0; a < MAX_ACTORS; a++) {
      if(actors[a] != NULL) dc_Actor_handle_collisions(actors);
    }

    for(int a = 0; a < MAX_ACTORS; a++) {
      if(actors[a] != NULL && actors[a]->should_be_freed) {
        if(actors[a]->ai != NULL) {
          rooms[current_room]->remaining_monsters--;
          if(rooms[current_room]->remaining_monsters == 0 && !rooms[current_room]->doors_opened) {
            PlaySound(sounds.door_open);
            rooms[current_room]->doors_opened = true;
          }
        }
        free(actors[a]); // we *shouldn't* need to make ->textures or ->sources NULL
        if(actors[a] == player) player = NULL;
        actors[a] = NULL;
      }
    }

    BeginDrawing();
      BeginTextureMode(r_target);
      ClearBackground(BLACK);

      BeginMode2D(cam);
      dc_Room_draw(tilesets, rooms[current_room]);

      for(int a = 0; a < MAX_ACTORS; a++) {
        if(actors[a] != NULL) dc_Actor_draw(actors[a]);
      }
      EndMode2D();

      DrawTexturePro(f_tex, f_rect, f_rect, (Vector2){0}, 0.f, WHITE);

      // removed debug shit because I am bad a trig
      /*{
        Vector2 mouse_pos = GetMousePosition();
        Vector2 screen_scaling = dc_get_screen_scaling_percent();
        float dx = mouse_pos.x * screen_scaling.x - player->position.x;
        float dy = mouse_pos.y * screen_scaling.y - player->position.y;
        float rot = atan2(dy, dx);
        DrawCircle(player->position.x + 16 * cos(rot), player->position.y + 16 * sin(rot), 4.f, BLUE);
      }*/

      if(player != NULL) {
        dc_draw_player_health(tilesets, player->hp, player->hp_max);
        dc_draw_player_targeting(tilesets, player);
      }


      if(player == NULL) {
        DrawTextEx(font, "Game Over!", (Vector2){100, 20}, 16.f, 0.1f, WHITE);
      } else {
        DrawTextEx(font, TextFormat("Remaining: %d", rooms[current_room]->remaining_monsters), (Vector2){100, 20}, 16.f, 0.1f, WHITE);
      }
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
  UnloadTexture(tilesets.zach);
  for(unsigned int i = 0; i < number_of_rooms; i++) {
    if(rooms[i] != NULL) free(rooms[i]);
  }
  free(rooms);

  for(unsigned int a = 0; a < MAX_ACTORS; a++) {
    if(actors[a] != NULL) free(actors[a]);
  }

  UnloadFont(font);

  UnloadSound(sounds.door_open);

  UnloadImage(w_icon);

  CloseAudioDevice();

  return 0;
}
