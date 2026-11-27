#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <fstream>
static HWND resolutionBox, fpsBox, fullscreenCheck, statusText;
static const char* resolutions[]={"1280x720","1600x900","1920x1080"};
static const int widths[]={1280,1600,1920}, heights[]={720,900,1080}, fpsValues[]={60,120,144,0};
static void SaveAndLaunch(HWND window) {
 int r=(int)SendMessageA(resolutionBox,CB_GETCURSEL,0,0), f=(int)SendMessageA(fpsBox,CB_GETCURSEL,0,0);
 if(r<0) r=0; if(f<0) f=1;
 bool full=SendMessageA(fullscreenCheck,BM_GETCHECK,0,0)==BST_CHECKED;
 std::ofstream cfg("settings.ini"); cfg<<"width="<<widths[r]<<"\nheight="<<heights[r]<<"\nfullscreen="<<(full?1:0)<<"\nfps="<<fpsValues[f]<<"\n"; cfg.close();
 if(GetFileAttributesA("Blocklands.exe")==INVALID_FILE_ATTRIBUTES) { SetWindowTextA(statusText,"Blocklands.exe not found. Keep launcher beside the game."); return; }
 ShellExecuteA(window,"open","Blocklands.exe",nullptr,nullptr,SW_SHOWNORMAL);
 SetWindowTextA(statusText,"Settings saved. Starting Blocklands...");
}
static LRESULT CALLBACK WindowProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp) {
 switch(msg) {
 case WM_CREATE: {
  HFONT font=(HFONT)GetStockObject(DEFAULT_GUI_FONT);
  auto label=[&](const char* t,int x,int y,int w,int h){HWND c=CreateWindowA("STATIC",t,WS_CHILD|WS_VISIBLE,x,y,w,h,hwnd,nullptr,nullptr,nullptr);SendMessageA(c,WM_SETFONT,(WPARAM)font,TRUE);};
  label("BLOCKLANDS",28,20,300,34); label("Launcher | Game settings",30,55,300,22);
  label("Resolution",30,105,130,22);
  resolutionBox=CreateWindowA("COMBOBOX","",WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST,160,100,190,150,hwnd,(HMENU)101,nullptr,nullptr);
  for(auto s:resolutions) SendMessageA(resolutionBox,CB_ADDSTRING,0,(LPARAM)s); SendMessageA(resolutionBox,CB_SETCURSEL,0,0);
  label("FPS limit",30,155,130,22);
  fpsBox=CreateWindowA("COMBOBOX","",WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST,160,150,190,150,hwnd,(HMENU)102,nullptr,nullptr);
  const char* fs[]={"60 FPS","120 FPS","144 FPS","Unlimited"}; for(auto s:fs) SendMessageA(fpsBox,CB_ADDSTRING,0,(LPARAM)s); SendMessageA(fpsBox,CB_SETCURSEL,1,0);
  fullscreenCheck=CreateWindowA("BUTTON","Fullscreen",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,160,198,150,26,hwnd,(HMENU)103,nullptr,nullptr);
  label("Settings are saved in settings.ini beside the game.",30,238,370,24);
  CreateWindowA("BUTTON","PLAY BLOCKLANDS",WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON,30,280,320,48,hwnd,(HMENU)201,nullptr,nullptr);
  statusText=CreateWindowA("STATIC","Choose settings, then press Play.",WS_CHILD|WS_VISIBLE,30,340,370,42,hwnd,nullptr,nullptr,nullptr); SendMessageA(statusText,WM_SETFONT,(WPARAM)font,TRUE);
  return 0;
 }
 case WM_COMMAND: if(LOWORD(wp)==201) SaveAndLaunch(hwnd); return 0;
 case WM_DESTROY: PostQuitMessage(0); return 0;
 }
 return DefWindowProcA(hwnd,msg,wp,lp);
}
int WINAPI WinMain(HINSTANCE inst,HINSTANCE,LPSTR,int show) {
 WNDCLASSA wc={}; wc.lpfnWndProc=WindowProc; wc.hInstance=inst; wc.lpszClassName="BlocklandsLauncherWindow"; wc.hCursor=LoadCursor(nullptr,IDC_ARROW); wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1); RegisterClassA(&wc);
 HWND w=CreateWindowA(wc.lpszClassName,"BLOCKLANDS Launcher",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,430,440,nullptr,nullptr,inst,nullptr);
 if(!w) return 1; ShowWindow(w,show); MSG m; while(GetMessageA(&m,nullptr,0,0)>0){TranslateMessage(&m);DispatchMessageA(&m);} return 0;
}