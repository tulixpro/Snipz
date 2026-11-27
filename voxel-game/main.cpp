#include "raylib.h"
#include <vector>
#include <cmath>
#include <cstdlib>

struct Block { Vector3 p; Color c; };

static float hashNoise(int x, int z) {
    unsigned int n = (unsigned int)(x * 374761393u + z * 668265263u);
    n = (n ^ (n >> 13)) * 1274126177u;
    n ^= n >> 16;
    return (n & 0xffff) / 65535.0f;
}

int main() {
    const int screenW = 1280, screenH = 720;
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(screenW, screenH, "Blocklands - Voxel Sandbox");
    SetTargetFPS(120);
    DisableCursor();

    Camera3D camera = {};
    camera.position = { 8.0f, 9.0f, 15.0f };
    camera.target = { 8.0f, 2.0f, 8.0f };
    camera.up = { 0.0f, 1.0f, 0.0f };
    camera.fovy = 75.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    std::vector<Block> blocks;
    const int worldSize = 32;
    for (int x = 0; x < worldSize; ++x) {
        for (int z = 0; z < worldSize; ++z) {
            int h = 2 + (int)(hashNoise(x, z) * 4.0f);
            for (int y = 0; y < h; ++y) {
                Color c = (y == h - 1) ? Color{92, 176, 88, 255} : Color{130, 105, 78, 255};
                blocks.push_back({{(float)x, (float)y, (float)z}, c});
            }
        }
    }

    int selected = 0;
    const Color palette[] = {
        {92,176,88,255}, {130,105,78,255}, {115,125,140,255},
        {190,160,90,255}, {80,145,180,255}
    };
    Vector3 velocity = {0,0,0};
    bool onGround = false;
    bool showHelp = true;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (IsKeyPressed(KEY_F1)) showHelp = !showHelp;
        if (IsKeyPressed(KEY_ONE)) selected = 0;
        if (IsKeyPressed(KEY_TWO)) selected = 1;
        if (IsKeyPressed(KEY_THREE)) selected = 2;
        if (IsKeyPressed(KEY_FOUR)) selected = 3;
        if (IsKeyPressed(KEY_FIVE)) selected = 4;

        UpdateCamera(&camera, CAMERA_FIRST_PERSON);
        Vector3 forward = Vector3Normalize(Vector3{camera.target.x-camera.position.x, 0, camera.target.z-camera.position.z});
        Vector3 right = Vector3Normalize(Vector3{forward.z, 0, -forward.x});
        Vector3 move = {0,0,0};
        if (IsKeyDown(KEY_W)) move = Vector3Add(move, forward);
        if (IsKeyDown(KEY_S)) move = Vector3Subtract(move, forward);
        if (IsKeyDown(KEY_D)) move = Vector3Add(move, right);
        if (IsKeyDown(KEY_A)) move = Vector3Subtract(move, right);
        if (Vector3Length(move) > 0) move = Vector3Scale(Vector3Normalize(move), 6.0f * dt);
        camera.position = Vector3Add(camera.position, move);
        camera.target = Vector3Add(camera.target, move);

        if (IsKeyDown(KEY_SPACE) && onGround) { velocity.y = 7.0f; onGround = false; }
        velocity.y -= 18.0f * dt;
        camera.position.y += velocity.y * dt;
        camera.target.y += velocity.y * dt;
        if (camera.position.y < 2.0f) {
            camera.position.y = 2.0f; camera.target.y = 2.0f;
            velocity.y = 0; onGround = true;
        }

        Ray ray = GetMouseRay({screenW/2.0f, screenH/2.0f}, camera);
        int hitIndex = -1;
        float nearest = 1000.0f;
        Vector3 hitNormal = {0,0,0};
        for (int i = 0; i < (int)blocks.size(); ++i) {
            BoundingBox box = {
                {blocks[i].p.x-0.5f, blocks[i].p.y-0.5f, blocks[i].p.z-0.5f},
                {blocks[i].p.x+0.5f, blocks[i].p.y+0.5f, blocks[i].p.z+0.5f}
            };
            RayCollision col = GetRayCollisionBox(ray, box);
            if (col.hit && col.distance < nearest) {
                nearest = col.distance; hitIndex = i; hitNormal = col.normal;
            }
        }
        if (hitIndex >= 0 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            blocks.erase(blocks.begin() + hitIndex);
        }
        if (hitIndex >= 0 && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            Vector3 p = Vector3Add(blocks[hitIndex].p, hitNormal);
            bool occupied = false;
            for (const auto& b : blocks) {
                if (b.p.x == p.x && b.p.y == p.y && b.p.z == p.z) { occupied = true; break; }
            }
            if (!occupied && p.y >= 0 && Vector3Distance(p, camera.position) > 1.2f)
                blocks.push_back({p, palette[selected]});
        }

        BeginDrawing();
        ClearBackground(Color{125,190,235,255});
        BeginMode3D(camera);
        DrawPlane({16, -0.55f, 16}, {100, 1}, Color{70,130,75,255});
        for (const auto& b : blocks) {
            DrawCube(b.p, 1.0f, 1.0f, 1.0f, b.c);
            DrawCubeWires(b.p, 1.0f, 1.0f, 1.0f, Fade(BLACK, 0.12f));
        }
        if (hitIndex >= 0) DrawCubeWires(blocks[hitIndex].p, 1.02f, 1.02f, 1.02f, YELLOW);
        EndMode3D();

        DrawText("BLOCKLANDS", 20, 18, 26, WHITE);
        DrawFPS(20, 54);
        DrawText("WASD: Move   Mouse: Look   SPACE: Jump", 20, screenH-62, 18, WHITE);
        DrawText("LMB: Mine   RMB: Place   1-5: Block", 20, screenH-36, 18, WHITE);
        DrawRectangle(screenW/2-2, screenH/2-10, 4, 20, WHITE);
        DrawRectangle(screenW/2-10, screenH/2-2, 20, 4, WHITE);
        DrawText(TextFormat("Selected block: %d", selected+1), screenW-210, 20, 18, WHITE);
        if (showHelp) {
            DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, 0.28f));
            DrawRectangle(screenW/2-250, screenH/2-115, 500, 230, Color{22,30,44,240});
            DrawText("WELCOME TO BLOCKLANDS", screenW/2-205, screenH/2-82, 25, RAYWHITE);
            DrawText("A tiny, lightweight voxel sandbox prototype.", screenW/2-210, screenH/2-40, 18, LIGHTGRAY);
            DrawText("WASD + mouse to explore. Mine and place blocks.", screenW/2-210, screenH/2-10, 17, LIGHTGRAY);
            DrawText("Press F1 to close this panel.", screenW/2-210, screenH/2+38, 18, YELLOW);
        }
        EndDrawing();
    }

    EnableCursor();
    CloseWindow();
    return 0;
}
