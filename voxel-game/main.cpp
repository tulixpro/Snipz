#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
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
static const char* worldNames[] = {"Green Valley","Flatlands","Highlands","Island"};
static const int paletteCount = 8;

static void RebuildHeightMap(const std::vector<Block>& blocks, int size, std::vector<int>& heights) {
    heights.assign(size * size, 0);
    for (const auto& b : blocks) {
        int x=(int)std::lround(b.p.x), z=(int)std::lround(b.p.z), y=(int)std::lround(b.p.y);
        if (x>=0 && x<size && z>=0 && z<size && y>=0) heights[x*size+z]=std::max(heights[x*size+z],y+1);
    }
}

static Model BuildWorldModel(const std::vector<Block>& blocks, int size) {
    constexpr int MAX_Y=64;
    std::vector<unsigned char> solid(size*size*MAX_Y,0);
    auto index=[&](int x,int y,int z){ return (y*size+z)*size+x; };
    for(const auto& block:blocks) {
        int x=(int)std::lround(block.p.x), y=(int)std::lround(block.p.y), z=(int)std::lround(block.p.z);
        if(x>=0&&x<size&&z>=0&&z<size&&y>=0&&y<MAX_Y) solid[index(x,y,z)]=1;
    }
    auto has=[&](int x,int y,int z){ return x>=0&&x<size&&z>=0&&z<size&&y>=0&&y<MAX_Y&&solid[index(x,y,z)]!=0; };
    std::vector<float> vertices, normals;
    std::vector<unsigned char> colors;
    auto face=[&](Vector3 n, Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color) {
        Vector3 p[6]={a,b,c,a,c,d};
        for(const auto& v:p) {
            vertices.push_back(v.x); vertices.push_back(v.y); vertices.push_back(v.z);
            normals.push_back(n.x); normals.push_back(n.y); normals.push_back(n.z);
            colors.push_back(color.r); colors.push_back(color.g); colors.push_back(color.b); colors.push_back(color.a);
        }
    };
    for(const auto& block:blocks) {
        int x=(int)std::lround(block.p.x), y=(int)std::lround(block.p.y), z=(int)std::lround(block.p.z);
        if(x<0||x>=size||z<0||z>=size||y<0||y>=MAX_Y) continue;
        float x0=x-.5f,x1=x+.5f,y0=y-.5f,y1=y+.5f,z0=z-.5f,z1=z+.5f;
        const Color col=block.c;
        if(!has(x,y+1,z)) face({0,1,0},{x0,y1,z0},{x0,y1,z1},{x1,y1,z1},{x1,y1,z0},col);
        if(!has(x,y-1,z)) face({0,-1,0},{x0,y0,z1},{x0,y0,z0},{x1,y0,z0},{x1,y0,z1},col);
        if(!has(x-1,y,z)) face({-1,0,0},{x0,y0,z0},{x0,y0,z1},{x0,y1,z1},{x0,y1,z0},col);
        if(!has(x+1,y,z)) face({1,0,0},{x1,y0,z1},{x1,y0,z0},{x1,y1,z0},{x1,y1,z1},col);
        if(!has(x,y,z-1)) face({0,0,-1},{x1,y0,z0},{x0,y0,z0},{x0,y1,z0},{x1,y1,z0},col);
        if(!has(x,y,z+1)) face({0,0,1},{x0,y0,z1},{x1,y0,z1},{x1,y1,z1},{x0,y1,z1},col);
    }
    Mesh mesh={};
    mesh.vertexCount=(int)(vertices.size()/3);
    mesh.triangleCount=mesh.vertexCount/3;
    if(mesh.vertexCount==0) return {};
    mesh.vertices=(float*)MemAlloc(vertices.size()*sizeof(float));
    mesh.normals=(float*)MemAlloc(normals.size()*sizeof(float));
    mesh.colors=(unsigned char*)MemAlloc(colors.size());
    std::memcpy(mesh.vertices,vertices.data(),vertices.size()*sizeof(float));
    std::memcpy(mesh.normals,normals.data(),normals.size()*sizeof(float));
    std::memcpy(mesh.colors,colors.data(),colors.size());
    UploadMesh(&mesh,false);
    return LoadModelFromMesh(mesh);
}

static void GenerateWorld(std::vector<Block>& blocks, int size, int type, int seed) {
    blocks.clear();
    blocks.reserve(size * size * 7);
    for (int x = 0; x < size; ++x) {
        for (int z = 0; z < size; ++z) {
            float n = 0.0f;
            for (int ox=-1; ox<=1; ++ox)
                for (int oz=-1; oz<=1; ++oz)
                    n += noise2(x+ox, z+oz, seed);
            n /= 9.0f;
            int h = 4;
            if (type == 0) h = 3 + (int)(n * 7);
            if (type == 1) h = 3;
            if (type == 2) h = 4 + (int)(n * 15);
            if (type == 3) {
                float dx = (x - size/2.0f) / (size*0.48f);
                float dz = (z - size/2.0f) / (size*0.48f);
                float edge = 1.0f - (dx*dx + dz*dz);
                h = edge > 0 ? 2 + (int)(edge * (4 + n*6)) : 1;
            }
            h = std::max(1, h);
            for (int y = 0; y < h; ++y) {
                Color c = palette[1];
                if (y == h-1) c = (type == 3 && h <= 2) ? palette[3] : palette[0];
                if (type == 2 && y == h-1 && h > 13) c = palette[5];
                if (type == 3 && h <= 2 && y == h-1) c = palette[3];
                blocks.push_back({{(float)x,(float)y,(float)z},c});
            }
        }
    }
}

int main() {
    int W=1280, H=720, fpsLimit=120, fullscreen=0, graphicsQuality=1;
    {
        std::ifstream cfg("settings.ini");
        std::string line;
        while(std::getline(cfg,line)) {
            int value=0;
            if(sscanf(line.c_str(),"width=%d",&value)==1) W=std::max(800,std::min(2560,value));
            else if(sscanf(line.c_str(),"height=%d",&value)==1) H=std::max(600,std::min(1600,value));
            else if(sscanf(line.c_str(),"fps=%d",&value)==1) fpsLimit=value;
            else if(sscanf(line.c_str(),"fullscreen=%d",&value)==1) fullscreen=value;
            else if(sscanf(line.c_str(),"graphics=%d",&value)==1) graphicsQuality=Clamp(value,0,2);
        }
    }
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | (fullscreen?FLAG_FULLSCREEN_MODE:0));
    InitWindow(W,H,"BLOCKLANDS");
    if (!IsWindowReady()) return 1;
    SetTargetFPS(fpsLimit<=0?0:fpsLimit);
    if(graphicsQuality==0) { W=960; H=540; SetWindowSize(W,H); }
    else if(graphicsQuality==2) { W=1600; H=900; SetWindowSize(W,H); }

    enum Screen { MAIN_MENU, PLAYING, PAUSED, WORLD_MENU, SETTINGS };
    Screen screen = MAIN_MENU;
    Screen settingsReturn = MAIN_MENU;
    std::vector<Block> blocks;
    std::vector<int> groundHeights;
    int worldSize=64, worldType=0, seed=2026, selected=0;
    bool creative=true, night=false, commandOpen=false;
    char command[128]={0};
    std::string message="Welcome to Blocklands!";
    float messageTime=4.0f;
    int menuFPS = fpsLimit;

    Camera3D camera={};
    camera.position={worldSize/2.0f,12.0f,worldSize/2.0f+18};
    camera.target={worldSize/2.0f,4.0f,worldSize/2.0f};
    camera.up={0,1,0}; camera.fovy=75; camera.projection=CAMERA_PERSPECTIVE;
    Vector3 velocity={0,0,0};
    bool onGround=false;
    float yaw=-PI/2.0f, pitch=-0.38f;

    auto resetCamera = [&]() {
        float sx=worldSize/2.0f;
        float sz=std::min(worldSize/2.0f+8.0f,worldSize-2.0f);
        int gx=Clamp((int)std::lround(sx),0,worldSize-1), gz=Clamp((int)std::lround(sz),0,worldSize-1);
        float floorY=(groundHeights.empty()?4.0f:(float)groundHeights[gx*worldSize+gz])+1.65f;
        camera.position={sx,floorY,sz};
        yaw=-PI/2.0f; pitch=-0.22f;
        Vector3 look={cosf(yaw)*cosf(pitch),sinf(pitch),sinf(yaw)*cosf(pitch)};
        camera.target=Vector3Add(camera.position,look);
        velocity={0,0,0}; onGround=false;
    };
    auto startWorld = [&]() {
        GenerateWorld(blocks,worldSize,worldType,seed);
        RebuildHeightMap(blocks,worldSize,groundHeights);
        resetCamera();
        screen=PLAYING;
        commandOpen=false;
        DisableCursor();
        message="World ready. Explore and build!";
        messageTime=4.0f;
    };
    auto runCommand = [&]() {
        std::string s(command);
        if (s=="/help") message="Commands: /help /day /night /creative /survival /give 1-8 /seed";
        else if (s=="/day") { night=false; message="Daytime enabled."; }
        else if (s=="/night") { night=true; message="Nighttime enabled."; }
        else if (s=="/creative") { creative=true; message="Creative mode enabled."; }
        else if (s=="/survival") { creative=false; message="Survival mode enabled (prototype)."; }
        else if (s=="/seed") message=TextFormat("World seed: %d",seed);
        else if (s.rfind("/give ",0)==0) {
            int v=atoi(s.substr(6).c_str());
            if (v>=1 && v<=8) { selected=v-1; message=TextFormat("Selected %s.",blockNames[selected]); }
            else message="Usage: /give 1-8";
        } else message="Unknown command. Type /help.";
        command[0]='\0'; commandOpen=false; messageTime=4.0f;
    };
    auto button = [&](Rectangle r, const char* label, bool hot=false) {
        Vector2 m=GetMousePosition();
        bool hover=CheckCollisionPointRec(m,r);
        DrawRectangleRounded(r,0.16f,10, hover?Color{67,105,151,255}:(hot?Color{190,139,54,255}:Color{34,48,70,255}));
        DrawRectangleRoundedLines(r,0.16f,10,Color{115,145,178,255});
        int tw=MeasureText(label,22);
        DrawText(label,(int)(r.x+r.width/2-tw/2),(int)(r.y+r.height/2-11),22,RAYWHITE);
        return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    };
    auto drawBackdrop = [&]() {
        ClearBackground(Color{13,22,38,255});
        DrawCircle(GetScreenWidth()-100,110,180,Fade(Color{47,91,139,255},0.18f));
        DrawCircle(90,GetScreenHeight()-70,210,Fade(Color{36,112,91,255},0.13f));
        DrawText("BLOCKLANDS",48,34,18,Color{125,183,224,255});
        DrawLine(48,64,GetScreenWidth()-48,64,Fade(WHITE,0.18f));
    };

    while (!WindowShouldClose()) {
        float dt=std::min(GetFrameTime(),0.05f);
        if (messageTime>0) messageTime-=dt;

        if (screen==PLAYING && !commandOpen) {
            if (IsKeyPressed(KEY_ESCAPE)) { screen=PAUSED; EnableCursor(); }
            if (IsKeyPressed(KEY_F2)) { screen=WORLD_MENU; EnableCursor(); }
            if (IsKeyPressed(KEY_T)) { commandOpen=true; command[0]='\0'; EnableCursor(); }
            if (IsKeyPressed(KEY_F1)) { message="WASD move | Mouse look | Space jump | LMB mine | RMB build"; messageTime=5; }
            if (!commandOpen) {
                Vector2 mouse=GetMouseDelta();
                yaw += mouse.x*0.0025f;
                pitch=Clamp(pitch-mouse.y*0.0025f,-1.45f,1.45f);
                Vector3 look={cosf(yaw)*cosf(pitch),sinf(pitch),sinf(yaw)*cosf(pitch)};
                Vector3 forward=Vector3Normalize({look.x,0,look.z});
                Vector3 right=Vector3Normalize({forward.z,0,-forward.x});
                Vector3 move={0,0,0};
                if(IsKeyDown(KEY_W)) move=Vector3Add(move,forward);
                if(IsKeyDown(KEY_S)) move=Vector3Subtract(move,forward);
                if(IsKeyDown(KEY_D)) move=Vector3Add(move,right);
                if(IsKeyDown(KEY_A)) move=Vector3Subtract(move,right);
                if(Vector3Length(move)>0) move=Vector3Scale(Vector3Normalize(move),(creative?9.0f:5.0f)*dt);
                camera.position=Vector3Add(camera.position,move);
                camera.position.x=Clamp(camera.position.x,0.0f,(float)worldSize-1.0f);
                camera.position.z=Clamp(camera.position.z,0.0f,(float)worldSize-1.0f);
                if(IsKeyPressed(KEY_SPACE) && onGround) { velocity.y=7.0f; onGround=false; }
                velocity.y-=18.0f*dt;
                camera.position.y+=velocity.y*dt;
                int gx=Clamp((int)std::lround(camera.position.x),0,worldSize-1);
                int gz=Clamp((int)std::lround(camera.position.z),0,worldSize-1);
                int safeHeight=groundHeights.empty()?0:groundHeights[gx*worldSize+gz];
                for(int ox=-1;ox<=1;++ox) for(int oz=-1;oz<=1;++oz) {
                    int sx=Clamp(gx+ox,0,worldSize-1), sz=Clamp(gz+oz,0,worldSize-1);
                    if(!groundHeights.empty()) safeHeight=std::max(safeHeight,groundHeights[sx*worldSize+sz]);
                }
                float floorY=(float)safeHeight+1.65f;
                if(camera.position.y<floorY) { camera.position.y=floorY; velocity.y=0; onGround=true; }
                if(camera.position.y<1.65f) { camera.position.y=1.65f; velocity.y=0; onGround=true; }
                camera.target=Vector3Add(camera.position,look);

                if(IsKeyPressed(KEY_ONE)) selected=0;
                if(IsKeyPressed(KEY_TWO)) selected=1;
                if(IsKeyPressed(KEY_THREE)) selected=2;
                if(IsKeyPressed(KEY_FOUR)) selected=3;
                if(IsKeyPressed(KEY_FIVE)) selected=4;
                if(IsKeyPressed(KEY_SIX)) selected=5;
                if(IsKeyPressed(KEY_SEVEN)) selected=6;
                if(IsKeyPressed(KEY_EIGHT)) selected=7;

                if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                    Ray ray=GetMouseRay({GetScreenWidth()/2.0f,GetScreenHeight()/2.0f},camera);
                    int hit=-1; float nearest=1000.0f; Vector3 normal={0,0,0};
                    for(int i=0;i<(int)blocks.size();++i) {
                        BoundingBox box={{blocks[i].p.x-.5f,blocks[i].p.y-.5f,blocks[i].p.z-.5f},{blocks[i].p.x+.5f,blocks[i].p.y+.5f,blocks[i].p.z+.5f}};
                        RayCollision c=GetRayCollisionBox(ray,box);
                        if(c.hit && c.distance<nearest) { nearest=c.distance; hit=i; normal=c.normal; }
                    }
                    if(hit>=0 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { blocks.erase(blocks.begin()+hit); RebuildHeightMap(blocks,worldSize,groundHeights); if(worldModelReady) UnloadModel(worldModel); worldModel=BuildWorldModel(blocks,worldSize); worldModelReady=true; }
                    else if(hit>=0 && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                        Vector3 p=Vector3Add(blocks[hit].p,normal);
                        bool occupied=false;
                        for(const auto& b:blocks) if(b.p.x==p.x&&b.p.y==p.y&&b.p.z==p.z) {occupied=true;break;}
                        if(!occupied && p.y>=0 && p.y<63 && Vector3Distance(p,camera.position)>1.4f) { blocks.push_back({p,palette[selected]}); RebuildHeightMap(blocks,worldSize,groundHeights); if(worldModelReady) UnloadModel(worldModel); worldModel=BuildWorldModel(blocks,worldSize); worldModelReady=true; }
                    }
                }
            }
        } else if (screen==PLAYING && commandOpen) {
            int ch=GetCharPressed();
            while(ch>0) {
                int len=(int)std::strlen(command);
                if(ch>=32 && ch<=125 && len<120) { command[len]=(char)ch; command[len+1]='\0'; }
                ch=GetCharPressed();
            }
            if(IsKeyPressed(KEY_BACKSPACE)) { int len=(int)std::strlen(command); if(len>0) command[len-1]='\0'; }
            if(IsKeyPressed(KEY_ENTER)) { runCommand(); DisableCursor(); }
            if(IsKeyPressed(KEY_ESCAPE)) { commandOpen=false; DisableCursor(); }
        }

        BeginDrawing();
        if(screen==PLAYING) {
            Color sky=night?Color{17,22,48,255}:Color{125,190,235,255};
            ClearBackground(sky);
            BeginMode3D(camera);
            DrawSphere(night?Vector3{worldSize*.78f,24,worldSize*.18f}:Vector3{worldSize*.18f,27,worldSize*.22f},2.4f,night?Color{220,226,245,255}:Color{255,224,142,255});
            DrawPlane({worldSize/2.0f,-.55f,worldSize/2.0f},{(float)worldSize+4,(float)worldSize+4},night?Color{30,55,45,255}:Color{70,130,75,255});
            if(worldModelReady) DrawModel(worldModel,Vector3Zero(),1.0f,WHITE);
            EndMode3D();

            DrawRectangleRounded({14,14,250,78},.12f,8,Fade(Color{8,14,26,255},.84f));
            DrawText("BLOCKLANDS",28,22,25,RAYWHITE);
            DrawFPS(29,55);
            DrawText(TextFormat("%s  |  %s",worldNames[worldType],creative?"CREATIVE":"SURVIVAL"),286,24,18,RAYWHITE);
            DrawText(TextFormat("WORLD %d x %d  |  SEED %d",worldSize,worldSize,seed),286,50,14,Fade(WHITE,.85f));
            DrawRectangleRounded({(float)GetScreenWidth()-210,14,196,64},.14f,8,Fade(Color{8,14,26,255},.8f));
            DrawText(night?"NIGHT":"DAY",GetScreenWidth()-190,24,18,night?SKYBLUE:GOLD);
            DrawText("ESC  PAUSE",GetScreenWidth()-190,49,14,WHITE);
            int cx=GetScreenWidth()/2, cy=GetScreenHeight()/2;
            DrawLine(cx-12,cy,cx-4,cy,WHITE); DrawLine(cx+4,cy,cx+12,cy,WHITE);
            DrawLine(cx,cy-12,cx,cy-4,WHITE); DrawLine(cx,cy+4,cx,cy+12,WHITE);
            int start=(GetScreenWidth()-paletteCount*62)/2, hotY=GetScreenHeight()-76;
            DrawRectangleRounded({(float)start-8,(float)hotY-7,(float)paletteCount*62+16,64},.12f,8,Fade(BLACK,.72f));
            for(int i=0;i<paletteCount;i++) {
                int x=start+i*62;
                DrawRectangleRounded({(float)x,(float)hotY,54,48},.12f,6,i==selected?Color{190,139,54,255}:Color{38,50,68,255});
                DrawRectangle(x+8,hotY+6,38,25,palette[i]);
                DrawText(TextFormat("%d",i+1),x+22,hotY+33,12,WHITE);
            }
            DrawText("WASD MOVE   SPACE JUMP   LMB MINE   RMB PLACE   T COMMANDS   F2 WORLDS",18,GetScreenHeight()-103,13,WHITE);
            if(messageTime>0) {
                DrawRectangleRounded({18,104,(float)GetScreenWidth()-36,34},.12f,6,Fade(BLACK,.48f));
                DrawText(message.c_str(),30,112,15,RAYWHITE);
            }
            if(commandOpen) {
                DrawRectangle(0,0,GetScreenWidth(),GetScreenHeight(),Fade(BLACK,.45f));
                DrawRectangleRounded({90.0f,(float)GetScreenHeight()-125.0f,(float)GetScreenWidth()-180.0f,90.0f},.12f,8,Color{20,29,45,248});
                DrawText("COMMAND  (ENTER TO RUN, ESC TO CLOSE)",112,GetScreenHeight()-112,14,SKYBLUE);
                DrawText(TextFormat(">%s_",command),112,GetScreenHeight()-80,24,WHITE);
            }
        } else {
            drawBackdrop();
            float cx=GetScreenWidth()/2.0f;
            if(screen==MAIN_MENU) {
                DrawText("A WORLD BUILT BY YOU", (int)cx-205,118,25,Color{188,210,232,255});
                DrawText("Explore. Build. Create.",(int)cx-160,153,19,LIGHTGRAY);
                if(button({cx-155,230,310,58},"PLAY",true)) screen=WORLD_MENU;
                if(button({cx-155,302,310,52},"SETTINGS")) { settingsReturn=MAIN_MENU; screen=SETTINGS; }
                if(button({cx-155,368,310,52},"QUIT")) break;
                DrawText("A small voxel sandbox prototype",(int)cx-145,GetScreenHeight()-54,14,Fade(WHITE,.55f));
            } else if(screen==PAUSED) {
                DrawText("PAUSED",(int)cx-85,126,34,WHITE);
                if(button({cx-155,215,310,56},"RESUME",true)) { screen=PLAYING; DisableCursor(); }
                if(button({cx-155,283,310,52},"NEW WORLD")) screen=WORLD_MENU;
                if(button({cx-155,347,310,52},"SETTINGS")) { settingsReturn=PAUSED; screen=SETTINGS; }
                if(button({cx-155,411,310,52},"MAIN MENU")) { screen=MAIN_MENU; EnableCursor(); }
            } else if(screen==WORLD_MENU) {
                DrawText("PLAY  /  WORLD SELECT",(int)cx-205,100,30,WHITE);
                DrawText("Build a custom world or let Blocklands surprise you.",(int)cx-225,137,16,LIGHTGRAY);
                DrawText(TextFormat("WORLD TYPE:  %s",worldNames[worldType]),(int)cx-155,175,20,SKYBLUE);
                if(button({cx-220,212,64,48},"<")) worldType=(worldType+3)%4;
                if(button({cx+156,212,64,48},">")) worldType=(worldType+1)%4;
                DrawText(TextFormat("WORLD SIZE:  %d x %d",worldSize,worldSize),(int)cx-130,285,20,WHITE);
                if(button({cx-220,322,64,48},"-")) worldSize=std::max(32,worldSize-16);
                if(button({cx+156,322,64,48},"+")) worldSize=std::min(96,worldSize+16);
                DrawText(TextFormat("SEED: %d",seed),(int)cx-75,397,18,LIGHTGRAY);
                if(button({cx+85,384,135,44},"RANDOM")) seed=GetRandomValue(1,999999);
                if(button({cx-155,466,310,58},"GENERATE WORLD",true)) startWorld();
                if(button({cx-155,536,310,48},"BACK")) screen=(blocks.empty()?MAIN_MENU:PAUSED);
            } else if(screen==SETTINGS) {
                DrawText("SETTINGS",(int)cx-100,130,32,WHITE);
                DrawText(TextFormat("GRAPHICS: %s",graphicsQuality==0?"LOW":(graphicsQuality==1?"NORMAL":"HIGH")),(int)cx-125,205,22,SKYBLUE);
                if(button({cx-155,245,310,48},"CHANGE GRAPHICS")) { graphicsQuality=(graphicsQuality+1)%3; int nw=graphicsQuality==0?960:(graphicsQuality==1?1280:1600); int nh=graphicsQuality==0?540:(graphicsQuality==1?720:900); SetWindowSize(nw,nh); }
                DrawText(TextFormat("FPS LIMIT: %s",menuFPS<=0?"UNLIMITED":TextFormat("%d",menuFPS)),(int)cx-125,320,22,SKYBLUE);
                if(button({cx-155,365,310,48},"CHANGE FPS LIMIT")) {
                    if(menuFPS==60) menuFPS=120; else if(menuFPS==120) menuFPS=144; else if(menuFPS==144) menuFPS=0; else menuFPS=60;
                    SetTargetFPS(menuFPS<=0?0:menuFPS);
                }
                DrawText("LOW: 960x540  |  NORMAL: 1280x720  |  HIGH: 1600x900",(int)cx-250,420,14,LIGHTGRAY);
                if(button({cx-155,470,310,50},"BACK")) screen=settingsReturn;
            }
        }
        EndDrawing();
    }
    { std::ofstream cfg("settings.ini"); cfg<<"width="<<GetScreenWidth()<<"\nheight="<<GetScreenHeight()<<"\nfps="<<menuFPS<<"\nfullscreen="<<fullscreen<<"\ngraphics="<<graphicsQuality<<"\n"; }
    if(worldModelReady) UnloadModel(worldModel);
    EnableCursor();
    CloseWindow();
    return 0;
}
