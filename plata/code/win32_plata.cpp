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

#define G 400
#define PLAYER_JUMP_SPD 350.0f
#define PLAYER_HOR_SPD 200.0f

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef struct Player {
    Vector2 position;
    float speed;
    float height;
    float width;
    bool canJump;
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
    
    Texture2D playerSprite = LoadTexture("plata/data/Sprite-0001.png");
    
    if(playerSprite.id == 0)
    {
        TraceLog(LOG_ERROR, "Failed to load player sprite!");
        TraceLog(LOG_INFO, "Working directory: %s", GetWorkingDirectory());
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
    player.width = (float)playerSprite.width;
    player.height = (float)playerSprite.height;
    player.speed = 0;
    player.canJump = false;
    
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
        
        DrawTexture(playerSprite,
                    (int)(player.position.x - playerSprite.width / 2),
                    (int)(player.position.y - playerSprite.height),
                    WHITE);
        
        EndMode2D();
        
        EndDrawing();
        //----------------------------------------------------------------------------------
    }
    
    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadTMX(map);
    UnloadTexture(playerSprite);
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
    
    if(collisionLayer == 0) return;
    
    TmxObjectGroup *objGroup = &collisionLayer->exact.objectGroup;
    
    // Horizontal movement
    float moveX = 0.0f;
    if(IsKeyDown(KEY_LEFT)) moveX = -PLAYER_HOR_SPD * delta;
    if(IsKeyDown(KEY_RIGHT)) moveX = PLAYER_HOR_SPD * delta;
    
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
    
    // Jumping
    if(IsKeyDown(KEY_SPACE) && player->canJump)
    {
        player->speed = -PLAYER_JUMP_SPD;
        player->canJump = false;
    }
    
    // Vertical movement
    bool hitObstacle = false;
    float futureY = player->position.y + player->speed * delta;
    
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
            if(player->speed >= 0 &&
               playerBottom <= platformTop + 1.0f &&
               futureY >= platformTop - 1.0f)
            {
                hitObstacle = true;
                player->speed = 0.0f;
                player->position.y = platformTop;
                break;
            }
            
            // Jumping into ceiling
            float futureTop = futureY - player->height;
            if(player->speed < 0 &&
               playerTop >= platformBottom &&
               futureTop <= platformBottom)
            {
                player->speed = 0.0f;
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
    }
    
    if(!hitObstacle)
    {
        player->position.y += player->speed * delta;
        player->speed += G * delta;
        player->canJump = false;
    }
    else
    {
        player->canJump = true;
    }
}