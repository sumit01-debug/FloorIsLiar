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
    NAME_INPUT,
    INSTRUCTIONS,
    HIGH_SCORES,
    PLAYING,
    PAUSED,
    GAME_OVER,
    LEVEL_COMPLETE,
    GAME_VICTORY
} GameState;

typedef struct ScoreEntry {
    char name[32];
    float time;
} ScoreEntry;

void InitGame(void);
void UpdateGame(void);
void DrawGame(void);
void ResetLevel(void);
void ResetGame(void);
void LoadScores(void);
void SaveScores(void);
void AddScore(const char* name, float time);

Texture2D bgTex;
Texture2D checkTex;
Texture2D playerTex;
Texture2D longPlatformTex;
Texture2D smallPlatformTex;
Music bgMusic;

GameState currentState = TITLE;
int currentLevel = 1;
int lives = 2;
float playTime = 0.0f;
char playerName[32] = "\0";
int letterCount = 0;
ScoreEntry topScores[3] = {0};

Rectangle startBtn = { 300, 250, 200, 40 };
Rectangle instBtn = { 300, 320, 200, 40 };
Rectangle scoreBtn = { 300, 390, 200, 40 };
Rectangle homeBtn = { 300, 420, 200, 40 };
Rectangle backBtn = { 300, 500, 200, 40 };
Rectangle musicBtn = { 680, 10, 100, 30 };
Rectangle pauseResumeBtn = { 300, 300, 200, 40 };
Rectangle pauseRestartBtn = { 300, 370, 200, 40 };
Rectangle pauseHomeBtn = { 300, 440, 200, 40 };
bool musicEnabled = true;

Player player;

Block blocks[MAX_BLOCKS];
int blockCount = 0;

Spike spikes[MAX_SPIKES];
int spikeCount = 0;

Trigger triggers[MAX_TRIGGERS];
int triggerCount = 0;

Rectangle door;
bool doorActive = false;
bool controlsReversed = false;
Rectangle pauseBtn = { 740, 10, 40, 40 };

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
        if (checkTex.id != 0) {
            DrawTexturePro(checkTex, (Rectangle){0, 0, checkTex.width, checkTex.height}, door, (Vector2){0, 0}, 0.0f, WHITE);
        } else {
            DrawRectangleRec(door, LIME);
            DrawRectangleLinesEx(door, 2, DARKGREEN);
            DrawRectangle(door.x + 5, door.y + door.height/2, 5, 5, DARKGREEN); // Door knob
        }
    }

    // Draw Blocks
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
            AddSpike(330, 470, 30, 30, SPIKE_UP);
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
    if (playerTex.id != 0) {
        DrawTexturePro(playerTex, (Rectangle){0, 0, playerTex.width, playerTex.height}, player.rect, (Vector2){0, 0}, 0.0f, WHITE);
    } else {
        DrawRectangleRec(player.rect, RED);
        // Draw face
        DrawRectangle(player.rect.x + 5, player.rect.y + 5, 5, 5, BLACK);
        DrawRectangle(player.rect.x + 20, player.rect.y + 5, 5, 5, BLACK);
        DrawRectangle(player.rect.x + 5, player.rect.y + 20, 20, 5, BLACK);
    }
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
            AddScore(playerName, playTime);
        } else {
            currentState = LEVEL_COMPLETE;
        }
    }
}

float MathAbs(float x) {
    return x < 0 ? -x : x;
}

void LoadScores(void) {
    for(int i=0; i<3; i++) {
        topScores[i].time = 9999.0f;
        topScores[i].name[0] = '\0';
    }
    FILE *f = fopen("scores.txt", "r");
    if (f) {
        for(int i=0; i<3; i++) {
            if (fscanf(f, "%31s %f", topScores[i].name, &topScores[i].time) != 2) break;
        }
        fclose(f);
    }
}

void SaveScores(void) {
    FILE *f = fopen("scores.txt", "w");
    if (f) {
        for(int i=0; i<3; i++) {
            if (topScores[i].time < 9999.0f) {
                fprintf(f, "%s %f\n", topScores[i].name, topScores[i].time);
            }
        }
        fclose(f);
    }
}

void AddScore(const char* name, float time) {
    ScoreEntry newEntry;
    int i = 0;
    while(name[i] != '\0' && i < 31) {
        newEntry.name[i] = name[i];
        i++;
    }
    newEntry.name[i] = '\0';
    newEntry.time = time;

    for (int j=0; j<3; j++) {
        if (newEntry.time < topScores[j].time) {
            for (int k=2; k>j; k--) {
                topScores[k] = topScores[k-1];
            }
            topScores[j] = newEntry;
            break;
        }
    }
    SaveScores();
}

void InitGame(void) {
    currentState = TITLE;
    currentLevel = 1;
    lives = 2;
    LoadScores();
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
    playTime = 0.0f;
    ResetLevel();
}

void UpdateGame(void) {
    float dt = GetFrameTime();

    switch(currentState) {
        case TITLE:
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                Vector2 m = GetMousePosition();
                if (CheckCollisionPointRec(m, musicBtn)) {
                    musicEnabled = !musicEnabled;
                    if (musicEnabled) {
                        ResumeMusicStream(bgMusic);
                    } else {
                        PauseMusicStream(bgMusic);
                    }
                } else if (CheckCollisionPointRec(m, startBtn)) {
                    currentState = NAME_INPUT;
                    letterCount = 0;
                    playerName[0] = '\0';
                } else if (CheckCollisionPointRec(m, instBtn)) {
                    currentState = INSTRUCTIONS;
                } else if (CheckCollisionPointRec(m, scoreBtn)) {
                    currentState = HIGH_SCORES;
                }
            }
            break;
        case NAME_INPUT:
            {
                int key = GetCharPressed();
                while (key > 0) {
                    if ((key >= 32) && (key <= 125) && (letterCount < 31)) {
                        playerName[letterCount] = (char)key;
                        playerName[letterCount+1] = '\0';
                        letterCount++;
                    }
                    key = GetCharPressed();
                }
                if (IsKeyPressed(KEY_BACKSPACE)) {
                    letterCount--;
                    if (letterCount < 0) letterCount = 0;
                    playerName[letterCount] = '\0';
                }
                if (IsKeyPressed(KEY_ENTER) && letterCount > 0) {
                    ResetGame();
                }
            }
            break;
        case INSTRUCTIONS:
        case HIGH_SCORES:
            if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER) || (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), backBtn))) {
                currentState = TITLE;
            }
            break;
        case PLAYING:
            if (IsKeyPressed(KEY_P) || (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), pauseBtn))) {
                currentState = PAUSED;
                PauseMusicStream(bgMusic);
            } else {
                playTime += dt;
                UpdatePlayer(dt);
                CheckCollisions();
                CheckTriggers(player.rect);
            }
            break;
        case PAUSED:
            if (IsKeyPressed(KEY_P) || (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), pauseBtn))) {
                currentState = PLAYING;
                if (musicEnabled) ResumeMusicStream(bgMusic);
            }
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                Vector2 m = GetMousePosition();
                if (CheckCollisionPointRec(m, pauseResumeBtn)) {
                    currentState = PLAYING;
                    if (musicEnabled) ResumeMusicStream(bgMusic);
                } else if (CheckCollisionPointRec(m, pauseRestartBtn)) {
                    ResetGame();
                } else if (CheckCollisionPointRec(m, pauseHomeBtn)) {
                    currentState = TITLE;
                }
            }
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
            if (IsKeyPressed(KEY_ENTER) || (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), homeBtn))) {
                currentState = TITLE;
            }
            break;
    }
}

void DrawGame(void) {
    ClearBackground(CLITERAL(Color){ 245, 245, 220, 255 }); // Pale yellow bg
    if (bgTex.id != 0) {
        DrawTexturePro(bgTex, (Rectangle){0, 0, bgTex.width, bgTex.height}, (Rectangle){0, 0, 800, 600}, (Vector2){0, 0}, 0.0f, WHITE);
    }

    switch(currentState) {
        case TITLE:
            DrawText("FLOOR IS LIAR", 200, 150, 40, WHITE);
            DrawRectangleRec(startBtn, DARKGRAY);
            DrawText("Start Game", startBtn.x + 30, startBtn.y + 10, 20, WHITE);
            DrawRectangleRec(instBtn, DARKGRAY);
            DrawText("Instructions", instBtn.x + 30, instBtn.y + 10, 20, WHITE);
            DrawRectangleRec(scoreBtn, DARKGRAY);
            DrawText("Leaderboard", scoreBtn.x + 35, scoreBtn.y + 10, 20, WHITE);
            
            DrawRectangleRec(musicBtn, DARKGRAY);
            DrawText(musicEnabled ? "Music: ON" : "Music: OFF", musicBtn.x + 10, musicBtn.y + 8, 15, WHITE);
            break;
        case NAME_INPUT:
            DrawText("Enter Your Name:", 250, 200, 30, WHITE);
            DrawRectangle(250, 250, 300, 40, LIGHTGRAY);
            DrawText(playerName, 260, 260, 20, BLACK);
            DrawText("Press ENTER to Confirm", 250, 320, 20, LIGHTGRAY);
            break;
        case INSTRUCTIONS:
            DrawText("INSTRUCTIONS", 300, 100, 30, WHITE);
            DrawText("- Arrow Keys / WASD to move", 200, 200, 20, LIGHTGRAY);
            DrawText("- UP / W / SPACE to jump", 200, 250, 20, LIGHTGRAY);
            DrawText("- Avoid spikes and traps!", 200, 300, 20, LIGHTGRAY);
            DrawText("- Reach the green door", 200, 350, 20, LIGHTGRAY);
            DrawRectangleRec(backBtn, DARKGRAY);
            DrawText("Back", backBtn.x + 75, backBtn.y + 10, 20, WHITE);
            break;
        case HIGH_SCORES:
            DrawText("LEADERBOARD", 310, 100, 30, WHITE);
            for(int i=0; i<3; i++) {
                if (topScores[i].time < 9999.0f) {
                    int m = (int)topScores[i].time / 60;
                    float s = topScores[i].time - (m * 60);
                    DrawText(TextFormat("%d. %s - %02d:%05.2f", i+1, topScores[i].name, m, s), 250, 200 + i*50, 20, LIGHTGRAY);
                } else {
                    DrawText(TextFormat("%d. ---", i+1), 250, 200 + i*50, 20, DARKGRAY);
                }
            }
            DrawRectangleRec(backBtn, DARKGRAY);
            DrawText("Back", backBtn.x + 75, backBtn.y + 10, 20, WHITE);
            break;
        case PLAYING:
            DrawEntities();
            DrawPlayer();
            DrawText(TextFormat("Level: %d", currentLevel), 10, 10, 20, WHITE);
            DrawText(TextFormat("Lives: %d", lives), 10, 40, 20, YELLOW);
            
            // Draw Pause Button
            DrawRectangleRec(pauseBtn, DARKGRAY);
            DrawRectangle(pauseBtn.x + 12, pauseBtn.y + 10, 6, 20, WHITE);
            DrawRectangle(pauseBtn.x + 22, pauseBtn.y + 10, 6, 20, WHITE);

            // Draw Time
            DrawText(TextFormat("Time: %02d:%05.2f", (int)playTime / 60, playTime - ((int)playTime / 60) * 60), 620, 60, 20, WHITE);
            break;
        case PAUSED:
            DrawEntities();
            DrawPlayer();
            DrawText(TextFormat("Level: %d", currentLevel), 10, 10, 20, WHITE);
            DrawText(TextFormat("Lives: %d", lives), 10, 40, 20, YELLOW);
            
            // Draw Play Button
            DrawRectangleRec(pauseBtn, DARKGRAY);
            DrawTriangle((Vector2){pauseBtn.x + 12, pauseBtn.y + 10}, (Vector2){pauseBtn.x + 12, pauseBtn.y + 30}, (Vector2){pauseBtn.x + 32, pauseBtn.y + 20}, WHITE);
            
            // Draw Time
            DrawText(TextFormat("Time: %02d:%05.2f", (int)playTime / 60, playTime - ((int)playTime / 60) * 60), 620, 60, 20, WHITE);

            DrawRectangle(0, 0, 800, 600, (Color){ 0, 0, 0, 150 });
            DrawText("PAUSED", 310, 200, 50, WHITE);
            
            DrawRectangleRec(pauseResumeBtn, DARKGRAY);
            DrawText("Resume", pauseResumeBtn.x + 60, pauseResumeBtn.y + 10, 20, WHITE);
            
            DrawRectangleRec(pauseRestartBtn, DARKGRAY);
            DrawText("Restart", pauseRestartBtn.x + 60, pauseRestartBtn.y + 10, 20, WHITE);
            
            DrawRectangleRec(pauseHomeBtn, DARKGRAY);
            DrawText("Home", pauseHomeBtn.x + 70, pauseHomeBtn.y + 10, 20, WHITE);
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
            DrawText(TextFormat("Time: %02d:%05.2f", (int)playTime / 60, playTime - ((int)playTime / 60) * 60), 200, 310, 20, YELLOW);
            DrawText("Press ENTER to Restart", 250, 370, 20, LIGHTGRAY);
            
            DrawRectangleRec(homeBtn, DARKGRAY);
            DrawText("Return Home", homeBtn.x + 35, homeBtn.y + 10, 20, WHITE);
            break;
    }
}

int main(void)
{
    const int screenWidth = 800;
    const int screenHeight = 600;

    InitWindow(screenWidth, screenHeight, "Floor is Liar");
    InitAudioDevice(); // Initialize audio device

    bgTex = LoadTexture("lava.png");
    checkTex = LoadTexture("check.png");
    playerTex = LoadTexture("player1.png");
    longPlatformTex = LoadTexture("longplatform.png");
    smallPlatformTex = LoadTexture("smallplatform.png");

    InitGame();

    bgMusic = LoadMusicStream("Mission_Impossible.mp3");
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

    UnloadTexture(bgTex);
    UnloadTexture(checkTex);
    UnloadTexture(playerTex);
    UnloadTexture(longPlatformTex);
    UnloadTexture(smallPlatformTex);

    UnloadMusicStream(bgMusic); // Unload music stream buffers from RAM
    CloseAudioDevice();         // Close audio device

    CloseWindow();

    return 0;
}
