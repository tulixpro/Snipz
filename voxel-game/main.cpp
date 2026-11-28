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
#include <unordered_map>
#include <unordered_set>

struct Block { Vector3 p; Color c; };
struct Animal { Vector3 p; Vector3 dir; float turnTimer; int type; };
struct CaveMob { Vector3 p; float attackTimer; int hp; int type; };
struct Villager { Vector3 p; Vector3 dir; float turnTimer; int skin; };


static float noise2(int x, int z, int seed) {
    unsigned int n = (unsigned int)(x * 374761393u + z * 668265263u + seed * 1442695041u);
    n = (n ^ (n >> 13)) * 1274126177u;
    n ^= n >> 16;
    return (n & 0xffff) / 65535.0f;
}

static float smoothNoise2(float x, float z, int seed) {
    auto smooth=[](float t){ return t*t*(3.0f-2.0f*t); };
    int x0=(int)std::floor(x), z0=(int)std::floor(z);
    float tx=smooth(x-x0), tz=smooth(z-z0);
    float a=noise2(x0,z0,seed), b=noise2(x0+1,z0,seed);
    float d=noise2(x0,z0+1,seed), e=noise2(x0+1,z0+1,seed);
    float top=a+(b-a)*tx, bottom=d+(e-d)*tx;
    return top+(bottom-top)*tz;
}

static const Color palette[] = {
    {92,176,88,255}, {130,105,78,255}, {115,125,140,255},
    {190,160,90,255}, {80,145,180,255}, {190,190,195,255},
    {65,120,65,255}, {150,90,65,255}, {235,82,24,255},
    {112,76,45,255}, {232,91,137,255}, {145,93,54,255},
    {42,43,47,255}, {190,132,91,255}, {238,190,55,255}, {70,205,220,255}
};
static const char* blockNames[] = {"Grass","Dirt","Stone","Sand","Water","Snow","Leaves","Brick","Lava","Wood","Flower","Crafting Table","Coal Ore","Iron Ore","Gold Ore","Diamond Ore"};
static const char* worldNames[] = {"Green Valley","Flatlands","Highlands","Island"};
static const int paletteCount = 16;

static void RebuildHeightMap(const std::vector<Block>& blocks, int size, std::vector<int>& heights) {
    heights.assign(size * size, 0);
    for (const auto& b : blocks) {
        bool decoration=(b.c.r==palette[6].r && b.c.g==palette[6].g && b.c.b==palette[6].b) ||
                        (b.c.r==palette[9].r && b.c.g==palette[9].g && b.c.b==palette[9].b) ||
                        (b.c.r==palette[10].r && b.c.g==palette[10].g && b.c.b==palette[10].b);
        int x=(int)std::lround(b.p.x), z=(int)std::lround(b.p.z), y=(int)std::lround(b.p.y);
        if (!decoration && x>=0 && x<size && z>=0 && z<size && y>=0) heights[x*size+z]=std::max(heights[x*size+z],y+1);
    }
}

static Texture2D CreateBlockAtlas() {
    // Clean, original voxel pixel-art tiles: broad color patches, not noisy per-pixel grain.
    Image image=GenImageColor(256,96,WHITE);
    const Color bases[24]={
        {95,166,65,255},{134,96,62,255},{126,130,136,255},{194,174,104,255},
        {62,126,180,255},{218,224,230,255},{66,125,57,255},{150,91,62,255},{224,83,27,255},
        {112,76,45,255},{232,91,137,255},{145,93,54,255},
        {42,43,47,255},{190,132,91,255},{238,190,55,255},{70,205,220,255},
        {126,94,58,255},{116,77,43,255},{150,125,83,255},{94,111,66,255},
        {112,86,57,255},{126,95,60,255},{139,108,70,255},{96,73,49,255}
    };
    for(int type=0;type<24;type++) {
        int ox=(type%8)*32, oy=(type/8)*32;
        for(int y=0;y<32;y++) for(int x=0;x<32;x++) {
            int gx=x/4, gy=y/4;
            float n=noise2(gx+type*37,gy+type*19,4242);
            float shade=0.91f+n*0.17f;
            Color p=bases[type];
            if(type==0) {
                // Grass: calm green surface with a few darker pixel clusters.
                if(n<0.20f) shade=0.78f;
                if(n>0.88f) shade=1.12f;
            } else if(type==1) {
                if(n<0.24f) shade=0.76f;
                if(n>0.86f) shade=1.13f;
            } else if(type==2) {
                if(((gx*7+gy*11)%13)==0) shade=0.58f;
                else if(((gx*3+gy*5)%9)==0) shade=1.22f;
                else if(n<0.20f) shade=0.82f;
            } else if(type==3) {
                if(((gx*5+gy*3)%11)==0) shade=0.72f;
                else if(((gx*7+gy)%13)==0) shade=1.18f;
            } else if(type==4) {
                shade=0.78f+n*0.18f;
                if((y%9==2 || y%9==3) && (x%16)<11) shade=1.30f;
            } else if(type==5) {
                if(((gx*9+gy*5)%17)==0) shade=0.72f;
                else if(n<0.22f) shade=0.91f;
            } else if(type==6) {
                shade=0.74f+n*0.38f;
                if(((gx*5+gy*7)%7)==0) shade=0.48f;
                else if(n>0.82f) shade=1.22f;
            } else if(type==7) {
                // Simple staggered brick courses with clear mortar lines.
                int row=y/8, localY=y%8, shift=(row%2)*8;
                if(localY==0 || ((x+shift)%16==0)) shade=0.48f;
                else if(n>0.82f) shade=1.08f;
            } else if(type==8) {
                shade=0.72f+n*0.26f;
                if(((gx+gy*2)%5)==0 || ((gx*3+gy)%11)==0) { shade=1.45f; p.r=255; p.g=190; p.b=48; }
            } else if(type==9) {
                shade=0.82f+n*0.24f;
                if(gx%4==0 || gx%4==1) shade*=0.72f;
                if((gx==3 || gx==4) && (gy==3 || gy==4)) shade=1.20f;
            } else if(type==10) {
                shade=0.78f+n*0.22f;
                if((gx==3 || gx==4) && (gy==3 || gy==4)) { p.r=255; p.g=220; p.b=75; }
                else if((gx==1 || gx==6) && (gy==3 || gy==4)) shade=1.20f;
            } else if(type==11) {
                shade=0.88f+n*0.16f;
                if(x<3 || x>28 || y<3 || y>28) shade=0.55f;
                else if((x/5+y/5)%2==0) shade=1.18f;
                if(y<8 && (x%8==0)) shade=0.62f;
            } else if(type==16) {
                // Grass side: a crisp green turf cap above a dirt face, like classic voxel blocks.
                if(y<7) { p=Color{91,158,54,255}; if(n>0.72f) shade=1.15f; }
                else { p=Color{126,91,57,255}; shade=0.84f+n*0.28f; }
                if(y>=7 && ((gx*5+gy*3)%11)==0) shade=0.62f;
            } else if(type==17) {
                // Log end grain with visible square-ish growth rings.
                int dx=x-15, dy=y-15; int ring=(int)(sqrtf((float)(dx*dx+dy*dy))/4.0f);
                shade=(ring%2==0)?1.16f:0.78f;
                if(x<3||x>28||y<3||y>28) shade*=0.72f;
            } else if(type>=12) {
                shade=0.70f+n*0.20f;
                if(((gx*7+gy*11+type*3)%13)==0) shade=1.75f;
                if(type==15 && ((gx+gy*3)%7)==0) shade=2.0f;
            }
            p.r=(unsigned char)Clamp(p.r*shade,0,255);
            p.g=(unsigned char)Clamp(p.g*shade,0,255);
            p.b=(unsigned char)Clamp(p.b*shade,0,255);
            ImageDrawPixel(&image,ox+x,oy+y,p);
        }
    }
    Texture2D texture=LoadTextureFromImage(image);
    UnloadImage(image);
    SetTextureFilter(texture,TEXTURE_FILTER_POINT);
    return texture;
}

static Model BuildWorldModel(const std::vector<Block>& blocks, int size, int chunkX=-1, int chunkZ=-1, int chunkSize=16) {
    constexpr int MAX_Y=128;
    std::unordered_set<long long> solid;
    solid.reserve(blocks.size()*2+1);
    auto voxelKey=[&](int x,int y,int z)->long long { return ((long long)y*size+(long long)z)*size+(long long)x; };
    for(const auto& block:blocks) {
        int x=(int)std::lround(block.p.x), y=(int)std::lround(block.p.y), z=(int)std::lround(block.p.z);
        if(x>=0&&x<size&&z>=0&&z<size&&y>=0&&y<MAX_Y) solid.insert(voxelKey(x,y,z));
    }
    auto has=[&](int x,int y,int z){ return x>=0&&x<size&&z>=0&&z<size&&y>=0&&y<MAX_Y&&solid.find(voxelKey(x,y,z))!=solid.end(); };
    std::vector<float> vertices, normals, texcoords;
    std::vector<unsigned char> colors;
    // Reserve a conservative amount up front to reduce allocator churn while meshing.
    size_t estimate=blocks.size();
    vertices.reserve(estimate*30); normals.reserve(estimate*30);
    texcoords.reserve(estimate*12); colors.reserve(estimate*24);
    auto face=[&](Vector3 n, Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color, int type) {
        Vector3 p[6]={a,b,c,a,c,d};
        int tile=32;
        float u0=((type%8)*tile+0.5f)/256.0f, u1=((type%8)*tile+31.5f)/256.0f;
        float v0=((type/8)*tile+0.5f)/96.0f, v1=((type/8)*tile+31.5f)/96.0f;
        const float uv[12]={u0,v0,u0,v1,u1,v1,u0,v0,u1,v1,u1,v0};
        for(int i=0;i<6;i++) {
            const auto& v=p[i];
            vertices.push_back(v.x); vertices.push_back(v.y); vertices.push_back(v.z);
            normals.push_back(n.x); normals.push_back(n.y); normals.push_back(n.z);
            texcoords.push_back(uv[i*2]); texcoords.push_back(uv[i*2+1]);
            colors.push_back(color.r); colors.push_back(color.g); colors.push_back(color.b); colors.push_back(color.a);
        }
    };
    for(const auto& block:blocks) {
        int x=(int)std::lround(block.p.x), y=(int)std::lround(block.p.y), z=(int)std::lround(block.p.z);
        if(x<0||x>=size||z<0||z>=size||y<0||y>=MAX_Y) continue;
        if(chunkX>=0 && (x/chunkSize!=chunkX || z/chunkSize!=chunkZ)) continue;
        float x0=x-.5f,x1=x+.5f,y0=y-.5f,y1=y+.5f,z0=z-.5f,z1=z+.5f;
        int blockType=0;
        for(int t=0;t<paletteCount;t++) if(block.c.r==palette[t].r && block.c.g==palette[t].g && block.c.b==palette[t].b) { blockType=t; break; }
        auto shade=[](Color c,float f){ return Color{(unsigned char)(c.r*f),(unsigned char)(c.g*f),(unsigned char)(c.b*f),c.a}; };
        int topTile=(blockType==9)?17:blockType;
        int sideTile=(blockType==0)?16:blockType;
        if(!has(x,y+1,z)) face({0,1,0},{x0,y1,z0},{x0,y1,z1},{x1,y1,z1},{x1,y1,z0},shade(WHITE,1.12f),topTile);
        if(!has(x,y-1,z)) face({0,-1,0},{x0,y0,z1},{x0,y0,z0},{x1,y0,z0},{x1,y0,z1},shade(WHITE,.55f),topTile);
        if(!has(x-1,y,z)) face({-1,0,0},{x0,y0,z0},{x0,y0,z1},{x0,y1,z1},{x0,y1,z0},shade(WHITE,.78f),sideTile);
        if(!has(x+1,y,z)) face({1,0,0},{x1,y0,z1},{x1,y0,z0},{x1,y1,z0},{x1,y1,z1},shade(WHITE,.86f),sideTile);
        if(!has(x,y,z-1)) face({0,0,-1},{x1,y0,z0},{x0,y0,z0},{x0,y1,z0},{x1,y1,z0},shade(WHITE,.72f),sideTile);
        if(!has(x,y,z+1)) face({0,0,1},{x0,y0,z1},{x1,y0,z1},{x1,y1,z1},{x0,y1,z1},shade(WHITE,.92f),sideTile);
    }
    Mesh mesh={};
    mesh.vertexCount=(int)(vertices.size()/3);
    mesh.triangleCount=mesh.vertexCount/3;
    if(mesh.vertexCount==0) return {};
    mesh.vertices=(float*)MemAlloc(vertices.size()*sizeof(float));
    mesh.normals=(float*)MemAlloc(normals.size()*sizeof(float));
    mesh.colors=(unsigned char*)MemAlloc(colors.size());
    mesh.texcoords=(float*)MemAlloc(texcoords.size()*sizeof(float));
    std::memcpy(mesh.vertices,vertices.data(),vertices.size()*sizeof(float));
    std::memcpy(mesh.normals,normals.data(),normals.size()*sizeof(float));
    std::memcpy(mesh.colors,colors.data(),colors.size());
    std::memcpy(mesh.texcoords,texcoords.data(),texcoords.size()*sizeof(float));
    UploadMesh(&mesh,false);
    Model model=LoadModelFromMesh(mesh);
    if(model.materialCount>0) model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture=CreateBlockAtlas();
    return model;
}

static void GenerateWorld(std::vector<Block>& blocks, int size, int type, int seed) {
    blocks.clear();
    blocks.reserve(size * size * 8);
    for (int x = 0; x < size; ++x) {
        for (int z = 0; z < size; ++z) {
            // Multi-scale interpolated terrain noise avoids the harsh, speckled look.
            float n = smoothNoise2(x*0.035f,z*0.035f,seed)*0.58f
                    + smoothNoise2(x*0.075f,z*0.075f,seed+31)*0.27f
                    + smoothNoise2(x*0.16f,z*0.16f,seed+79)*0.15f;
            int h = 20;
            if (type == 0) h = 18 + (int)(n * 12);
            if (type == 1) h = 18;
            if (type == 2) h = 22 + (int)(n * 20);
            if (type == 3) {
                float dx = (x - size/2.0f) / (size*0.48f);
                float dz = (z - size/2.0f) / (size*0.48f);
                float edge = 1.0f - (dx*dx + dz*dz);
                h = edge > 0 ? 14 + (int)(edge * (5 + n*9)) : 10;
            }
            h = std::max(1, h);
            for (int y = 0; y < h; ++y) {
                // Layered deterministic 3D-style fields create connected tunnels and larger chambers.
                float caveA=smoothNoise2(x*0.082f+y*0.115f,z*0.082f-y*0.091f,seed+y*41);
                float caveB=smoothNoise2(x*0.047f-y*0.073f,z*0.047f+y*0.061f,seed+700+y*23);
                float chamber=smoothNoise2(x*0.031f+y*0.043f,z*0.031f-y*0.057f,seed+1900+y*13);
                float narrow=smoothNoise2(x*0.14f+y*0.19f,z*0.14f-y*0.16f,seed+2900+y*29);
                bool tunnel=(caveA>0.72f && caveB>0.43f);
                bool cavern=(chamber>0.69f && caveB>0.40f && narrow>0.34f);
                if(y>3 && y<h-2 && (tunnel || cavern)) continue;
                Color c = (y < h-4) ? palette[2] : palette[1];
                if(y<h-5) {
                    float ore=noise2(x*17+y*31,z*19-y*13,seed+5100);
                    if(y<9 && ore>0.982f) c=palette[15];
                    else if(y<15 && ore>0.972f) c=palette[14];
                    else if(ore>0.955f) c=palette[13];
                    else if(ore<0.035f) c=palette[12];
                }
                if (y == h-1) c = (type == 3 && h <= 2) ? palette[3] : palette[0];
                if (type == 2 && y == h-1 && h > 13) c = palette[5];
                if (type == 3 && h <= 2 && y == h-1) c = palette[3];
                // Rare glowing-looking lava pockets below the surface.
                if (y==1 && noise2(x+911,z+577,seed+91)>0.986f) c=palette[8];
                if (y==h-1 && noise2(x+177,z+391,seed+173)>0.997f) c=palette[8];
                blocks.push_back({{(float)x,(float)y,(float)z},c});
            }
            // Water fills low coastal and valley tiles as a visible, non-flowing block.
            if(h<18) for(int wy=h;wy<18;wy++) blocks.push_back({{(float)x,(float)wy,(float)z},palette[4]});
            if(type!=3 && h>=5 && noise2(x+733,z+191,seed+57)>0.994f)
                blocks.push_back({{(float)x,(float)h,(float)z},palette[10]});
            if(type!=3 && h>=5 && x>=4 && x<size-4 && z>=4 && z<size-4 &&
               noise2(x+319,z+827,seed+123)>0.9915f) {
                int trunkHeight=3+(int)(noise2(x+17,z+29,seed+5)*2.0f);
                for(int ty=0;ty<trunkHeight;ty++)
                    blocks.push_back({{(float)x,(float)(h+ty),(float)z},palette[9]});
                int crownY=h+trunkHeight-1;
                for(int dy=-1;dy<=2;dy++) {
                    int radius=(dy==2)?1:2;
                    for(int dx=-radius;dx<=radius;dx++) for(int dz=-radius;dz<=radius;dz++) {                        if(dx==0 && dz==0 && dy<1) continue;
                        if(std::abs(dx)+std::abs(dz)>radius+1) continue;
                        if(noise2(x+dx*13+dy*7,z+dz*17-dy*5,seed+211)<0.10f) continue;
                        blocks.push_back({{(float)(x+dx),(float)(crownY+dy),(float)(z+dz)},palette[6]});
                    }
                }
            }
        }
    }
    // Procedural villages: choose broad, reasonably flat clearings after terrain generation.
    std::vector<int> villageHeights;
    RebuildHeightMap(blocks,size,villageHeights);
    auto heightAt=[&](int x,int z)->int {
        if(x<0||x>=size||z<0||z>=size) return 0;
        return villageHeights[x*size+z];
    };
    auto flatSite=[&](int cx,int cz)->bool {
        int lo=999, hi=0;
        for(int dx=-7;dx<=7;dx+=3) for(int dz=-7;dz<=7;dz+=3) {
            int h=heightAt(cx+dx,cz+dz);
            if(h<=18) return false;
            lo=std::min(lo,h); hi=std::max(hi,h);
        }
        return hi-lo<=2;
    };
    std::vector<Vector2> villages;
    int desiredVillages=std::max(1,size/150);
    for(int v=0;v<desiredVillages;v++) {
        int tx=size*(v+1)/(desiredVillages+1), tz=size*(v%2==0?1:2)/3;
        bool found=false;
        for(int radius=0;radius<size/3 && !found;radius+=4) {
            for(int a=-radius;a<=radius && !found;a+=4) for(int b=-radius;b<=radius && !found;b+=4) {
                int cx=tx+a, cz=tz+b;
                if(cx<18||cz<18||cx>=size-18||cz>=size-18) continue;
                if(flatSite(cx,cz)) { villages.push_back({(float)cx,(float)cz}); found=true; }
            }
        }
    }
    for(const Vector2& village:villages) {
        int vx=(int)village.x, vz=(int)village.y;
        int base=heightAt(vx,vz);
        // A compact four-house settlement around a small central square.
        for(int hx=-9;hx<=9;hx+=18) for(int hz=-9;hz<=9;hz+=18) {
            int x0=vx+hx-3, z0=vz+hz-3;
            int floorY=heightAt(vx+hx,vz+hz);
            if(floorY<=18) continue;
            for(int dx=0;dx<7;dx++) for(int dz=0;dz<7;dz++)
                blocks.push_back({{(float)(x0+dx),(float)floorY,(float)(z0+dz)},palette[9]});
            for(int dy=1;dy<=3;dy++) for(int dx=0;dx<7;dx++) for(int dz=0;dz<7;dz++) {
                if(dx!=0&&dx!=6&&dz!=0&&dz!=6) continue;
                bool doorway=(dx==3&&dz==6&&dy<=2);
                bool window=(dy==2&&((dx==0||dx==6)&&(dz==2||dz==4)));
                if(doorway||window) continue;
                Color wall=((dx==0||dx==6)&&(dz==0||dz==6))?palette[9]:palette[7];
                blocks.push_back({{(float)(x0+dx),(float)(floorY+dy),(float)(z0+dz)},wall});
            }
            // Two stepped roof layers make the silhouette read like a voxel house.
            for(int dx=-1;dx<=7;dx++) for(int dz=-1;dz<=7;dz++) {
                if(dx==0||dx==6||dz==0||dz==6)
                    blocks.push_back({{(float)(x0+dx),(float)(floorY+4),(float)(z0+dz)},palette[9]});
            }
            for(int dx=1;dx<=5;dx++) for(int dz=1;dz<=5;dz++)
                if(dx==1||dx==5||dz==1||dz==5)
                    blocks.push_back({{(float)(x0+dx),(float)(floorY+5),(float)(z0+dz)},palette[9]});
            blocks.push_back({{(float)(x0+3),(float)(floorY+1),(float)(z0+2)},palette[11]});
        }
        // Sand paths connect the houses to the village square.
        for(int t=-13;t<=13;t++) for(int w=-1;w<=1;w++) {
            int px=vx+t, pz=vz+w, qx=vx+w, qz=vz+t;
            int py=heightAt(px,pz), qy=heightAt(qx,qz);
            if(py>0) blocks.push_back({{(float)px,(float)(py-1),(float)pz},palette[3]});
            if(qy>0) blocks.push_back({{(float)qx,(float)(qy-1),(float)qz},palette[3]});
        }
        blocks.push_back({{(float)vx,(float)base,(float)vz},palette[11]});
    }

}


static bool SaveWorldFile(const std::vector<Block>& blocks, int size, int type, int seed, bool creative) {
    std::ofstream out("world.save", std::ios::binary);
    if(!out) return false;
    const char magic[8]={'B','L','K','W','O','R','L','D'};
    out.write(magic,8);
    int version=1, count=(int)blocks.size(), mode=creative?1:0;
    out.write((char*)&version,sizeof(version));
    out.write((char*)&size,sizeof(size));
    out.write((char*)&type,sizeof(type));
    out.write((char*)&seed,sizeof(seed));
    out.write((char*)&mode,sizeof(mode));
    out.write((char*)&count,sizeof(count));
    for(const auto& b:blocks) {
        out.write((char*)&b.p,sizeof(b.p));
        out.write((char*)&b.c,sizeof(b.c));
    }
    return out.good();
}

static bool LoadWorldFile(std::vector<Block>& blocks, int& size, int& type, int& seed, bool& creative) {
    std::ifstream in("world.save", std::ios::binary);
    if(!in) return false;
    char magic[8]={};
    in.read(magic,8);
    const char expected[8]={'B','L','K','W','O','R','L','D'};
    if(!in || std::memcmp(magic,expected,8)!=0) return false;
    int version=0, newSize=0, newType=0, newSeed=0, mode=1, count=0;
    in.read((char*)&version,sizeof(version));    in.read((char*)&newSize,sizeof(newSize));    in.read((char*)&newType,sizeof(newType));
    in.read((char*)&newSeed,sizeof(newSeed));
    in.read((char*)&mode,sizeof(mode));    in.read((char*)&count,sizeof(count));
    if(!in || version!=1 || newSize<32 || newSize>512 || newType<0 || newType>3 || count<0 || count>2000000) return false;
    std::vector<Block> loaded;
    loaded.reserve(count);
    for(int i=0;i<count;i++) {
        Block b{};
        in.read((char*)&b.p,sizeof(b.p));
        in.read((char*)&b.c,sizeof(b.c));
        if(!in || !std::isfinite(b.p.x) || !std::isfinite(b.p.y) || !std::isfinite(b.p.z) ||
           b.p.x<0 || b.p.x>=newSize || b.p.z<0 || b.p.z>=newSize || b.p.y<0 || b.p.y>=128) return false;
        loaded.push_back(b);
    }
    blocks.swap(loaded);
    size=newSize; type=newType; seed=newSeed; creative=(mode!=0);
    return true;
}

int main() {
    int W=960, H=540, fpsLimit=60, fullscreen=0, graphicsQuality=0, fov=75, mouseSensitivity=25, controllerSensitivity=24;
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
            else if(sscanf(line.c_str(),"fov=%d",&value)==1) fov=Clamp(value,60,100);
            else if(sscanf(line.c_str(),"mouse_sens=%d",&value)==1) mouseSensitivity=Clamp(value,10,60);
            else if(sscanf(line.c_str(),"controller_sens=%d",&value)==1) controllerSensitivity=Clamp(value,10,60);
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
int worldSize=96, worldType=0, seed=2026, selected=0, inventoryCursor=0;
    int blockCounts[16]={0};
    std::unordered_map<long long,int> blockLookup;
    auto blockKey=[&](int x,int y,int z)->long long { return ((long long)y*worldSize+(long long)z)*worldSize+(long long)x; };
    auto rebuildBlockLookup=[&]() { blockLookup.clear(); blockLookup.reserve(blocks.size()*2); for(int i=0;i<(int)blocks.size();++i) blockLookup[blockKey((int)std::lround(blocks[i].p.x),(int)std::lround(blocks[i].p.y),(int)std::lround(blocks[i].p.z))]=i; };
    bool creative=false, creativeFlying=false, pickaxeEquipped=true, night=false, commandOpen=false, inventoryOpen=false;
    float dayTime=0.25f;
    char command[128]={0};
    std::string message="Welcome to Blocklands!";
    float messageTime=4.0f;
    int menuFPS = fpsLimit;
    int activeGamepad=-1;
    bool previousPadMine=false, previousPadPlace=false;
    float miningTimer=0.0f, miningDuration=0.65f, meleeCooldown=0.0f;
    int miningTarget=-1;

    Camera3D camera={};
    camera.position={worldSize/2.0f,12.0f,worldSize/2.0f+18};
    camera.target={worldSize/2.0f,4.0f,worldSize/2.0f};
    camera.up={0,1,0}; camera.fovy=(float)fov; camera.projection=CAMERA_PERSPECTIVE;
    Vector3 velocity={0,0,0};
    bool onGround=false;
    struct ChunkRender { int x; int z; Model model; };
    std::vector<ChunkRender> renderChunks;
    const int CHUNK_SIZE=16;
    const int RENDER_DISTANCE_CHUNKS=2;
    auto clearChunks = [&]() {
        for(auto& chunk:renderChunks) if(chunk.model.meshCount>0) UnloadModel(chunk.model);
        renderChunks.clear();
    };
    auto updateChunks = [&]() {
        if(screen!=PLAYING || blocks.empty()) return;
        int pcx=Clamp((int)std::floor(camera.position.x/CHUNK_SIZE),0,(worldSize-1)/CHUNK_SIZE);
        int pcz=Clamp((int)std::floor(camera.position.z/CHUNK_SIZE),0,(worldSize-1)/CHUNK_SIZE);
        for(auto it=renderChunks.begin();it!=renderChunks.end();) {
            if(std::abs(it->x-pcx)>RENDER_DISTANCE_CHUNKS || std::abs(it->z-pcz)>RENDER_DISTANCE_CHUNKS) {
                if(it->model.meshCount>0) UnloadModel(it->model);
                it=renderChunks.erase(it);
            } else ++it;
        }
        for(int dz=-RENDER_DISTANCE_CHUNKS;dz<=RENDER_DISTANCE_CHUNKS;dz++) {
            for(int dx=-RENDER_DISTANCE_CHUNKS;dx<=RENDER_DISTANCE_CHUNKS;dx++) {
                int cx=pcx+dx, cz=pcz+dz;
                if(cx<0 || cz<0 || cx*CHUNK_SIZE>=worldSize || cz*CHUNK_SIZE>=worldSize) continue;
                bool loaded=false;
                for(const auto& chunk:renderChunks) if(chunk.x==cx && chunk.z==cz) { loaded=true; break; }
                if(loaded) continue;
                Model model=BuildWorldModel(blocks,worldSize,cx,cz,CHUNK_SIZE);
                if(model.meshCount>0) renderChunks.push_back({cx,cz,model});
                return;
            }
        }
    };
    int health=20, hunger=20, foodCount=5;
    float stamina=100.0f, hungerTimer=0.0f, starvationTimer=0.0f, regenTimer=0.0f;
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
                camera.fovy=(float)fov;
        velocity={0,0,0}; onGround=false;
    };
    std::vector<Animal> animals;
    std::vector<Villager> villagers;
    auto populateVillagers = [&]() {
        villagers.clear();
        if(groundHeights.empty()) return;
        int made=0;
        for(const auto& b:blocks) {
            if(b.c.r!=palette[11].r || b.c.g!=palette[11].g || b.c.b!=palette[11].b) continue;
            int x=(int)std::lround(b.p.x), z=(int)std::lround(b.p.z), y=(int)std::lround(b.p.y);
            if(x<2 || x>=worldSize-2 || z<2 || z>=worldSize-2) continue;
            int surface=groundHeights[x*worldSize+z];
            if(surface<=0 || y!=surface-1 || made>=std::max(2,worldSize/100)) continue;
            int vx=Clamp(x+3,1,worldSize-2), vz=Clamp(z+2,1,worldSize-2);
            int vh=groundHeights[vx*worldSize+vz];
            if(vh<=0) continue;
            int skin=(int)(noise2(x,z,seed+707)*4.0f);
            villagers.push_back({{(float)vx,(float)vh-0.5f,(float)vz},{1,0,0},2.0f+noise2(x,z,seed+17)*3.0f,skin});
            made++;
        }
    };
    auto populateAnimals = [&]() {
        animals.clear();
        if(groundHeights.empty()) return;
        int count=std::max(12,worldSize/10);
        for(int i=0;i<count;i++) {
            int x=8+(int)(noise2(i+seed,31,seed+7)*(worldSize-16));
            int z=8+(int)(noise2(i+seed,79,seed+19)*(worldSize-16));
            int h=groundHeights[x*worldSize+z];
            if(h<19 || h>48) continue;
            int type=(int)(noise2(i+seed,113,seed+29)*4.0f);
            animals.push_back({{(float)x,(float)h+0.55f,(float)z},
                               {noise2(i,7,seed)-0.5f,0,noise2(i,13,seed)-0.5f},
                               noise2(i,23,seed)*4.0f,type});
        }
    };
    std::vector<CaveMob> caveMobs;
    bool caveMobsSpawned=false;
    auto populateCaveMobs = [&]() {
        caveMobs.clear();
        caveMobsSpawned=true;
        if(groundHeights.empty()) return;
        int targetCount=std::min(16,std::max(8,worldSize/48));
        for(int i=0;i<targetCount*18 && (int)caveMobs.size()<targetCount;i++) {
            int x=5+(int)(noise2(i+seed,503,seed+313)*(worldSize-10));
            int z=5+(int)(noise2(i+seed,907,seed+419)*(worldSize-10));
            int top=groundHeights[x*worldSize+z];
            if(top<10) continue;
            for(int y=5;y<top-3;y++) {
                float a=smoothNoise2(x*0.082f+y*0.115f,z*0.082f-y*0.091f,seed+y*41);
                float b=smoothNoise2(x*0.047f-y*0.073f,z*0.047f+y*0.061f,seed+700+y*23);
                float c=smoothNoise2(x*0.031f+y*0.043f,z*0.031f-y*0.057f,seed+1900+y*13);
                float n=smoothNoise2(x*0.14f+y*0.19f,z*0.14f-y*0.16f,seed+2900+y*29);
                bool open=(a>0.72f && b>0.43f) || (c>0.69f && b>0.40f && n>0.34f);
                if(open) {
                    int mobType=noise2(x+31*y,z-17*y,seed+811)>0.68f?1:0;
                    caveMobs.push_back({{(float)x,(float)y+0.55f,(float)z},1.0f,mobType==0?3:2,mobType});
                    break;
                }
            }
        }
    };
    auto startWorld = [&]() {
        caveMobs.clear(); caveMobsSpawned=false;
        creative=false; creativeFlying=false; pickaxeEquipped=true;
        health=20; hunger=20; stamina=100.0f; foodCount=5;
        for(int i=0;i<paletteCount;i++) blockCounts[i]=0;
        GenerateWorld(blocks,worldSize,worldType,seed);
        RebuildHeightMap(blocks,worldSize,groundHeights);
        populateVillagers();
        populateAnimals();
        clearChunks();
        rebuildBlockLookup();
        resetCamera();
        screen=PLAYING;
        commandOpen=false;
        DisableCursor();
        message="World ready. Explore and build!";
        messageTime=4.0f;
    };
    auto saveCurrentWorld = [&]() { return !blocks.empty() && SaveWorldFile(blocks,worldSize,worldType,seed,creative); };
    auto runCommand = [&]() {
        std::string s(command);
        if (s=="/help") message="Commands: /help /day /night /creative /survival /give 1-16 /seed /save /load";
        else if (s=="/day") { dayTime=0.25f; night=false; message="Daytime set. The cycle continues."; }
        else if (s=="/night") { dayTime=0.75f; night=true; message="Nighttime set. The cycle continues."; }
        else if (s=="/creative") { creative=true; creativeFlying=false; message="Creative mode enabled. Press V to fly."; }
        else if (s=="/survival") { creative=false; creativeFlying=false; health=20; hunger=20; stamina=100.0f; foodCount=5; message="Survival mode enabled! Collect blocks to build."; }
        else if (s=="/seed") message=TextFormat("World seed: %d",seed);
        else if (s=="/save") message=SaveWorldFile(blocks,worldSize,worldType,seed,creative)?"World saved.":"Could not save world.";
        else if (s=="/load") {
            if(LoadWorldFile(blocks,worldSize,worldType,seed,creative)) {
                RebuildHeightMap(blocks,worldSize,groundHeights);
                clearChunks();
                rebuildBlockLookup();
                  resetCamera();
                message="Saved world loaded.";
            } else message="No valid save found.";
        }
        else if (s.rfind("/give ",0)==0) {
            int v=atoi(s.substr(6).c_str());
            if (v>=1 && v<=paletteCount) { selected=v-1; message=TextFormat("Selected %s.",blockNames[selected]); }
            else message="Usage: /give 1-16";
        } else message="Unknown command. Type /help.";
        command[0]='\0'; commandOpen=false; messageTime=4.0f;
    };
    int menuFocus=0, menuItemCounter=0;
    auto button = [&](Rectangle r, const char* label, bool hot=false) {
        int item=menuItemCounter++;
        int totalItems=screen==MAIN_MENU?4:(screen==PAUSED?5:(screen==WORLD_MENU?7:7));
        if((activeGamepad>=0) && item==0) {
            if(IsGamepadButtonPressed(activeGamepad,GAMEPAD_BUTTON_LEFT_FACE_DOWN) || IsGamepadButtonPressed(activeGamepad,GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) menuFocus=(menuFocus+1)%totalItems;
            if(IsGamepadButtonPressed(activeGamepad,GAMEPAD_BUTTON_LEFT_FACE_UP) || IsGamepadButtonPressed(activeGamepad,GAMEPAD_BUTTON_RIGHT_FACE_UP)) menuFocus=(menuFocus-1+totalItems)%totalItems;
        }
        bool focused=(item==menuFocus);
        Vector2 m=GetMousePosition();
        bool hover=CheckCollisionPointRec(m,r);
        bool gamepadActivate=focused && (activeGamepad>=0) && IsGamepadButtonPressed(activeGamepad,GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
        DrawRectangleRounded(r,0.16f,10, (hover||focused)?Color{67,105,151,255}:(hot?Color{190,139,54,255}:Color{34,48,70,255}));
        DrawRectangleRoundedLines(r,0.16f,10,focused?GOLD:Color{115,145,178,255});
        int tw=MeasureText(label,22);
        DrawText(label,(int)(r.x+r.width/2-tw/2),(int)(r.y+r.height/2-11),22,RAYWHITE);
        return (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) || gamepadActivate;
    };
    auto drawBackdrop = [&]() {
        int sw=GetScreenWidth(), sh=GetScreenHeight();
        ClearBackground(Color{105,166,211,255});
        DrawCircle(sw-190,125,54,Color{255,226,155,255});
        DrawRectangle(0,sh/2,sw,sh/2,Color{91,151,83,255});
        // Layered blocky hills create a cleaner voxel-style panorama.
        for(int i=0;i<8;i++) {
            int hx=i*(sw/7)-100, hh=95+(int)(noise2(i,5,21)*125);
            DrawRectangle(hx,sh/2-hh,sw/5+40,hh+sh/2, i%2?Color{76,128,85,255}:Color{84,139,91,255});
            DrawRectangle(hx,sh/2-hh,sw/5+40,14,Color{125,174,105,255});
        }
        for(int i=0;i<7;i++) {
            int tx=70+i*(sw/7), ty=sh/2-55-(int)(noise2(i,8,2)*45);
            DrawRectangle(tx+12,ty+28,12,55,Color{105,75,48,255});
            DrawRectangle(tx,ty,36,38,Color{47,111,59,255});
            DrawRectangle(tx+7,ty-9,24,18,Color{63,133,67,255});
        }
        DrawRectangle(0,sh-132,sw,132,Fade(Color{12,22,32,255},.88f));
        DrawText("BLOCKLANDS",48,30,36,WHITE);
        DrawText("BUILD YOUR OWN WORLD",51,70,14,Color{206,224,237,255});
        DrawLine(48,96,sw-48,96,Fade(WHITE,.28f));
    };

    while (!WindowShouldClose()) {
        menuItemCounter=0;
        activeGamepad=-1;
        for(int gp=0;gp<4;gp++) if(IsGamepadAvailable(gp)) { activeGamepad=gp; break; }
        float dt=std::min(GetFrameTime(),0.05f);
        if(screen==PLAYING && !blocks.empty() && animals.empty()) populateAnimals();
        if(screen==PLAYING && !blocks.empty() && villagers.empty()) populateVillagers();
        if(screen==PLAYING && !blocks.empty() && !caveMobsSpawned && !creative) populateCaveMobs();
        if(screen==PLAYING) {
            for(auto& villager:villagers) {
                villager.turnTimer-=dt;
                if(villager.turnTimer<=0.0f) {
                    float a=noise2((int)(villager.p.x*3+villager.p.z),(int)(villager.p.z*5),seed+villager.skin*97+(int)GetTime())*2.0f*PI;
                    villager.dir={cosf(a),0,sinf(a)};
                    villager.turnTimer=2.5f+noise2((int)villager.p.x,(int)villager.p.z,seed+villager.skin*31)*3.5f;
                }
                Vector3 next=villager.p;
                next.x+=villager.dir.x*dt*0.42f; next.z+=villager.dir.z*dt*0.42f;
                int vx=Clamp((int)std::lround(next.x),1,worldSize-2), vz=Clamp((int)std::lround(next.z),1,worldSize-2);
                int vh=groundHeights[vx*worldSize+vz];
                if(vh>=19 && vh<=50) { villager.p.x=next.x; villager.p.z=next.z; villager.p.y=(float)vh-0.5f; }
                else villager.dir=Vector3Scale(villager.dir,-1.0f);
            }
            for(auto& animal:animals) {
                animal.turnTimer-=dt;
                if(animal.turnTimer<=0.0f) {
                    float a=noise2((int)(animal.p.x*3+animal.p.z), (int)(animal.p.z*5), seed+(int)(GetTime()*2))*2.0f*PI;
                    animal.dir={cosf(a),0,sinf(a)};
                    animal.turnTimer=2.0f+noise2((int)animal.p.x,(int)animal.p.z,seed+71)*4.0f;
                }
                animal.p.x+=animal.dir.x*dt*0.8f;
                animal.p.z+=animal.dir.z*dt*0.8f;
                int ax=Clamp((int)animal.p.x,1,worldSize-2), az=Clamp((int)animal.p.z,1,worldSize-2);
                animal.p.y=(float)groundHeights[ax*worldSize+az]+0.55f;
                if(animal.p.y<19.0f || animal.p.y>50.0f) animal.dir=Vector3Scale(animal.dir,-1.0f);
            }
            dayTime+=dt/720.0f; if(dayTime>=1.0f) dayTime-=1.0f; night=(dayTime<0.22f || dayTime>0.78f);
            if(!creative) {
                for(auto it=caveMobs.begin();it!=caveMobs.end();) {
                    Vector3 delta=Vector3Subtract(camera.position,it->p);
                    float dist=Vector3Length(delta);
                    if(dist<20.0f) {
                        if(dist>1.45f) {
                            Vector3 step=Vector3Scale(Vector3Normalize({delta.x,0,delta.z}),dt*1.25f);
                            it->p.x+=step.x; it->p.z+=step.z;
                        } else {
                            it->attackTimer-=dt;
                            if(it->attackTimer<=0.0f) {
                                health=std::max(0,health-2);
                                it->attackTimer=1.35f;
                                message="A cave zombie hit you! -1 heart";
                                messageTime=1.8f;
                            }
                        }
                    }
                    if(it->hp<=0) it=caveMobs.erase(it); else ++it;
                }
            }
        }
        if (messageTime>0) messageTime-=dt;
        meleeCooldown=std::max(0.0f,meleeCooldown-dt);

        // Mode switching stays available even while the inventory is open.
        if (screen==PLAYING && !commandOpen && (IsKeyPressed(KEY_G) || IsKeyPressed(KEY_M))) {
            creative=!creative;
            creativeFlying=false;
            velocity.y=0.0f;
            if(!creative) { health=20; hunger=20; stamina=100.0f; foodCount=5; }
            saveCurrentWorld();
            message=creative?"Creative mode enabled. Press V to fly.":"Survival mode enabled.";
            messageTime=3.0f;
        }
        if (screen==PLAYING && !commandOpen && !inventoryOpen) {
            if(IsKeyPressed(KEY_E) || ((activeGamepad>=0)&&IsGamepadButtonPressed(activeGamepad,GAMEPAD_BUTTON_RIGHT_FACE_UP))) { inventoryOpen=true; inventoryCursor=selected; EnableCursor(); }
        }
        else if (screen==PLAYING && inventoryOpen) {
            if(IsKeyPressed(KEY_E) || IsKeyPressed(KEY_ESCAPE) || ((activeGamepad>=0)&&IsGamepadButtonPressed(activeGamepad,GAMEPAD_BUTTON_RIGHT_FACE_UP))) { inventoryOpen=false; DisableCursor(); }
            if((activeGamepad>=0)) {
                if(IsGamepadButtonPressed(activeGamepad,GAMEPAD_BUTTON_LEFT_FACE_LEFT)) inventoryCursor=std::max(0,inventoryCursor-1);
                if(IsGamepadButtonPressed(activeGamepad,GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) inventoryCursor=std::min(paletteCount-1,inventoryCursor+1);
                if(IsGamepadButtonPressed(activeGamepad,GAMEPAD_BUTTON_LEFT_FACE_UP)) inventoryCursor=std::max(0,inventoryCursor-3);                if(IsGamepadButtonPressed(activeGamepad,GAMEPAD_BUTTON_LEFT_FACE_DOWN)) inventoryCursor=std::min(paletteCount-1,inventoryCursor+3);
                if(IsGamepadButtonPressed(activeGamepad,GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) selected=inventoryCursor;
            }
            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                float panelX=GetScreenWidth()/2.0f-270.0f, panelY=GetScreenHeight()/2.0f-190.0f;
                Rectangle modeButton={panelX+350,panelY+18,190,42};
                if(CheckCollisionPointRec(GetMousePosition(),modeButton)) {
                    creative=!creative;
                    creativeFlying=false;
                    velocity.y=0.0f;
                    if(!creative) { health=20; hunger=20; stamina=100.0f; foodCount=5; }
                    saveCurrentWorld();
                    message=creative?"Creative mode enabled. Press V to fly.":"Survival mode enabled.";
                    messageTime=3.0f;
                } else {
                    for(int i=0;i<27;i++) {
                        Rectangle slot={panelX+28+(i%9)*56.0f,panelY+72+(i/9)*56.0f,48,48};
                        if(CheckCollisionPointRec(GetMousePosition(),slot) && i<paletteCount) { selected=i; inventoryCursor=i; }
                    }
                }
            }
        }
        if (screen==PLAYING && !commandOpen && !inventoryOpen) {
            if (IsKeyPressed(KEY_ESCAPE) || ((activeGamepad>=0)&&IsGamepadButtonPressed(activeGamepad,GAMEPAD_BUTTON_MIDDLE_RIGHT))) { screen=PAUSED; EnableCursor(); }
            if (IsKeyPressed(KEY_F2)) { screen=WORLD_MENU; EnableCursor(); }
            if(!creative && (IsKeyPressed(KEY_Q) || ((activeGamepad>=0)&&IsGamepadButtonPressed(activeGamepad,GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)))) {
                if(foodCount>0 && hunger<20) { foodCount--; hunger=std::min(20,hunger+6); message="You ate an apple. Hunger restored."; messageTime=2.5f; }
                else if(foodCount<=0) { message="No food left!"; messageTime=2.0f; }
            }
            if (IsKeyPressed(KEY_T)) { commandOpen=true; command[0]='\0'; EnableCursor(); }
            if (IsKeyPressed(KEY_F1)) { message="WASD move | G or M switch mode | V creative flight | SPACE jump/up"; messageTime=5; }
            if(IsKeyPressed(KEY_V) && creative) { creativeFlying=!creativeFlying; velocity.y=0.0f; message=creativeFlying?"Flight enabled: SPACE up, SHIFT down.":"Flight disabled."; messageTime=2.5f; }
            if(IsKeyPressed(KEY_P)) { pickaxeEquipped=!pickaxeEquipped; miningTarget=-1; miningTimer=0.0f; message=pickaxeEquipped?"Pickaxe equipped.":"Hand selected."; messageTime=2.0f; }
            if(IsKeyPressed(KEY_C) && !creative) { if(blockCounts[9]>=4) { blockCounts[9]-=4; blockCounts[11]++; message="Crafted 1 crafting table from 4 wood."; } else { message="Need 4 wood to craft a crafting table."; } messageTime=3.0f; }
            if (!commandOpen) {
                Vector2 mouse=GetMouseDelta();
                yaw += mouse.x*(mouseSensitivity/10000.0f);
                pitch=Clamp(pitch-mouse.y*(mouseSensitivity/10000.0f),-1.45f,1.45f);
                Vector3 look={cosf(yaw)*cosf(pitch),sinf(pitch),sinf(yaw)*cosf(pitch)};
                Vector3 forward=Vector3Normalize({look.x,0,look.z});
                Vector3 right=Vector3Normalize({-forward.z,0,forward.x});
                Vector3 move={0,0,0};
                if(IsKeyDown(KEY_W)) move=Vector3Add(move,forward);
                if(IsKeyDown(KEY_S)) move=Vector3Subtract(move,forward);
                if(IsKeyDown(KEY_D)) move=Vector3Add(move,right);
                if(IsKeyDown(KEY_A)) move=Vector3Subtract(move,right);
                if((activeGamepad>=0)) {
                    float sx=GetGamepadAxisMovement(activeGamepad,GAMEPAD_AXIS_LEFT_X);
                    float sy=GetGamepadAxisMovement(activeGamepad,GAMEPAD_AXIS_LEFT_Y);
                    float lookX=GetGamepadAxisMovement(activeGamepad,GAMEPAD_AXIS_RIGHT_X);
                    float lookY=GetGamepadAxisMovement(activeGamepad,GAMEPAD_AXIS_RIGHT_Y);
                    if(fabsf(sx)<0.16f) sx=0;
                    if(fabsf(sy)<0.16f) sy=0;
                    if(fabsf(lookX)<0.16f) lookX=0;
                    if(fabsf(lookY)<0.16f) lookY=0;
                    move=Vector3Add(move,Vector3Scale(right,sx));
                    move=Vector3Add(move,Vector3Scale(forward,-sy));
                    yaw+=lookX*dt*(controllerSensitivity/10.0f);
                    pitch=Clamp(pitch+lookY*dt*(controllerSensitivity/12.0f),-1.45f,1.45f);
                }
                bool sprinting=IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
                if(sprinting && Vector3Length(move)>0 && (creative || stamina>0.5f)) {
                    move=Vector3Scale(Vector3Normalize(move),(creative?14.0f:9.0f)*dt);
                    if(!creative) stamina=std::max(0.0f,stamina-24.0f*dt);
                } else {
                    if(Vector3Length(move)>0) move=Vector3Scale(Vector3Normalize(move),(creative?9.0f:5.0f)*dt);
                    if(!sprinting) stamina=std::min(100.0f,stamina+16.0f*dt);
                }
                // Axis-separated voxel collision: trees, leaves, houses and placed blocks are solid.
                auto blockedAt=[&](Vector3 pos)->bool {
                    float feet=pos.y-2.10f, head=pos.y-0.30f;
                    int minX=(int)std::floor(pos.x-0.28f+0.5f), maxX=(int)std::floor(pos.x+0.28f+0.5f);
                    int minZ=(int)std::floor(pos.z-0.28f+0.5f), maxZ=(int)std::floor(pos.z+0.28f+0.5f);
                    int minY=(int)std::floor(feet+0.5f), maxY=(int)std::floor(head+0.5f);
                    for(int by=minY;by<=maxY;by++) for(int bz=minZ;bz<=maxZ;bz++) for(int bx=minX;bx<=maxX;bx++) {
                        auto it=blockLookup.find(blockKey(bx,by,bz));
                        if(it==blockLookup.end()) continue;
                        const Vector3 bp=blocks[it->second].p;
                        bool overlapX=pos.x+0.28f>bp.x-0.5f && pos.x-0.28f<bp.x+0.5f;
                        bool overlapZ=pos.z+0.28f>bp.z-0.5f && pos.z-0.28f<bp.z+0.5f;
                        bool overlapY=head>bp.y-0.5f+0.02f && feet<bp.y+0.5f-0.02f;
                        if(overlapX && overlapZ && overlapY) return true;
                    }
                    return false;
                };
                Vector3 tryX=camera.position; tryX.x+=move.x;
                if(!blockedAt(tryX)) camera.position.x=tryX.x;
                Vector3 tryZ=camera.position; tryZ.z+=move.z;
                if(!blockedAt(tryZ)) camera.position.z=tryZ.z;
                if((IsKeyPressed(KEY_SPACE) || ((activeGamepad>=0) && IsGamepadButtonPressed(activeGamepad,GAMEPAD_BUTTON_RIGHT_FACE_DOWN))) && onGround) { velocity.y=7.0f; onGround=false; }
                bool wasOnGround=onGround;
                onGround=false;
                if(creative && creativeFlying) {
                    velocity.y=0.0f;
                    float vertical=(IsKeyDown(KEY_SPACE)?1.0f:0.0f)-(IsKeyDown(KEY_LEFT_SHIFT)||IsKeyDown(KEY_RIGHT_SHIFT)?1.0f:0.0f);
                    camera.position.y+=vertical*10.0f*dt;
                } else {
                    velocity.y-=18.0f*dt;
                    float previousY=camera.position.y;
                    camera.position.y+=velocity.y*dt;
                    // Resolve vertical collisions with block tops and ceilings.
                    float oldFeet=previousY-2.10f, newFeet=camera.position.y-2.10f;
                    float oldHead=previousY-0.30f, newHead=camera.position.y-0.30f;
                    int px=(int)std::floor(camera.position.x+0.5f), pz=(int)std::floor(camera.position.z+0.5f);
                    bool blockLanded=false;
                    for(int by=(int)std::floor(std::min(oldFeet,newFeet)-0.5f);by<=(int)std::ceil(std::max(oldHead,newHead)+0.5f);by++) {
                        auto it=blockLookup.find(blockKey(px,by,pz));
                        if(it==blockLookup.end()) continue;
                        const Vector3 bp=blocks[it->second].p;
                        bool horizontal=std::fabs(camera.position.x-bp.x)<0.78f && std::fabs(camera.position.z-bp.z)<0.78f;
                        if(!horizontal) continue;
                        float top=bp.y+0.5f, bottom=bp.y-0.5f;
                        if(velocity.y<=0.0f && oldFeet>=top-0.04f && newFeet<top) {
                            camera.position.y=top+2.10f; velocity.y=0.0f; onGround=true; blockLanded=true;
                        } else if(velocity.y>0.0f && oldHead<=bottom+0.04f && newHead>bottom) {
                            camera.position.y=bottom+0.30f; velocity.y=0.0f;
                        }
                    }
                    // Underground floors are resolved from actual blocks, not surface height.
                    if(blockLanded && !creative && !wasOnGround && velocity.y < -11.0f) {
                        int impact=(int)std::floor((-velocity.y-10.0f)*0.75f);
                        health=std::max(0,health-impact);
                        message=TextFormat("Fall damage! -%d HP",impact);
                        messageTime=2.5f;
                    }
                }
                if(!creative) {
                    hungerTimer+=dt;
                    if(hungerTimer>=35.0f) { hungerTimer=0.0f; hunger=std::max(0,hunger-1); }
                    if(hunger==0) {                        starvationTimer+=dt;                        if(starvationTimer>=3.0f) { starvationTimer=0.0f; health=std::max(0,health-1); }
                    } else starvationTimer=0.0f;
                    if(hunger>=18 && health<20) {
                        regenTimer+=dt;
                        if(regenTimer>=5.0f) { regenTimer=0.0f; health=std::min(20,health+1); }                    } else regenTimer=0.0f;
                    if(stamina<100.0f && !sprinting) stamina=std::min(100.0f,stamina+12.0f*dt);
                    if(health<=0) {
                        health=20; hunger=16; stamina=100.0f;
                        resetCamera();
                        message="You ran out of health. Respawned!";
                        messageTime=4.0f;
                    }
                }
                if(camera.position.y<-100.0f) {
                    if(!creative) { health=std::max(0,health-5); hunger=std::max(0,hunger-1); }
                    resetCamera();
                    message="You fell into the void. Respawned at spawn.";
                    messageTime=4.0f;
                }
                camera.target=Vector3Add(camera.position,look);

                if((activeGamepad>=0)) {
                    if(IsGamepadButtonPressed(activeGamepad,GAMEPAD_BUTTON_LEFT_TRIGGER_1)) selected=(selected+paletteCount-1)%paletteCount;
                    if(IsGamepadButtonPressed(activeGamepad,GAMEPAD_BUTTON_RIGHT_TRIGGER_1)) selected=(selected+1)%paletteCount;
                }
                if(IsKeyPressed(KEY_ONE)) selected=0;
                if(IsKeyPressed(KEY_TWO)) selected=1;
                if(IsKeyPressed(KEY_THREE)) selected=2;
                if(IsKeyPressed(KEY_FOUR)) selected=3;
                if(IsKeyPressed(KEY_FIVE)) selected=4;
                if(IsKeyPressed(KEY_SIX)) selected=5;
                if(IsKeyPressed(KEY_SEVEN)) selected=6;
                if(IsKeyPressed(KEY_EIGHT)) selected=7;
                if(IsKeyPressed(KEY_NINE)) selected=8;
                if(IsKeyPressed(KEY_ZERO)) selected=9;
                float wheel=GetMouseWheelMove();
                if(wheel>0.0f) selected=(selected+paletteCount-1)%paletteCount;
                else if(wheel<0.0f) selected=(selected+1)%paletteCount;

                bool padMine=false, padPlace=false;
                if(activeGamepad>=0) {
                    float rt=GetGamepadAxisMovement(activeGamepad,GAMEPAD_AXIS_RIGHT_TRIGGER);
                    float lt=GetGamepadAxisMovement(activeGamepad,GAMEPAD_AXIS_LEFT_TRIGGER);
                    padMine=IsGamepadButtonDown(activeGamepad,GAMEPAD_BUTTON_RIGHT_TRIGGER_2) || rt>0.35f;
                    padPlace=IsGamepadButtonDown(activeGamepad,GAMEPAD_BUTTON_LEFT_TRIGGER_2) || lt>0.35f;
                }
                bool minePressed=IsMouseButtonDown(MOUSE_BUTTON_LEFT) || padMine;
                bool placePressed=IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) || (padPlace && !previousPadPlace);
                previousPadMine=padMine; previousPadPlace=padPlace;
            if(minePressed || placePressed) {
                    Ray ray=GetMouseRay({GetScreenWidth()/2.0f,GetScreenHeight()/2.0f},camera);
                    int mobHit=-1; float mobNearest=8.0f;
                    if(minePressed && !creative) {
                        for(int i=0;i<(int)caveMobs.size();++i) {
                            Vector3 p=caveMobs[i].p;
                            BoundingBox mobBox={{p.x-0.38f,p.y-0.55f,p.z-0.38f},{p.x+0.38f,p.y+0.85f,p.z+0.38f}};
                            RayCollision mc=GetRayCollisionBox(ray,mobBox);
                            if(mc.hit && mc.distance<=8.0f && mc.distance<mobNearest) { mobNearest=mc.distance; mobHit=i; }
                        }
                    }
                    if(mobHit>=0) {
                        miningTarget=-1; miningTimer=0.0f;
                        if(meleeCooldown<=0.0f) {
                            caveMobs[mobHit].hp--;
                            meleeCooldown=0.48f;
                            message=caveMobs[mobHit].hp<=0?"Zombie defeated!":"Zombie hit!";
                            messageTime=1.1f;
                            if(caveMobs[mobHit].hp<=0) caveMobs.erase(caveMobs.begin()+mobHit);
                        }
                    }
                    int hit=-1; float nearest=1000.0f; Vector3 normal={0,0,0};
                    // Fast voxel ray march: test only cells crossed by the crosshair ray,
                    // instead of checking every block in the entire world.
                    if(mobHit<0) {
                        for(float t=0.0f;t<=8.0f;t+=0.06f) {
                            Vector3 sample=Vector3Add(ray.position,Vector3Scale(ray.direction,t));
                            int bx=(int)std::floor(sample.x+0.5f);
                            int by=(int)std::floor(sample.y+0.5f);
                            int bz=(int)std::floor(sample.z+0.5f);
                            auto found=blockLookup.find(blockKey(bx,by,bz));
                            if(found==blockLookup.end()) continue;
                            int candidate=found->second;
                            if(candidate<0 || candidate>=(int)blocks.size()) continue;
                            BoundingBox box={{blocks[candidate].p.x-.5f,blocks[candidate].p.y-.5f,blocks[candidate].p.z-.5f},{blocks[candidate].p.x+.5f,blocks[candidate].p.y+.5f,blocks[candidate].p.z+.5f}};
                            RayCollision collision=GetRayCollisionBox(ray,box);
                            if(collision.hit && collision.distance<=8.0f && collision.distance<nearest) {
                                nearest=collision.distance; hit=candidate; normal=collision.normal;
                            }
                        }
                    }
                    if(hit>=0 && minePressed) {
                        if(miningTarget!=hit) {
                            miningTarget=hit; miningTimer=0.0f;
                            int bt=0;
                            for(int t=0;t<paletteCount;t++) if(blocks[hit].c.r==palette[t].r && blocks[hit].c.g==palette[t].g && blocks[hit].c.b==palette[t].b) { bt=t; break; }
                            // Material-specific mining times in seconds: stone and rare ores take noticeably longer.
                            const float hardness[16]={0.55f,0.72f,2.25f,0.42f,0.70f,0.38f,0.30f,1.65f,1.25f,1.10f,0.22f,1.30f,1.55f,1.90f,2.65f,3.40f};
                            miningDuration=hardness[bt]*(pickaxeEquipped?0.48f:1.0f);
                        }
                        miningTimer+=dt;
                        if(miningTimer>=miningDuration) {
                            if(!creative) { for(int bt=0;bt<paletteCount;bt++) if(blocks[hit].c.r==palette[bt].r && blocks[hit].c.g==palette[bt].g && blocks[hit].c.b==palette[bt].b) { blockCounts[bt]++; break; } }
                            blocks.erase(blocks.begin()+hit);
                            miningTarget=-1; miningTimer=0.0f;
                            RebuildHeightMap(blocks,worldSize,groundHeights);
                            clearChunks();
                rebuildBlockLookup();
                            
                            
                        }
                    }
                    else if(hit>=0 && placePressed) {
                        miningTarget=-1; miningTimer=0.0f;
                        Vector3 p=Vector3Add(blocks[hit].p,normal);
                        int px=(int)std::lround(p.x), py=(int)std::lround(p.y), pz=(int)std::lround(p.z);
                        bool occupied=blockLookup.find(blockKey(px,py,pz))!=blockLookup.end();
                rebuildBlockLookup();
                        if(!occupied && p.y>=0 && p.y<127 && Vector3Distance(p,camera.position)>1.4f && (creative || blockCounts[selected]>0)) { if(!creative) blockCounts[selected]--; blocks.push_back({p,palette[selected]}); rebuildBlockLookup(); RebuildHeightMap(blocks,worldSize,groundHeights); clearChunks();   }
                    }
                } else { miningTarget=-1; miningTimer=0.0f; }
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

        updateChunks();
        BeginDrawing();
        if(screen==PLAYING) {
            float daylight=Clamp(sinf(dayTime*2.0f*PI)*0.5f+0.5f,0.0f,1.0f);
            Color sky=ColorLerp(Color{8,13,34,255},Color{111,181,232,255},daylight);
            ClearBackground(sky);
            // Layered horizon gradient gives the sky more depth without a costly shader.
            DrawRectangleGradientV(0,0,GetScreenWidth(),GetScreenHeight(),
                ColorLerp(Color{28,39,76,255},Color{115,181,226,255},daylight),
                ColorLerp(Color{9,17,39,255},Color{191,218,237,255},daylight));
            BeginMode3D(camera);
            float sunAngle=dayTime*2.0f*PI;
            Vector3 sunPos=Vector3Add(camera.position,{cosf(sunAngle)*90.0f,sinf(sunAngle)*58.0f,24.0f});
            DrawSphere(sunPos,2.4f,ColorLerp(Color{235,239,255,255},Color{255,224,142,255},daylight));
            if(night) DrawSphere(Vector3Add(camera.position,{-cosf(sunAngle)*82.0f,-sinf(sunAngle)*52.0f,-18.0f}),1.8f,Color{220,228,248,255});
            // Clouds are anchored to world coordinates, so they do not orbit the camera.
            if(daylight>0.18f) {
                for(int cloud=0;cloud<9;cloud++) {
                    float cx=6.0f+noise2(cloud*17+seed,31,seed+177)*(worldSize-12);
                    float cz=6.0f+noise2(cloud*29+seed,73,seed+233)*(worldSize-12);
                    float cy=48.0f+(cloud%3)*3.0f;
                    Color cloudColor=Color{244,249,255,220};
                    DrawCube({cx,cy,cz},8.0f,1.0f,4.0f,cloudColor);
                    DrawCube({cx+2.8f,cy+0.25f,cz},5.2f,0.9f,3.6f,cloudColor);
                    DrawCube({cx-2.6f,cy+0.12f,cz+0.5f},4.6f,0.8f,3.2f,cloudColor);
                }
            }
            
            for(const auto& chunk:renderChunks) DrawModel(chunk.model,Vector3Zero(),1.0f,WHITE);
            // Cave mobs: green zombies and pale skeletons.
            for(const auto& mob:caveMobs) {
                Vector3 p=mob.p;
                Color mobColor=mob.type==0?Color{63,139,78,255}:Color{190,194,181,255};
                DrawCubeV({p.x,p.y+0.42f,p.z},{0.66f,0.82f,0.38f},mobColor);
                DrawCubeV({p.x,p.y+1.02f,p.z},{0.48f,0.48f,0.44f},mob.type==0?Color{77,157,91,255}:Color{220,220,205,255});
                DrawCubeV({p.x-0.13f,p.y+1.08f,p.z-0.225f},{0.09f,0.08f,0.035f},WHITE);
                DrawCubeV({p.x+0.13f,p.y+1.08f,p.z-0.225f},{0.09f,0.08f,0.035f},WHITE);
                DrawCubeV({p.x-0.13f,p.y+1.08f,p.z-0.245f},{0.04f,0.04f,0.02f},RED);
                DrawCubeV({p.x+0.13f,p.y+1.08f,p.z-0.245f},{0.04f,0.04f,0.02f},RED);
                DrawCubeV({p.x-0.43f,p.y+0.45f,p.z},{0.20f,0.62f,0.20f},mobColor);
                DrawCubeV({p.x+0.43f,p.y+0.45f,p.z},{0.20f,0.62f,0.20f},mobColor);
                Color legColor=mob.type==0?Color{48,65,112,255}:Color{105,108,102,255};
                DrawCubeV({p.x-0.16f,p.y+0.08f,p.z},{0.20f,0.36f,0.22f},legColor);
                DrawCubeV({p.x+0.16f,p.y+0.08f,p.z},{0.20f,0.36f,0.22f},legColor);
            }
            // Friendly village NPCs with original voxel clothing and varied skin tones.
            const Color villagerSkin[4]={{194,139,103,255},{224,174,132,255},{151,100,72,255},{181,123,88,255}};
            const Color villagerCoat[4]={{76,111,153,255},{116,75,126,255},{83,126,91,255},{150,99,63,255}};
            for(const auto& v:villagers) {
                Vector3 p=v.p; Color skin=villagerSkin[v.skin%4], coat=villagerCoat[v.skin%4];
                DrawCubeV({p.x,p.y+0.88f,p.z},{0.62f,0.82f,0.38f},coat);
                DrawCubeV({p.x,p.y+1.52f,p.z},{0.52f,0.52f,0.48f},skin);
                DrawCubeV({p.x,p.y+1.43f,p.z-0.255f},{0.20f,0.22f,0.08f},skin);
                DrawCubeV({p.x,p.y+1.54f,p.z-0.27f},{0.055f,0.06f,0.025f},Color{45,38,34,255});
                DrawCubeV({p.x-0.36f,p.y+0.88f,p.z},{0.20f,0.68f,0.20f},coat);
                DrawCubeV({p.x+0.36f,p.y+0.88f,p.z},{0.20f,0.68f,0.20f},coat);
                DrawCubeV({p.x-0.16f,p.y+0.28f,p.z},{0.20f,0.48f,0.22f},Color{55,57,65,255});
                DrawCubeV({p.x+0.16f,p.y+0.28f,p.z},{0.20f,0.48f,0.22f},Color{55,57,65,255});
                DrawCubeV({p.x,p.y+1.51f,p.z-0.31f},{0.12f,0.20f,0.16f},skin);
            }
            // Friendly wandering voxel animals: sheep, cows, pigs and chickens.
            for(const auto& animal:animals) {
                Vector3 p=animal.p;
                Color body=animal.type==0?Color{225,225,215,255}:(animal.type==1?Color{112,78,52,255}:(animal.type==2?Color{224,145,155,255}:Color{245,240,220,255}));
                float scale=animal.type==3?0.48f:1.0f;
                DrawCubeV({p.x,p.y+0.32f*scale,p.z},{0.82f*scale,0.58f*scale,0.48f*scale},body);
                DrawCubeV({p.x+0.43f*scale,p.y+0.48f*scale,p.z},{0.30f*scale,0.34f*scale,0.32f*scale},body);
                DrawCubeV({p.x+0.56f*scale,p.y+0.49f*scale,p.z+0.02f*scale},{0.10f*scale,0.12f*scale,0.12f*scale},Color{35,35,35,255});
                for(int leg=0;leg<4;leg++) {
                    float lx=p.x+((leg<2)?-0.25f:0.25f)*scale;
                    float lz=p.z+((leg%2)?0.15f:-0.15f)*scale;
                    DrawCubeV({lx,p.y+0.10f*scale,lz},{0.13f*scale,0.24f*scale,0.13f*scale},Color{65,55,48,255});
                }
                if(animal.type==1) {
                    DrawCubeV({p.x+0.36f,p.y+0.72f,p.z-0.12f},{0.12f,0.18f,0.12f},Color{235,235,225,255});
                    DrawCubeV({p.x+0.36f,p.y+0.72f,p.z+0.12f},{0.12f,0.18f,0.12f},Color{235,235,225,255});
                }
                if(animal.type==3) DrawCubeV({p.x+0.61f*scale,p.y+0.40f*scale,p.z},{0.12f*scale,0.10f*scale,0.12f},Color{230,75,55,255});
            }
            EndMode3D();

            DrawRectangleRounded({14,14,250,78},.12f,8,Fade(Color{8,14,26,255},.84f));
            DrawText("BLOCKLANDS",28,22,25,RAYWHITE);
            DrawFPS(29,55);
            DrawText(TextFormat("%s  |  %s%s",worldNames[worldType],creative?"CREATIVE":"SURVIVAL",creativeFlying?" | FLY":""),286,24,18,RAYWHITE);
            DrawText(pickaxeEquipped?"TOOL: PICKAXE (P)":"TOOL: HAND (P)",286,70,13,GOLD);
            if(!creative) {
                DrawRectangleRounded({14,100,270,104},.12f,8,Fade(Color{8,14,26,255},.84f));
                DrawText("HEALTH",28,108,13,Color{255,145,155,255});
                for(int heart=0;heart<10;heart++) {
                    int hx=30+heart*22, hy=130;
                    Color hc=(health>=heart*2+2)?Color{235,62,82,255}:((health==heart*2+1)?Color{235,137,91,255}:Color{75,55,67,255});
                    DrawCircle(hx+5,hy+5,5,hc); DrawCircle(hx+13,hy+5,5,hc);
                    DrawTriangle({(float)hx,(float)hy+7},{(float)hx+18,(float)hy+7},{(float)hx+9,(float)hy+19},hc);
                }
                DrawText(TextFormat("HUNGER  %d / 20   |   APPLES %d",hunger,foodCount),28,154,14,Color{255,205,120,255});
                DrawRectangle(28,174,230,8,Color{65,52,35,255});
                DrawRectangle(28,174,(int)(230.0f*hunger/20.0f),8,Color{215,151,60,255});
                DrawText(TextFormat("STAMINA  %d%%", (int)stamina),28,187,11,SKYBLUE);
            }
            DrawText(TextFormat("WORLD %d x %d  |  SEED %d",worldSize,worldSize,seed),286,50,14,Fade(WHITE,.85f));
            DrawRectangleRounded({(float)GetScreenWidth()-210,14,196,64},.14f,8,Fade(Color{8,14,26,255},.8f));
            DrawText(TextFormat("%s  %02d:%02d",night?"NIGHT":"DAY",(int)(dayTime*24.0f),(int)(fmodf(dayTime*1440.0f,60.0f))),GetScreenWidth()-190,24,18,night?SKYBLUE:GOLD);
            DrawText("ESC  PAUSE",GetScreenWidth()-190,49,14,WHITE);
            if(!creative) DrawText(TextFormat("CAVE MOBS: %d  |  ZOMBIES + SKELETONS", (int)caveMobs.size()),GetScreenWidth()-330,88,13,Color{145,225,155,255});
            int cx=GetScreenWidth()/2, cy=GetScreenHeight()/2;
            DrawCircle(cx,cy,3.2f,Fade(BLACK,.55f));
            DrawLine(cx-13,cy+1,cx-5,cy+1,Fade(BLACK,.55f)); DrawLine(cx+5,cy+1,cx+13,cy+1,Fade(BLACK,.55f));
            DrawLine(cx+1,cy-13,cx+1,cy-5,Fade(BLACK,.55f)); DrawLine(cx+1,cy+5,cx+1,cy+13,Fade(BLACK,.55f));
            DrawLine(cx-12,cy,cx-4,cy,Color{250,250,255,245}); DrawLine(cx+4,cy,cx+12,cy,Color{250,250,255,245});
            DrawLine(cx,cy-12,cx,cy-4,Color{250,250,255,245}); DrawLine(cx,cy+4,cx,cy+12,Color{250,250,255,245});
            if(miningTimer>0.0f) {
                float progress=Clamp(miningTimer/miningDuration,0.0f,1.0f);
                DrawRectangle(cx-38,cy+22,76,8,Fade(BLACK,.78f));
                DrawRectangle(cx-38,cy+22,(int)(76.0f*progress),8,Color{255,205,92,255});
                int crackStage=(int)(progress*6.0f);
                for(int k=0;k<crackStage;k++) {
                    int ox=(k%3)*9-9, oy=(k/3)*4;
                    DrawLine(cx+ox,cy+33+oy,cx+ox+5,cy+37+oy,Color{35,35,38,220});
                }            }
            int slotW=44, slotGap=4, hotWidth=paletteCount*(slotW+slotGap)-slotGap;
            int start=(GetScreenWidth()-hotWidth)/2, hotY=GetScreenHeight()-76;
            DrawRectangleRounded({(float)start-5,(float)hotY-7,(float)hotWidth+10,64},.12f,8,Fade(BLACK,.72f));
            for(int i=0;i<paletteCount;i++) {
                int x=start+i*(slotW+slotGap);
                DrawRectangleRounded({(float)x,(float)hotY,(float)slotW,48},.12f,5,i==selected?Color{190,139,54,255}:Color{38,50,68,255});
                DrawRectangle(x+7,hotY+6,30,25,palette[i]);
                DrawText(creative?"INF":TextFormat("%d",blockCounts[i]),x+7,hotY+33,9,WHITE);
            }
            DrawText((activeGamepad>=0)?"PAD: LS MOVE | RS LOOK | A JUMP | RT MINE | LT PLACE | Y INVENTORY":"WASD MOVE | SHIFT SPRINT/DOWN | SPACE JUMP/UP | P PICKAXE | G/M MODE | V FLY | LMB MINE | RMB PLACE",18,GetScreenHeight()-103,13,WHITE);
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
            if(inventoryOpen) {
                float panelX=GetScreenWidth()/2.0f-270.0f, panelY=GetScreenHeight()/2.0f-190.0f;
                DrawRectangle(0,0,GetScreenWidth(),GetScreenHeight(),Fade(BLACK,.58f));
                DrawRectangleRounded({panelX,panelY,540,380},.06f,12,Color{24,34,49,250});
                DrawRectangleRoundedLines({panelX,panelY,540,380},.06f,12,Color{112,151,190,255});
                DrawText("INVENTORY", (int)panelX+28,(int)panelY+20,24,WHITE);
                Rectangle modeButton={panelX+350,panelY+18,190,42};
                DrawRectangleRounded(modeButton,.15f,8,creative?Color{59,126,87,255}:Color{145,91,55,255});
                DrawText(creative?"SWITCH TO SURVIVAL":"SWITCH TO CREATIVE",(int)modeButton.x+9,(int)modeButton.y+13,13,WHITE);
                for(int i=0;i<27;i++) {
                    Rectangle slot={panelX+28+(i%9)*56.0f,panelY+72+(i/9)*56.0f,48,48};
                    DrawRectangleRounded(slot,.10f,5,i==selected?Color{179,132,57,255}:(i==inventoryCursor?Color{74,108,148,255}:Color{48,61,79,255}));
                    if(i<paletteCount) {
                        DrawRectangle((int)slot.x+9,(int)slot.y+7,30,25,palette[i]);
                        DrawText(blockNames[i],(int)slot.x+1,(int)slot.y+34,8,WHITE);
                        if(!creative) DrawText(TextFormat("%d",blockCounts[i]),(int)slot.x+34,(int)slot.y+34,9,GOLD);
                    }
                }
                DrawText("Click a block to select  |  E or ESC to close",(int)panelX+28,(int)panelY+342,13,Color{198,213,229,255});
            }
        } else {
            drawBackdrop();
            float cx=GetScreenWidth()/2.0f;
            if(screen==MAIN_MENU) {
                const float panelX=cx-230, panelY=82;
                DrawRectangleRounded({panelX,panelY,460,470},.045f,16,Fade(Color{7,15,25,255},.88f));
                DrawRectangleRoundedLines({panelX,panelY,460,470},.045f,16,Fade(Color{92,176,220,255},.42f));
                DrawRectangleRounded({panelX+22,panelY+22,5,92},.5f,8,Color{63,184,255,255});
                DrawText("BLOCKLANDS",(int)panelX+48,(int)panelY+26,38,WHITE);
                DrawText("A WORLD OF YOUR OWN",(int)panelX+50,(int)panelY+72,14,Color{117,198,235,255});
                DrawLine((int)panelX+28,(int)panelY+126,(int)panelX+432,(int)panelY+126,Fade(WHITE,.16f));
                DrawText("Explore. Build. Create.",(int)panelX+30,(int)panelY+145,19,LIGHTGRAY);
                DrawText("Your worlds are saved on this PC.",(int)panelX+30,(int)panelY+174,14,Color{166,188,207,255});
                if(button({cx-175,panelY+218,350,54},"CREATE NEW WORLD",true)) screen=WORLD_MENU;
                if(button({cx-175,panelY+284,350,48},"CONTINUE WORLD")) {
                    if(LoadWorldFile(blocks,worldSize,worldType,seed,creative)) {
                        RebuildHeightMap(blocks,worldSize,groundHeights);
                        clearChunks();
                rebuildBlockLookup();
                        
                        
                        resetCamera();
                        screen=PLAYING; commandOpen=false; inventoryOpen=false; DisableCursor();
                        message="Saved world loaded."; messageTime=3.0f;
                    } else { message="No saved world yet - create one first."; messageTime=3.0f; }
                }
                if(button({cx-175,panelY+344,168,44},"SETTINGS")) { settingsReturn=MAIN_MENU; screen=SETTINGS; }
                if(button({cx+7,panelY+344,168,44},"QUIT")) break;
                DrawText("WASD  /  MOUSE  /  CONTROLLER",(int)panelX+83,(int)panelY+423,13,Fade(WHITE,.66f));
            } else if(screen==PAUSED) {
                DrawText("PAUSED",(int)cx-85,126,34,WHITE);
                if(button({cx-155,215,310,56},"RESUME",true)) { screen=PLAYING; DisableCursor(); }
                if(button({cx-155,283,310,52},"NEW WORLD")) { saveCurrentWorld(); screen=WORLD_MENU; }
                if(button({cx-155,347,310,52},"SETTINGS")) { settingsReturn=PAUSED; screen=SETTINGS; }
                if(button({cx-155,411,310,52},"MAIN MENU")) { saveCurrentWorld(); screen=MAIN_MENU; EnableCursor(); }
                if(button({cx-155,475,310,52},creative?"SWITCH TO SURVIVAL":"SWITCH TO CREATIVE",true)) {
                    creative=!creative;
                    creativeFlying=false;
                    velocity.y=0.0f;
                    if(!creative) { health=20; hunger=20; stamina=100.0f; foodCount=5; }
                    saveCurrentWorld();
                    message=creative?"Creative mode enabled. Press V to fly.":"Survival mode enabled.";
                    messageTime=3.0f;
                }
            } else if(screen==WORLD_MENU) {
                DrawText("PLAY  /  WORLD SELECT",(int)cx-205,100,30,WHITE);
                DrawText("Build a custom world or let Blocklands surprise you.",(int)cx-225,137,16,LIGHTGRAY);
                DrawText(TextFormat("WORLD TYPE:  %s",worldNames[worldType]),(int)cx-155,175,20,SKYBLUE);
                if(button({cx-220,212,64,48},"<")) worldType=(worldType+3)%4;
                if(button({cx+156,212,64,48},">")) worldType=(worldType+1)%4;
                DrawText(TextFormat("WORLD SIZE:  %d x %d",worldSize,worldSize),(int)cx-130,285,20,WHITE);
                if(button({cx-220,322,64,48},"-")) worldSize=std::max(32,worldSize-16);
                if(button({cx+156,322,64,48},"+")) worldSize=std::min(512,worldSize+16);
                DrawText(TextFormat("SEED: %d",seed),(int)cx-75,397,18,LIGHTGRAY);
                if(button({cx+85,384,135,44},"RANDOM")) seed=GetRandomValue(1,999999);
                if(button({cx-155,466,310,58},"GENERATE WORLD",true)) startWorld();
                if(button({cx-155,536,310,48},"BACK")) screen=(blocks.empty()?MAIN_MENU:PAUSED);
            } else if(screen==SETTINGS) {
                DrawText("SETTINGS",(int)cx-100,90,30,WHITE);
                DrawText(TextFormat("GRAPHICS: %s",graphicsQuality==0?"LOW":(graphicsQuality==1?"NORMAL":"HIGH")),(int)cx-310,155,17,SKYBLUE);
                if(button({cx-310,182,285,42},"CHANGE QUALITY")) { graphicsQuality=(graphicsQuality+1)%3; int nw=graphicsQuality==0?960:(graphicsQuality==1?1280:1600); int nh=graphicsQuality==0?540:(graphicsQuality==1?720:900); SetWindowSize(nw,nh); }
                DrawText(TextFormat("FPS LIMIT: %s",menuFPS<=0?"UNLIMITED":TextFormat("%d",menuFPS)),(int)cx+25,155,17,SKYBLUE);
                if(button({cx+25,182,285,42},"CHANGE FPS")) {
                    if(menuFPS==60) menuFPS=120; else if(menuFPS==120) menuFPS=144; else if(menuFPS==144) menuFPS=0; else menuFPS=60;
                    SetTargetFPS(menuFPS<=0?0:menuFPS);
                }
                DrawText(TextFormat("FIELD OF VIEW: %d",fov),(int)cx-310,252,17,SKYBLUE);
                if(button({cx-310,279,285,42},"CHANGE FOV")) { fov+=10; if(fov>100) fov=60; camera.fovy=(float)fov; }
                DrawText(TextFormat("MOUSE SENSITIVITY: %d",mouseSensitivity),(int)cx+25,252,17,SKYBLUE);
                if(button({cx+25,279,285,42},"CHANGE MOUSE")) { mouseSensitivity+=5; if(mouseSensitivity>60) mouseSensitivity=10; }
                DrawText(TextFormat("CONTROLLER SENSITIVITY: %d",controllerSensitivity),(int)cx-310,349,17,SKYBLUE);
                if(button({cx-310,376,285,42},"CHANGE CONTROLLER")) { controllerSensitivity+=5; if(controllerSensitivity>60) controllerSensitivity=10; }
                DrawText(TextFormat("FULLSCREEN: %s",fullscreen?"ON":"OFF"),(int)cx+25,349,17,SKYBLUE);
                if(button({cx+25,376,285,42},"TOGGLE FULLSCREEN")) { ToggleFullscreen(); fullscreen=!fullscreen; }
                DrawText("LOW 960x540 | NORMAL 1280x720 | HIGH 1600x900",(int)cx-190,455,14,LIGHTGRAY);
                if(button({cx-155,495,310,48},"BACK")) screen=settingsReturn;
            }
        }
        EndDrawing();
    }
    saveCurrentWorld();
    { std::ofstream cfg("settings.ini"); cfg<<"width="<<GetScreenWidth()<<"\nheight="<<GetScreenHeight()<<"\nfps="<<menuFPS<<"\nfullscreen="<<fullscreen<<"\ngraphics="<<graphicsQuality<<"\nfov="<<fov<<"\nmouse_sens="<<mouseSensitivity<<"\ncontroller_sens="<<controllerSensitivity<<"\n"; }
    clearChunks();
    EnableCursor();
    CloseWindow();
    return 0;
}