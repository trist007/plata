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
        
        camera.target = player.position;
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
    // NOTE(trist007): delta is to keep the player speed consistent among
    // different computers running at different fps
    if (IsKeyDown(KEY_LEFT)) player->position.x -= PLAYER_HOR_SPD * delta;
    if (IsKeyDown(KEY_RIGHT)) player->position.x += PLAYER_HOR_SPD * delta;
    if (IsKeyDown(KEY_SPACE) && player->canJump)
    {
        player->speed = -PLAYER_JUMP_SPD;
        player->canJump = false;
    }
    
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
    
    bool hitObstacle = false;
    
    if(collisionLayer != NULL)
    {
        TmxObjectGroup *objGroup = &collisionLayer->exact.objectGroup;
        
        // Check collision with each object in the layer
        for(uint32_t i = 0;
            i < objGroup->objectsLength;
            i++)
        {
            TmxObject *obj = &objGroup->objects[i];
            
            // Only check for rectangle objects
            if(obj->type == OBJECT_TYPE_RECTANGLE)
            {
                
                float platformTop = (float)obj->y;
                float platformBottom = (float)(obj->y + obj->height);
                float futureY = player->position.y + player->speed * delta;
                
                // Check horizontal bounds
                if(player->position.x >= obj->x &&
                   player->position.x <= obj->x + obj->width)
                {
                    // Falling onto a platform
                    if(player->speed >= 0 &&
                       player->position.y <= platformTop + 1.0f &&
                       futureY >= platformTop - 1.0f)
                    {
                        hitObstacle = true;
                        player->speed = 0.0f;
                        player->position.y = platformTop;
                        break;
                    }
                    
                    // Jumping into ceiling
                    if(player->position.y >= platformBottom &&
                       futureY <= platformBottom)
                    {
                        player->speed = 0.0f;
                        player->position.y = platformBottom;
                        break;
                    }
                }
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
}