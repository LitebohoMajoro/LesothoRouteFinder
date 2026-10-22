#include "MainWindow.h"
#ifdef _WIN32
#include <commctrl.h>
#include <fstream>
#include <string>
#include <climits>
#include <algorithm>

namespace {
constexpr int ID_INPUT=101, ID_INSERT=102, ID_SEARCH=103, ID_REMOVE=104,
 ID_OPEN=105, ID_CLOSED=106, ID_STEP=107, ID_PLAY=108, ID_PAUSE=109,
 ID_CLEAR=110, ID_SAVE=111, ID_SPEED=112, ID_BACK=113, ID_MORE=114, ID_OPEN_CAP=115, ID_CLOSED_CAP=116;
constexpr UINT TIMER_ID=1;
std::wstring widen(const std::string& s){return std::wstring(s.begin(),s.end());}
void font(HWND h){SendMessageW(h,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),TRUE);}
void drawButton(const DRAWITEMSTRUCT* item){
 HDC dc=item->hDC;RECT r=item->rcItem;int id=(int)item->CtlID;
 COLORREF fill=RGB(24,33,52),border=RGB(35,47,70),text=RGB(226,232,240);
 if(id==ID_INSERT||id==ID_OPEN||id==ID_CLOSED){fill=RGB(34,211,238);border=fill;text=RGB(8,12,22);}
 if(id==ID_PLAY){fill=RGB(52,211,153);border=fill;text=RGB(8,12,22);}
 if(item->itemState&ODS_SELECTED){fill=RGB(71,85,105);border=RGB(34,211,238);}
 HBRUSH bg=CreateSolidBrush(RGB(17,24,39));FillRect(dc,&r,bg);DeleteObject(bg);
 HBRUSH b=CreateSolidBrush(fill);HPEN p=CreatePen(PS_SOLID,1,border);auto ob=SelectObject(dc,b);auto op=SelectObject(dc,p);RoundRect(dc,r.left,r.top,r.right,r.bottom,12,12);SelectObject(dc,ob);SelectObject(dc,op);DeleteObject(b);DeleteObject(p);
 wchar_t label[96]{};GetWindowTextW(item->hwndItem,label,96);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,text);auto old=SelectObject(dc,GetStockObject(DEFAULT_GUI_FONT));DrawTextW(dc,label,-1,&r,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);SelectObject(dc,old);
}
}

int MainWindow::create(HINSTANCE instance){
 INITCOMMONCONTROLSEX ic{sizeof(ic),ICC_BAR_CLASSES}; InitCommonControlsEx(&ic);
 WNDCLASSW wc{}; wc.lpfnWndProc=proc; wc.hInstance=instance; wc.lpszClassName=L"HashingVisualizerWindow";
 wc.hCursor=LoadCursor(nullptr,IDC_ARROW); wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);
 RegisterClassW(&wc);
 hwnd_=CreateWindowW(wc.lpszClassName,L"Hashing Visualizer",WS_OVERLAPPEDWINDOW|WS_VISIBLE,
  CW_USEDEFAULT,CW_USEDEFAULT,1180,800,nullptr,nullptr,instance,this);
 return hwnd_?0:1;
}
LRESULT CALLBACK MainWindow::proc(HWND w,UINT m,WPARAM a,LPARAM b){
 MainWindow* self=(MainWindow*)GetWindowLongPtrW(w,GWLP_USERDATA);
 if(m==WM_NCCREATE){self=(MainWindow*)((CREATESTRUCTW*)b)->lpCreateParams;SetWindowLongPtrW(w,GWLP_USERDATA,(LONG_PTR)self);self->hwnd_=w;}
 return self?self->handle(m,a,b):DefWindowProcW(w,m,a,b);
}
void MainWindow::log(const std::string& s){std::ofstream f("hash_results.txt",std::ios::app);if(f)f<<s<<'\n';else MessageBoxW(hwnd_,L"Could not write hash_results.txt.",L"Save error",MB_OK|MB_ICONERROR);}
void MainWindow::updateStatus(){
 int count=mode_==Mode::Open?open_.size():closed_.size(), cap=mode_==Mode::Open?open_.capacity():closed_.capacity();
 std::wstring s=L"Capacity  "+std::to_wstring(cap)+L"       Values  "+std::to_wstring(count)+L"       Load factor  "+std::to_wstring((double)count/cap);
 if(status_)SetWindowTextW(status_,s.c_str());
 if(hwnd_)InvalidateRect(hwnd_,nullptr,FALSE);
}
bool MainWindow::exportCurrent(){std::string error;bool ok=mode_==Mode::Open?HashTableFileExporter::writeOpen(open_,"output/open_hash_table.txt",&error):HashTableFileExporter::writeClosed(closed_,"output/closed_hash_table.txt",&error);if(!ok)MessageBoxW(hwnd_,widen(error).c_str(),L"Table export warning",MB_OK|MB_ICONWARNING);return ok;}
void MainWindow::run(int action){
 if(!events_.empty())return;
 wchar_t text[64]{};GetWindowTextW(input_,text,64);wchar_t* end=nullptr;long long parsed=wcstoll(text,&end,10);
 if(end==text||*end||parsed<INT_MIN||parsed>INT_MAX){SetWindowTextW(status_,L"Enter a valid 32-bit integer to continue.");SetFocus(input_);return;}
 int value=(int)parsed, cap=mode_==Mode::Open?open_.capacity():closed_.capacity(), h=value%cap;if(h<0)h+=cap;
 std::vector<Event> ev;bool ok=false;
 if(mode_==Mode::Open){
  ev.push_back({"Hash: "+std::to_string(value)+" % "+std::to_string(cap)+" = "+std::to_string(h),h});
  ev.push_back({"Inspect bucket "+std::to_string(h),h});
  if(action==ID_INSERT){bool collision=!open_.buckets()[h].values().empty();if(collision)ev.push_back({"Collision: open hashing adds this value to the bucket chain",h,true});ok=open_.add(value);ev.push_back({ok?"Inserted "+std::to_string(value)+" into the chain":"Duplicate value; table unchanged",h,!ok});}
  else if(action==ID_SEARCH){ok=open_.contains(value);ev.push_back({ok?"Value found in this chain":"Value not found in this chain",h,!ok});}
  else {ok=open_.remove(value);ev.push_back({ok?"Removed value from its chain":"Value not found; table unchanged",h,!ok});}
 } else {
  if(action==ID_INSERT)ok=closed_.add(value,&ev);else if(action==ID_SEARCH)ok=closed_.contains(value,&ev);else ok=closed_.remove(value,&ev);
  if(!ok&&action==ID_INSERT)ev.push_back({"Duplicate or table is full; table unchanged",-1,true});
  else if(!ok)ev.push_back({"Value not found; table unchanged",-1,true});
 }
 if(ok&&(action==ID_INSERT||action==ID_REMOVE))exportCurrent();
 for(const auto& e:ev){events_.push_back(e);log(e.text);}
 if(!events_.empty()){current_={"Starting guided operation",-1};SetTimer(hwnd_,TIMER_ID,speed_,nullptr);}
 updateStatus();SetFocus(input_);SendMessageW(input_,EM_SETSEL,0,-1);
}
LRESULT MainWindow::handle(UINT m,WPARAM a,LPARAM b){
 switch(m){
 case WM_DRAWITEM:drawButton((DRAWITEMSTRUCT*)b);return TRUE;
 case WM_CTLCOLORSTATIC:{HDC dc=(HDC)a;SetTextColor(dc,RGB(148,163,184));SetBkColor(dc,RGB(10,14,24));static HBRUSH brush=CreateSolidBrush(RGB(10,14,24));return (LRESULT)brush;}
 case WM_CTLCOLOREDIT:{HDC dc=(HDC)a;SetTextColor(dc,RGB(226,232,240));SetBkColor(dc,RGB(24,33,52));static HBRUSH brush=CreateSolidBrush(RGB(24,33,52));return (LRESULT)brush;}
 case WM_CREATE:{
  input_=CreateWindowW(L"EDIT",L"",WS_CHILD|WS_VISIBLE|WS_BORDER|ES_AUTOHSCROLL,112,105,96,34,hwnd_,(HMENU)ID_INPUT,nullptr,nullptr);
  status_=CreateWindowW(L"STATIC",L"",WS_CHILD|WS_VISIBLE,24,182,900,26,hwnd_,nullptr,nullptr,nullptr);
  const struct {const wchar_t* name;int id;int x;int width;} buttons[]={{L"Insert value",ID_INSERT,220,142},{L"Search",ID_SEARCH,372,86},{L"Remove",ID_REMOVE,466,86},{L"More actions",ID_MORE,560,120},{L"Step",ID_STEP,700,78},{L"Pause",ID_PAUSE,786,78},{L"Resume",ID_PLAY,872,86},{L"Back",ID_BACK,24,80}};
  for(auto& v:buttons){DWORD style=WS_CHILD|WS_VISIBLE|BS_OWNERDRAW;if(v.id==ID_BACK)style=WS_CHILD|BS_OWNERDRAW;HWND h=CreateWindowW(L"BUTTON",v.name,style,v.x,105,v.width,34,hwnd_,(HMENU)(INT_PTR)v.id,nullptr,nullptr);font(h);}
  HWND clear=CreateWindowW(L"BUTTON",L"Clear",WS_CHILD|BS_OWNERDRAW,560,145,120,32,hwnd_,(HMENU)ID_CLEAR,nullptr,nullptr);font(clear);
  HWND save=CreateWindowW(L"BUTTON",L"Save snapshot",WS_CHILD|BS_OWNERDRAW,688,145,120,32,hwnd_,(HMENU)ID_SAVE,nullptr,nullptr);font(save);
  HWND openCap=CreateWindowW(L"EDIT",L"11",WS_CHILD|WS_VISIBLE|WS_BORDER|ES_AUTOHSCROLL,215,452,70,32,hwnd_,(HMENU)ID_OPEN_CAP,nullptr,nullptr);font(openCap);
  HWND closedCap=CreateWindowW(L"EDIT",L"11",WS_CHILD|WS_VISIBLE|WS_BORDER|ES_AUTOHSCROLL,715,452,70,32,hwnd_,(HMENU)ID_CLOSED_CAP,nullptr,nullptr);font(closedCap);
  HWND openCard=CreateWindowW(L"BUTTON",L"Open Hashing",WS_CHILD|WS_VISIBLE|BS_OWNERDRAW,95,500,170,36,hwnd_,(HMENU)ID_OPEN,nullptr,nullptr);font(openCard);
  HWND closedCard=CreateWindowW(L"BUTTON",L"Closed Hashing",WS_CHILD|WS_VISIBLE|BS_OWNERDRAW,595,500,190,36,hwnd_,(HMENU)ID_CLOSED,nullptr,nullptr);font(closedCard);
  CreateWindowW(TRACKBAR_CLASSW,L"",WS_CHILD|WS_VISIBLE|TBS_NOTICKS,936,105,170,34,hwnd_,(HMENU)ID_SPEED,nullptr,nullptr);
  SendDlgItemMessageW(hwnd_,ID_SPEED,TBM_SETRANGE,TRUE,MAKELONG(120,1000));SendDlgItemMessageW(hwnd_,ID_SPEED,TBM_SETPOS,TRUE,speed_);
  mode_=Mode::Open;for(int id:{ID_BACK,ID_INPUT,ID_INSERT,ID_SEARCH,ID_REMOVE,ID_MORE,ID_STEP,ID_PLAY,ID_PAUSE,ID_SPEED,ID_CLEAR,ID_SAVE})ShowWindow(GetDlgItem(hwnd_,id),SW_HIDE);ShowWindow(status_,SW_HIDE);updateStatus();return 0;
 }
 case WM_SIZE:InvalidateRect(hwnd_,nullptr,FALSE);return 0;
 case WM_COMMAND:switch(LOWORD(a)){
 case ID_INSERT:case ID_SEARCH:case ID_REMOVE:run(LOWORD(a));return 0;
 case ID_OPEN:case ID_CLOSED:{if(!events_.empty())return 0;int capId=LOWORD(a)==ID_OPEN?ID_OPEN_CAP:ID_CLOSED_CAP;wchar_t capText[64]{};GetWindowTextW(GetDlgItem(hwnd_,capId),capText,64);wchar_t* end=nullptr;long long n=wcstoll(capText,&end,10);if(end==capText||*end||n<1||n>1000){MessageBoxW(hwnd_,L"Enter a whole number from 1 to 1000 for the initial capacity.",L"Invalid capacity",MB_OK|MB_ICONWARNING);SetFocus(GetDlgItem(hwnd_,capId));return 0;}mode_=LOWORD(a)==ID_OPEN?Mode::Open:Mode::Closed;int requested=(int)n;welcome_=false;if(mode_==Mode::Open)open_=HashSetOpen(requested);else{closed_=HashSetClosed(requested);if(closed_.capacity()!=requested)MessageBoxW(hwnd_,(L"Closed Hashing requires a prime capacity; using "+std::to_wstring(closed_.capacity())+L".").c_str(),L"Capacity adjusted",MB_OK|MB_ICONINFORMATION);}exportCurrent();for(int id:{ID_OPEN,ID_CLOSED,ID_OPEN_CAP,ID_CLOSED_CAP})ShowWindow(GetDlgItem(hwnd_,id),SW_HIDE);for(int id:{ID_BACK,ID_INPUT,ID_INSERT,ID_SEARCH,ID_REMOVE,ID_MORE,ID_STEP,ID_PLAY,ID_PAUSE,ID_SPEED})ShowWindow(GetDlgItem(hwnd_,id),SW_SHOW);ShowWindow(status_,SW_SHOW);updateStatus();return 0;}
 case ID_BACK:if(!events_.empty())return 0;welcome_=true;ShowWindow(status_,SW_HIDE);ShowWindow(GetDlgItem(hwnd_,ID_BACK),SW_HIDE);for(int id:{ID_INPUT,ID_INSERT,ID_SEARCH,ID_REMOVE,ID_MORE,ID_STEP,ID_PLAY,ID_PAUSE,ID_SPEED,ID_CLEAR,ID_SAVE})ShowWindow(GetDlgItem(hwnd_,id),SW_HIDE);for(int id:{ID_OPEN,ID_CLOSED,ID_OPEN_CAP,ID_CLOSED_CAP})ShowWindow(GetDlgItem(hwnd_,id),SW_SHOW);InvalidateRect(hwnd_,nullptr,TRUE);return 0;
 case ID_MORE:{bool show=IsWindowVisible(GetDlgItem(hwnd_,ID_CLEAR));ShowWindow(GetDlgItem(hwnd_,ID_CLEAR),show?SW_HIDE:SW_SHOW);ShowWindow(GetDlgItem(hwnd_,ID_SAVE),show?SW_HIDE:SW_SHOW);return 0;}
 case ID_STEP:if(!events_.empty()){current_=events_.front();events_.pop_front();if(events_.empty())KillTimer(hwnd_,TIMER_ID);InvalidateRect(hwnd_,nullptr,FALSE);}return 0;
 case ID_PLAY:if(!events_.empty())SetTimer(hwnd_,TIMER_ID,speed_,nullptr);return 0;
 case ID_PAUSE:KillTimer(hwnd_,TIMER_ID);return 0;
 case ID_CLEAR:if(!events_.empty())return 0;if(MessageBoxW(hwnd_,L"Clear the current table?",L"Clear table",MB_YESNO|MB_ICONQUESTION)==IDYES){int cap=mode_==Mode::Open?open_.capacity():closed_.capacity();if(mode_==Mode::Open)open_=HashSetOpen(cap);else closed_=HashSetClosed(cap);current_={};exportCurrent();updateStatus();}return 0;
 case ID_SAVE:{std::ofstream f("hash_results.txt",std::ios::app);if(!f){MessageBoxW(hwnd_,L"Could not open hash_results.txt.",L"Save error",MB_OK|MB_ICONERROR);return 0;}f<<"\n=== "<<(mode_==Mode::Open?"Open":"Closed")<<" table snapshot ===\n";if(mode_==Mode::Open){for(int i=0;i<open_.capacity();++i){f<<i<<": ";for(int v:open_.buckets()[i].values())f<<v<<" -> ";f<<"NULL\n";}}else for(int i=0;i<closed_.capacity();++i){const auto&s=closed_.slots()[i];f<<i<<": "<<(s.state==SlotState::Empty?"EMPTY":s.state==SlotState::Deleted?"DELETED":std::to_string(s.value))<<'\n';}if(!f){MessageBoxW(hwnd_,L"The snapshot could not be written.",L"Save error",MB_OK|MB_ICONERROR);return 0;}MessageBoxW(hwnd_,L"Snapshot appended to hash_results.txt.",L"Saved",MB_OK|MB_ICONINFORMATION);return 0;}
 }break;
 case WM_HSCROLL:if((HWND)b==GetDlgItem(hwnd_,ID_SPEED)){speed_=(int)SendDlgItemMessageW(hwnd_,ID_SPEED,TBM_GETPOS,0,0);if(speed_<120)speed_=120;return 0;}break;
 case WM_TIMER:if(a==TIMER_ID){if(!events_.empty()){current_=events_.front();events_.pop_front();InvalidateRect(hwnd_,nullptr,FALSE);}if(events_.empty())KillTimer(hwnd_,TIMER_ID);}return 0;
 case WM_PAINT:{PAINTSTRUCT ps;HDC dc=BeginPaint(hwnd_,&ps);draw(dc);EndPaint(hwnd_,&ps);return 0;}
 case WM_DESTROY:KillTimer(hwnd_,TIMER_ID);events_.clear();PostQuitMessage(0);return 0;
 }
 return DefWindowProcW(hwnd_,m,a,b);
}
void MainWindow::draw(HDC dc){
 RECT r;GetClientRect(hwnd_,&r);HBRUSH bg=CreateSolidBrush(RGB(10,14,24));FillRect(dc,&r,bg);DeleteObject(bg);SetBkMode(dc,TRANSPARENT);
 HFONT title=CreateFontW(27,0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
 HFONT body=CreateFontW(17,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
 auto old=(HFONT)SelectObject(dc,title);
 RECT header{0,0,r.right,80};HBRUSH navy=CreateSolidBrush(RGB(13,19,33));FillRect(dc,&header,navy);DeleteObject(navy);
 SelectObject(dc,title);SetTextColor(dc,RGB(226,232,240));TextOutW(dc,30,12,L"HASHING ",8);SetTextColor(dc,RGB(34,211,238));TextOutW(dc,30+124,12,L"VISUALIZER",10);
 SelectObject(dc,body);SetTextColor(dc,RGB(148,163,184));TextOutW(dc,30,48,L"Explore hash tables and see how collisions are handled.",54);
 if(welcome_){
  RECT cards[2]={{65,190,535,510},{565,190,1035,510}};
  for(int i=0;i<2;++i){HBRUSH p=CreateSolidBrush(RGB(17,24,39));FillRect(dc,&cards[i],p);DeleteObject(p);HPEN pen=CreatePen(PS_SOLID,1,RGB(35,47,70));auto op=(HPEN)SelectObject(dc,pen);auto ob=(HBRUSH)SelectObject(dc,GetStockObject(NULL_BRUSH));RoundRect(dc,cards[i].left,cards[i].top,cards[i].right,cards[i].bottom,16,16);SelectObject(dc,ob);SelectObject(dc,op);DeleteObject(pen);}
  SelectObject(dc,title);SetTextColor(dc,RGB(226,232,240));TextOutW(dc,95,224,L"Open Hashing",13);TextOutW(dc,595,224,L"Closed Hashing",14);
  SelectObject(dc,body);SetTextColor(dc,RGB(148,163,184));TextOutW(dc,95,276,L"Collisions join a linked list in the same bucket.",48);TextOutW(dc,595,276,L"Collisions probe the next available table slot.",47);TextOutW(dc,95,458,L"Initial capacity:",17);TextOutW(dc,595,458,L"Initial capacity:",17);
  SetTextColor(dc,RGB(34,211,238));TextOutW(dc,95,414,L"Separate chaining",17);TextOutW(dc,595,414,L"Linear probing",14);
  SelectObject(dc,old);DeleteObject(title);DeleteObject(body);return;
 }
 std::wstring heading=mode_==Mode::Open?L"OPEN HASHING  |  Separate chaining":L"CLOSED HASHING  |  Linear probing";SelectObject(dc,title);SetTextColor(dc,RGB(226,232,240));TextOutW(dc,24,216,heading.c_str(),(int)heading.size());
 SelectObject(dc,body);SetTextColor(dc,RGB(148,163,184));int width=r.right-48,cap=mode_==Mode::Open?open_.capacity():closed_.capacity();int cols=std::max(1,std::min(cap,width/146));int cell=width/cols;int top=252;
 for(int i=0;i<cap;++i){int x=24+(i%cols)*cell,y=top+(i/cols)*96;RECT box{x,y,x+cell-10,y+76};COLORREF color=i==current_.index?(current_.warning?RGB(72,25,35):RGB(10,54,70)):RGB(17,24,39);HBRUSH p=CreateSolidBrush(color);FillRect(dc,&box,p);DeleteObject(p);HPEN pen=CreatePen(PS_SOLID,1,i==current_.index?(current_.warning?RGB(248,113,113):RGB(34,211,238)):RGB(35,47,70));auto op=(HPEN)SelectObject(dc,pen);auto ob=(HBRUSH)SelectObject(dc,GetStockObject(NULL_BRUSH));RoundRect(dc,box.left,box.top,box.right,box.bottom,12,12);SelectObject(dc,ob);SelectObject(dc,op);DeleteObject(pen);SetTextColor(dc,RGB(148,163,184));std::wstring idx=L"Bucket "+std::to_wstring(i);TextOutW(dc,x+11,y+7,idx.c_str(),(int)idx.size());std::wstring val;if(mode_==Mode::Open){auto nodes=open_.buckets()[i].values();val=nodes.empty()?L"Empty":L"";for(size_t n=0;n<nodes.size();++n){if(n)val+=L"  →  ";val+=std::to_wstring(nodes[n]);}}else{auto s=closed_.slots()[i];val=s.state==SlotState::Empty?L"Empty":s.state==SlotState::Deleted?L"Deleted":std::to_wstring(s.value);}SetTextColor(dc,RGB(226,232,240));TextOutW(dc,x+11,y+36,val.c_str(),(int)val.size());}
 int y=top+((cap+cols-1)/cols)*96+20;RECT panel{24,y,r.right-24,y+104};HBRUSH p=CreateSolidBrush(RGB(17,24,39));FillRect(dc,&panel,p);DeleteObject(p);SetTextColor(dc,RGB(34,211,238));std::wstring label=L"GUIDED STEP";TextOutW(dc,42,y+15,label.c_str(),(int)label.size());SetTextColor(dc,RGB(226,232,240));std::wstring line=widen(current_.text.empty()?"Enter a value and choose Insert value to begin.":current_.text);TextOutW(dc,42,y+49,line.c_str(),(int)line.size());
 SelectObject(dc,old);DeleteObject(title);DeleteObject(body);
}
#endif
