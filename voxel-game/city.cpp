#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>
#include <cmath>

enum Tool { ROAD, ZONE_RES, ZONE_COM, ZONE_IND, PARK, POWER, WATER, BULLDOZER };
struct Tile { int tool=-1; int level=0; int growth=0; };
struct Car { Vector3 p; int dx=1,dz=0; float speed=2.2f; Color color=RED; };
static constexpr int N=48;
static Tile city[N][N];
static std::vector<Car> cars;
static int selected=ROAD;
static bool paused=false;
static int money=320000, population=0, happiness=62, month=1, year=1;
static int jobs=0, electricity=0, waterSupply=0, connectedZones=0, servedHomes=0, servedShops=0, servedFactories=0;
static float simTimer=0, carTimer=0, demandR=.7f, demandC=.45f, demandI=.5f;
static Camera3D camera={};
static Vector3 target={N/2.0f,0,N/2.0f};
static float zoom=1.0f;
static const char* toolNames[]={"Road","Residential zone","Commercial zone","Industrial zone","Park","Power plant","Water tower","Bulldozer"};
static Color toolColors[]={{65,68,76,255},{75,178,91,255},{65,145,220,255},{220,157,63,255},{55,170,95,255},{224,91,70,255},{65,190,205,255},{180,180,190,255}};
static bool inBounds(int x,int z){return x>=0&&z>=0&&x<N&&z<N;}
static void saveCity(){std::ofstream f("snipz_city.save",std::ios::binary);if(!f)return;f.write((char*)&money,sizeof money);f.write((char*)&population,sizeof population);f.write((char*)&happiness,sizeof happiness);f.write((char*)&month,sizeof month);f.write((char*)&year,sizeof year);f.write((char*)city,sizeof city);}
static void loadCity(){std::ifstream f("snipz_city.save",std::ios::binary);if(!f)return;f.read((char*)&money,sizeof money);f.read((char*)&population,sizeof population);f.read((char*)&happiness,sizeof happiness);f.read((char*)&month,sizeof month);f.read((char*)&year,sizeof year);f.read((char*)city,sizeof city);}
static bool roadAccess(int x,int z){for(int dz=-1;dz<=1;dz++)for(int dx=-1;dx<=1;dx++)if((dx||dz)&&inBounds(x+dx,z+dz)&&city[x+dx][z+dz].tool==ROAD)return true;return false;}
static int nearbyCount(int x,int z,int tool,int radius){int n=0;for(int a=std::max(0,x-radius);a<=std::min(N-1,x+radius);a++)for(int b=std::max(0,z-radius);b<=std::min(N-1,z+radius);b++)if(city[a][b].tool==tool)n++;return n;}
static void initCamera(){camera.position={N*.5f+34,39,N*.5f+34};camera.target=target;camera.up={0,1,0};camera.fovy=45;camera.projection=CAMERA_PERSPECTIVE;}
static float buildingHeight(Tile t){if(t.tool==ZONE_RES)return 2.2f+t.level*.95f;if(t.tool==ZONE_COM)return 3.0f+t.level*1.35f;if(t.tool==ZONE_IND)return 1.8f+t.level*.35f;if(t.tool==POWER)return 2.5f;if(t.tool==WATER)return 2.0f;return 1.0f;}
static void drawBuilding(int x,int z,Tile t){
 float h=buildingHeight(t);Color c=toolColors[t.tool];Vector3 p={(float)x+.5f,h*.5f,(float)z+.5f};
 if(t.tool==ZONE_RES||t.tool==ZONE_COM){
  Color wall=t.tool==ZONE_RES?Color{190,174,147,255}:Color{105,151,177,255};
  Color roof=t.tool==ZONE_RES?Color{112,69,57,255}:Color{48,62,82,255};
  DrawCube(p,.82f,h,.82f,wall);
  DrawCube({x+.5f,h+.07f,z+.5f},.94f,.16f,.94f,roof);
  int floors=std::max(1,(int)(h/.48f));
  for(int floor=0;floor<floors;floor++) for(int side=0;side<4;side++) for(int w=0;w<2;w++){
   float yy=.34f+floor*.48f, off=(w==0?-.19f:.19f);
   Color glass=t.level>=3?Color{93,184,211,255}:Color{105,163,188,255};
   if(side==0)DrawCube({x+.5f+off,yy,z+.086f},.13f,.19f,.025f,glass);
   if(side==1)DrawCube({x+.5f+off,yy,z+.914f},.13f,.19f,.025f,glass);
   if(side==2)DrawCube({x+.086f,yy,z+.5f+off},.025f,.19f,.13f,glass);
   if(side==3)DrawCube({x+.914f,yy,z+.5f+off},.025f,.19f,.13f,glass);
  }
  DrawCube({x+.5f,.16f,z+.5f},.88f,.12f,.88f,t.tool==ZONE_RES?Color{117,94,76,255}:Color{64,79,91,255});
  if(t.level>=3){DrawCube({x+.5f,h+.22f,z+.5f},.25f,.22f,.25f,roof);DrawCube({x+.5f,h+.36f,z+.5f},.12f,.08f,.12f,WHITE);}
 } else if(t.tool==ZONE_IND){
  DrawCube(p,.86f,h,.86f,{153,147,128,255});DrawCube({x+.5f,h+.12f,z+.5f},.92f,.22f,.92f,{94,99,102,255});
  DrawCylinder({x+.72f,1.55f,z+.28f},.14f,.10f,1.35f,10,{100,103,105,255});
  DrawCylinder({x+.27f,1.20f,z+.72f},.10f,.08f,.85f,10,{120,112,102,255});
  DrawCube({x+.5f,.5f,z+.06f},.32f,.22f,.035f,{226,177,79,255});
 } else if(t.tool==POWER){
  DrawCube(p,.86f,h,.86f,{112,116,124,255});DrawCube({x+.5f,1.8f,z+.5f},.24f,1.5f,.24f,LIGHTGRAY);
  DrawCube({x+.5f,2.6f,z+.5f},1.05f,.14f,.14f,{224,190,92,255});
  DrawCube({x+.5f,2.6f,z+.5f},.14f,.14f,1.05f,{224,190,92,255});
 } else if(t.tool==WATER){
  DrawCylinder({x+.5f,1.3f,z+.5f},.39f,.39f,1.35f,16,{91,163,194,255});
  DrawCylinder({x+.5f,2.05f,z+.5f},.48f,.48f,.20f,16,{190,204,211,255});
  DrawCylinder({x+.5f,.52f,z+.5f},.08f,.08f,.85f,10,{150,163,174,255});
 }
 DrawCubeWires(p,.84f,h,.84f,Fade(BLACK,.18f));
}
static void spawnCar(){
 std::vector<Vector2> roads;for(int x=0;x<N;x++)for(int z=0;z<N;z++)if(city[x][z].tool==ROAD)roads.push_back({(float)x,(float)z});
 if(roads.empty()||cars.size()>90)return;Vector2 q=roads[GetRandomValue(0,(int)roads.size()-1)];
 Car c;c.p={q.x+.5f,.22f,q.y+.5f};c.speed=(float)GetRandomValue(16,28)/10.0f;
 Color palette[]={RED,BLUE,WHITE,YELLOW,{35,38,44,255},{40,150,110,255}};c.color=palette[GetRandomValue(0,5)];
 int dirs[4][2]={{1,0},{-1,0},{0,1},{0,-1}};int d=GetRandomValue(0,3);c.dx=dirs[d][0];c.dz=dirs[d][1];cars.push_back(c);
}
static void updateCars(float dt){
 for(auto &c:cars){
  c.p.x+=c.dx*c.speed*dt;c.p.z+=c.dz*c.speed*dt;
  int x=(int)floorf(c.p.x),z=(int)floorf(c.p.z);
  if(!inBounds(x,z)||city[x][z].tool!=ROAD){
   c.p.x=Clamp(c.p.x,0.1f,N-.1f);c.p.z=Clamp(c.p.z,0.1f,N-.1f);
   x=(int)floorf(c.p.x);z=(int)floorf(c.p.z);
   std::vector<Vector2> options;int dirs[4][2]={{1,0},{-1,0},{0,1},{0,-1}};
   for(auto &d:dirs)if(inBounds(x+d[0],z+d[1])&&city[x+d[0]][z+d[1]].tool==ROAD)options.push_back({(float)d[0],(float)d[1]});
   if(options.empty()){c.p.x=x+.5f;c.p.z=z+.5f;c.speed=0;continue;}
   Vector2 d=options[GetRandomValue(0,(int)options.size()-1)];c.dx=(int)d.x;c.dz=(int)d.y;c.p.x=x+.5f;c.p.z=z+.5f;
  }
 }
 cars.erase(std::remove_if(cars.begin(),cars.end(),[](const Car&c){return c.speed<=0;}),cars.end());
}
static void simulateMonth(){
 int homes=0,shops=0,factories=0,parks=0,plants=0,towers=0,roads=0;
 for(int x=0;x<N;x++)for(int z=0;z<N;z++){Tile &t=city[x][z];if(t.tool==ZONE_RES)homes++;if(t.tool==ZONE_COM)shops++;if(t.tool==ZONE_IND)factories++;if(t.tool==PARK)parks++;if(t.tool==POWER)plants++;if(t.tool==WATER)towers++;if(t.tool==ROAD)roads++;}
 int powerCapacity=plants*18,waterCapacity=towers*16,powerUsed=0,waterUsed=0;
 int connected=0,activeHomes=0,activeShops=0,activeFactories=0,workplaces=0;
 for(int x=0;x<N;x++)for(int z=0;z<N;z++){Tile&t=city[x][z];if(t.tool!=ZONE_RES&&t.tool!=ZONE_COM&&t.tool!=ZONE_IND)continue;
  bool road=roadAccess(x,z),power=nearbyCount(x,z,POWER,9)>0,water=nearbyCount(x,z,WATER,8)>0;
  if(road)connected++;
  bool serviced=road&&power&&water&&powerUsed<powerCapacity&&waterUsed<waterCapacity;
  if(!serviced)continue;
  powerUsed++;waterUsed++;
  if(t.tool==ZONE_RES){activeHomes++;if(t.level<5&&money>1500&&GetRandomValue(0,100)<(int)(demandR*22)){t.level++;money-=260;}}
  if(t.tool==ZONE_COM){activeShops++;workplaces+=t.level+1;if(t.level<5&&GetRandomValue(0,100)<(int)(demandC*18)){t.level++;money-=330;}}
  if(t.tool==ZONE_IND){activeFactories++;workplaces+=2+t.level;if(t.level<4&&GetRandomValue(0,100)<(int)(demandI*18)){t.level++;money-=290;}}
 }
 population=activeHomes*5+activeShops*2; jobs=workplaces;
 connectedZones=connected;servedHomes=activeHomes;servedShops=activeShops;servedFactories=activeFactories;
 electricity=powerCapacity-powerUsed;waterSupply=waterCapacity-waterUsed;
 int servicePenalty=(homes-activeHomes)*2+(shops-activeShops)*2+(factories-activeFactories)*2;
 happiness=Clamp(55+parks*3+std::min(12,roads/8)+std::min(10,plants+towers)-factories/4-servicePenalty,5,99);
 money+=population*3+activeShops*55+activeFactories*38-parks*24-plants*120-towers*85-roads*2;
 demandR=Clamp(.78f-(float)activeHomes/130.0f,0.05f,.95f);
 demandC=Clamp(.55f-(float)activeShops/75.0f,0.05f,.9f);
 demandI=Clamp(.58f-(float)activeFactories/70.0f,0.05f,.9f);
 month++;if(month>12){month=1;year++;}
}
int main(){
 SetConfigFlags(FLAG_MSAA_4X_HINT|FLAG_WINDOW_RESIZABLE);InitWindow(1360,850,"SNIPZ CITY | City Builder");SetTargetFPS(60);initCamera();
 for(int x=0;x<N;x++)for(int z=0;z<N;z++)city[x][z].tool=-1;loadCity();
 while(!WindowShouldClose()){
  float dt=GetFrameTime();
  if(IsKeyPressed(KEY_SPACE))paused=!paused;
  if(IsKeyPressed(KEY_ONE))selected=ROAD;if(IsKeyPressed(KEY_TWO))selected=ZONE_RES;if(IsKeyPressed(KEY_THREE))selected=ZONE_COM;
  if(IsKeyPressed(KEY_FOUR))selected=ZONE_IND;if(IsKeyPressed(KEY_FIVE))selected=PARK;if(IsKeyPressed(KEY_SIX))selected=POWER;if(IsKeyPressed(KEY_SEVEN))selected=WATER;if(IsKeyPressed(KEY_E))selected=BULLDOZER;
  if(IsKeyPressed(KEY_F5))saveCity();if(IsKeyPressed(KEY_F9))loadCity();
  float pan=18.0f*dt/zoom;if(IsKeyDown(KEY_W)||IsKeyDown(KEY_UP))target.z-=pan;if(IsKeyDown(KEY_S)||IsKeyDown(KEY_DOWN))target.z+=pan;
  if(IsKeyDown(KEY_A)||IsKeyDown(KEY_LEFT))target.x-=pan;if(IsKeyDown(KEY_D)||IsKeyDown(KEY_RIGHT))target.x+=pan;
  float wheel=GetMouseWheelMove();if(wheel!=0)zoom=Clamp(zoom-wheel*.12f,.45f,2.4f);
  target.x=Clamp(target.x,0.0f,(float)N);target.z=Clamp(target.z,0.0f,(float)N);camera.target=target;Vector3 off=Vector3Subtract(camera.position,camera.target);camera.position=Vector3Add(camera.target,Vector3Scale(Vector3Normalize(off),35.0f/zoom));
  if(!paused){simTimer+=dt;carTimer+=dt;if(carTimer>1.8f){carTimer=0;spawnCar();}updateCars(dt);if(simTimer>5.0f){simTimer=0;simulateMonth();}}
  if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)&&GetMouseY()>104&&GetMouseY()<GetScreenHeight()-88){
   Ray ray=GetMouseRay(GetMousePosition(),camera);float best=1e9f;int bx=-1,bz=-1;
   for(int x=0;x<N;x++)for(int z=0;z<N;z++){BoundingBox b={{(float)x,-.1f,(float)z},{x+1.0f,.15f,z+1.0f}};RayCollision c=GetRayCollisionBox(ray,b);if(c.hit&&c.distance<best){best=c.distance;bx=x;bz=z;}}
   if(inBounds(bx,bz)){Tile&t=city[bx][bz];int costs[]={180,420,620,560,850,6500,4200,0};int cost=costs[selected];
    if(selected==BULLDOZER){if(t.tool>=0){money+=90;t=Tile{};}}
    else if(money>=cost&&t.tool<0){t.tool=selected;t.level=0;t.growth=0;money-=cost;}
   }
  }
  if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)&&GetMouseY()>104&&GetMouseY()<GetScreenHeight()-88){
   Ray ray=GetMouseRay(GetMousePosition(),camera);float best=1e9f;int bx=-1,bz=-1;
   for(int x=0;x<N;x++)for(int z=0;z<N;z++){BoundingBox b={{(float)x,0,(float)z},{x+1.0f,5.0f,z+1.0f}};RayCollision c=GetRayCollisionBox(ray,b);if(c.hit&&c.distance<best){best=c.distance;bx=x;bz=z;}}
   if(inBounds(bx,bz)&&city[bx][bz].tool>=0){money+=90;city[bx][bz]=Tile{};}
  }
  BeginDrawing();
  ClearBackground({145,190,218,255});
  DrawRectangleGradientV(0,0,GetScreenWidth(),GetScreenHeight(),{116,166,205,255},{211,220,211,255});
  BeginMode3D(camera);
   DrawPlane({N/2.0f,-.12f,N/2.0f},{N,N},{67,105,75,255});
   DrawSphere({N*.78f,24,N*.18f},3.2f,{255,224,151,255});
   for(int x=0;x<N;x++)for(int z=0;z<N;z++){
    Color ground=((x*17+z*31)%7==0)?Color{75,112,78,255}:Color{68,101,72,255};
    DrawCube({x+.5f,-.035f,z+.5f},.99f,.07f,.99f,ground);Tile t=city[x][z];if(t.tool<0)continue;
    if(t.tool==ROAD){DrawCube({x+.5f,.035f,z+.5f},1.0f,.12f,1.0f,{54,59,66,255});
     DrawCube({x+.5f,.105f,z+.5f},.09f,.012f,.34f,{239,211,116,255});
     DrawCube({x+.5f,.112f,z+.08f},.76f,.018f,.035f,{210,213,213,255});
     DrawCube({x+.5f,.112f,z+.92f},.76f,.018f,.035f,{210,213,213,255});}
    else if(t.tool==PARK){DrawCube({x+.5f,.12f,z+.5f},.9f,.22f,.9f,{65,155,78,255});
     DrawCylinder({x+.5f,.62f,z+.5f},.075f,.055f,.88f,7,{105,78,53,255});
     DrawSphere({x+.5f,1.13f,z+.5f},.39f,{40,135,66,255});
     DrawSphere({x+.34f,1.05f,z+.31f},.24f,{54,151,74,255});
     DrawSphere({x+.66f,1.04f,z+.62f},.25f,{47,142,67,255});}
    else if(t.tool==ZONE_RES||t.tool==ZONE_COM||t.tool==ZONE_IND){Color zone=Fade(toolColors[t.tool],.32f);DrawCube({x+.5f,.012f,z+.5f},.96f,.025f,.96f,zone);if(t.level>0)drawBuilding(x,z,t);}
    else drawBuilding(x,z,t);
   }
   for(const auto&c:cars){DrawCube(c.p,.38f,.22f,.62f,c.color);
     DrawCube({c.p.x,c.p.y+.14f,c.p.z},.27f,.13f,.30f,{104,174,205,255});
     DrawCube({c.p.x-.13f,c.p.y-.105f,c.p.z-.20f},.10f,.07f,.08f,{28,30,34,255});
     DrawCube({c.p.x+.13f,c.p.y-.105f,c.p.z+.20f},.10f,.07f,.08f,{28,30,34,255});}
   for(int x=0;x<=N;x+=4)DrawLine3D({(float)x,.02f,0},{(float)x,.02f,(float)N},Fade(WHITE,.1f));
   for(int z=0;z<=N;z+=4)DrawLine3D({0,.02f,(float)z},{(float)N,.02f,(float)z},Fade(WHITE,.1f));
  EndMode3D();
  DrawRectangleGradientV(0,0,GetScreenWidth(),96,{31,48,72,255},{13,22,37,255});
  DrawRectangle(0,94,GetScreenWidth(),2,{83,148,185,255});
  DrawText("SNIPZ CITY",20,12,25,RAYWHITE);DrawText(TextFormat("$ %d",money),220,15,23,{108,220,143,255});
  DrawText(TextFormat("POP %d",population),390,18,18,RAYWHITE);DrawText(TextFormat("JOBS %d",jobs),520,18,18,RAYWHITE);DrawText(TextFormat("HAPPY %d%%",happiness),635,18,18,{255,211,113,255});
  DrawText(TextFormat("POWER %d  WATER %d",electricity,waterSupply),790,18,17,{130,210,235,255});DrawText(TextFormat("YEAR %d / MONTH %d",year,month),1050,18,17,RAYWHITE);
  DrawText(paused?"PAUSED":"SIMULATION RUNNING",20,58,16,paused?ORANGE:LIME);
  DrawText(TextFormat("DEMAND   RES %d%%   COM %d%%   IND %d%%", (int)(demandR*100),(int)(demandC*100),(int)(demandI*100)),260,59,16,RAYWHITE);
  DrawRectangle(0,GetScreenHeight()-82,GetScreenWidth(),82,{15,22,36,248});
  const char* keys[]={"1 ROAD","2 RES ZONE","3 COM ZONE","4 IND ZONE","5 PARK","6 POWER","7 WATER","E BULLDOZE"};
  for(int i=0;i<8;i++){int x=12+i*GetScreenWidth()/8;int w=GetScreenWidth()/8-8;DrawRectangle(x,GetScreenHeight()-67,w,48,selected==i?Color{56,103,145,255}:Color{34,48,68,255});DrawRectangle(x+7,GetScreenHeight()-54,17,17,toolColors[i]);DrawText(keys[i],x+30,GetScreenHeight()-53,14,RAYWHITE);}
  DrawText("BUILD: click empty land | ZONES: place beside a road; they grow when serviced | RMB demolish",18,108,15,Fade(RAYWHITE,.9f));
  DrawText(TextFormat("ACTIVE ZONES  RES %d  COM %d  IND %d   |   POWER LEFT %d  WATER LEFT %d",servedHomes,servedShops,servedFactories,electricity,waterSupply),18,130,14,Fade(RAYWHITE,.9f));
  DrawText("WASD move  |  Wheel zoom  |  SPACE pause  |  F5 save  |  F9 load",18,151,14,Fade(RAYWHITE,.75f));
  if(electricity==0||waterSupply==0)DrawText("TIP: Place a power plant and water tower near road-connected zones; capacity is limited.",18,174,14,ORANGE);
  EndDrawing();
 }
 saveCity();CloseWindow();return 0;
}
