#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <string>
#include <cstring>

static const COLORREF BG=RGB(12,18,31), PANEL=RGB(23,33,52), PANEL2=RGB(30,45,68), TEXT=RGB(235,242,252), MUTED=RGB(155,174,199), ACCENT=RGB(91,160,255), GREEN=RGB(91,205,151);
static HFONT titleFont, subFont, bodyFont, smallFont;
static std::string status="Wahle ein Spiel aus und starte direkt.";
static RECT blockCard={52,230,980,470}, cityCard={52,500,980,740};
static bool hit(POINT p,RECT r){return p.x>=r.left&&p.x<=r.right&&p.y>=r.top&&p.y<=r.bottom;}
static void launchGame(HWND hwnd,const char* exe,const char* name){
 char path[MAX_PATH]={0};GetModuleFileNameA(nullptr,path,MAX_PATH);char* slash=strrchr(path,'\\');if(slash)*slash=0;
 std::string full=std::string(path)+"\\"+exe;
 if(GetFileAttributesA(full.c_str())==INVALID_FILE_ATTRIBUTES){status=std::string(name)+" fehlt im Launcher-Ordner.";InvalidateRect(hwnd,nullptr,FALSE);return;}
 HINSTANCE r=ShellExecuteA(hwnd,"open",full.c_str(),nullptr,path,SW_SHOWNORMAL);
 status=(INT_PTR)r<=32?"Spiel konnte nicht gestartet werden.":std::string(name)+" wird gestartet...";
 InvalidateRect(hwnd,nullptr,FALSE);
}
static void text(HDC dc,const char* s,int x,int y,HFONT f,COLORREF color){SetBkMode(dc,TRANSPARENT);SetTextColor(dc,color);SelectObject(dc,f);TextOutA(dc,x,y,s,(int)strlen(s));}
static void card(HDC dc,RECT r,COLORREF edge,const char* tag,const char* name,const char* desc,const char* button){
 HBRUSH b=CreateSolidBrush(PANEL);HPEN p=CreatePen(PS_SOLID,2,edge);HGDIOBJ ob=SelectObject(dc,b),op=SelectObject(dc,p);
 RoundRect(dc,r.left,r.top,r.right,r.bottom,18,18);SelectObject(dc,ob);SelectObject(dc,op);DeleteObject(b);DeleteObject(p);
 text(dc,tag,r.left+34,r.top+28,smallFont,edge);text(dc,name,r.left+34,r.top+76,titleFont,TEXT);text(dc,desc,r.left+34,r.top+150,bodyFont,MUTED);
 // Original mini-scene artwork gives each game card a clear visual identity.
 RECT art={r.right-390,r.top+18,r.right-28,r.bottom-78};
 HBRUSH ab=CreateSolidBrush(PANEL2);HGDIOBJ oldBrush=SelectObject(dc,ab);HPEN ap=CreatePen(PS_SOLID,1,RGB(56,76,101));HGDIOBJ oldPen=SelectObject(dc,ap);
 RoundRect(dc,art.left,art.top,art.right,art.bottom,14,14);
 SelectObject(dc,oldBrush);SelectObject(dc,oldPen);DeleteObject(ab);DeleteObject(ap);
 if(tag[1]=='1'){
  HBRUSH hill=CreateSolidBrush(RGB(49,104,75));RECT h1={art.left+10,art.bottom-65,art.right-8,art.bottom-8};FillRect(dc,&h1,hill);DeleteObject(hill);
  COLORREF blocks[5]={RGB(104,164,92),RGB(123,94,69),RGB(112,122,139),RGB(81,137,83),RGB(147,112,78)};
  for(int k=0;k<5;k++){int x=art.left+25+k*54,y=art.top+25+(k%2)*18;HBRUSH b=CreateSolidBrush(blocks[k]);RECT q={x,y,x+42,y+42};FillRect(dc,&q,b);DeleteObject(b);HPEN edgePen=CreatePen(PS_SOLID,1,RGB(27,43,48));HGDIOBJ op=SelectObject(dc,edgePen);Rectangle(dc,x,y,x+42,y+42);SelectObject(dc,op);DeleteObject(edgePen);}
  HBRUSH trunk=CreateSolidBrush(RGB(94,68,45));RECT tr={art.right-70,art.bottom-65,art.right-58,art.bottom-22};FillRect(dc,&tr,trunk);DeleteObject(trunk);
  HBRUSH leaves=CreateSolidBrush(RGB(65,139,74));RECT crown={art.right-91,art.bottom-95,art.right-38,art.bottom-50};FillRect(dc,&crown,leaves);DeleteObject(leaves);
 } else {
  HBRUSH skyBrush=CreateSolidBrush(RGB(83,139,177));RECT sr={art.left+1,art.top+1,art.right-1,art.bottom-35};FillRect(dc,&sr,skyBrush);DeleteObject(skyBrush);
  COLORREF buildings[5]={RGB(89,112,139),RGB(135,147,157),RGB(75,96,123),RGB(171,133,98),RGB(95,130,143)};
  for(int k=0;k<5;k++){int x=art.left+18+k*62,h=45+(k*19)%58;HBRUSH b=CreateSolidBrush(buildings[k]);RECT q={x,art.bottom-36-h,x+48,art.bottom-36};FillRect(dc,&q,b);DeleteObject(b);
   HBRUSH win=CreateSolidBrush(RGB(226,205,139));for(int wy=art.bottom-29-h;wy<art.bottom-48;wy+=17)for(int wx=x+8;wx<x+40;wx+=17){RECT wq={wx,wy,wx+7,wy+9};FillRect(dc,&wq,win);}DeleteObject(win);}
  HBRUSH road=CreateSolidBrush(RGB(46,52,61));RECT rr={art.left+1,art.bottom-36,art.right-1,art.bottom-1};FillRect(dc,&rr,road);DeleteObject(road);
  HPEN lane=CreatePen(PS_DASH,2,RGB(240,213,132));HGDIOBJ op=SelectObject(dc,lane);MoveToEx(dc,art.left+8,art.bottom-18,nullptr);LineTo(dc,art.right-8,art.bottom-18);SelectObject(dc,op);DeleteObject(lane);
 }
 RECT br={r.right-230,r.bottom-88,r.right-28,r.bottom-28};HBRUSH bb=CreateSolidBrush(edge);ob=SelectObject(dc,bb);RoundRect(dc,br.left,br.top,br.right,br.bottom,12,12);SelectObject(dc,ob);DeleteObject(bb);
 text(dc,button,br.left+42,br.top+17,bodyFont,BG);
}
static LRESULT CALLBACK Proc(HWND w,UINT m,WPARAM a,LPARAM b){
 switch(m){
 case WM_CREATE:
  titleFont=CreateFontA(42,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,"Segoe UI");
  subFont=CreateFontA(20,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,"Segoe UI");
  bodyFont=CreateFontA(23,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,"Segoe UI");
  smallFont=CreateFontA(14,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,"Segoe UI");
  return 0;
 case WM_ERASEBKGND:return 1;
 case WM_PAINT:{PAINTSTRUCT ps;HDC dc=BeginPaint(w,&ps);RECT rc;GetClientRect(w,&rc);HBRUSH bg=CreateSolidBrush(BG);FillRect(dc,&rc,bg);DeleteObject(bg);
  blockCard={42,210,rc.right-42,rc.top+430};cityCard={42,rc.top+465,rc.right-42,rc.top+685};
  text(dc,"SNIPZ",48,32,titleFont,TEXT);text(dc,"GAME HUB   /   DEINE SPIELE",52,91,smallFont,ACCENT);text(dc,"Wahle ein Spiel und starte es.",48,145,subFont,MUTED);
  card(dc,blockCard,ACCENT,"01   /   VOXEL SANDBOX","BLOCKLANDS","Erkunden, bauen und in einer Voxel-Welt uberleben.","SPIELEN");
  card(dc,cityCard,GREEN,"02   /   CITY BUILDER","SNIPZ CITY","Strassen planen, Gebiete ausweisen und eine Stadt entwickeln.","SPIELEN");
  text(dc,status.c_str(),48,rc.bottom-92,subFont,TEXT);text(dc,"Beide Spiele liegen als EXE-Dateien neben diesem Launcher.",48,rc.bottom-55,subFont,MUTED);
  EndPaint(w,&ps);return 0;}
 case WM_LBUTTONUP:{POINT p={(short)LOWORD(b),(short)HIWORD(b)};if(hit(p,blockCard))launchGame(w,"Blocklands.exe","Blocklands");else if(hit(p,cityCard))launchGame(w,"SnipzCity.exe","Snipz City");return 0;}
 case WM_DESTROY:DeleteObject(titleFont);DeleteObject(subFont);DeleteObject(bodyFont);DeleteObject(smallFont);PostQuitMessage(0);return 0;
 }return DefWindowProcA(w,m,a,b);
}
int WINAPI WinMain(HINSTANCE i,HINSTANCE,LPSTR,int show){
 WNDCLASSA c={};c.lpfnWndProc=Proc;c.hInstance=i;c.lpszClassName="SnipzLauncherV2";c.hCursor=LoadCursor(nullptr,IDC_ARROW);c.hbrBackground=CreateSolidBrush(BG);RegisterClassA(&c);
 HWND w=CreateWindowA(c.lpszClassName,"Snipz Launcher",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_THICKFRAME|WS_MAXIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,1080,900,nullptr,nullptr,i,nullptr);
 if(!w)return 1;ShowWindow(w,show);UpdateWindow(w);MSG msg;while(GetMessageA(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessageA(&msg);}return 0;
}