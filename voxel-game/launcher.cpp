#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <string>
#include <cstring>
static HWND statusText;
static HFONT titleFont, bodyFont;
static void launchGame(HWND hwnd,const char* exe,const char* name){
 char path[MAX_PATH]={0};GetModuleFileNameA(nullptr,path,MAX_PATH);char* slash=strrchr(path,'\\');if(slash)*slash=0;
 std::string full=std::string(path)+"\\"+exe;
 DWORD attr=GetFileAttributesA(full.c_str());
 if(attr==INVALID_FILE_ATTRIBUTES){SetWindowTextA(statusText,(std::string(name)+" is not installed beside SnipzLauncher.exe.").c_str());return;}
 HINSTANCE r=ShellExecuteA(hwnd,"open",full.c_str(),nullptr,path,SW_SHOWNORMAL);
 if((INT_PTR)r<=32)SetWindowTextA(statusText,"Could not start the game. Check that the EXE is present.");
 else SetWindowTextA(statusText,(std::string("Starting ")+name+"...").c_str());
}
static LRESULT CALLBACK Proc(HWND w,UINT m,WPARAM a,LPARAM b){
 switch(m){
 case WM_CREATE:{
  titleFont=CreateFontA(34,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,"Segoe UI");
  bodyFont=CreateFontA(17,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,"Segoe UI");
  auto label=[&](const char*t,int x,int y,int ww,int hh,HFONT f){HWND c=CreateWindowA("STATIC",t,WS_CHILD|WS_VISIBLE,x,y,ww,hh,w,nullptr,nullptr,nullptr);SendMessageA(c,WM_SETFONT,(WPARAM)f,TRUE);return c;};
  label("SNIPZ",30,22,360,46,titleFont);label("GAME LAUNCHER",34,67,360,26,bodyFont);
  label("YOUR GAMES",34,116,360,25,bodyFont);
  HWND b1=CreateWindowA("BUTTON","PLAY  |  BLOCKLANDS",WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,34,153,390,58,w,(HMENU)101,nullptr,nullptr);SendMessageA(b1,WM_SETFONT,(WPARAM)bodyFont,TRUE);
  label("Voxel sandbox  -  explore, build and survive",44,215,390,25,bodyFont);
  HWND b2=CreateWindowA("BUTTON","PLAY  |  SNIPZ CITY",WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,34,260,390,58,w,(HMENU)102,nullptr,nullptr);SendMessageA(b2,WM_SETFONT,(WPARAM)bodyFont,TRUE);
  label("City builder  -  roads, zones and growing neighborhoods",44,322,410,40,bodyFont);
  statusText=label("Select a game to launch.",34,385,420,42,bodyFont);
  label("Snipz Launcher  |  Free indie games",34,450,390,24,bodyFont);
  return 0;}
 case WM_COMMAND:if(LOWORD(a)==101)launchGame(w,"Blocklands.exe","Blocklands");if(LOWORD(a)==102)launchGame(w,"SnipzCity.exe","Snipz City");return 0;
 case WM_CTLCOLORSTATIC:{HDC dc=(HDC)a;SetTextColor(dc,RGB(224,233,246));SetBkColor(dc,RGB(20,29,44));return (LRESULT)GetStockObject(NULL_BRUSH);}
 case WM_DESTROY:DeleteObject(titleFont);DeleteObject(bodyFont);PostQuitMessage(0);return 0;
 }return DefWindowProcA(w,m,a,b);
}
int WINAPI WinMain(HINSTANCE i,HINSTANCE,LPSTR,int show){
 WNDCLASSA c={};c.lpfnWndProc=Proc;c.hInstance=i;c.lpszClassName="SnipzLauncher";c.hCursor=LoadCursor(nullptr,IDC_ARROW);c.hbrBackground=CreateSolidBrush(RGB(20,29,44));RegisterClassA(&c);
 HWND w=CreateWindowA(c.lpszClassName,"SNIPZ Launcher",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,500,540,nullptr,nullptr,i,nullptr);
 if(!w)return 1;ShowWindow(w,show);MSG msg;while(GetMessageA(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessageA(&msg);}return 0;
}