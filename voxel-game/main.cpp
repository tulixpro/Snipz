#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <algorithm>
#include <cmath>

struct Block { Vector3 p; Color c; };

static float noise2(int x, int z, int seed) {
    unsigned int n = (unsigned int)(x * 374761393u + z * 668265263u + seed * 1442695041u);
    n = (n ^ (n >> 13)) * 1274126177u;
    n ^= n >> 16;
    return (n & 0xffff) / 65535.0f;
}

static const Color palette[] = {
    {92,176,88,255}, {130,105,78,255}, {115,125,140,255},
    {190,160,90,255}, {80,145,180,255}, {190,190,195,255},
    {65,120,65,255}, {150,90,65,255}
};
static const char* blockNames[] = {"Grass","Dirt","Stone","Sand","Water","Snow","Leaves","Brick"};
static const char* worldNames[] = {"Rolling Plains","Superflat","Mountain Peaks","Island"};
static const int paletteCount = 8;

static void GenerateWorld(std::vector<Block>& blocks, int size, int type, int seed) {
    blocks.clear();
    for (int x = 0; x < size; ++x) {
        for (int z = 0; z < size; ++z) {
            float n = noise2(x, z, seed);
            int h = 4;
            if (type == 0) h = 3 + (int)(n * 5);
            if (type == 1) h = 3;
            if (type == 2) h = 3 + (int)(n * 13);
            if (type == 3) {
                float dx = (x - size/2.0f) / (size*0.48f);
                float dz = (z - size/2.0f) / (size*0.48f);
                float edge = 1.0f - (dx*dx + dz*dz);
                h = edge > 0 ? 2 + (int)(edge * (3 + n*5)) : 1;
            }
            h = std::max(1, h);
            for (int y = 0; y < h; ++y) {
                Color c = palette[1];
                if (y == h-1) c = (type == 3 && h <= 2) ? palette[3] : palette[0];
                if (type == 2 && y == h-1 && h > 12) c = palette[5];
                if (type == 3 && h <= 2 && y == h-1) c = palette[3];
                blocks.push_back({{(float)x,(float)y,(float)z},c});
            }
        }
    }
}

int main() {
    int W=1280, H=720, fpsLimit=120, fullscreen=0;
    {
        std::ifstream cfg("settings.ini");
        std::string line;
        while(std::getline(cfg,line)) {
            int value=0;
            if(sscanf(line.c_str(),"width=%d",&value)==1) W=std::max(800,std::min(2560,value));
            else if(sscanf(line.c_str(),"height=%d",&value)==1) H=std::max(600,std::min(1600,value));
            else if(sscanf(line.c_str(),"fps=%d",&value)==1) fpsLimit=value;
            else if(sscanf(line.c_str(),"fullscreen=%d",&value)==1) fullscreen=value;
        }
    }
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | (fullscreen?FLAG_FULLSCREEN_MODE:0));
    InitWindow(W,H,"BLOCKLANDS - Voxel Sandbox");
    SetTargetFPS(fpsLimit<=0?0:fpsLimit);
    DisableCursor();

    std::vector<Block> blocks;
    int worldSize=40, worldType=0, seed=2026, selected=0;
    GenerateWorld(blocks,worldSize,worldType,seed);

    Camera3D camera={};
    camera.position={worldSize/2.0f,10.0f,worldSize/2.0f+12};
    camera.target={worldSize/2.0f,3.0f,worldSize/2.0f};
    camera.up={0,1,0}; camera.fovy=75; camera.projection=CAMERA_PERSPECTIVE;

    Vector3 velocity={0,0,0};
    bool onGround=false, commandOpen=false, creatorOpen=false, creative=true, night=false;
    bool showHelp=true;
    char command[128]={0};
    std::string message="Welcome! Press T for commands or F2 for World Creator.";
    float messageTime=8.0f;

    auto runCommand = [&]() {
        std::string s(command);
        if (s=="/help") message="Commands: /help /day /night /creative /survival /give 1-8 /seed";
        else if (s=="/day") { night=false; message="Time set to day."; }
        else if (s=="/night") { night=true; message="Time set to night."; }
        else if (s=="/creative") { creative=true; message="Creative mode enabled."; }
        else if (s=="/survival") { creative=false; message="Survival mode enabled (prototype)."; }
        else if (s=="/seed") message=TextFormat("World seed: %d",seed);
        else if (s.rfind("/give ",0)==0) {
            int v=atoi(s.substr(6).c_str());
            if (v>=1 && v<=8) { selected=v-1; message=TextFormat("Selected %s.",blockNames[selected]); }
            else message="Usage: /give 1-8";
        } else message="Unknown command. Type /help.";
        command[0]='\0'; commandOpen=false; messageTime=5.0f;
    };

    while (!WindowShouldClose()) {
        float dt=GetFrameTime();
        if (messageTime>0) messageTime-=dt;

        if (IsKeyPressed(KEY_F2) && !commandOpen) { creatorOpen=!creatorOpen; if(creatorOpen) EnableCursor(); else DisableCursor(); }
        if (IsKeyPressed(KEY_T) && !creatorOpen) { commandOpen=!commandOpen; if(commandOpen) { EnableCursor(); command[0]='\0'; } else DisableCursor(); }
        if (IsKeyPressed(KEY_F1)) showHelp=!showHelp;

        if (creatorOpen) {
            if (IsKeyPressed(KEY_LEFT)) worldType=(worldType+3)%4;
            if (IsKeyPressed(KEY_RIGHT)) worldType=(worldType+1)%4;
            if (IsKeyPressed(KEY_MINUS)) worldSize=std::max(24,worldSize-8);
            if (IsKeyPressed(KEY_EQUAL)) worldSize=std::min(80,worldSize+8);
            if (IsKeyPressed(KEY_R)) seed=GetRandomValue(1,999999);
            if (IsKeyPressed(KEY_ENTER)) {
                GenerateWorld(blocks,worldSize,worldType,seed);
                camera.position={(float)worldSize/2,10,(float)worldSize/2+12};
                camera.target={(float)worldSize/2,3,(float)worldSize/2};
                creatorOpen=false; DisableCursor();
                message="New world generated! Have fun."; messageTime=5;
            }
        } else if (commandOpen) {
            int ch=GetCharPressed();
            while(ch>0) {
                int len=(int)std::strlen(command);
                if(ch>=32 && ch<=125 && len<120) { command[len]=(char)ch; command[len+1]='\0'; }
                ch=GetCharPressed();
            }
            if(IsKeyPressed(KEY_BACKSPACE)) { int len=(int)std::strlen(command); if(len>0) command[len-1]='\0'; }
            if(IsKeyPressed(KEY_ENTER)) runCommand();
            if(IsKeyPressed(KEY_ESCAPE)) { commandOpen=false; DisableCursor(); }
        } else {
            // Use one movement system only; UpdateCamera would also apply WASD movement.
            static float yaw = -PI/2.0f;
            static float pitch = -0.50f;
            Vector2 mouse = GetMouseDelta();
            yaw += mouse.x * 0.0030f;
            pitch = Clamp(pitch - mouse.y * 0.0030f, -1.50f, 1.50f);
            Vector3 look = {cosf(yaw)*cosf(pitch), sinf(pitch), sinf(yaw)*cosf(pitch)};
            Vector3 f=Vector3Normalize({look.x,0,look.z});
            Vector3 r=Vector3Normalize({f.z,0,-f.x});
            Vector3 move={0,0,0};
            if(IsKeyDown(KEY_W)) move=Vector3Add(move,f);
            if(IsKeyDown(KEY_S)) move=Vector3Subtract(move,f);
            if(IsKeyDown(KEY_D)) move=Vector3Add(move,r);
            if(IsKeyDown(KEY_A)) move=Vector3Subtract(move,r);
            if(Vector3Length(move)>0) move=Vector3Scale(Vector3Normalize(move),creative?8.0f*dt:5.0f*dt);
            camera.position=Vector3Add(camera.position,move);
            camera.target=Vector3Add(camera.position,look);

            if(IsKeyDown(KEY_SPACE)&&onGround) { velocity.y=7; onGround=false; }
            velocity.y-=18*dt;
            camera.position.y+=velocity.y*dt; camera.target.y+=velocity.y*dt;
            if(camera.position.y<2) { camera.position.y=2; camera.target.y=2; velocity.y=0; onGround=true; }

            if(IsKeyPressed(KEY_ONE)) selected=0;
            if(IsKeyPressed(KEY_TWO)) selected=1;
            if(IsKeyPressed(KEY_THREE)) selected=2;
            if(IsKeyPressed(KEY_FOUR)) selected=3;
            if(IsKeyPressed(KEY_FIVE)) selected=4;
            if(IsKeyPressed(KEY_SIX)) selected=5;
            if(IsKeyPressed(KEY_SEVEN)) selected=6;
            if(IsKeyPressed(KEY_EIGHT)) selected=7;

            // Only raycast when the player actually clicks a block.
            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                Ray ray=GetMouseRay({GetScreenWidth()/2.0f,GetScreenHeight()/2.0f},camera);
                int hit=-1; float nearest=1000; Vector3 normal={0,0,0};
                for(int i=0;i<(int)blocks.size();++i) {
                    BoundingBox box={{blocks[i].p.x-.5f,blocks[i].p.y-.5f,blocks[i].p.z-.5f},
                                     {blocks[i].p.x+.5f,blocks[i].p.y+.5f,blocks[i].p.z+.5f}};
                    RayCollision c=GetRayCollisionBox(ray,box);
                    if(c.hit && c.distance<nearest) { nearest=c.distance; hit=i; normal=c.normal; }
                }
                if(hit>=0 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) blocks.erase(blocks.begin()+hit);
                if(hit>=0 && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                    Vector3 p=Vector3Add(blocks[hit].p,normal);
                    bool occupied=false;
                    for(const auto& b:blocks) if(b.p.x==p.x&&b.p.y==p.y&&b.p.z==p.z) {occupied=true;break;}
                    if(!occupied && p.y>=0 && Vector3Distance(p,camera.position)>1.3f)
                        blocks.push_back({p,palette[selected]});
                }
            }
            if(IsKeyPressed(KEY_ESCAPE)) { EnableCursor(); message="Cursor released. Press F2 for world creator."; messageTime=3; }
        }

        Color sky=night?Color{17,22,48,255}:Color{125,190,235,255};
        BeginDrawing();
        ClearBackground(sky);
        if(!creatorOpen) {
            BeginMode3D(camera);
            DrawPlane({worldSize/2.0f,-.55f,worldSize/2.0f},{(float)worldSize+4,(float)worldSize+4},night?Color{30,55,45,255}:Color{70,130,75,255});
            for(const auto& b:blocks) DrawCube(b.p,1,1,1,b.c);
            EndMode3D();

            DrawRectangle(14,14,230,72,Fade(BLACK,.38f));
            DrawText("BLOCKLANDS",26,22,25,RAYWHITE);
            DrawFPS(27,54);
            DrawText(TextFormat("%s | %s",worldNames[worldType],creative?"CREATIVE":"SURVIVAL"),260,24,18,RAYWHITE);
            DrawText(TextFormat("Seed: %d",seed),260,49,16,RAYWHITE);
            DrawRectangle(GetScreenWidth()/2-1,GetScreenHeight()/2-10,2,20,WHITE);
            DrawRectangle(GetScreenWidth()/2-10,GetScreenHeight()/2-1,20,2,WHITE);

            int barW=paletteCount*68, start=(GetScreenWidth()-barW)/2;
            DrawRectangle(start-8,GetScreenHeight()-88,barW+16,70,Fade(BLACK,.55f));
            for(int i=0;i<paletteCount;i++) {
                int x=start+i*68;
                DrawRectangle(x,GetScreenHeight()-78,58,48,palette[i]);
                DrawRectangleLines(x,GetScreenHeight()-78,58,48,i==selected?YELLOW:Fade(WHITE,.55f));
                DrawText(TextFormat("%d",i+1),x+23,GetScreenHeight()-72,14,BLACK);
            }
            DrawText("WASD move  |  Mouse look  |  Space jump  |  LMB mine  |  RMB place",18,GetScreenHeight()-112,17,RAYWHITE);
            DrawText("T Commands  |  F2 World Creator  |  F1 Help",18,GetScreenHeight()-24,16,RAYWHITE);
        }

        if(commandOpen) {
            DrawRectangle(0,0,GetScreenWidth(),GetScreenHeight(),Fade(BLACK,.35f));
            DrawRectangle(110,GetScreenHeight()-112,GetScreenWidth()-220,76,Color{20,27,42,245});
            DrawText("COMMAND",130,GetScreenHeight()-99,15,SKYBLUE);
            DrawText(TextFormat(">%s_",command),130,GetScreenHeight()-72,23,WHITE);
            DrawText("Enter = run    Esc = close    Try /help",130,GetScreenHeight()-35,14,LIGHTGRAY);
        }
        if(creatorOpen) {
            DrawRectangle(0,0,GetScreenWidth(),GetScreenHeight(),Color{12,18,32,255});
            DrawText("WORLD CREATOR",GetScreenWidth()/2-190,70,38,RAYWHITE);
            DrawText("Create a fresh world and choose your landscape.",GetScreenWidth()/2-230,120,18,LIGHTGRAY);
            DrawRectangle(GetScreenWidth()/2-260,190,520,110,Color{30,43,65,255});
            DrawText("WORLD TYPE",GetScreenWidth()/2-225,205,16,SKYBLUE);
            DrawText(TextFormat("<   %s   >",worldNames[worldType]),GetScreenWidth()/2-175,242,28,WHITE);
            DrawRectangle(GetScreenWidth()/2-260,325,520,90,Color{30,43,65,255});
            DrawText("WORLD SIZE",GetScreenWidth()/2-225,340,16,SKYBLUE);
            DrawText(TextFormat("%d x %d blocks   (- / +)",worldSize,worldSize),GetScreenWidth()/2-175,372,24,WHITE);
            DrawRectangle(GetScreenWidth()/2-260,440,520,72,Color{30,43,65,255});
            DrawText(TextFormat("SEED: %d   (R = random seed)",seed),GetScreenWidth()/2-225,463,20,WHITE);
            DrawText("LEFT / RIGHT: type    - / +: size    R: random seed",GetScreenWidth()/2-250,550,16,LIGHTGRAY);
            DrawText("ENTER: CREATE WORLD       F2: BACK",GetScreenWidth()/2-230,600,20,GOLD);
        }
        if(showHelp && !creatorOpen && !commandOpen) {
            DrawRectangle(0,0,GetScreenWidth(),GetScreenHeight(),Fade(BLACK,.45f));
            DrawRectangle(GetScreenWidth()/2-260,GetScreenHeight()/2-145,520,290,Color{20,29,45,245});
            DrawText("WELCOME TO BLOCKLANDS",GetScreenWidth()/2-210,GetScreenHeight()/2-112,25,WHITE);
            DrawText("Explore, build and shape your own voxel world.",GetScreenWidth()/2-220,GetScreenHeight()/2-68,17,LIGHTGRAY);
            DrawText("WASD move | Mouse look | Space jump",GetScreenWidth()/2-205,GetScreenHeight()/2-25,17,WHITE);
            DrawText("Left click mine | Right click place",GetScreenWidth()/2-205,GetScreenHeight()/2+5,17,WHITE);
            DrawText("T commands | F2 world creator | 1-8 blocks",GetScreenWidth()/2-205,GetScreenHeight()/2+35,17,WHITE);
            DrawText("Press F1 to start playing",GetScreenWidth()/2-150,GetScreenHeight()/2+88,19,GOLD);
        }
        if(messageTime>0 && !creatorOpen && !commandOpen) {
            DrawRectangle(18,100,GetScreenWidth()-36,34,Fade(BLACK,.45f));
            DrawText(message.c_str(),30,108,16,RAYWHITE);
        }
        EndDrawing();
    }
    EnableCursor();
    CloseWindow();
    return 0;
}
