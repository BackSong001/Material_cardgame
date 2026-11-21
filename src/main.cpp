#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <string>
#include "game.hpp"

namespace {
using namespace material;
#include "palette.inc"
Game game;

constexpr int tile=200, detail=230, flip=231, board=232;
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
void label(HDC dc,RECT r,const std::wstring& s,int size,UINT align=DT_LEFT,COLORREF color=navy) {
    auto font=CreateFontW(-px(size),0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
    auto old=SelectObject(dc,font);
    SetTextColor(dc,color); SetBkMode(dc,TRANSPARENT);
    DrawTextW(dc,s.c_str(),-1,&r,align|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
    SelectObject(dc,old); DeleteObject(font);
}
std::wstring number(double v) {
    std::wostringstream s; s << std::setprecision(10) << v; return s.str();
}
void spectrumStops(Gdiplus::LinearGradientBrush& brush) {
    using namespace Gdiplus;
    Color stops[6];
    const COLORREF colors[]={weak,indigo,aqua,leaf,gold,strong};
    for(int i=0;i<6;++i) stops[i].SetFromCOLORREF(colors[i]);
    REAL positions[]={0,.2f,.4f,.6f,.8f,1};
    brush.SetInterpolationColors(stops,positions,6);
}
void chart(HDC dc,RECT area,const Card& c,bool large) {
    int l=area.left+px(large?85:12), r=area.right-px(large?48:12);
    int t=area.top+px(large?56:20), b=area.bottom-px(large?74:8);
    if(r<=l || b<=t) return;
    double lo=std::min(0.,*std::min_element(c.y.begin(),c.y.end()));
    double hi=std::max(0.,c.y[c.peak()]);
    if(hi==lo) hi=lo+1;
    constexpr int xMin=200, xMax=1000;
    auto x=[&](double wave){return l+(wave-xMin)/(xMax-xMin)*(r-l);};
    auto y=[&](double z){return b-int((z-lo)/(hi-lo)*(b-t));};
    line(dc,l,t,l,b,edge); line(dc,l,y(0),r,y(0),navy);
    if(large) {
        {
            using namespace Gdiplus;
            Graphics graphics(dc);
            graphics.TranslateTransform(REAL((area.left+l)/2),REAL((t+b)/2));
            graphics.RotateTransform(-90);
            FontFamily family(L"Segoe UI");
            Font font(family.IsAvailable()?&family:FontFamily::GenericSansSerif(),
                      REAL(px(15)),FontStyleBold,UnitPixel);
            StringFormat format;
            format.SetAlignment(StringAlignmentCenter);
            format.SetLineAlignment(StringAlignmentCenter);
            Color ink;
            ink.SetFromCOLORREF(navy);
            SolidBrush brush(ink);
            graphics.DrawString(L"Scattered Light Intensity",-1,&font,PointF(0,0),&format,&brush);
        }
        for(double value : {lo,(lo+hi)/2,hi}) {
            int tick=y(value);
            if(value!=0) line(dc,l,tick,r,tick,edge);
        }
        for(int wave=xMin;wave<=xMax;wave+=200) {
            int tick=std::lround(x(wave));
            if(wave!=xMin && wave!=xMax) line(dc,tick,t,tick,b,edge);
            label(dc,{tick-px(42),b+px(4),tick+px(42),b+px(26)},number(wave),14,DT_CENTER);
        }
        label(dc,{l,b+px(31),r,area.bottom},L"Incident Wavelength (nm)",16,DT_CENTER);
    }
    size_t p=c.peak();
    const bool peakVisible=c.x[p]>=xMin && c.x[p]<=xMax;
    int peakX=std::lround(x(c.x[p]));
    if(peakVisible) line(dc,peakX,y(c.y[p]),peakX,y(0),navy,1,PS_DOT);
    using namespace Gdiplus;
    {
        Graphics graphics(dc);
        graphics.SetClip(Rect(l,t-px(3),r-l+1,b-t+px(6)));
        graphics.SetSmoothingMode(SmoothingModeAntiAlias);
        std::vector<PointF> points;
        points.reserve(c.x.size());
        for(size_t i=0;i<c.x.size();++i)
            points.emplace_back(REAL(x(c.x[i])),
                                REAL(b-(c.y[i]-lo)/(hi-lo)*(b-t)));
        REAL top=points[p].Y;
        REAL bottom=REAL(y(*std::min_element(c.y.begin(),c.y.end())));
        LinearGradientBrush gradient(PointF(0,std::max(bottom,top+1)+1),PointF(0,top-1),Color::Black,Color::White);
        spectrumStops(gradient);
        Pen pen(&gradient,REAL(px(large?4:3)));
        pen.SetLineJoin(LineJoinRound);
        graphics.DrawLines(&pen,points.data(),static_cast<INT>(points.size()));
    }
    if(!peakVisible) return;
    int a=px(large?5:3);
    auto brush=SelectObject(dc,GetStockObject(DC_BRUSH)); SetDCBrushColor(dc,strong);
    Ellipse(dc,peakX-a,y(c.y[p])-a,peakX+a+1,y(c.y[p])+a+1); SelectObject(dc,brush);
    int width=std::min(r-l,px(large?180:115));
    int left=std::clamp(peakX-width/2,l,r-width);
    label(dc,{left,y(c.y[p])-px(large?32:21),left+width,y(c.y[p])-a},number(c.x[p])+L" nm",large?17:12,DT_CENTER);
}
std::wstring tileName(int i) {
    return (i<9?L"0":L"")+std::to_wstring(i+1)+L"   Scattering Spectrum";
}
void draw(const DRAWITEMSTRUCT& d) {
    RECT r=d.rcItem;
    HDC dc=d.hDC;
    if(d.CtlID==flip || d.CtlID==board) {
        bool primary=d.CtlID==flip, pressed=d.itemState&ODS_SELECTED;
        fill(dc,r,paper);
        auto brush=SelectObject(dc,GetStockObject(DC_BRUSH));
        auto pen=SelectObject(dc,GetStockObject(DC_PEN));
        SetDCBrushColor(dc,primary?(pressed?indigo:navy):(pressed?edge:card));
        SetDCPenColor(dc,primary?navy:edge);
        RoundRect(dc,r.left,r.top,r.right,r.bottom,px(18),px(18));
        SelectObject(dc,brush); SelectObject(dc,pen);
        wchar_t caption[64]{};
        GetWindowTextW(d.hwndItem,caption,64);
        label(dc,r,caption,16,DT_CENTER,primary?card:navy);
        if(d.itemState&ODS_FOCUS) { InflateRect(&r,-px(5),-px(5)); DrawFocusRect(dc,&r); }
        return;
    }
    bool small=d.CtlID>=tile && d.CtlID<tile+20;
    int i=small?int(d.CtlID)-tile:game.slot;
    if(i<0) return;
    const Card& c=cards[i];
    fill(dc,r,edge); InflateRect(&r,-1,-1); fill(dc,r,card);
    RECT heading=r;
    heading.left+=px(10); heading.right-=px(10); heading.bottom=r.top+px(small?28:64);
    if(small) {
        label(dc,heading,tileName(i).substr(0,2),14);
        heading.left+=px(24);
        label(dc,heading,L"Scattering Spectrum",14,DT_CENTER);
    } else label(dc,heading,game.back?L"Material":L"Scattering Spectrum",26,DT_CENTER);
    RECT plot=r; plot.top=heading.bottom+(small?px(7):0);
    if(!small && game.back) label(dc,plot,names[c.kind],36,DT_CENTER);
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
    bool grid=game.slot<0;
    int gridBottom=bottom;
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
    SetDlgItemTextW(win,flip,game.back?L"View spectrum":L"Show answer");
    if(!grid) {
        const auto& c=cards[game.slot];
        std::wstring desc=game.back?names[c.kind]:L"Scattering Spectrum, peak "+number(c.x[c.peak()])+L" nm";
        SetDlgItemTextW(win,detail,desc.c_str());
    }
    RedrawWindow(win,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN);
}
INT_PTR CALLBACK dialog(HWND h,UINT msg,WPARAM wp,LPARAM lp) {
    switch(msg) {
    case WM_INITDIALOG: {
        win=h;
        auto add=[&](int id,const wchar_t* cls,const wchar_t* title,DWORD style){
            HWND ctl=CreateWindowW(cls,title,WS_CHILD|style,0,0,0,0,h,reinterpret_cast<HMENU>(INT_PTR(id)),nullptr,nullptr);
            SendMessageW(ctl,WM_SETFONT,SendMessageW(h,WM_GETFONT,0,0),FALSE);
        };
        for(int i=0;i<20;++i) add(tile+i,L"BUTTON",L"",BS_OWNERDRAW|WS_TABSTOP);
        add(detail,L"STATIC",L"",SS_OWNERDRAW);
        add(flip,L"BUTTON",L"",BS_OWNERDRAW|WS_TABSTOP); add(board,L"BUTTON",L"Back to board",BS_OWNERDRAW|WS_TABSTOP);
        ShowWindow(h,SW_SHOWMAXIMIZED); layout(); SetFocus(GetDlgItem(h,tile)); return FALSE;
    }
    case WM_GETMINMAXINFO: {
        auto limits=reinterpret_cast<MINMAXINFO*>(lp);
        limits->ptMinTrackSize={820,540};
        return TRUE;
    }
    case WM_SIZE: if(win && GetDlgItem(h,board)) layout(); return TRUE;
    case WM_DRAWITEM: draw(*reinterpret_cast<DRAWITEMSTRUCT*>(lp)); return TRUE;
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC dc=BeginPaint(h,&ps); RECT r; GetClientRect(h,&r);
        fill(dc,r,paper); RECT bar=r; bar.bottom=px(68); fill(dc,bar,navy);
        label(dc,{px(20),0,r.right/2,bar.bottom},L"Material Classifier",30,DT_LEFT,card);
        int right=r.right-px(20), center=bar.bottom/2, width=px(110);
        label(dc,{right-width-px(150),0,right-width-px(38),bar.bottom},L"Intensity",14,DT_RIGHT,card);
        label(dc,{right-width-px(36),0,right-width,bar.bottom},L"Low",13,DT_CENTER,card);
        label(dc,{right-px(28),0,right,bar.bottom},L"High",13,DT_RIGHT,card);
        {
            using namespace Gdiplus;
            Graphics g(dc);
            REAL left=REAL(right-width);
            LinearGradientBrush ramp(PointF(left,0),PointF(REAL(right-px(32)),0),Color::Black,Color::White);
            spectrumStops(ramp);
            g.FillRectangle(&ramp,left,REAL(center-px(4)),REAL(width-px(32)),REAL(px(8)));
        }
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
    Gdiplus::GdiplusStartupInput input;
    ULONG_PTR token;
    if(Gdiplus::GdiplusStartup(&token,&input,nullptr)!=Gdiplus::Ok) {
        MessageBoxW(nullptr,L"Unable to initialize graphics.",L"Material Classifier",MB_ICONERROR);
        return 1;
    }
    int result=DialogBoxW(instance,MAKEINTRESOURCEW(100),nullptr,dialog)==-1;
    Gdiplus::GdiplusShutdown(token);
    return result;
}
