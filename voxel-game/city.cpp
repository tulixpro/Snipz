#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>
#include <cmath>

enum Tool { ROAD, RESIDENTIAL, COMMERCIAL, INDUSTRIAL, PARK, POWER, WATER };
struct Tile { int tool=-1; int level=0; };
static constexpr int N=48;
static Tile city[N][N];
static int selected=ROAD;
static bool paused=false;
static int money=250000, population=0, happiness=62, month=1, year=1;
static float simTimer=0, demandR=0.55f, demandC=0.35f, demandI=0.42f;
static Camera3D camera={};
static Vector3 target={N/2.0f,0,N/2.0f};
static float zoom=1.0f;
static const char* toolNames[]={"Road","Residential","Commercial","Industrial","Park","Power plant","Water tower"};
static Color toolColors[]={{65,68,76,255},{75,178,91,255},{65,145,220,255},{220,157,63,255},{55,170,95,255},{224,91,70,255},{65,190,205,255}};
static bool inBounds(int x,int z){return x>=0&&z>=0&&x<N&&z<N;}
static void saveCity(){std::ofstream f("snipz_city.save",std::ios::binary);if(!f)return;f.write((char*)&money,sizeof money);f.write((char*)&population,sizeof population);f.write((char*)&happiness,sizeof happiness);f.write((char*)city,sizeof city);}
static void loadCity(){std::ifstream f("snipz_city.save",std::ios::binary);if(!f)return;f.read((char*)&money,sizeof money);f.read((char*)&population,sizeof population);f.read((char*)&happiness,sizeof happiness);f.read((char*)city,sizeof city);}
static bool roadAccess(int x,int z){for(int dz=-1;dz<=1;dz++)for(int dx=-1;dx<=1;dx++)if(inBounds(x+dx,z+dz)&&city[x+dx][z+dz].tool==ROAD)return true;return false;}
static void initCamera(){camera.position={N*0.5f+34,39,N*0.5f+34};camera.target=target;camera.up={0,1,0};camera.fovy=45;camera.projection=CAMERA_PERSPECTIVE;}
static void drawBuilding(int x,int z,Tile t){
 float h=1.0f; Color c=toolColors[t.tool];
 if(t.tool==RESIDENTIAL)h=2.2f+t.level*0.75f;
 if(t.tool==COMMERCIAL)h=3.0f+t.level*1.05f;
 if(t.tool==INDUSTRIAL)h=1.6f+t.level*0.35f;
 if(t.tool==POWER)h=2.5f;
 if(t.tool==WATER)h=2.0f;
 Vector3 p={(float)x+0.5f,h*0.5f,(float)z+0.5f};
 DrawCube(p,0.82f,h,0.82f,c);DrawCubeWires(p,0.82f,h,0.82f,Fade(BLACK,0.25f));
 if(t.tool==RESIDENTIAL||t.tool==COMMERCIAL){
  for(float y=0.55f;y<h-0.15f;y+=0.48f){DrawCube({x+0.5f,y,z+0.5f},0.84f,0.045f,0.84f,Fade(SKYBLUE,0.8f));}
  DrawCube({x+0.5f,h+0.04f,z+0.5f},0.9f,0.08f,0.9f,Fade(c,0.8f));
 }
 if(t.tool==INDUSTRIAL){DrawCylinder({x+0.7f,1.3f,z+0.3f},0.12f,0.12f,1.0f,8,GRAY);}
}
int main(){
 SetConfigFlags(FLAG_MSAA_4X_HINT|FLAG_WINDOW_RESIZABLE);InitWindow(1280,800,"SNIPZ CITY | Build your world");SetTargetFPS(60);initCamera();
 for(int x=0;x<N;x++)for(int z=0;z<N;z++)city[x][z].tool=-1;
 loadCity();
 while(!WindowShouldClose()){
  float dt=GetFrameTime();
  if(IsKeyPressed(KEY_SPACE))paused=!paused;
  if(IsKeyPressed(KEY_ONE))selected=ROAD;if(IsKeyPressed(KEY_TWO))selected=RESIDENTIAL;if(IsKeyPressed(KEY_THREE))selected=COMMERCIAL;
  if(IsKeyPressed(KEY_FOUR))selected=INDUSTRIAL;if(IsKeyPressed(KEY_FIVE))selected=PARK;if(IsKeyPressed(KEY_SIX))selected=POWER;if(IsKeyPressed(KEY_SEVEN))selected=WATER;
  if(IsKeyPressed(KEY_F5))saveCity();if(IsKeyPressed(KEY_F9))loadCity();
  float pan=18.0f*dt/zoom;
  if(IsKeyDown(KEY_W)||IsKeyDown(KEY_UP))target.z-=pan;if(IsKeyDown(KEY_S)||IsKeyDown(KEY_DOWN))target.z+=pan;
  if(IsKeyDown(KEY_A)||IsKeyDown(KEY_LEFT))target.x-=pan;if(IsKeyDown(KEY_D)||IsKeyDown(KEY_RIGHT))target.x+=pan;
  float wheel=GetMouseWheelMove();if(wheel!=0){zoom=Clamp(zoom-wheel*0.12f,0.45f,2.4f);}
  camera.target=target;Vector3 offset=Vector3Subtract(camera.position,camera.target);camera.position=Vector3Add(camera.target,Vector3Scale(Vector3Normalize(offset),35.0f/zoom));
  if(!paused){simTimer+=dt;if(simTimer>2.5f){simTimer=0;month++;if(month>12){month=1;year++;}
    int homes=0,shops=0,factories=0,parks=0,utilities=0;
    for(int x=0;x<N;x++)for(int z=0;z<N;z++){Tile &t=city[x][z];if(t.tool==RESIDENTIAL){homes++;if(t.level<5&&roadAccess(x,z)&&money>1000&&GetRandomValue(0,100)<(int)(demandR*16)){t.level++;money-=180;}}
      if(t.tool==COMMERCIAL){shops++;if(t.level<5&&roadAccess(x,z)&&GetRandomValue(0,100)<(int)(demandC*12)){t.level++;money-=260;}}
      if(t.tool==INDUSTRIAL){factories++;if(t.level<4&&roadAccess(x,z)&&GetRandomValue(0,100)<(int)(demandI*12)){t.level++;money-=220;}}
      if(t.tool==PARK)parks++;if(t.tool==POWER||t.tool==WATER)utilities++;
    }
    population=homes* (3+2)+shops*2+factories*2;
    happiness=Clamp(58+parks*2+utilities*2-factories/3,10,98);
    money+=population*2+shops*35+factories*25-parks*18-utilities*45;
    demandR=Clamp(0.65f-(float)homes/180.0f,0.08f,0.95f);demandC=Clamp(0.48f-(float)shops/100.0f,0.08f,0.9f);demandI=Clamp(0.52f-(float)factories/90.0f,0.08f,0.9f);
  }}
  if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)&&GetMouseY()>72&&GetMouseY()<GetScreenHeight()-70){
    Ray ray=GetMouseRay(GetMousePosition(),camera);RayCollision hit={};float best=1e9f;int bx=-1,bz=-1;
    for(int x=0;x<N;x++)for(int z=0;z<N;z++){BoundingBox b={{(float)x,0,(float)z},{x+1.0f,0.15f,z+1.0f}};RayCollision c=GetRayCollisionBox(ray,b);if(c.hit&&c.distance<best){best=c.distance;bx=x;bz=z;}}
    if(inBounds(bx,bz)){Tile &t=city[bx][bz];int cost[]={180,500,700,650,900,4500,2800};if(money>=cost[selected]&&(t.tool<0||t.tool==ROAD)){t.tool=selected;t.level=0;money-=cost[selected];}}
  }
  if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)&&GetMouseY()>72&&GetMouseY()<GetScreenHeight()-70){
    Ray ray=GetMouseRay(GetMousePosition(),camera);RayCollision hit={};float best=1e9f;int bx=-1,bz=-1;
    for(int x=0;x<N;x++)for(int z=0;z<N;z++){BoundingBox b={{(float)x,0,(float)z},{x+1.0f,4.0f,z+1.0f}};RayCollision c=GetRayCollisionBox(ray,b);if(c.hit&&c.distance<best){best=c.distance;bx=x;bz=z;}}
    if(inBounds(bx,bz)&&city[bx][bz].tool>=0){money+=90;city[bx][bz]=Tile{};}
  }
  BeginDrawing();ClearBackground({18,25,37,255});
  BeginMode3D(camera);
   DrawPlane({N/2.0f,-0.12f,N/2.0f},{N,N}, {47,70,66,255});
   for(int x=0;x<N;x++)for(int z=0;z<N;z++){
    DrawCube({x+0.5f,-0.035f,z+0.5f},0.99f,0.07f,0.99f,{67,91,77,255});
    Tile t=city[x][z];if(t.tool<0)continue;
    if(t.tool==ROAD){DrawCube({x+0.5f,0.04f,z+0.5f},1.0f,0.12f,1.0f,{58,62,70,255});DrawCube({x+0.5f,0.105f,z+0.5f},0.08f,0.012f,0.32f,{225,204,115,255});}
    else if(t.tool==PARK){DrawCube({x+0.5f,0.12f,z+0.5f},0.9f,0.22f,0.9f,{55,151,78,255});DrawSphere({x+0.5f,0.65f,z+0.5f},0.28f,{37,112,59,255});}
    else drawBuilding(x,z,t);
   }
   for(int x=0;x<=N;x+=4)DrawLine3D({(float)x,0.02f,0},{(float)x,0.02f,(float)N},Fade(WHITE,0.12f));
   for(int z=0;z<=N;z+=4)DrawLine3D({0,0.02f,(float)z},{(float)N,0.02f,(float)z},Fade(WHITE,0.12f));
  EndMode3D();
  DrawRectangleGradientV(0,0,GetScreenWidth(),72,{25,37,57,255},{15,22,36,255});
  DrawText("SNIPZ CITY",22,13,25,RAYWHITE);DrawText(TextFormat("$ %d",money),245,12,23,{108,220,143,255});
  DrawText(TextFormat("POPULATION  %d",population),410,16,18,RAYWHITE);DrawText(TextFormat("HAPPINESS  %d%%",happiness),620,16,18,{255,211,113,255});
  DrawText(TextFormat("YEAR %d  |  MONTH %d",year,month),850,16,18,RAYWHITE);
  DrawText(paused?"PAUSED":"CITY RUNNING",GetScreenWidth()-190,45,15,paused?ORANGE:LIME);
  DrawRectangle(0,GetScreenHeight()-66,GetScreenWidth(),66,{15,22,36,245});
  const char* keys[]={"1 ROAD","2 HOMES","3 SHOPS","4 INDUSTRY","5 PARK","6 POWER","7 WATER"};
  for(int i=0;i<7;i++){int x=16+i*142;DrawRectangle(x,GetScreenHeight()-54,132,42,selected==i?Color{56,103,145,255}:Color{34,48,68,255});DrawRectangle(x+7,GetScreenHeight()-43,18,18,toolColors[i]);DrawText(keys[i],x+31,GetScreenHeight()-42,14,RAYWHITE);}
  DrawText("LMB build | RMB demolish | WASD move | Wheel zoom | SPACE pause | F5 save | F9 load",18,82,16,Fade(RAYWHITE,0.85f));
  if(money<500)DrawText("LOW FUNDS - zone carefully and grow your city!",18,105,16,ORANGE);
  EndDrawing();
 }
 saveCity();CloseWindow();return 0;
}