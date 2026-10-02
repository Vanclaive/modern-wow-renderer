#include "src/Environment/LocationTuning.h"
#pragma once
#include <windows.h>
#include <d3d9.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>
#include "src/Environment/OverlayFont.h"
#include "src/Environment/EnvironmentProfileManager.h"

namespace tuningoverlay {
enum ItemKind { KIND_HEADER, KIND_TOGGLE, KIND_SLIDER };
struct Item { ItemKind kind; const wchar_t* section; const wchar_t* key; const char* label; int lo,hi,step,value; };
inline std::wstring ini;
inline bool visible=false,f7Down=false,mouseDown=false;
inline int hover=-1,dragItem=-1;
// WOTLK-COMPAT: input state (see the "WOTLK-COMPAT input handling" block above Update()).
enum ControlZone { CZ_NONE, CZ_TOGGLE, CZ_TRACK, CZ_VALUE };
inline bool inputHookEnabled=true,hookActive=false,hookUnicode=true,panelPress=false,pendingApply=false,forceApply=false,dragFine=false;
inline std::vector<char> dirty;
inline ULONGLONG lastApply=0;
inline float dragAnchorX=0.f,dragAnchorT=0.f;
inline int entryItem=-1,wheelSteps=0,wheelAccum=0;
inline std::string entryText;
inline HWND gameWindow=nullptr;
inline WNDPROC prevProc=nullptr;
inline std::vector<Item> items={
 // === ATMOSPHERE & GOD RAYS ===
 {KIND_HEADER,nullptr,nullptr,"--- ATMOSPHERE & GOD RAYS ---",0,0,0,0},
 {KIND_TOGGLE,L"Atmosphere",L"ShaftsEnabled","SUN RAYS",0,1,1,1},
 {KIND_SLIDER,L"Atmosphere",L"ShaftPercent","RAY STRENGTH",0,400,5,160},
 {KIND_SLIDER,L"Atmosphere",L"SunGlowPercent","SUN GLOW",0,200,5,80},
 {KIND_TOGGLE,L"Atmosphere",L"FogEnabled","FOG",0,1,1,1},
 // Derives fog distance/colour from WoW's own authored values for the
 // current zone/shader (read-only capture) instead of one fixed profile
 // everywhere - FOG DISTANCE still scales it. Off = old fixed profile.
 {KIND_TOGGLE,L"Atmosphere",L"UseEnvironmentFog","WOW FOG BASELINE",0,1,1,0},
 {KIND_SLIDER,L"Atmosphere",L"AtmosphereQuality","ATMOSPHERE QUALITY",0,2,1,1},
 {KIND_SLIDER,L"Atmosphere",L"DensityPermille","FOG DENSITY",0,20,1,5},
 {KIND_SLIDER,L"Atmosphere",L"AtmosphereMaxDistance","FOG DISTANCE",200,1200,20,520},
 {KIND_SLIDER,L"Atmosphere",L"AtmosphereWashPercent","FOG WASH",0,100,2,55},
 // Separate near-ground world-space volume integrated by the atmosphere
 // raymarch. Movement opens a soft wake which gradually fills back in.
 {KIND_HEADER,nullptr,nullptr,"--- LOCAL VOLUMETRIC FOG ---",0,0,0,0},
 {KIND_TOGGLE,L"LocalFog",L"Enabled","LOCAL FOG",0,1,1,1},
 {KIND_SLIDER,L"LocalFog",L"DensityPermille","LOCAL DENSITY",0,100,2,12},
 {KIND_SLIDER,L"LocalFog",L"HeightUnits","LOCAL HEIGHT",1,8,1,3},
 {KIND_SLIDER,L"LocalFog",L"WakeStrengthPercent","PLAYER WAKE",0,150,5,85},
 {KIND_SLIDER,L"LocalFog",L"WakeRadius","WAKE RADIUS",2,14,1,5},
 {KIND_SLIDER,L"LocalFog",L"TrailLength","WAKE TRAIL",4,40,2,18},
 // Rescales the GAME's OWN native distance fog (its real per-shader fog
 // constants, the same ones vanilla WoW uses to hide its own render/
 // streaming distance edge) - independent of the atmosphere controls
 // above. This is what actually guarantees the world edge stays covered
 // regardless of any other setting here.
 {KIND_TOGGLE,L"DistanceFog",L"Enabled","WORLD EDGE FOG",0,1,1,1},
 {KIND_SLIDER,L"DistanceFog",L"DistancePercent","EDGE FOG DISTANCE",10,300,5,130},
 {KIND_SLIDER,L"DistanceFog",L"PowerPercent","EDGE FOG POWER",10,300,5,30},

 // === WEATHER VISUALS ===
 {KIND_HEADER,nullptr,nullptr,"--- WEATHER VISUALS ---",0,0,0,0},
 {KIND_TOGGLE,L"WeatherVisuals",L"Enabled","WEATHER SYSTEM",0,1,1,1},
 {KIND_SLIDER,L"WeatherVisuals",L"Mode","WEATHER HINT (0=AUTO 1=RAIN 2=SNOW)",0,2,1,0},
 {KIND_SLIDER,L"WeatherVisuals",L"IntensityPercent","WEATHER STRENGTH",25,200,5,100},

 // === WATER & REFLECTIONS ===
 {KIND_HEADER,nullptr,nullptr,"--- WATER & REFLECTIONS ---",0,0,0,0},
 {KIND_TOGGLE,L"Water",L"Enabled","WATER",0,1,1,1},
 {KIND_TOGGLE,L"Water",L"WavesEnabled","WAVES",0,1,1,1},
 {KIND_TOGGLE,L"Water",L"ReflectionsEnabled","REFLECTIONS",0,1,1,1},
 {KIND_SLIDER,L"Water",L"ReflectionPercent","SSR STRENGTH",0,150,2,80},
 {KIND_SLIDER,L"Water",L"EnvironmentPercent","SKY REFLECTION",0,150,2,45},
 {KIND_TOGGLE,L"Water",L"SunGlintEnabled","SUN MOON GLINT",0,1,1,1},
 {KIND_SLIDER,L"Water",L"SunGlintPercent","GLINT STRENGTH",0,500,5,160},
 {KIND_TOGGLE,L"Water",L"RefractionEnabled","REFRACTION",0,1,1,1},
 {KIND_SLIDER,L"Water",L"RefractionPercent","REFRACT AMOUNT",0,50,1,14},
 {KIND_SLIDER,L"Water",L"DepthAbsorptionPercent","WATER DEPTH",0,100,1,38},
 {KIND_TOGGLE,L"Water",L"FoamEnabled","FOAM",0,1,1,1},
 {KIND_SLIDER,L"Water",L"ShoreFoamPercent","FOAM AMOUNT",0,60,1,28},
 {KIND_SLIDER,L"Water",L"SpecularPercent","WATER SHINE",0,400,5,115},

 // === LOCAL LIGHTING ===
 {KIND_HEADER,nullptr,nullptr,"--- LOCAL LIGHTING ---",0,0,0,0},
 {KIND_TOGGLE,L"DynamicLighting",L"Enabled","DYNAMIC LIGHTS",0,1,1,1},
 {KIND_SLIDER,L"DynamicLighting",L"IntensityPercent","LIGHT INTENSITY",0,250,5,100},
 {KIND_SLIDER,L"DynamicLighting",L"RayPercent","LIGHT RAYS",0,200,5,75},
 {KIND_SLIDER,L"DynamicLighting",L"Quality","LIGHT QUALITY (0 LOW 1 BAL 2 HIGH)",0,2,1,1},
 {KIND_SLIDER,L"DynamicLighting",L"DebugMode","LIGHT DEBUG",0,4,1,0},

 // === DYNAMIC SHADOWS ===
 {KIND_HEADER,nullptr,nullptr,"--- DYNAMIC SHADOWS ---",0,0,0,0},
 {KIND_TOGGLE,L"NativeShadows",L"Enabled","NATIVE SHADOWS",0,1,1,1},
 {KIND_SLIDER,L"NativeShadows",L"SoftnessPercent","SHADOW SOFTNESS",50,220,5,145},
 // SHADOW STRENGTH: c12 (the darkening constant) is a `def`-declared shader
 // literal, so writing the constant register at runtime is a no-op - this
 // slider instead swaps in a byte-patched COPY of the shader (only the two
 // c12 floats changed, everything else byte-identical) for the 4 confirmed
 // receiver hashes, restoring the original shader after the draw.
 {KIND_SLIDER,L"NativeShadows",L"StrengthPercent","SHADOW STRENGTH",30,250,10,100},

 // === AI MATERIALS & RELIEF ===
 {KIND_HEADER,nullptr,nullptr,"--- AI MATERIALS & RELIEF ---",0,0,0,0},
 {KIND_TOGGLE,L"AIMaterials",L"Enabled","AI MATERIALS",0,1,1,1},
 {KIND_SLIDER,L"AIMaterials",L"NormalStrengthPercent","NORMAL BUMP",0,200,5,90},
 {KIND_SLIDER,L"AIMaterials",L"ParallaxDepthPercent","PARALLAX DEPTH",0,100,2,35},
 {KIND_SLIDER,L"AIMaterials",L"SelfShadowStrength","SELF SHADOWS",0,150,5,90},

 // === IMAGE & COLOR ===
 {KIND_HEADER,nullptr,nullptr,"--- IMAGE & COLOR (GLOBAL) ---",0,0,0,0},
 {KIND_TOGGLE,L"PostProcess",L"Enabled","POST PROCESS",0,1,1,0},
 {KIND_SLIDER,L"PostProcess",L"BrightnessPercent","BRIGHTNESS",-50,50,1,0},
 {KIND_SLIDER,L"PostProcess",L"ContrastPercent","CONTRAST",50,180,2,100},
 {KIND_SLIDER,L"PostProcess",L"GammaPercent","GAMMA",50,180,2,100},
 {KIND_SLIDER,L"PostProcess",L"SharpnessPercent","SHARPNESS",0,100,2,35}
};
inline const std::vector<int> defaults=[](){std::vector<int> result;for(const auto& i:items)result.push_back(i.value);return result;}();
inline void Configure(const std::wstring& base){ini=base+L"GraphicsEffects.ini";inputHookEnabled=GetPrivateProfileIntW(L"Overlay",L"InputHook",1,ini.c_str())!=0;for(auto& i:items){if(i.kind==KIND_HEADER)continue;i.value=std::clamp(static_cast<int>(renderer::locationtuning::ReadEditorInt(i.section,i.key,defaults[&i-items.data()],ini.c_str())),i.lo,i.hi);}}
inline void Save(Item& i){if(i.kind==KIND_HEADER)return;renderer::locationtuning::Save(i.section,i.key,i.value);}
inline const std::array<unsigned char,7>& Glyph(char c){
 if(c>='a'&&c<='z')c=char(c-'a'+'A');
 static const std::array<unsigned char,7> blank{};
 static const std::array<unsigned char,7> dash={0,0,0,31,0,0,0};
 static const std::array<unsigned char,7> slash={1,2,4,8,16,0,0};
 static const std::array<unsigned char,7> amp={12,18,12,10,18,19,13};
 static const std::array<unsigned char,7> glyphs[]={
  {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},{30,17,17,17,17,17,30},
  {31,16,16,30,16,16,31},{31,16,16,30,16,16,16},{14,17,16,23,17,17,15},{17,17,17,31,17,17,17},
  {14,4,4,4,4,4,14},{7,2,2,2,2,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
  {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},{30,17,17,30,16,16,16},
  {14,17,17,17,21,18,13},{30,17,17,30,20,18,17},{15,16,16,14,1,1,30},{31,4,4,4,4,4,4},
  {17,17,17,17,17,17,14},{17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
  {17,17,10,4,4,4,4},{31,1,2,4,8,16,31},
  {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},{30,1,1,14,1,1,30},
  {2,6,10,18,31,2,2},{31,16,16,30,1,1,30},{14,16,16,30,17,17,14},{31,1,2,4,8,8,8},
  {14,17,17,14,17,17,14},{14,17,17,15,1,1,14}
 };
 if(c>='A'&&c<='Z')return glyphs[c-'A'];if(c>='0'&&c<='9')return glyphs[26+c-'0'];
 if(c=='-')return dash;if(c=='/')return slash;if(c=='&')return amp;return blank;
}
struct V {float x,y,z,w;DWORD color;};
inline void Rect(std::vector<V>& out,float x,float y,float w,float h,DWORD color){V a{x,y,0,1,color},b{x+w,y,0,1,color},c{x,y+h,0,1,color},d{x+w,y+h,0,1,color};out.insert(out.end(),{a,b,c,c,b,d});}
inline void BitmapText(std::vector<V>& out,float x,float y,const char* s,DWORD color,float scale=2.f){for(;*s;++s,x+=6*scale){if(*s==' '){continue;}auto& g=Glyph(*s);for(int yy=0;yy<7;++yy)for(int xx=0;xx<5;++xx)if(g[yy]&(16>>xx))Rect(out,x+xx*scale,y+yy*scale,scale,scale,color);}}

inline OverlayFont font;
inline float uiScale=1;
inline std::vector<std::string> environmentLines;
inline constexpr int columns=3;
inline constexpr float columnWidth=350,controlRow=24;
inline const std::array<int,4>& ColumnStarts(){
 static const std::array<int,4> starts=[](){
  std::array<int,4> result{0,0,0,int(items.size())};
  for(int i=0;i<int(items.size());++i)if(items[i].kind==KIND_HEADER){
   if(std::string(items[i].label)=="--- WEATHER VISUALS ---")result[1]=i;
   if(std::string(items[i].label)=="--- LOCAL LIGHTING ---")result[2]=i;
  }
  return result;
 }();return starts;
}
inline int ItemColumn(int index){const auto& starts=ColumnStarts();return index>=starts[2]?2:index>=starts[1]?1:0;}
inline int columnRows=1;
inline float panelHeight=0;
inline int renderWidth=0,renderHeight=0;
inline float PanelX(){return 24*uiScale;}
inline float PanelY(){return 24*uiScale;}
inline float ControlsY(){return PanelY()+196*uiScale;}
inline void Layout(int width,int height,HWND window){
 using GetDpi=UINT(WINAPI*)(HWND);
 auto getDpi=reinterpret_cast<GetDpi>(GetProcAddress(GetModuleHandleW(L"user32.dll"),"GetDpiForWindow"));
 float dpi=getDpi&&window?getDpi(window)/96.f:1.f;
 const auto& starts=ColumnStarts();columnRows=std::max({starts[1]-starts[0],starts[2]-starts[1],starts[3]-starts[2]});
 const float logicalHeight=196+columnRows*controlRow+18;
 uiScale=std::clamp(std::max(height/1440.f,std::min(dpi,1.15f)),.85f,1.5f);
 uiScale=std::min(uiScale,std::min((width-48.f)/(columns*columnWidth),(height-48.f)/logicalHeight));
 uiScale=std::max(.5f,uiScale);
 panelHeight=logicalHeight*uiScale;
}
inline int HitItem(float px,float py){
 const float x=PanelX(),start=ControlsY();
 if(px<x||px>=x+columns*columnWidth*uiScale||py<start)return -1;
 int column=int((px-x)/(columnWidth*uiScale));
 int row=int((py-start)/(controlRow*uiScale));
 int index=ColumnStarts()[column]+row;
 return row>=0&&row<columnRows&&index<ColumnStarts()[column+1]&&items[index].kind!=KIND_HEADER?index:-1;
}
inline std::string CompactLabel(const Item& item){
 std::string result=item.label;
 if(item.kind==KIND_HEADER){if(result.starts_with("--- "))result.erase(0,4);if(result.ends_with(" ---"))result.resize(result.size()-4);}
 const auto hint=result.find(" (");if(hint!=result.npos)result.resize(hint);
 return result;
}
inline int HitScope(float px,float py){
 if(py<PanelY()+132*uiScale||py>=PanelY()+157*uiScale)return -1;
 const float local=(px-PanelX())/uiScale;
 if(local>=155&&local<585)return 0;
 if(local>=595&&local<1038)return 1;
 return -1;
}
inline bool SelectScope(int scope){
 if(!renderer::locationtuning::editable||scope<0||scope>1||(scope==1&&!renderer::locationtuning::active.areaId))return false;
 renderer::locationtuning::editZone=scope==0;dragItem=-1;
 for(auto& i:items)if(i.kind!=KIND_HEADER)i.value=std::clamp(static_cast<int>(renderer::locationtuning::ReadEditorInt(i.section,i.key,defaults[&i-items.data()],ini.c_str())),i.lo,i.hi);
 return true;
}
// =====================================================================
// WOTLK-COMPAT input handling (everything between here and Draw()).
//  - Only the bar itself drags; clicking a label or empty row space does nothing.
//  - Values are shown instantly, but the ini write + module reload is throttled
//    to ~8/second while dragging (and always runs once on release).
//  - Hold SHIFT while dragging for fine control. Mouse wheel = one step (CTRL = 5).
//  - Click a number to type a value (Enter = apply, Esc = cancel).
//  - A small window hook keeps panel clicks / wheel / typed keys away from the
//    game (no camera turning while dragging, no action-bar keys while typing).
//    Disable with   [Overlay]  InputHook=0   in GraphicsEffects.ini.
// =====================================================================
inline void MapClient(HWND window,POINT client,float& px,float& py){
 px=float(client.x);py=float(client.y);
 RECT rc{};
 if(!window||!GetClientRect(window,&rc))return;
 if(renderWidth&&renderHeight&&rc.right>0&&rc.bottom>0){px=float(client.x)*renderWidth/rc.right;py=float(client.y)*renderHeight/rc.bottom;}
}
inline bool PanelContains(float px,float py){return px>=PanelX()&&px<PanelX()+columns*columnWidth*uiScale&&py>=PanelY()&&py<PanelY()+panelHeight;}
inline bool CanEdit(int index){return index>=0&&(renderer::locationtuning::editable||renderer::locationtuning::IsGlobal(items[size_t(index)].section));}
inline ControlZone ItemZone(int index,float px){
 if(index<0)return CZ_NONE;
 const Item& it=items[size_t(index)];
 if(it.kind==KIND_TOGGLE)return CZ_TOGGLE;
 if(it.kind!=KIND_SLIDER)return CZ_NONE;
 const float local=(px-(PanelX()+ItemColumn(index)*columnWidth*uiScale))/uiScale;
 if(local>=248.f&&local<=338.f)return CZ_TRACK;
 if(local>=206.f&&local<248.f)return CZ_VALUE;
 return CZ_NONE;
}
inline void MarkDirty(int idx){
 if(dirty.size()!=items.size())dirty.assign(items.size(),0);
 dirty[size_t(idx)]=1;pendingApply=true;
}
inline void SetValue(int idx,int v){
 Item& i=items[size_t(idx)];
 v=std::clamp(v,i.lo,i.hi);
 if(v!=i.value){i.value=v;MarkDirty(idx);}
}
inline int ValueAtT(const Item& i,float t){
 const int steps=int(std::lround(float(i.hi-i.lo)*t/float(std::max(1,i.step))));
 return std::clamp(i.lo+steps*i.step,i.lo,i.hi);
}
// Writes changed values (the caller then reloads every module).
inline void FlushDirty(){
 for(size_t k=0;k<dirty.size()&&k<items.size();++k)if(dirty[k]){Save(items[k]);dirty[k]=0;}
 pendingApply=false;
}
inline void CancelEntry(){entryItem=-1;entryText.clear();}
inline void CommitEntry(){
 if(entryItem>=0&&entryItem<int(items.size())&&!entryText.empty()&&entryText!="-"){
  const long v=std::strtol(entryText.c_str(),nullptr,10);
  SetValue(entryItem,int(std::clamp(v,-100000L,100000L)));
  forceApply=true;
 }
 CancelEntry();
}
inline void EntryKey(WPARAM vk){
 if(entryItem<0)return;
 const Item& i=items[size_t(entryItem)];
 if(vk>='0'&&vk<='9'){if(entryText.size()<5)entryText.push_back(char(vk));}
 else if(vk>=VK_NUMPAD0&&vk<=VK_NUMPAD9){if(entryText.size()<5)entryText.push_back(char('0'+(vk-VK_NUMPAD0)));}
 else if((vk==VK_OEM_MINUS||vk==VK_SUBTRACT)&&i.lo<0&&entryText.empty())entryText="-";
 else if(vk==VK_BACK){if(!entryText.empty())entryText.pop_back();}
 else if(vk==VK_RETURN)CommitEntry();
 else if(vk==VK_ESCAPE)CancelEntry();
}
inline LRESULT CallPrev(HWND h,UINT m,WPARAM w,LPARAM l){
 if(!prevProc)return hookUnicode?DefWindowProcW(h,m,w,l):DefWindowProcA(h,m,w,l);
 return hookUnicode?CallWindowProcW(prevProc,h,m,w,l):CallWindowProcA(prevProc,h,m,w,l);
}
inline LRESULT CALLBACK HookProc(HWND h,UINT msg,WPARAM w,LPARAM l){
 if(visible){
  switch(msg){
  case WM_LBUTTONDOWN:case WM_LBUTTONDBLCLK:{
   POINT pt{LONG(short(LOWORD(l))),LONG(short(HIWORD(l)))};float px=0,py=0;MapClient(h,pt,px,py);
   if(PanelContains(px,py)){panelPress=true;return 0;}
   break;}
  case WM_LBUTTONUP:
   if(panelPress){panelPress=false;return 0;}
   break;
  case WM_MOUSEWHEEL:{
   POINT pt{};GetCursorPos(&pt);ScreenToClient(h,&pt);float px=0,py=0;MapClient(h,pt,px,py);
   if(PanelContains(px,py)){
    wheelAccum+=int(GET_WHEEL_DELTA_WPARAM(w));
    const int st=wheelAccum/WHEEL_DELTA;wheelAccum-=st*WHEEL_DELTA;wheelSteps+=st;
    return 0;
   }
   break;}
  case WM_KEYDOWN:case WM_SYSKEYDOWN:
   // Key-UP is deliberately not swallowed, so a key held before typing began can never stick in the game.
   if(entryItem>=0){if(msg==WM_KEYDOWN)EntryKey(w);return 0;}
   break;
  case WM_CHAR:case WM_SYSCHAR:case WM_DEADCHAR:case WM_SYSDEADCHAR:
   if(entryItem>=0)return 0;
   break;
  default:break;
  }
 }
 return CallPrev(h,msg,w,l);
}
inline void EnsureHook(){
 if(!inputHookEnabled||hookActive||!gameWindow)return;
 hookUnicode=IsWindowUnicode(gameWindow)!=FALSE;
 const LONG_PTR fn=reinterpret_cast<LONG_PTR>(&HookProc);
 const LONG_PTR old=hookUnicode?SetWindowLongPtrW(gameWindow,GWLP_WNDPROC,fn):SetWindowLongPtrA(gameWindow,GWLP_WNDPROC,fn);
 if(old){prevProc=reinterpret_cast<WNDPROC>(old);hookActive=true;}
}
inline bool Update(){
 DWORD pid=0;HWND window=GetForegroundWindow();GetWindowThreadProcessId(window,&pid);bool focused=pid==GetCurrentProcessId();
 bool f7=(GetAsyncKeyState(VK_F7)&0x8000)!=0;if(focused&&f7&&!f7Down)visible=!visible;f7Down=f7;
 const bool down=(GetAsyncKeyState(VK_LBUTTON)&0x8000)!=0;
 const ULONGLONG now=GetTickCount64();
 bool changed=false;
 if(!visible||!focused){
  hover=-1;dragItem=-1;panelPress=false;CancelEntry();mouseDown=down;
  if(pendingApply){FlushDirty();changed=true;}
  return changed;
 }
 EnsureHook();
 RECT client{};GetClientRect(window,&client);
 if(!renderWidth||!renderHeight)Layout(client.right,client.bottom,window);
 POINT p{};GetCursorPos(&p);ScreenToClient(window,&p);
 float px=0,py=0;MapClient(window,p,px,py);
 const bool shift=(GetAsyncKeyState(VK_SHIFT)&0x8000)!=0,ctrl=(GetAsyncKeyState(VK_CONTROL)&0x8000)!=0;
 const bool pressed=down&&!mouseDown,released=!down&&mouseDown;
 if(!down)dragItem=-1;
 if(pressed&&HitScope(px,py)>=0){
  if(pendingApply){FlushDirty();changed=true;}   // never carry unsaved edits across a zone/subarea switch
  CancelEntry();SelectScope(HitScope(px,py));mouseDown=down;return changed;
 }
 const int curHover=HitItem(px,py);
 const ControlZone zone=ItemZone(curHover,px);
 hover=dragItem>=0?dragItem:curHover;
 if(pressed){
  if(entryItem>=0&&!(curHover==entryItem&&zone==CZ_VALUE))CommitEntry();
  if(curHover>=0&&CanEdit(curHover)){
   Item& it=items[size_t(curHover)];
   if(zone==CZ_TOGGLE){it.value=!it.value;MarkDirty(curHover);forceApply=true;}
   else if(zone==CZ_TRACK){
    dragItem=curHover;dragFine=shift;
    const float left=PanelX()+ItemColumn(curHover)*columnWidth*uiScale+252*uiScale;
    const float t=std::clamp((px-left)/(82*uiScale),0.f,1.f);
    dragAnchorX=px;dragAnchorT=t;
    SetValue(curHover,ValueAtT(it,t));
   }
   else if(zone==CZ_VALUE){entryItem=curHover;entryText.clear();}
  }
 }
 if(down&&!pressed&&dragItem>=0&&CanEdit(dragItem)){
  Item& it=items[size_t(dragItem)];
  const float trackW=82*uiScale;
  if(shift!=dragFine){ // re-anchor so switching fine mode never makes the bar jump
   dragFine=shift;dragAnchorX=px;
   dragAnchorT=float(it.value-it.lo)/float(std::max(1,it.hi-it.lo));
  }
  // Fallback when the hook is off: if the game grabs the cursor (mouselook) it sits
  // far from the panel; ignore that instead of slamming the bar to min/max.
  const float trackLeft=PanelX()+ItemColumn(dragItem)*columnWidth*uiScale+252*uiScale;
  const bool wild=!hookActive&&(px>trackLeft+trackW+250*uiScale||px<trackLeft-250*uiScale);
  if(!wild){
   const float gain=dragFine?0.125f:1.f;
   const float t=std::clamp(dragAnchorT+(px-dragAnchorX)/trackW*gain,0.f,1.f);
   SetValue(dragItem,ValueAtT(it,t));
  }
 }
 if(wheelSteps!=0){
  if(curHover>=0&&CanEdit(curHover)&&items[size_t(curHover)].kind==KIND_SLIDER){
   Item& it=items[size_t(curHover)];
   SetValue(curHover,it.value+wheelSteps*it.step*(ctrl?5:1));
  }
  wheelSteps=0;
 }
 mouseDown=down;
 if(pendingApply&&(forceApply||released||now-lastApply>=120)){
  FlushDirty();forceApply=false;lastApply=now;changed=true;
 }
 if(!pendingApply)forceApply=false;
 return changed;
}
inline void Draw(IDirect3DDevice9* d){
 if(!visible||!d)return;
 IDirect3DStateBlock9* raw=nullptr;if(FAILED(d->CreateStateBlock(D3DSBT_ALL,&raw)))return;if(FAILED(raw->Capture())){raw->Release();return;}
 D3DVIEWPORT9 viewport{};d->GetViewport(&viewport);D3DDEVICE_CREATION_PARAMETERS creation{};d->GetCreationParameters(&creation);gameWindow=creation.hFocusWindow;
 environmentLines=renderer::EnvironmentProfileManager::Instance().CompactDebugLines();
 renderWidth=int(viewport.Width);renderHeight=int(viewport.Height);
 Layout(viewport.Width,viewport.Height,creation.hFocusWindow);
 struct Label {float x,y,right;std::string text;DWORD color;};std::vector<Label> labels;std::vector<V> v;
 float x=PanelX(),y=PanelY(),w=columns*columnWidth*uiScale,row=controlRow*uiScale;
 auto label=[&](float xx,float yy,const std::string& text,DWORD color,float right){labels.push_back({xx,yy,right,text,color});};
 Rect(v,x,y,w,panelHeight,0xf5121820);Rect(v,x,y,w,30*uiScale,0xff1b3340);
 label(x+12*uiScale,y+5*uiScale,"Modern WoW Renderer",0xff8effbb,x+w);
 label(x+w-220*uiScale,y+5*uiScale,entryItem>=0?"Type a number - Enter OK":(renderer::locationtuning::editable?"Local preset / F7 close":"Global settings / F7 close"),0xffb5cbd6,x+w-12*uiScale);
 for(size_t i=0;i<environmentLines.size();++i)label(x+12*uiScale,y+(36+23*i)*uiScale,environmentLines[i],i?0xffdce5ea:0xffffdc82,x+w-12*uiScale);
 const auto& manager=renderer::EnvironmentProfileManager::Instance();
 const bool zone=renderer::locationtuning::editZone||!renderer::locationtuning::active.areaId;
 label(x+12*uiScale,y+135*uiScale,"Settings for:",0xffdce5ea,x+150*uiScale);
 Rect(v,x+155*uiScale,y+132*uiScale,430*uiScale,25*uiScale,zone?0xff246b47:0xff26343b);
 Rect(v,x+595*uiScale,y+132*uiScale,443*uiScale,25*uiScale,!zone?0xff246b47:0xff26343b);
 label(x+162*uiScale,y+135*uiScale,"Entire zone - "+manager.ZoneName(),0xffdce5ea,x+578*uiScale);
 label(x+602*uiScale,y+135*uiScale,renderer::locationtuning::active.areaId?"This subarea - "+manager.AreaName():"Subarea unavailable (Area 0)",0xffdce5ea,x+w-12*uiScale);
 label(x+12*uiScale,y+165*uiScale,"Subarea settings override zone settings.   Drag a bar (hold SHIFT for fine), use the mouse wheel for single steps, or click a number to type one.",0xffb5cbd6,x+w-12*uiScale);
 for(int column=0;column<columns;++column){
  float xx=x+column*columnWidth*uiScale;
  if(column)Rect(v,xx-2*uiScale,ControlsY(),uiScale,panelHeight-196*uiScale,0xff2a3d48);
  for(int n=0;n<columnRows;++n){
   const int index=ColumnStarts()[column]+n;if(index>=ColumnStarts()[column+1])break;
   auto& i=items[index];float yy=ControlsY()+n*row;
   if(i.kind==KIND_HEADER){Rect(v,xx,yy,columnWidth*uiScale,row-1,0xff162836);label(xx+12*uiScale,yy+3*uiScale,CompactLabel(i),0xffffdc82,xx+(columnWidth-8)*uiScale);}
   else{
    label(xx+12*uiScale,yy+3*uiScale,CompactLabel(i),index==hover?0xffffffff:0xffdce5ea,xx+210*uiScale);
    if(i.kind==KIND_TOGGLE){Rect(v,xx+276*uiScale,yy+3*uiScale,58*uiScale,18*uiScale,i.value?0xff246b47:0xff4a3232);label(xx+289*uiScale,yy+3*uiScale,i.value?"ON":"OFF",i.value?0xff9dffc0:0xffffaaaa,xx+340*uiScale);}
    else{Rect(v,xx+252*uiScale,yy+8*uiScale,82*uiScale,9*uiScale,0xff26343b);float t=float(i.value-i.lo)/float(std::max(1,i.hi-i.lo));Rect(v,xx+252*uiScale,yy+8*uiScale,82*t*uiScale,9*uiScale,0xff45c985);if(index==entryItem)Rect(v,xx+208*uiScale,yy+2*uiScale,44*uiScale,20*uiScale,0xff2a5a72);label(xx+212*uiScale,yy+3*uiScale,index==entryItem?entryText+"_":std::to_string(i.value),index==entryItem?0xffffffff:0xffffdc82,xx+(index==entryItem?252:248)*uiScale);}
   }
  }
 }
 bool hasFont=font.Ensure(d,std::max(12,int(std::round(16*uiScale))));
 if(!hasFont)for(const auto& l:labels)BitmapText(v,l.x,l.y,l.text.c_str(),l.color,1.6f*uiScale);
 d->SetVertexShader(nullptr);d->SetPixelShader(nullptr);d->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE);d->SetTexture(0,nullptr);
 d->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);d->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_DIFFUSE);
 d->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);d->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_DIFFUSE);
 d->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE);d->SetRenderState(D3DRS_ZENABLE,FALSE);d->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);
 d->SetRenderState(D3DRS_LIGHTING,FALSE);d->SetRenderState(D3DRS_FOGENABLE,FALSE);d->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);
 d->SetRenderState(D3DRS_STENCILENABLE,FALSE);d->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE);d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);
 d->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);d->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);d->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
 d->SetRenderState(D3DRS_COLORWRITEENABLE,15);d->DrawPrimitiveUP(D3DPT_TRIANGLELIST,UINT(v.size()/3),v.data(),sizeof(V));
 if(hasFont)for(const auto& l:labels)font.Draw(d,l.x,l.y,l.text,l.color,l.right);
 raw->Apply();raw->Release();
}
}
