#include <windows.h>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <string>
#include "game.hpp"

namespace {
using namespace material;
#include "palette.inc"
Game game;
std::mt19937 rng;
constexpr int tile=200, detail=230, flip=231, board=232, restart=233;
HWND win;
int scale=100;
int px(int n) { return MulDiv(n,scale,100); }
void fill(HDC dc,RECT r,COLORREF c) {
    SetDCBrushColor(dc,c);
    FillRect(dc,&r,static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
}
void line(HDC dc,int x,int y,int u,int v,COLORREF c,int width=1,int style=PS_SOLID) {
    HPEN pen=CreatePen(style,width,c);
    auto old=SelectObject(dc,pen);
    MoveToEx(dc,x,y,nullptr); LineTo(dc,u,v);
    SelectObject(dc,old); DeleteObject(pen);
}
void label(HDC dc,RECT r,const std::wstring& s,int size,bool bold=false,UINT align=DT_LEFT,COLORREF color=navy) {
    auto font=CreateFontW(-px(size),0,0,0,bold?FW_BOLD:FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Malgun Gothic");
    auto old=SelectObject(dc,font);
    SetTextColor(dc,color); SetBkMode(dc,TRANSPARENT);
    DrawTextW(dc,s.c_str(),-1,&r,align|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
    SelectObject(dc,old); DeleteObject(font);
}
std::wstring number(double v) {
    std::wostringstream s; s << std::setprecision(10) << v; return s.str();
}
void chart(HDC dc,RECT area,const Card& c,bool large) {
    int l=area.left+px(large?85:12), r=area.right-px(large?48:12);
    int t=area.top+px(large?56:20), b=area.bottom-px(large?74:8);
    if(r<=l || b<=t) return;
    double lo=std::min(0.,*std::min_element(c.y.begin(),c.y.end()));
    double hi=std::max(0.,c.y[c.peak()]);
    if(hi==lo) hi=lo+1;
    auto x=[&](size_t i){return l+int((c.x[i]-c.x.front())/(c.x.back()-c.x.front())*(r-l));};
    auto y=[&](double z){return b-int((z-lo)/(hi-lo)*(b-t));};
    line(dc,l,t,l,b,edge); line(dc,l,y(0),r,y(0),navy);
    if(large) {
        label(dc,{l,area.top,r,t},L"산란광 세기"+std::wstring(demo?L" (a.u.)":L""),15);
        for(double value : {lo,(lo+hi)/2,hi}) {
            int tick=y(value);
            label(dc,{area.left,tick-px(12),l-px(7),tick+px(12)},number(value),13,false,DT_RIGHT);
            if(value!=0) line(dc,l,tick,r,tick,edge);
        }
        for(size_t i : {size_t(0),c.x.size()-1})
            label(dc,{x(i)-px(47),b+px(4),x(i)+px(47),b+px(26)},number(c.x[i]),14,false,DT_CENTER);
        label(dc,{l,b+px(31),r,area.bottom},L"입사광 파장 (nm)",16,false,DT_CENTER);
    }
    size_t p=c.peak();
    line(dc,x(p),y(c.y[p]),x(p),y(0),navy,1,PS_DOT);
    HPEN pen=CreatePen(PS_SOLID,px(large?3:2),navy);
    auto old=SelectObject(dc,pen); MoveToEx(dc,x(0),y(c.y[0]),nullptr);
    for(size_t i=1;i<c.x.size();++i) LineTo(dc,x(i),y(c.y[i]));
    SelectObject(dc,old); DeleteObject(pen);
    int a=px(large?5:3);
    auto brush=SelectObject(dc,GetStockObject(DC_BRUSH)); SetDCBrushColor(dc,amber);
    Ellipse(dc,x(p)-a,y(c.y[p])-a,x(p)+a+1,y(c.y[p])+a+1); SelectObject(dc,brush);
    int width=std::min(r-l,px(large?180:115));
    int left=std::clamp(x(p)-width/2,l,r-width);
    label(dc,{left,y(c.y[p])-px(large?32:21),left+width,y(c.y[p])-a},number(c.x[p])+L" nm",large?17:12,true,DT_CENTER);
}
std::wstring tileName(int i) {
    return (i<9?L"0":L"")+std::to_wstring(i+1)+L"   산란광 스펙트럼 세기"+(game.seen[i]?L"   확인함":L"");
}
void draw(const DRAWITEMSTRUCT& d) {
    RECT r=d.rcItem;
    HDC dc=d.hDC;
    bool small=d.CtlID>=tile && d.CtlID<tile+20;
    int i=small?int(d.CtlID)-tile:game.slot;
    if(i<0) return;
    const Card& c=cards[game.order[i]];
    fill(dc,r,edge); InflateRect(&r,-1,-1); fill(dc,r,card);
    RECT heading=r;
    heading.left+=px(10); heading.right-=px(10); heading.bottom=r.top+px(small?28:64);
    if(small) {
        label(dc,heading,tileName(i).substr(0,2),14,true);
        label(dc,heading,L"산란광 스펙트럼 세기",14,true,DT_CENTER);
        if(game.seen[i]) label(dc,{heading.left,heading.bottom,heading.right,heading.bottom+px(16)},L"확인함",12,true,DT_RIGHT);
    } else label(dc,heading,game.back?L"정답 물질":L"산란광 스펙트럼 세기",26,true,DT_CENTER);
    RECT plot=r; plot.top=heading.bottom+(small?px(7):0);
    if(!small && game.back) label(dc,plot,names[c.kind],36,true,DT_CENTER);
    else chart(dc,plot,c,!small);
    if(d.itemState&ODS_FOCUS) { InflateRect(&r,-3,-3); DrawFocusRect(dc,&r); }
}
void place(int id,int x,int y,int w,int h,bool visible) {
    HWND ctl=GetDlgItem(win,id);
    if(visible) MoveWindow(ctl,x,y,w,h,TRUE);
    ShowWindow(ctl,visible?SW_SHOW:SW_HIDE);
}
void layout() {
    RECT r; GetClientRect(win,&r);
    int w=r.right,h=r.bottom;
    scale=std::clamp(std::min(w*100/1100,h*100/640),70,145);
    int m=px(20),gap=px(8),top=px(84),bottom=h-m;
    bool grid=game.slot<0, done=game.count()==20;
    int gridBottom=bottom-(done?px(48):0);
    for(int i=0;i<20;++i) {
        int col=i%5,row=i/5;
        int stepX=w-2*m+gap,stepY=gridBottom-top+gap;
        place(tile+i,m+col*stepX/5,top+row*stepY/4,stepX/5-gap,stepY/4-gap,grid);
        SetDlgItemTextW(win,tile+i,tileName(i).c_str());
    }
    int left=std::max(m,(w-px(960))/2);
    place(detail,left,top,w-2*left,bottom-top-px(54),!grid);
    place(flip,w/2-px(174),bottom-px(40),px(168),px(40),!grid);
    place(board,w/2+px(6),bottom-px(40),px(168),px(40),!grid);
    place(restart,w/2-px(90),bottom-px(40),px(180),px(40),grid&&done);
    SetDlgItemTextW(win,flip,game.back?L"앞면 다시 보기":L"정답 보기");
    if(!grid) {
        const auto& c=cards[game.order[game.slot]];
        std::wstring desc=game.back?names[c.kind]:L"산란광 스펙트럼 세기, 피크 "+number(c.x[c.peak()])+L" nm";
        SetDlgItemTextW(win,detail,desc.c_str());
    }
    RedrawWindow(win,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN);
}
INT_PTR CALLBACK dialog(HWND h,UINT msg,WPARAM wp,LPARAM lp) {
    switch(msg) {
    case WM_INITDIALOG: {
        win=h; game.start(rng);
        auto add=[&](int id,const wchar_t* cls,const wchar_t* title,DWORD style){
            HWND ctl=CreateWindowW(cls,title,WS_CHILD|style,0,0,0,0,h,reinterpret_cast<HMENU>(INT_PTR(id)),nullptr,nullptr);
            SendMessageW(ctl,WM_SETFONT,SendMessageW(h,WM_GETFONT,0,0),FALSE);
        };
        for(int i=0;i<20;++i) add(tile+i,L"BUTTON",L"",BS_OWNERDRAW|WS_TABSTOP);
        add(detail,L"STATIC",L"",SS_OWNERDRAW);
        add(flip,L"BUTTON",L"",WS_TABSTOP); add(board,L"BUTTON",L"보드로 돌아가기",WS_TABSTOP);
        add(restart,L"BUTTON",L"다시 섞어 시작",WS_TABSTOP);
        ShowWindow(h,SW_SHOWMAXIMIZED); layout(); SetFocus(GetDlgItem(h,tile)); return FALSE;
    }
    case WM_SIZE: if(win && GetDlgItem(h,restart)) layout(); return TRUE;
    case WM_DRAWITEM: draw(*reinterpret_cast<DRAWITEMSTRUCT*>(lp)); return TRUE;
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC dc=BeginPaint(h,&ps); RECT r; GetClientRect(h,&r);
        fill(dc,r,paper); RECT bar=r; bar.bottom=px(68); fill(dc,bar,navy);
        label(dc,{px(20),0,r.right/2,bar.bottom},L"Material Classifier",30,true,DT_LEFT,card);
        std::wstring status=demo?L"학습용 모의 데이터    ":L"";
        status+=L"정답 확인 "+std::to_wstring(game.count())+L" / 20";
        label(dc,{r.right/2,0,r.right-px(20),bar.bottom},status,16,false,DT_RIGHT,card);
        COLORREF strip[]={coral,amber,green,blue,violet};
        for(int i=0;i<5;++i) fill(dc,{r.right*i/5,bar.bottom,r.right*(i+1)/5,bar.bottom+px(4)},strip[i]);
        EndPaint(h,&ps); return TRUE;
    }
    case WM_COMMAND: {
        if(HIWORD(wp)!=BN_CLICKED) return FALSE;
        int id=LOWORD(wp),focus=flip;
        if(id>=tile && id<tile+20) game.select(id-tile);
        else if(id==flip && game.slot>=0) game.flip();
        else if((id==board || id==IDCANCEL) && game.slot>=0) { focus=tile+game.slot; game.select(-1); }
        else if(id==restart && game.count()==20) { game.start(rng); focus=tile; }
        else if(id==IDCANCEL) { EndDialog(h,0); return TRUE; }
        else return FALSE;
        layout(); SetFocus(GetDlgItem(h,focus)); return TRUE;
    }
    case WM_CLOSE: EndDialog(h,0); return TRUE;
    }
    return FALSE;
}
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,PWSTR,int) {
    SetProcessDPIAware();
    rng.seed(GetTickCount());
    return DialogBoxW(instance,MAKEINTRESOURCEW(100),nullptr,dialog)==-1;
}
