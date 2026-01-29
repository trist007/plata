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

extern "C" 
{
#include "raytmx.h"
}

#define Gravity 400
#define PLAYER_JUMP_SPD 350.0f
#define PLAYER_HOR_SPD 300.0f

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef struct animationFrame
{
    int currentFrame;
    int frameCount;
    float frameTimer;
    float frameSpeed;
} animationFrame;

typedef struct Gun
{
    float coolDown;;
    Sound gunSound;
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
    animationFrame running;
    
    // Gun firing animation
    animationFrame firing;
    
    // Gun parameters
    Gun gun;
} Player;

typedef struct EnvItem {
    Rectangle rect;
    int blocking;
    Color color;
} EnvItem;

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
void UpdatePlayer(Player *player, TmxMap *map, float delta);
//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 1024;
    const int screenHeight = 768;
    
    InitWindow(screenWidth, screenHeight, "raylib [core] example - 2d camera platformer");
    SetWindowPosition(60, 30);
    
    InitAudioDevice();
    
    //Texture2D playerSprite = LoadTexture("plata/data/Sprite-0001.png");
    Texture2D idle_right = LoadTexture("plata/data/player_idle-right.png");
    Texture2D idle_left = LoadTexture("plata/data/player_idle-left.png");
    Texture2D runSheet_right = LoadTexture("plata/data/player_run-right.png");
    Texture2D runSheet_left = LoadTexture("plata/data/player_run-left.png");
    Texture2D idle_left_fire = LoadTexture("plata/data/player_idle_left_fire.png");
    Texture2D idle_right_fire = LoadTexture("plata/data/player_idle_right_fire.png");
    
    if(runSheet_left.id == 0 ||
       runSheet_right.id == 0 ||
       idle_left.id == 0 ||
       idle_right.id == 0)
    {
        TraceLog(LOG_ERROR, "Failed to load player sprites!");
        //TraceLog(LOG_INFO, "Working directory: %s", GetWorkingDirectory());
        return(1);
    }
    
    
    // Load tilemap
    TmxMap* map = LoadTMX("plata/data/plata.tmx");
    if(map == 0)
    {
        TraceLog(LOG_ERROR, "Failed to load TMX \"%s\"", "plata/data/plata.tmx");
        return(1);
    }
    
    Player player = { 0 };
    player.position = { 400, 280 };
    player.velocityX = 0.0f;
    player.velocityY = 0.0f;
    player.width = (float)idle_right.width;
    player.height = (float)idle_right.height;
    player.facingRight = true;
    player.canJump = false;
    player.inAir = false;
    player.idle = true;
    player.gunFiring = false;
    
    player.running.currentFrame = 0;
    player.running.frameCount = 8;
    player.running.frameTimer = 0.0f;
    player.running.frameSpeed = 0.1f;  // 10 frames per second (1.0/10)
    
    player.firing.currentFrame = 0;
    player.firing.frameCount = 4;
    player.firing.frameTimer = 0.0f;
    player.firing.frameSpeed = 0.1f;  // 10 frames per second (1.0/10)
    
    player.gun.coolDown = 0.2f;
    player.gun.gunSound = LoadSound("plata/data/sounds/pistol-fire.wav");
    
    Camera2D camera = { 0 };
    camera.target = player.position;
    camera.offset = { screenWidth/2.0f, screenHeight/2.0f };
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
        
        //UpdatePlayer(&player, envItems, envItemsLength, deltaTime);
        UpdatePlayer(&player, map, deltaTime);
        
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
        
        // Player animations
        
        // Shooting gun
        if(player.gunFiring)
        {
            float frameWidth = idle_right_fire.width / player.firing.frameCount;
            float frameHeight = idle_right_fire.height;
            
            Rectangle sourceRec =
            {
                player.firing.currentFrame * frameWidth, // x position in runSheet
                0,                                // y position (only one row, so 0)
                frameWidth,                       // width of one frame
                frameHeight                       // height of one frame
            };
            
            Rectangle destRec =
            {
                player.position.x - frameWidth / 2,
                player.position.y - frameHeight,
                frameWidth,
                frameHeight
            };
            
            if(player.facingRight)
            {
                DrawTexturePro(idle_right_fire, sourceRec, destRec, {0, 0}, 0.0f, WHITE);
            }
            else
            {
                DrawTexturePro(idle_left_fire, sourceRec, destRec, {0, 0}, 0.0f, WHITE);
            }
            
        }
        else if(!player.idle)
        {
            float frameWidth = runSheet_right.width / player.running.frameCount;
            float frameHeight = runSheet_right.height;
            
            Rectangle sourceRec =
            {
                player.running.currentFrame * frameWidth, // x position in runSheet
                0,                                // y position (only one row, so 0)
                frameWidth,                       // width of one frame
                frameHeight                       // height of one frame
            };
            
            Rectangle destRec =
            {
                player.position.x - frameWidth / 2,
                player.position.y - frameHeight,
                frameWidth,
                frameHeight
            };
            
            if(player.facingRight)
            {
                DrawTexturePro(runSheet_right, sourceRec, destRec, {0, 0}, 0.0f, WHITE);
            }
            else
            {
                DrawTexturePro(runSheet_left, sourceRec, destRec, {0, 0}, 0.0f, WHITE);
            }
        }
        else
        {
            if(player.facingRight)
            {
                DrawTexture(idle_right,
                            (int)(player.position.x - idle_right.width / 2),
                            (int)(player.position.y - idle_right.height),
                            WHITE);
            }
            else
            {
                DrawTexture(idle_left,
                            (int)(player.position.x - idle_left.width / 2),
                            (int)(player.position.y - idle_left.height),
                            WHITE);
            }
        }
        
        EndMode2D();
        
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
    UnloadTexture(runSheet_right);
    UnloadTexture(runSheet_left);
    UnloadTexture(idle_right);
    UnloadTexture(idle_left);
    UnloadSound(player.gun.gunSound);
    CloseAudioDevice();
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------
    
    return 0;
}

void
UpdatePlayer(Player *player, TmxMap *map, float delta)
{
    // Find the collision layer
    TmxLayer *collisionLayer = 0;
    for(uint32_t i = 0;
        i < map->layersLength;
        i++)
    {
        if(map->layers[i].type == LAYER_TYPE_OBJECT_GROUP &&
           TextIsEqual(map->layers[i].name, "Collision"))
        {
            collisionLayer = &map->layers[i];
            break;
        }
    }
    
    if(collisionLayer == 0)
    {
        TraceLog(LOG_ERROR, "Could not locate Object Layer");
        return;
    }
    
    TmxObjectGroup *objGroup = &collisionLayer->exact.objectGroup;
    
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
    
    if(inputX != 0.0f)
    {
        // Acceleration
        player->velocityX += inputX * acceleration * delta;
        
        // Max speed
        if(player->velocityX > max_speed) player->velocityX = max_speed;
        if(player->velocityX < -max_speed) player->velocityX = -max_speed;
    }
    else
    {
        // Deceleration
        if(player->velocityX > 0 && !player->inAir)
        {
            player->velocityX -= deceleration * delta;
            if(player->velocityX < 0) player->velocityX = 0;
        }
        else if(player->velocityX < 0 && !player->inAir)
        {
            player->velocityX += deceleration * delta;
            if(player->velocityX > 0) player->velocityX = 0;
        }
    }
    
    float moveX = player->velocityX * delta;
    
    // Player hitbox
    float playerLeft = player->position.x - player->width / 2;
    float playerRight = player->position.x + player->width / 2;
    float playerTop = player->position.y - player->height;
    float playerBottom = player->position.y;
    
    float futureLeft = playerLeft + moveX;
    float futureRight = playerRight + moveX;
    
    // Collisions
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
    
    // Vertical movement
    bool hitObstacle = false;
    float futureY = player->position.y + player->velocityY * delta;
    
    // Recalculate player bounds after horizontal movement
    playerLeft = player->position.x - player->width / 2;
    playerRight = player->position.x + player->width / 2;
    playerTop = player->position.y - player->height;
    playerBottom = player->position.y;
    
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
            // Falling onto platform
            if(player->velocityY >= 0 &&
               playerBottom <= platformTop + 1.0f &&
               futureY >= platformTop - 1.0f)
            {
                hitObstacle = true;
                player->velocityY = 0.0f;
                player->position.y = platformTop;
                break;
            }
            
            // Jumping into ceiling
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
        player->velocityY += Gravity * delta;
        player->canJump = false;
    }
    else
    {
        player->canJump = true;
        player->inAir = false;
    }
    
    // Animation when idle and when running
    if(!player->inAir && player->velocityX != 0)
    {
        player->idle = false;
        player->running.frameTimer += delta;
        if(player->running.frameTimer >= player->running.frameSpeed)
        {
            player->running.frameTimer = 0.0f;
            player->running.currentFrame++;
            
            if(player->running.currentFrame >= player->running.frameCount) player->running.currentFrame = 0;
        }
    }
    else
    {
        player->idle = true;
    }
    
    player->gun.coolDown -= delta;
    
    // Firing gun
    if(IsKeyPressed(KEY_BACKSPACE) && player->gun.coolDown <= 0)
    {
        player->gunFiring = true;
        player->firing.currentFrame = 0;
        PlaySound(player->gun.gunSound);
        player->gun.coolDown = 0.2f;
        
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
}