//gcc zero.c -o zero.exe -Iraylib_lib/include -Lraylib_lib/lib -lraylib -lopengl32 -lgdi32 -lwinmm
#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <stdarg.h>

// ==========================================
// MACROS & DEFINES
// ==========================================
#define MAX_BLOCKS 200
#define MAX_SPIKES 50
#define MAX_TRIGGERS 20

#define ROOM_W 800
#define ROOM_H 600
#define BLOCK_SIZE 50

// ==========================================
// ENUMS & STRUCTURES
// ==========================================
typedef enum GameState {
    TITLE,
    PLAYING,
    GAME_OVER,
    LEVEL_COMPLETE,
    GAME_VICTORY
} GameState;

typedef struct Block {
    Rectangle rect;
    bool active;
    Color color;
} Block;

typedef enum SpikeDir {
    SPIKE_UP,
    SPIKE_DOWN,
    SPIKE_LEFT,
    SPIKE_RIGHT
} SpikeDir;

typedef struct Spike {
    Rectangle rect;
    SpikeDir dir;
    bool active;
    Color color;
} Spike;

typedef enum TrollAction {
    TROLL_NONE,
    TROLL_PIT,           
    TROLL_MOVING_DOOR,   
    TROLL_FALLING_SPIKE, 
    TROLL_DISAPPEAR_ALL, 
    TROLL_REVERSE_CONTROLS 
} TrollAction;

typedef struct Trigger {
    Rectangle rect;
    TrollAction action;
    bool triggered;
    bool active;
} Trigger;

typedef struct Player {
    Vector2 position;
    Vector2 velocity;
    float speed;
    float jumpSpeed;
    float gravity;
    bool isGrounded;
    Rectangle rect;
} Player;

// ==========================================
// GLOBAL VARIABLES
// ==========================================
Texture2D bgTex;
Texture2D checkTex;
Texture2D playerTex;
Texture2D longPlatformTex;
Texture2D smallPlatformTex;

GameState currentState = TITLE;
int currentLevel = 1;
int lives = 2;

Block blocks[MAX_BLOCKS];
int blockCount = 0;

Spike spikes[MAX_SPIKES];
int spikeCount = 0;

Trigger triggers[MAX_TRIGGERS];
int triggerCount = 0;

Rectangle door;
bool doorActive = false;
bool controlsReversed = false;

Player player;

// ==========================================
// FUNCTION PROTOTYPES
// ==========================================
// Math / Fix
void sincosf(float x, float *s, float *c);
void sincos(double x, double *s, double *c);
int __mingw_vsscanf(const char *str, const char *format, va_list ap);
void* my_acrt_iob_func(unsigned index);
float MathAbs(float x);

// Game
void InitGame(void);
void UpdateGame(void);
void DrawGame(void);
void ResetLevel(void);
void ResetGame(void);

// Level
void LoadLevel(int levelIndex);

// Entities
void InitEntities(void);
void DrawEntities(void);
void AddBlock(float x, float y, float w, float h, Color c);
void AddSpike(float x, float y, float w, float h, SpikeDir dir);
void AddTrigger(float x, float y, float w, float h, TrollAction action);
void CheckTriggers(Rectangle playerRect);

// Player
void InitPlayer(void);
void UpdatePlayer(float dt);
void DrawPlayer(void);
void CheckCollisions(void);

// ==========================================
// IMPLEMENTATION (fix.c)
// ==========================================
void sincosf(float x, float *s, float *c) {
    *s = sinf(x);
    *c = cosf(x);
}

void sincos(double x, double *s, double *c) {
    *s = sin(x);
    *c = cos(x);
}

int __mingw_vsscanf(const char *str, const char *format, va_list ap) {
    return vsscanf(str, format, ap);
}

void* my_acrt_iob_func(unsigned index) {
    if (index == 0) return stdin;
    if (index == 1) return stdout;
    if (index == 2) return stderr;
    return NULL;
}

void* (*_imp____acrt_iob_func)(unsigned) = my_acrt_iob_func;

// ==========================================
// IMPLEMENTATION (entities.c)
// ==========================================
void InitEntities(void) {
    blockCount = 0;
    spikeCount = 0;
    triggerCount = 0;
    doorActive = false;
    controlsReversed = false;
}

void AddBlock(float x, float y, float w, float h, Color c) {
    if (blockCount < MAX_BLOCKS) {
        blocks[blockCount].rect = (Rectangle){x, y, w, h};
        blocks[blockCount].active = true;
        blocks[blockCount].color = c;
        blockCount++;
    }
}

void AddSpike(float x, float y, float w, float h, SpikeDir dir) {
    if (spikeCount < MAX_SPIKES) {
        spikes[spikeCount].rect = (Rectangle){x, y, w, h};
        spikes[spikeCount].dir = dir;
        spikes[spikeCount].active = true;
        spikes[spikeCount].color = GRAY;
        spikeCount++;
    }
}

void AddTrigger(float x, float y, float w, float h, TrollAction action) {
    if (triggerCount < MAX_TRIGGERS) {
        triggers[triggerCount].rect = (Rectangle){x, y, w, h};
        triggers[triggerCount].action = action;
        triggers[triggerCount].triggered = false;
        triggers[triggerCount].active = true;
        triggerCount++;
    }
}

void DrawEntities(void) {
    if (doorActive) {
        if (checkTex.id != 0) {
            DrawTexturePro(checkTex, (Rectangle){0, 0, checkTex.width, checkTex.height}, door, (Vector2){0, 0}, 0.0f, WHITE);
        } else {
            DrawRectangleRec(door, LIME);
            DrawRectangleLinesEx(door, 2, DARKGREEN);
            DrawRectangle(door.x + 5, door.y + door.height/2, 5, 5, DARKGREEN); 
        }
    }

    for (int i = 0; i < blockCount; i++) {
        if (blocks[i].active) {
            if (longPlatformTex.id != 0) {
                if (blocks[i].rect.y == 500 && blocks[i].rect.width == 50) {
                    float srcX = (blocks[i].rect.x / 800.0f) * longPlatformTex.width;
                    float srcW = (blocks[i].rect.width / 800.0f) * longPlatformTex.width;
                    DrawTexturePro(longPlatformTex, (Rectangle){srcX, 0, srcW, longPlatformTex.height}, blocks[i].rect, (Vector2){0, 0}, 0.0f, WHITE);
                } else {
                    Texture2D texToUse = smallPlatformTex.id != 0 ? smallPlatformTex : longPlatformTex;
                    DrawTexturePro(texToUse, (Rectangle){0, 0, texToUse.width, texToUse.height}, blocks[i].rect, (Vector2){0, 0}, 0.0f, WHITE);
                }
            } else if (smallPlatformTex.id != 0) {
                DrawTexturePro(smallPlatformTex, (Rectangle){0, 0, smallPlatformTex.width, smallPlatformTex.height}, blocks[i].rect, (Vector2){0, 0}, 0.0f, WHITE);
            } else {
                DrawRectangleRec(blocks[i].rect, blocks[i].color);
                DrawRectangleLinesEx(blocks[i].rect, 2, BLACK);
            }
        }
    }

    for (int i = 0; i < spikeCount; i++) {
        if (spikes[i].active) {
            Rectangle r = spikes[i].rect;
            float h3 = r.height / 3.0f;
            if (spikes[i].dir == SPIKE_UP) {
                DrawRectangle(r.x, r.y + r.height - h3, r.width, h3, GREEN);
                float wMid = r.width * 0.6f;
                DrawRectangle(r.x + (r.width - wMid)/2.0f, r.y + r.height - 2.0f*h3, wMid, h3, GREEN);
                float wTop = r.width * 0.2f;
                DrawRectangle(r.x + (r.width - wTop)/2.0f, r.y, wTop, h3, GREEN);
            } else if (spikes[i].dir == SPIKE_DOWN) {
                DrawRectangle(r.x, r.y, r.width, h3, GREEN);
                float wMid = r.width * 0.6f;
                DrawRectangle(r.x + (r.width - wMid)/2.0f, r.y + h3, wMid, h3, GREEN);
                float wTop = r.width * 0.2f;
                DrawRectangle(r.x + (r.width - wTop)/2.0f, r.y + 2.0f*h3, wTop, h3, GREEN);
            }
        }
    }
}

void CheckTriggers(Rectangle playerRect) {
    for (int i = 0; i < triggerCount; i++) {
        if (triggers[i].active && !triggers[i].triggered) {
            if (CheckCollisionRecs(playerRect, triggers[i].rect)) {
                triggers[i].triggered = true;
                
                switch(triggers[i].action) {
                    case TROLL_PIT:
                        for(int j=0; j<blockCount; j++){
                            if(blocks[j].rect.x > playerRect.x - 50 && blocks[j].rect.x < playerRect.x + 100){
                                blocks[j].active = false;
                            }
                        }
                        break;
                    case TROLL_MOVING_DOOR:
                        door.x = 50; 
                        AddTrigger(150, 400, 50, 100, TROLL_PIT); 
                        break;
                    case TROLL_FALLING_SPIKE:
                        AddSpike(playerRect.x + 5, playerRect.y - 400, 30, 40, SPIKE_DOWN);
                        break;
                    case TROLL_DISAPPEAR_ALL:
                        for(int j=0; j<blockCount; j++){
                            if (blocks[j].rect.x > 100 && blocks[j].rect.x < 700 && blocks[j].rect.y >= 490) {
                                blocks[j].active = false;
                            }
                        }
                        break;
                    case TROLL_REVERSE_CONTROLS:
                        controlsReversed = true;
                        break;
                    case TROLL_NONE:
                        break;
                }
            }
        }
    }
}

// ==========================================
// IMPLEMENTATION (player.c)
// ==========================================
void InitPlayer(void) {
    player.position = (Vector2){ 50, 450 };
    player.velocity = (Vector2){ 0, 0 };
    player.speed = 300.0f;
    player.jumpSpeed = 500.0f;
    player.gravity = 1500.0f;
    player.isGrounded = false;
    player.rect = (Rectangle){ player.position.x, player.position.y, 40, 40 };
}

void UpdatePlayer(float dt) {
    if (!controlsReversed) {
        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) player.velocity.x = player.speed;
        else if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) player.velocity.x = -player.speed;
        else player.velocity.x = 0;
    } else {
        if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) player.velocity.x = player.speed;
        else if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) player.velocity.x = -player.speed;
        else player.velocity.x = 0;
    }

    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W) || IsKeyPressed(KEY_SPACE)) {
        if (player.isGrounded) {
            player.velocity.y = -player.jumpSpeed;
            player.isGrounded = false;
        }
    }

    player.velocity.y += player.gravity * dt;

    player.position.x += player.velocity.x * dt;
    player.position.y += player.velocity.y * dt;

    player.rect.x = player.position.x;
    player.rect.y = player.position.y;
    
    for(int i=0; i<spikeCount; i++) {
        if (spikes[i].active && spikes[i].dir == SPIKE_DOWN) {
            if (spikes[i].rect.width == 30 && spikes[i].rect.height == 40) {
                if (spikes[i].rect.y < 460) {
                    spikes[i].rect.y += 1800.0f * dt;
                    if (spikes[i].rect.y > 460) spikes[i].rect.y = 460;
                }
            }
        }
    }
}

void DrawPlayer(void) {
    if (playerTex.id != 0) {
        DrawTexturePro(playerTex, (Rectangle){0, 0, playerTex.width, playerTex.height}, player.rect, (Vector2){0, 0}, 0.0f, WHITE);
    } else {
        DrawRectangleRec(player.rect, RED);
        DrawRectangle(player.rect.x + 8, player.rect.y + 8, 6, 6, BLACK);
        DrawRectangle(player.rect.x + 26, player.rect.y + 8, 6, 6, BLACK);
        DrawRectangle(player.rect.x + 8, player.rect.y + 26, 24, 6, BLACK);
    }
}

void CheckCollisions(void) {
    player.isGrounded = false;
    
    if (player.position.x < 0) { player.position.x = 0; player.rect.x = 0; }
    if (player.position.x > 800 - player.rect.width) { player.position.x = 800 - player.rect.width; player.rect.x = player.position.x; }
    
    if (player.position.y > 600) {
        lives--;
        if (lives < 0) {
            currentState = GAME_OVER;
        } else {
            ResetLevel(); 
        }
        return;
    }

    for (int i = 0; i < blockCount; i++) {
        if (blocks[i].active && CheckCollisionRecs(player.rect, blocks[i].rect)) {
            Rectangle b = blocks[i].rect;
            Rectangle p = player.rect;
            
            float dx = (p.x + p.width/2) - (b.x + b.width/2);
            float dy = (p.y + p.height/2) - (b.y + b.height/2);
            float width = (p.width + b.width)/2;
            float height = (p.height + b.height)/2;
            float crossWidth = width * dy;
            float crossHeight = height * dx;

            if (MathAbs(dx) <= width && MathAbs(dy) <= height) {
                if (crossWidth > crossHeight) {
                    if (crossWidth > -crossHeight) {
                        player.position.y = b.y + b.height;
                        player.velocity.y = 0;
                    } else {
                        player.position.x = b.x - p.width;
                        player.velocity.x = 0;
                    }
                } else {
                    if (crossWidth > -crossHeight) {
                        player.position.x = b.x + b.width;
                        player.velocity.x = 0;
                    } else {
                        player.position.y = b.y - p.height;
                        player.velocity.y = 0;
                        player.isGrounded = true;
                    }
                }
            }
            player.rect.x = player.position.x;
            player.rect.y = player.position.y;
        }
    }

    for (int i = 0; i < spikeCount; i++) {
        if (spikes[i].active && CheckCollisionRecs(player.rect, spikes[i].rect)) {
            Rectangle hitBox = { player.rect.x + 5, player.rect.y + 5, player.rect.width - 10, player.rect.height - 10 };
            if (CheckCollisionRecs(hitBox, spikes[i].rect)) {
                lives--;
                if (lives < 0) {
                    currentState = GAME_OVER;
                } else {
                    ResetLevel();
                }
                return;
            }
        }
    }

    if (doorActive && CheckCollisionRecs(player.rect, door)) {
        if (currentLevel == 5) {
            currentState = GAME_VICTORY;
        } else {
            currentState = LEVEL_COMPLETE;
        }
    }
}

float MathAbs(float x) {
    return x < 0 ? -x : x;
}

// ==========================================
// IMPLEMENTATION (level.c)
// ==========================================
void LoadLevel(int levelIndex) {
    InitEntities();
    
    player.position = (Vector2){ 50, 450 };
    
    door = (Rectangle){ 700, 420, 40, 80 };
    doorActive = true;
    
    for (int i = 0; i < 800/BLOCK_SIZE; i++) {
        AddBlock(i * BLOCK_SIZE, 500, BLOCK_SIZE, BLOCK_SIZE, BLACK);
    }
    
    switch(levelIndex) {
        case 1:
            AddTrigger(550, 400, 50, 100, TROLL_PIT);
            break;
            
        case 2:
            AddTrigger(600, 300, 50, 200, TROLL_MOVING_DOOR);
            break;
            
        case 3:
            AddTrigger(400, 400, 50, 100, TROLL_FALLING_SPIKE);
            AddSpike(200, 470, 30, 30, SPIKE_UP);
            AddSpike(330, 470, 30, 30, SPIKE_UP);
            break;
            
        case 4:
            AddTrigger(150, 400, 50, 100, TROLL_DISAPPEAR_ALL);
            AddBlock(250, 450, BLOCK_SIZE, BLOCK_SIZE, BLACK);
            AddBlock(420, 450, BLOCK_SIZE, BLOCK_SIZE, BLACK);
            AddBlock(580, 450, BLOCK_SIZE, BLOCK_SIZE, BLACK);
            break;
            
        case 5:
            AddTrigger(350, 400, 50, 100, TROLL_REVERSE_CONTROLS);
            AddBlock(400, 470, BLOCK_SIZE, BLOCK_SIZE, BLACK);
            AddSpike(410, 440, 30, 30, SPIKE_UP);
            AddBlock(520, 470, BLOCK_SIZE, BLOCK_SIZE, BLACK);
            break;
    }
}

// ==========================================
// IMPLEMENTATION (game.c)
// ==========================================
void InitGame(void) {
    currentState = TITLE;
    currentLevel = 1;
    lives = 2;
}

void ResetLevel(void) {
    InitPlayer();
    LoadLevel(currentLevel);
    controlsReversed = false;
    currentState = PLAYING;
}

void ResetGame(void) {
    currentLevel = 1;
    lives = 2;
    ResetLevel();
}

void UpdateGame(void) {
    float dt = GetFrameTime();

    switch(currentState) {
        case TITLE:
            if (IsKeyPressed(KEY_ENTER)) {
                ResetGame();
            }
            break;
        case PLAYING:
            UpdatePlayer(dt);
            CheckCollisions();
            CheckTriggers(player.rect);
            break;
        case GAME_OVER:
            if (IsKeyPressed(KEY_ENTER)) {
                ResetGame();
            }
            break;
        case LEVEL_COMPLETE:
            if (IsKeyPressed(KEY_ENTER)) {
                currentLevel++;
                ResetLevel();
            }
            break;
        case GAME_VICTORY:
            if (IsKeyPressed(KEY_ENTER)) {
                ResetGame();
            }
            break;
    }
}

void DrawGame(void) {
    ClearBackground(CLITERAL(Color){ 245, 245, 220, 255 }); 
    if (bgTex.id != 0) {
        DrawTexturePro(bgTex, (Rectangle){0, 0, bgTex.width, bgTex.height}, (Rectangle){0, 0, 800, 600}, (Vector2){0, 0}, 0.0f, WHITE);
    }

    switch(currentState) {
        case TITLE:
            DrawText("FLOOR IS LIAR", 200, 200, 40, WHITE);
            DrawText("Press ENTER to Start", 250, 300, 20, LIGHTGRAY);
            break;
        case PLAYING:
            DrawEntities();
            DrawPlayer();
            DrawText(TextFormat("Level: %d", currentLevel), 10, 10, 20, WHITE);
            DrawText(TextFormat("Lives: %d", lives), 10, 40, 20, YELLOW);
            break;
        case GAME_OVER:
            DrawText("GAME OVER", 250, 200, 50, WHITE);
            DrawText("Press ENTER to Restart", 250, 300, 20, LIGHTGRAY);
            break;
        case LEVEL_COMPLETE:
            DrawText("LEVEL COMPLETE", 200, 200, 40, YELLOW);
            DrawText("Press ENTER to Continue", 250, 300, 20, LIGHTGRAY);
            break;
        case GAME_VICTORY:
            DrawText("Congratulations!", 200, 200, 40, GOLD);
            DrawText("You won over RageBait!!", 200, 260, 30, WHITE);
            DrawText("Press ENTER to Restart", 250, 350, 20, LIGHTGRAY);
            break;
    }
}

// ==========================================
// IMPLEMENTATION (main.c)
// ==========================================
int main(void)
{
    const int screenWidth = 800;
    const int screenHeight = 600;

    InitWindow(screenWidth, screenHeight, "Floor is Liar");
    InitAudioDevice(); 

    bgTex = LoadTexture("lava.png");
    checkTex = LoadTexture("check.png");
    playerTex = LoadTexture("player1.png");
    longPlatformTex = LoadTexture("longplatform.png");
    smallPlatformTex = LoadTexture("smallplatform.png");

    InitGame();

    Music bgMusic = LoadMusicStream("Mission_Impossible.mp3");
    PlayMusicStream(bgMusic); 

    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        UpdateMusicStream(bgMusic); 
        UpdateGame();
        
        BeginDrawing();
        
        DrawGame();
        
        EndDrawing();
    }

    UnloadTexture(bgTex);
    UnloadTexture(checkTex);
    UnloadTexture(playerTex);
    UnloadTexture(longPlatformTex);
    UnloadTexture(smallPlatformTex);

    UnloadMusicStream(bgMusic); 
    CloseAudioDevice();         

    CloseWindow();

    return 0;
}
