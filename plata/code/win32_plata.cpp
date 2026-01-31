/*******************************************************************************************
*
*   raylib [core] example - 2d camera platformer
*
*   Example complexity rating: [★★★☆] 3/4
*
*   Example originally created with raylib 2.5, last time updated with raylib 3.0
*
*   Example contributed by arvyy (@arvyy) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2019-2025 arvyy (@arvyy)
*
********************************************************************************************/

#include "raylib.h"
#include "raymath.h"

// DON'T include windows.h at all - use this workaround instead:
//extern "C" __declspec(dllimport) void __stdcall OutputDebugStringA(const char* lpOutputString);

extern "C" 
{
#include "raytmx.h"
}

#define GRAVITY 400
#define PLAYER_JUMP_SPD 350.0f
#define PLAYER_HOR_SPD 300.0f
#define MAX_PROJECTILES 20
#define PROJECTILE_SPEED 900.0f

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef struct Projectile
{
    Vector2 velocity;
    Vector2 position;
    bool active;
} Projectile;

typedef struct AnimationRectangles
{
    Rectangle source;
    Rectangle destination;
} AnimationRectangles;

typedef struct PlayerTextures
{
    Texture2D idle_right;
    Texture2D idle_left;
    Texture2D run_right;
    Texture2D run_left;
    Texture2D idle_right_fire;
    Texture2D idle_left_fire;
} PlayerTextures;

typedef struct AnimationFrame
{
    int currentFrame;
    int frameCount;
    float frameTimer;
    float frameSpeed;
} AnimationFrame;

typedef enum
{
    PISTOL_SOUND_FIRE = 0,
    PISTOL_SOUND_DRYFIRE,
    PISTOL_SOUND_RELOAD,
    PISTOL_SOUND_STEAM,
    PISTOL_SOUND_COUNT
} PistolSoundType;

typedef struct Gun
{
    float coolDown;;
    int rounds;
    int roundsPerMagazine;
    bool overHeated;
    float overHeatTimer;
    
    Projectile bullets[MAX_PROJECTILES];
    float bulletRadius;
    float bulletSpeed;
    
    Sound pistolSounds[PISTOL_SOUND_COUNT];
} Gun;

typedef struct Player {
    Vector2 position;
    float velocityX;
    float velocityY;
    float height;
    float width;
    bool facingRight;
    bool canJump;
    bool inAir;
    bool idle;
    bool gunFiring;
    
    // Running animation
    AnimationFrame running;
    
    // Gun firing animation
    AnimationFrame firing;
    
    // Gun parameters
    Gun gun;
} Player;

typedef struct GameState
{
    int screenWidth;
    int screenHeight;
} GameState;

//----------------------------------------------------------------------------------
// Function Forward Declarations / Prototypes
//----------------------------------------------------------------------------------
void UpdatePlayer(Player *player, TmxMap *map, float delta);
static TmxObjectGroup *GetCollisionLayer(TmxMap *map);
static void UpdatePlayerMovement(Player *player, float delta);
static void UpdatePlayerHorizontalCollision(Player *player, TmxObjectGroup *objGroup, float delta);
static void UpdatePlayerVerticalCollision(Player *player, TmxObjectGroup *objGroup, float delta);
static void UpdatePlayerAnimation(Player *player, float delta);
static void UpdatePlayerWeapon(Player *player, float delta);
static void UpdateBullets(Projectile *projectiles, GameState *gameState, float delta);
void DrawPlayer(Player *player, PlayerTextures *textures);
void DrawBullets(Projectile *projectiles);
int InitPlayerTextures(PlayerTextures *playerTextures);
void UnloadPlayerTextures(PlayerTextures *playerTextures);
void UnloadSounds(Gun *pistol);
static void SpawnBullet(Player *player);
int InitPlayer(Player *player, PlayerTextures *textures);
AnimationRectangles GenerateAnimationRectangle(Player *player, AnimationFrame *sheet, Texture2D *texture);

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // For debugging
    //OutputDebugStringA("=== REACHED MAIN===\n");
    // Initialization
    //--------------------------------------------------------------------------------------
    
    GameState gameState = {};
    gameState.screenWidth = 1024;
    gameState.screenHeight = 768;
    
    InitWindow(gameState.screenWidth, gameState.screenHeight, "raylib [core] example - 2d camera platformer");
    SetWindowPosition(60, 30);
    
    InitAudioDevice();
    
    PlayerTextures playerTextures = {};
    InitPlayerTextures(&playerTextures);
    
    // Load tilemap
    TmxMap* map = LoadTMX("plata/data/plata.tmx");
    if(map == 0)
    {
        TraceLog(LOG_ERROR, "Failed to load TMX \"%s\"", "plata/data/plata.tmx");
        return(1);
    }
    
    Player player = {};
    InitPlayer(&player, &playerTextures);
    
    
    Camera2D camera = {};
    camera.target = player.position;
    camera.offset = { gameState.screenWidth/2.0f, gameState.screenHeight/2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;
    
    SetTargetFPS(60);
    //--------------------------------------------------------------------------------------
    
    // Main game loop
    while (!WindowShouldClose())
    {
        // Update
        //----------------------------------------------------------------------------------
        float deltaTime = GetFrameTime();
        
        UpdatePlayer(&player, map, deltaTime);
        UpdateBullets(player.gun.bullets, &gameState, deltaTime);
        
        camera.target.x = floorf(player.position.x);
        camera.target.y = floorf(player.position.y);
        
        //----------------------------------------------------------------------------------
        // Draw
        //----------------------------------------------------------------------------------
        
        BeginDrawing();
        
        ClearBackground(LIGHTGRAY);
        
        BeginMode2D(camera);
        
        //AnimateTMX(map);
        DrawTMX(map, &camera, 0, 0, 0, WHITE);
        
        DrawPlayer(&player, &playerTextures);
        DrawBullets(player.gun.bullets);
        
        EndMode2D();
        
        // Debug Information
        DrawText(TextFormat("Jumping: %s", player.inAir ? "true" : "false"), 
                 10, 10, 20, RED);
        
        DrawText(TextFormat("Firing: %s", player.gunFiring ? "true" : "false"), 
                 10, 30, 20, RED);
        
        EndDrawing();
        
        //----------------------------------------------------------------------------------
    }
    
    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadTMX(map);
    UnloadPlayerTextures(&playerTextures);
    UnloadSounds(&player.gun);
    CloseAudioDevice();
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------
    
    return 0;
}

void
UpdatePlayer(Player *player, TmxMap *map, float delta)
{
    TmxObjectGroup *collisionGroup = GetCollisionLayer(map);
    if(!collisionGroup) return;
    
    UpdatePlayerMovement(player, delta);
    UpdatePlayerHorizontalCollision(player, collisionGroup, delta);
    UpdatePlayerVerticalCollision(player, collisionGroup, delta);
    UpdatePlayerAnimation(player, delta);
    UpdatePlayerWeapon(player, delta);
}

static TmxObjectGroup*
GetCollisionLayer(TmxMap *map)
{
    for(uint32_t i = 0;
        i < map->layersLength;
        i++)
    {
        if(map->layers[i].type == LAYER_TYPE_OBJECT_GROUP &&
           TextIsEqual(map->layers[i].name, "Collision"))
        {
            return &map->layers[i].exact.objectGroup;
        }
    }
    
    TraceLog(LOG_ERROR, "Could not locate Collision layer");
    return(0);
}

static void
UpdatePlayerMovement(Player *player, float delta)
{
    // Movement
    float acceleration = 1000.0f;
    float deceleration = 600.0f;
    float max_speed = PLAYER_HOR_SPD;
    
    float inputX = 0.0f;
    if(IsKeyDown(KEY_LEFT) && !player->inAir)
    {
        inputX = -1.0f;
        player->facingRight = false;
    }
    if(IsKeyDown(KEY_RIGHT) && !player->inAir)
    {
        inputX = 1.0f;
        player->facingRight = true;
    }
    
    // Apply acceleration and deceleration
    if(inputX != 0.0f)
    {
        player->velocityX += inputX * acceleration * delta;
        
        // Clamp to max speed
        player->velocityX = Clamp(player->velocityX, -max_speed, max_speed);
    }
    else if(!player->inAir)
    {
        float decelerateAmount = deceleration * delta;
        
        if(player->velocityX > 0)
        {
            player->velocityX -= decelerateAmount;
            if(player->velocityX < 0) player->velocityX = 0;
        }
        else if(player->velocityX < 0)
        {
            player->velocityX += decelerateAmount;
            if(player->velocityX > 0) player->velocityX = 0;
        }
    }
    
    // Jumping
    if(IsKeyPressed(KEY_SPACE) && player->canJump)
    {
        player->velocityY = -PLAYER_JUMP_SPD;
        player->canJump = false;
        player->inAir = true;
    }
    
    if(IsKeyReleased(KEY_SPACE) && player->velocityY < 0)
    {
        player->velocityY *= 0.5f;
    }
}

static void
UpdatePlayerHorizontalCollision(Player *player, TmxObjectGroup *objGroup, float delta)
{
    float moveX = player->velocityX * delta;
    
    // Player hitbox
    float playerLeft = player->position.x - player->width / 2;
    float playerRight = player->position.x + player->width / 2;
    float playerTop = player->position.y - player->height;
    float playerBottom = player->position.y;
    
    float futureLeft = playerLeft + moveX;
    float futureRight = playerRight + moveX;
    
    for(uint32_t i = 0;
        i < objGroup->objectsLength;
        i++)
    {
        TmxObject *obj = &objGroup->objects[i];
        if(obj->type != OBJECT_TYPE_RECTANGLE) continue;
        
        float wallLeft = (float)obj->x;
        float wallRight = (float)(obj->x + obj->width);
        float wallTop = (float)obj->y;
        float wallBottom = (float)(obj->y + obj->height);
        
        // Check if player overlaps vertically with wall
        if(playerBottom > wallTop && playerTop < wallBottom)
        {
            // Moving right and hitting wall with playerTop
            if(moveX > 0 && playerRight <= wallLeft && futureRight >= wallLeft)
            {
                player->position.x = wallLeft - player->width / 2;
                moveX = 0;
                break;
            }
            // Moving left and hitting wall with playerTop
            if(moveX < 0 && playerLeft >= wallRight && futureLeft <= wallRight)
            {
                player->position.x = wallRight + player->width / 2;
                moveX = 0;
                break;
            }
        }
    }
    
    player->position.x += moveX;
}

static void
UpdatePlayerVerticalCollision(Player *player, TmxObjectGroup *objGroup, float delta)
{
    bool hitObstacle = false;
    float futureY = player->position.y + player->velocityY * delta;
    
    // Recalculate player bounds after horizontal movement
    float playerLeft = player->position.x - player->width / 2;
    float playerRight = player->position.x + player->width / 2;
    float playerTop = player->position.y - player->height;
    float playerBottom = player->position.y;
    
    for(uint32_t i = 0;
        i < objGroup->objectsLength;
        i++)
    {
        TmxObject *obj = &objGroup->objects[i];
        if(obj->type != OBJECT_TYPE_RECTANGLE) continue;
        
        float platformLeft = (float)obj->x;
        float platformRight = (float)(obj->x + obj->width);
        float platformTop = (float)obj->y;
        float platformBottom = (float)(obj->y + obj->height);
        
        // Check if player overlaps horizontally
        if(playerRight > platformLeft && playerLeft < platformRight)
        {
            // Landing on platform
            if(player->velocityY >= 0 &&
               playerBottom <= platformTop + 1.0f &&
               futureY >= platformTop - 1.0f)
            {
                hitObstacle = true;
                player->velocityY = 0.0f;
                player->position.y = platformTop;
                break;
            }
            
            // Hitting ceiling
            float futureTop = futureY - player->height;
            if(player->velocityY < 0 &&
               playerTop >= platformBottom &&
               futureTop <= platformBottom)
            {
                player->velocityY = 0.0f;
                player->position.y = platformBottom + player->height;
                break;
            }
        }
        
        // NEW: Check if player would be inside wall after vertical movement
        float futureTop = futureY - player->height;
        float futureBottom = futureY;
        
        if(playerRight > platformLeft && playerLeft < platformRight &&
           futureBottom > platformTop && futureTop < platformBottom)
        {
            // Player would be inside this wall - push them out horizontally
            float playerCenterX = player->position.x;
            float wallCenterX = platformLeft + (platformRight - platformLeft) / 2;
            
            if(playerCenterX < wallCenterX)
            {
                player->position.x = platformLeft - player->width / 2;
            }
            else
            {
                player->position.x = platformRight + player->width / 2;
            }
            
            // Recalculate bounds for next iteration
            playerLeft = player->position.x - player->width / 2;
            playerRight = player->position.x + player->width / 2;
        }
        
        // Check if player is in the air
        if(futureY < platformBottom) player->inAir = true;
        
    }
    
    if(!hitObstacle)
    {
        player->position.y += player->velocityY * delta;
        player->velocityY += GRAVITY * delta;
        player->canJump = false;
    }
    else
    {
        player->canJump = true;
        player->inAir = false;
    }
}

static void
UpdatePlayerAnimation(Player *player, float delta)
{
    if(!player->inAir && player->velocityX != 0)
    {
        player->idle = false;
        player->running.frameTimer += delta;
        if(player->running.frameTimer >= player->running.frameSpeed)
        {
            player->running.frameTimer = 0.0f;
            player->running.currentFrame++;
            
            if(player->running.currentFrame >= player->running.frameCount)
                player->running.currentFrame = 0;
        }
    }
    else
    {
        player->idle = true;
    }
}

static void
SpawnBullet(Player *player)
{
    for(int i = 0;
        i < MAX_PROJECTILES;
        i++)
    {
        if(!player->gun.bullets[i].active)
        {
            player->gun.bullets[i].active = true;
            player->gun.bullets[i].position.x = player->position.x;
            player->gun.bullets[i].position.y = player->position.y - 33;
            
            float direction = player->facingRight ? 1.0f : -1.0f;
            player->gun.bullets[i].velocity.x = direction * player->gun.bulletSpeed;
            
            // just fire one bullet
            break;
        }
    }
}

static void
UpdateBullets(Projectile *projectiles, GameState *gameState, float delta)
{
    for(int i = 0;
        i < MAX_PROJECTILES;
        i++)
    {
        if(projectiles[i].active)
        {
            projectiles[i].position.x +=  projectiles[i].velocity.x * delta;
            
            // Despawn if bullet is offscreen
            if(projectiles[i].position.x < -400 || projectiles[i].position.x > gameState->screenWidth + 400)
            {
                projectiles[i].active = false;
            }
        }
    }
}

static void
UpdatePlayerWeapon(Player *player, float delta)
{
    player->gun.coolDown -= delta;
    
    // Handle overheat
    if(player->gun.overHeated)
    {
        player->gun.overHeatTimer -= delta;
        if(player->gun.overHeatTimer <= 0)
        {
            player->gun.overHeated = false;
            
        }
        
        // Exit function now if gun is overheated
        return;
    }
    
    // Rapid fire
    if(IsKeyDown(KEY_BACKSPACE) && player->gun.coolDown <= 0 && player->gun.rounds > 0)
    {
        player->gunFiring = true;
        SpawnBullet(player);
        player->firing.currentFrame = 0;
        PlaySound(player->gun.pistolSounds[PISTOL_SOUND_FIRE]);
        
        // Lower cool down if holding down fire
        player->gun.coolDown = 0.1f;
        player->gun.rounds--;
        
        // Check if mag is empty overheat
        if(player->gun.rounds == 0)
        {
            player->gun.overHeated = true;
            PlaySound(player->gun.pistolSounds[PISTOL_SOUND_STEAM]);
        }
        
    }
    
    // Firing gun
    if(IsKeyPressed(KEY_BACKSPACE) && player->gun.coolDown <= 0)
    {
        if(player->gun.rounds > 0)
        {
            player->gunFiring = true;
            SpawnBullet(player);
            player->gun.rounds--;
            player->firing.currentFrame = 0;
            PlaySound(player->gun.pistolSounds[PISTOL_SOUND_FIRE]);
            
        }
        else
        {
            PlaySound(player->gun.pistolSounds[PISTOL_SOUND_DRYFIRE]);
        }
        
    }
    
    if(player->gunFiring)
    {
        
        player->firing.frameTimer += delta;
        if(player->firing.frameTimer >= player->firing.frameSpeed)
        {
            player->firing.frameTimer = 0.0f;
            player->firing.currentFrame++;
            
            if(player->firing.currentFrame >= player->firing.frameCount)
            {
                player->gunFiring = false;
                player->firing.currentFrame = 0;
            }
        }
    }
    
    // Reloading
    if(IsKeyPressed(KEY_R) && !player->gun.overHeated)
    {
        PlaySound(player->gun.pistolSounds[PISTOL_SOUND_RELOAD]);
        player->gun.rounds = player->gun.roundsPerMagazine;
    }
}

AnimationRectangles
GenerateAnimationRectangle(Player *player, AnimationFrame *sheet, Texture2D *texture)
{
    float frameWidth = texture->width / sheet->frameCount; 
    float frameHeight = texture->height;
    
    AnimationRectangles rectangles;
    
    rectangles.source =
    {
        sheet->currentFrame * frameWidth,
        0,
        frameWidth,
        frameHeight
    };
    
    rectangles.destination =
    {
        player->position.x - frameWidth / 2,
        player->position.y - frameHeight,
        frameWidth,
        frameHeight
    };
    
    return rectangles;
}

void
DrawBullets(Projectile *projectiles)
{
    for(int i = 0;
        i < MAX_PROJECTILES;
        i++)
    {
        if(projectiles[i].active)
        {
            DrawCircleV(projectiles[i].position, 4.0f, YELLOW);
            DrawCircleV(projectiles[i].position, 2.0f, RED);
        }
    }
}

void
DrawPlayer(Player *player, PlayerTextures *textures)
{
    // Shooting gun
    if(player->gunFiring)
    {
        AnimationRectangles rectangle = GenerateAnimationRectangle(player, &player->firing, &textures->idle_right_fire);
        
        if(player->facingRight)
        {
            DrawTexturePro(textures->idle_right_fire, rectangle.source, rectangle.destination, {0, 0}, 0.0f, WHITE);
        }
        else
        {
            DrawTexturePro(textures->idle_left_fire, rectangle.source, rectangle.destination, {0, 0}, 0.0f, WHITE);
        }
        
    }
    else if(!player->idle)
    {
        AnimationRectangles rectangle = GenerateAnimationRectangle(player, &player->running, &textures->run_right);
        
        if(player->facingRight)
        {
            DrawTexturePro(textures->run_right, rectangle.source, rectangle.destination, {0, 0}, 0.0f, WHITE);
        }
        else
        {
            DrawTexturePro(textures->run_left, rectangle.source, rectangle.destination, {0, 0}, 0.0f, WHITE);
        }
    }
    else
    {
        if(player->facingRight)
        {
            DrawTexture(textures->idle_right,
                        (int)(player->position.x - textures->idle_right.width / 2),
                        (int)(player->position.y - textures->idle_right.height),
                        WHITE);
        }
        else
        {
            DrawTexture(textures->idle_left,
                        (int)(player->position.x - textures->idle_left.width / 2),
                        (int)(player->position.y - textures->idle_left.height),
                        WHITE);
        }
    }
}

int
InitPlayerTextures(PlayerTextures *playerTextures)
{
    playerTextures->idle_right = LoadTexture("plata/data/player_idle-right.png");
    playerTextures->idle_left = LoadTexture("plata/data/player_idle-left.png");
    playerTextures->run_right = LoadTexture("plata/data/player_run-right.png");
    playerTextures->run_left = LoadTexture("plata/data/player_run-left.png");
    playerTextures->idle_right_fire = LoadTexture("plata/data/player_idle_right_fire.png");
    playerTextures->idle_left_fire = LoadTexture("plata/data/player_idle_left_fire.png");
    
    if(!playerTextures->run_left.id  ||
       !playerTextures->run_right.id ||
       !playerTextures->idle_left.id ||
       !playerTextures->idle_right.id ||
       !playerTextures->idle_left_fire.id ||
       !playerTextures->idle_right_fire.id)
    {
        TraceLog(LOG_ERROR, "Failed to load player textures!");
        return(1);
    }
    
    return(0);
}

void
UnloadPlayerTextures(PlayerTextures *playerTextures)
{
    UnloadTexture(playerTextures->run_right);
    UnloadTexture(playerTextures->run_left);
    UnloadTexture(playerTextures->idle_right);
    UnloadTexture(playerTextures->idle_left);
    UnloadTexture(playerTextures->idle_right_fire);
    UnloadTexture(playerTextures->idle_left_fire);
}

void
UnloadSounds(Gun *pistol)
{
    for(int i = 0;
        i < PISTOL_SOUND_COUNT;
        i++)
    {
        UnloadSound(pistol->pistolSounds[i]);
    }
}

int
InitPlayer(Player *player, PlayerTextures *textures)
{
    player->position = { 400, 280 };
    //player->velocityX = 0.0f;
    //player->velocityY = 0.0f;
    
    // NOTE(trist007): in Aseprite there were about 28 pixels to the right if player was facing right
    // if player was facing left there were about 18 pixels to the right, I'm going to have to adjust this
    // in Aseprite at some point
    player->width = (float)(textures->idle_right.width) - 30;
    player->height = (float)textures->idle_right.height;
    player->facingRight = true;
    player->canJump = false;
    player->inAir = false;
    player->idle = true;
    player->gunFiring = false;
    
    //player->running.currentFrame = 0;
    player->running.frameCount = 8;
    //player->running.frameTimer = 0.0f;
    player->running.frameSpeed = 0.1f;  // 10 frames per second (1.0/10)
    
    //player->firing.currentFrame = 0;
    player->firing.frameCount = 4;
    //player->firing.frameTimer = 0.0f;
    player->firing.frameSpeed = 0.1f;  // 10 frames per second (1.0/10)
    
    player->gun.coolDown = 0.2f;
    player->gun.overHeatTimer = 2.0f;
    player->gun.rounds = 7;
    player->gun.roundsPerMagazine = 7;
    player->gun.bulletRadius = 2.0f;
    player->gun.bulletSpeed = PROJECTILE_SPEED;
    player->gun.pistolSounds[PISTOL_SOUND_FIRE] = LoadSound("plata/data/sounds/pistol-fire.wav");
    player->gun.pistolSounds[PISTOL_SOUND_DRYFIRE] = LoadSound("plata/data/sounds/pistol-dry-fire.wav");
    player->gun.pistolSounds[PISTOL_SOUND_RELOAD] = LoadSound("plata/data/sounds/pistol-reload.ogg");
    player->gun.pistolSounds[PISTOL_SOUND_STEAM] = LoadSound("plata/data/sounds/pistol-steam.wav");
    
    
    for(int i = 0;
        i < MAX_PROJECTILES;
        i++)
    {
        player->gun.bullets[i].active = false;
    }
    
    return(0);
}