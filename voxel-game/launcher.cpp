#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <string>
#include <cstring>

static const COLORREF BG=RGB(12,18,31), PANEL=RGB(23,33,52), PANEL2=RGB(30,45,68), TEXT=RGB(235,242,252), MUTED=RGB(155,174,199), ACCENT=RGB(91,160,255), GREEN=RGB(91,205,151);
static HFONT titleFont, subFont, bodyFont, smallFont;
static std::string status="Wahle ein Spiel aus und starte direkt.";
static RECT blockCard={34,178,596,330}, cityCard={34,350,596,502};
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
 text(dc,tag,r.left+24,r.top+20,smallFont,edge);text(dc,name,r.left+24,r.top+51,titleFont,TEXT);text(dc,desc,r.left+24,r.top+101,bodyFont,MUTED);
 RECT br={r.right-180,r.bottom-62,r.right-22,r.bottom-18};HBRUSH bb=CreateSolidBrush(edge);ob=SelectObject(dc,bb);RoundRect(dc,br.left,br.top,br.right,br.bottom,12,12);SelectObject(dc,ob);DeleteObject(bb);
 text(dc,button,br.left+28,br.top+12,bodyFont,BG);
}
static LRESULT CALLBACK Proc(HWND w,UINT m,WPARAM a,LPARAM b){
 switch(m){
 case WM_CREATE:
  titleFont=CreateFontA(27,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,"Segoe UI");
  subFont=CreateFontA(15,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,"Segoe UI");
  bodyFont=CreateFontA(17,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,"Segoe UI");
  smallFont=CreateFontA(11,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,"Segoe UI");
  return 0;
 case WM_ERASEBKGND:return 1;
 case WM_PAINT:{PAINTSTRUCT ps;HDC dc=BeginPaint(w,&ps);RECT rc;GetClientRect(w,&rc);HBRUSH bg=CreateSolidBrush(BG);FillRect(dc,&rc,bg);DeleteObject(bg);
  text(dc,"SNIPZ",34,27,titleFont,TEXT);text(dc,"GAME HUB   /   DEINE SPIELE",36,68,smallFont,ACCENT);text(dc,"Wahle ein Spiel",34,116,subFont,MUTED);
  card(dc,blockCard,ACCENT,"01   /   VOXEL SANDBOX","BLOCKLANDS","Erkunden, bauen und in einer Voxel-Welt uberleben.","SPIELEN");
  card(dc,cityCard,GREEN,"02   /   CITY BUILDER","SNIPZ CITY","Strassen planen, Gebiete ausweisen und eine Stadt entwickeln.","SPIELEN");
  text(dc,status.c_str(),36,530,subFont,TEXT);text(dc,"Beide Spiele liegen als EXE-Dateien neben diesem Launcher.",36,565,subFont,MUTED);
  EndPaint(w,&ps);return 0;}
 case WM_LBUTTONUP:{POINT p={(short)LOWORD(b),(short)HIWORD(b)};if(hit(p,blockCard))launchGame(w,"Blocklands.exe","Blocklands");else if(hit(p,cityCard))launchGame(w,"SnipzCity.exe","Snipz City");return 0;}
 case WM_DESTROY:DeleteObject(titleFont);DeleteObject(subFont);DeleteObject(bodyFont);DeleteObject(smallFont);PostQuitMessage(0);return 0;
 }return DefWindowProcA(w,m,a,b);
}
int WINAPI WinMain(HINSTANCE i,HINSTANCE,LPSTR,int show){
 WNDCLASSA c={};c.lpfnWndProc=Proc;c.hInstance=i;c.lpszClassName="SnipzLauncherV2";c.hCursor=LoadCursor(nullptr,IDC_ARROW);c.hbrBackground=CreateSolidBrush(BG);RegisterClassA(&c);
 HWND w=CreateWindowA(c.lpszClassName,"Snipz Launcher",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,680,660,nullptr,nullptr,i,nullptr);
 if(!w)return 1;ShowWindow(w,show);UpdateWindow(w);MSG msg;while(GetMessageA(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessageA(&msg);}return 0;
}