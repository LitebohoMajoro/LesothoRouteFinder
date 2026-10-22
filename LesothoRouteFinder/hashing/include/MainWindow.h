#pragma once
#ifdef _WIN32
#include <windows.h>
#include "HashSetOpen.h"
#include "HashSetClosed.h"
#include "HashTableFileExporter.h"
#include "HashTableTypes.h"
#include <deque>
class MainWindow { HWND hwnd_=nullptr,input_=nullptr,status_=nullptr;Mode mode_=Mode::Open;bool welcome_=true;HashSetOpen open_;HashSetClosed closed_;std::deque<Event> events_;Event current_;int speed_=450;static LRESULT CALLBACK proc(HWND,UINT,WPARAM,LPARAM);LRESULT handle(UINT,WPARAM,LPARAM);void draw(HDC);void run(int);void log(const std::string&);void updateStatus();bool exportCurrent();public:int create(HINSTANCE);};
#endif
