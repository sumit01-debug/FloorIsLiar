#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <stdarg.h>

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
#define MAX_BLOCKS 200
#define MAX_SPIKES 50
#define MAX_TRIGGERS 20

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
    Rectangle rect; // Bounding box for logic/collision
    SpikeDir dir;
    bool active;
    Color color;
} Spike;

typedef enum TrollAction {
    TROLL_NONE,
    TROLL_PIT,           // Floor disappears
    TROLL_MOVING_DOOR,   // Door moves away
    TROLL_FALLING_SPIKE, // Spikes fall from ceiling
    TROLL_DISAPPEAR_ALL, // Whole floor except safe spots disappears
    TROLL_REVERSE_CONTROLS // Controls flip
} TrollAction;

typedef struct Trigger {
    Rectangle rect;
    TrollAction action;
    bool triggered;
    bool active;
} Trigger;

void InitEntities(void);
void DrawEntities(void);
void AddBlock(float x, float y, float w, float h, Color c);
void AddSpike(float x, float y, float w, float h, SpikeDir dir);
void AddTrigger(float x, float y, float w, float h, TrollAction action);
void CheckTriggers(Rectangle playerRect);

typedef struct Player {
    Vector2 position;
    Vector2 velocity;
    float speed;
    float jumpSpeed;
    float gravity;
    bool isGrounded;
    Rectangle rect;
} Player;

void InitPlayer(void);
void UpdatePlayer(float dt);
void DrawPlayer(void);
void CheckCollisions(void);

void LoadLevel(int levelIndex);

typedef enum GameState {
    TITLE,
    PLAYING,
    GAME_OVER,
    LEVEL_COMPLETE,
    GAME_VICTORY
} GameState;

void InitGame(void);
void UpdateGame(void);
void DrawGame(void);
void ResetLevel(void);
void ResetGame(void);

Block blocks[MAX_BLOCKS];
int blockCount = 0;

Spike spikes[MAX_SPIKES];
int spikeCount = 0;

Trigger triggers[MAX_TRIGGERS];
int triggerCount = 0;

Rectangle door;
bool doorActive = false;
bool controlsReversed = false;

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
    // Draw Door
    if (doorActive) {
        DrawRectangleRec(door, LIME);
        DrawRectangleLinesEx(door, 2, DARKGREEN);
        DrawRectangle(door.x + 5, door.y + door.height/2, 5, 5, DARKGREEN); // Door knob
    }

    // Draw Blocks
    for (int i = 0; i < blockCount; i++) {
        if (blocks[i].active) {
            DrawRectangleRec(blocks[i].rect, blocks[i].color);
            DrawRectangleLinesEx(blocks[i].rect, 2, BLACK);
        }
    }

    // Draw Spikes
    for (int i = 0; i < spikeCount; i++) {
        if (spikes[i].active) {
            Rectangle r = spikes[i].rect;
            DrawRectangleRec(r, spikes[i].color);
            // Draw red tip
            if (spikes[i].dir == SPIKE_UP) {
                DrawTriangle((Vector2){r.x + r.width/2, r.y}, (Vector2){r.x, r.y + r.height}, (Vector2){r.x + r.width, r.y + r.height}, RED);
            } else if (spikes[i].dir == SPIKE_DOWN) {
                DrawTriangle((Vector2){r.x, r.y}, (Vector2){r.x + r.width/2, r.y + r.height}, (Vector2){r.x + r.width, r.y}, RED);
            }
        }
    }
}

void CheckTriggers(Rectangle playerRect) {
    for (int i = 0; i < triggerCount; i++) {
        if (triggers[i].active && !triggers[i].triggered) {
            if (CheckCollisionRecs(playerRect, triggers[i].rect)) {
                triggers[i].triggered = true;
                
                // Execute Troll Action
                switch(triggers[i].action) {
                    case TROLL_PIT:
                        // Make blocks under player disappear
                        for(int j=0; j<blockCount; j++){
                            if(blocks[j].rect.x > playerRect.x - 50 && blocks[j].rect.x < playerRect.x + 100){
                                blocks[j].active = false;
                            }
                        }
                        break;
                    case TROLL_MOVING_DOOR:
                        door.x = 50; // Teleport door to the start
                        AddTrigger(150, 400, 50, 100, TROLL_PIT); // Trap when returning
                        break;
                    case TROLL_FALLING_SPIKE:
                        // Spawn a falling spike above player
                        AddSpike(playerRect.x, playerRect.y - 400, 30, 40, SPIKE_DOWN);
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

// Define a simple room size
#define ROOM_W 800
#define ROOM_H 600
#define BLOCK_SIZE 50

void LoadLevel(int levelIndex) {
    InitEntities();
    
    // Default player position
    player.position = (Vector2){ 50, 450 };
    
    // Default door
    door = (Rectangle){ 700, 420, 40, 80 };
    doorActive = true;
    
    // Basic flat floor for all levels
    for (int i = 0; i < 800/BLOCK_SIZE; i++) {
        AddBlock(i * BLOCK_SIZE, 500, BLOCK_SIZE, BLOCK_SIZE, BLACK);
    }
    
    switch(levelIndex) {
        case 1:
            // Level 1: Normal straight path, BUT wait! There's a pit trap right before the door.
            // When player reaches x=550, blocks at 600 and 650 disappear.
            AddTrigger(550, 400, 50, 100, TROLL_PIT);
            break;
            
        case 2:
            // Level 2: The moving door. Door moves away when player approaches.
            AddTrigger(600, 300, 50, 200, TROLL_MOVING_DOOR);
            break;
            
        case 3:
            // Level 3: Falling spikes.
            // Spikes fall from ceiling when you cross the middle.
            AddTrigger(400, 400, 50, 100, TROLL_FALLING_SPIKE);
            // Some static spikes on the floor
            AddSpike(200, 470, 30, 30, SPIKE_UP);
            AddSpike(300, 470, 30, 30, SPIKE_UP);
            break;
            
        case 4:
            // Level 4: Disappear all floor.
            AddTrigger(150, 400, 50, 100, TROLL_DISAPPEAR_ALL);
            // Safe platforms to jump on after floor disappears
            AddBlock(250, 450, BLOCK_SIZE, BLOCK_SIZE, BLACK);
            AddBlock(420, 450, BLOCK_SIZE, BLOCK_SIZE, BLACK);
            AddBlock(580, 450, BLOCK_SIZE, BLOCK_SIZE, BLACK);
            break;
            
        case 5:
            // Level 5: Reverse Controls.
            // Middle trigger flips controls
            AddTrigger(350, 400, 50, 100, TROLL_REVERSE_CONTROLS);
            // Add some obstacles so it's not trivial
            AddBlock(400, 470, BLOCK_SIZE, BLOCK_SIZE, BLACK);
            AddSpike(410, 440, 30, 30, SPIKE_UP);
            AddBlock(520, 470, BLOCK_SIZE, BLOCK_SIZE, BLACK);
            break;
    }
}

Player player;

float MathAbs(float x);

void InitPlayer(void) {
    player.position = (Vector2){ 50, 450 };
    player.velocity = (Vector2){ 0, 0 };
    player.speed = 300.0f;
    player.jumpSpeed = 500.0f;
    player.gravity = 1500.0f;
    player.isGrounded = false;
    player.rect = (Rectangle){ player.position.x, player.position.y, 30, 30 };
}

void UpdatePlayer(float dt) {
    // Movement
    if (!controlsReversed) {
        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) player.velocity.x = player.speed;
        else if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) player.velocity.x = -player.speed;
        else player.velocity.x = 0;
    } else {
        if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) player.velocity.x = player.speed;
        else if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) player.velocity.x = -player.speed;
        else player.velocity.x = 0;
    }

    // Jumping
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W) || IsKeyPressed(KEY_SPACE)) {
        if (player.isGrounded) {
            player.velocity.y = -player.jumpSpeed;
            player.isGrounded = false;
        }
    }

    // Apply gravity
    player.velocity.y += player.gravity * dt;

    // Apply velocity to position
    player.position.x += player.velocity.x * dt;
    player.position.y += player.velocity.y * dt;

    // Update rect
    player.rect.x = player.position.x;
    player.rect.y = player.position.y;
    
    // Falling spike logic update
    for(int i=0; i<spikeCount; i++) {
        if (spikes[i].active && spikes[i].dir == SPIKE_DOWN) {
            // A simple hack: if spike is not at ground level and not static, move it down
            if (spikes[i].rect.width == 30 && spikes[i].rect.height == 40) { // Specific to our falling spike
                if (spikes[i].rect.y < 460) { // Stop at ground (500 - height)
                    spikes[i].rect.y += 1800.0f * dt; // Faster falling speed
                    if (spikes[i].rect.y > 460) spikes[i].rect.y = 460;
                }
            }
        }
    }
}

void DrawPlayer(void) {
    DrawRectangleRec(player.rect, RED);
    // Draw face
    DrawRectangle(player.rect.x + 5, player.rect.y + 5, 5, 5, BLACK);
    DrawRectangle(player.rect.x + 20, player.rect.y + 5, 5, 5, BLACK);
    DrawRectangle(player.rect.x + 5, player.rect.y + 20, 20, 5, BLACK);
}

void CheckCollisions(void) {
    player.isGrounded = false;
    
    // Level Bounds
    if (player.position.x < 0) { player.position.x = 0; player.rect.x = 0; }
    if (player.position.x > 800 - player.rect.width) { player.position.x = 800 - player.rect.width; player.rect.x = player.position.x; }
    
    if (player.position.y > 600) {
        // Fall out of level -> Death
        lives--;
        if (lives < 0) {
            currentState = GAME_OVER;
        } else {
            ResetLevel(); // Restart same level
        }
        return;
    }

    // Block Collisions
    for (int i = 0; i < blockCount; i++) {
        if (blocks[i].active && CheckCollisionRecs(player.rect, blocks[i].rect)) {
            Rectangle b = blocks[i].rect;
            Rectangle p = player.rect;
            
            // Resolve collision (simple AABB)
            float dx = (p.x + p.width/2) - (b.x + b.width/2);
            float dy = (p.y + p.height/2) - (b.y + b.height/2);
            float width = (p.width + b.width)/2;
            float height = (p.height + b.height)/2;
            float crossWidth = width * dy;
            float crossHeight = height * dx;

            if (MathAbs(dx) <= width && MathAbs(dy) <= height) {
                if (crossWidth > crossHeight) {
                    if (crossWidth > -crossHeight) {
                        // Bottom collision
                        player.position.y = b.y + b.height;
                        player.velocity.y = 0;
                    } else {
                        // Left collision
                        player.position.x = b.x - p.width;
                        player.velocity.x = 0;
                    }
                } else {
                    if (crossWidth > -crossHeight) {
                        // Right collision
                        player.position.x = b.x + b.width;
                        player.velocity.x = 0;
                    } else {
                        // Top collision
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

    // Spike Collisions -> Death
    for (int i = 0; i < spikeCount; i++) {
        if (spikes[i].active && CheckCollisionRecs(player.rect, spikes[i].rect)) {
            // Shrink hitbox slightly to be fair
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

    // Door Collision -> Level Complete
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

GameState currentState = TITLE;
int currentLevel = 1;
int lives = 2;

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
    ClearBackground(CLITERAL(Color){ 245, 245, 220, 255 }); // Pale yellow bg

    switch(currentState) {
        case TITLE:
            DrawText("FLOOR IS LIAR", 200, 200, 40, BLACK);
            DrawText("Press ENTER to Start", 250, 300, 20, DARKGRAY);
            break;
        case PLAYING:
            DrawEntities();
            DrawPlayer();
            DrawText(TextFormat("Level: %d", currentLevel), 10, 10, 20, BLACK);
            DrawText(TextFormat("Lives: %d", lives), 10, 40, 20, RED);
            break;
        case GAME_OVER:
            DrawText("GAME OVER", 250, 200, 50, RED);
            DrawText("Press ENTER to Restart", 250, 300, 20, DARKGRAY);
            break;
        case LEVEL_COMPLETE:
            DrawText("LEVEL COMPLETE", 200, 200, 40, GREEN);
            DrawText("Press ENTER to Continue", 250, 300, 20, DARKGRAY);
            break;
        case GAME_VICTORY:
            DrawText("Congratulations!", 200, 200, 40, GOLD);
            DrawText("You won over RageBait!!", 200, 260, 30, DARKGRAY);
            DrawText("Press ENTER to Restart", 250, 350, 20, DARKGRAY);
            break;
    }
}

int main(void)
{
    const int screenWidth = 800;
    const int screenHeight = 600;

    InitWindow(screenWidth, screenHeight, "Floor is Liar");
    InitAudioDevice(); // Initialize audio device

    InitGame();

    Music bgMusic = LoadMusicStream("Mission_Impossible.mp3");
    PlayMusicStream(bgMusic); // Start playing

    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        UpdateMusicStream(bgMusic); // Update music buffer
        UpdateGame();
        
        BeginDrawing();
        
        DrawGame();
        
        EndDrawing();
    }

    UnloadMusicStream(bgMusic); // Unload music stream buffers from RAM
    CloseAudioDevice();         // Close audio device

    CloseWindow();

    return 0;
}
