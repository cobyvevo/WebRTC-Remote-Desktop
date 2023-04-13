#include <node.h> 
#include <v8.h>

#include <opencv2/core.hpp>
#include <windows.h>
#include <WinUser.h>
#include <iostream>

//INPUTSIMULATOR.HPP
enum {
    LEFT_DOWN,
    LEFT_UP,

    RIGHT_DOWN,
    RIGHT_UP,

    MIDDLE_DOWN,
    MIDDLE_UP,

    SCROLL,
};

CURSORINFO cursor;
ICONINFO info;
BITMAP cursorimage;
int screen_width = 0;
int screen_height = 0;

class InputSimulator {

public:
       
    InputSimulator();

    void MoveMouse(int deltax, int deltay, bool absolute);
    void MouseEvent(int x, int y, int type, int param);


    void KeyPress( SHORT c, bool up);

};
//
//.CPP
InputSimulator::InputSimulator() {}

void InputSimulator::MoveMouse(int deltax, int deltay, bool absolute) {
    INPUT input;
    ZeroMemory(&input, sizeof(INPUT));

    if (absolute == true) {
        int posx = deltax * screen_width / 25565;
        int posy = deltay * screen_height / 25565;

        int realdeltax = posx-cursor.ptScreenPos.x;
        int realdeltay = posy-cursor.ptScreenPos.y;

        input.mi.dx = realdeltax/2;
        input.mi.dy = realdeltay/2;
    } else {
        input.mi.dx = deltax;
        input.mi.dy = deltay;
    }
    
    input.mi.dwFlags = MOUSEEVENTF_MOVE;

    SendInput(1, &input, sizeof(INPUT));
}

void InputSimulator::MouseEvent(int x = -1, int y = -1, int type = 0, int param = 0) {

    INPUT input;
    ZeroMemory(&input, sizeof(INPUT));

    input.type = INPUT_MOUSE;

    switch (type) {

    case LEFT_DOWN:
        input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        break;
    case LEFT_UP:
        input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
        break;
    case RIGHT_DOWN:
        input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
        break;

    case RIGHT_UP:
        input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
        break;

    case MIDDLE_DOWN:
        input.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
        break;

    case MIDDLE_UP:
        input.mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
        break;

    case SCROLL:
        input.mi.dwFlags = MOUSEEVENTF_WHEEL;
        input.mi.mouseData = param;
        break;

    }

    if (x != -1 && y != -1) {
        //change mouse
        input.mi.dx = x;
        input.mi.dy = y;
        input.mi.dwFlags = input.mi.dwFlags | MOUSEEVENTF_MOVE;
    }

    SendInput(1, &input, sizeof(INPUT));

}

UINT keyCodeToScan(SHORT c) {
    return MapVirtualKey(c, MAPVK_VK_TO_VSC);
}

void InputSimulator::KeyPress(SHORT c, bool up) {
   
    INPUT input;
    ZeroMemory(&input, sizeof(INPUT));

    SHORT keycode = c;
    UINT scan = keyCodeToScan(keycode);

    input.type = INPUT_KEYBOARD;
    input.ki.dwExtraInfo = GetMessageExtraInfo();
    input.ki.dwFlags = (up == false ? KEYEVENTF_KEYUP : 0);

    if (scan != 0) {
        input.ki.wScan = scan;
        input.ki.dwFlags = input.ki.dwFlags | KEYEVENTF_SCANCODE;
    } else {
        input.ki.wVk = keycode; 
    } 
    SendInput(1, &input, sizeof(INPUT));

}
//
             

//DESKTOPCAPTURE.HPP
class DesktopCaptureDirect {

public:
    DesktopCaptureDirect();
    ~DesktopCaptureDirect();

    void read(int scale);

    cv::Mat frame;

    unsigned int* RGBframe;


    int RGBframesize;
    int w_s,h_s;

private:

    RECT desktopsize;

    int w, h;
  
    int ow, oh;
    int oldscale = 0;

    HDC desktopDC;
    HDC compatableDC;

    HBITMAP framebitmap;
    BITMAPINFOHEADER bitmapinfo;

};
//


//.CPP
DesktopCaptureDirect::DesktopCaptureDirect() {

	//get desktop window
    desktopDC = GetDC(0);

    //create DC for drawing onto
    compatableDC = CreateCompatibleDC(desktopDC);
    SetStretchBltMode(compatableDC,COLORONCOLOR); //set blt to overwrite pixels

    GetClientRect(WindowFromDC(desktopDC), &desktopsize); //get size of desktop

    w = desktopsize.right;
    h = desktopsize.bottom;

    //create new frame bitmap
    framebitmap = CreateCompatibleBitmap(desktopDC, (int)desktopsize.right, (int)desktopsize.bottom);

    bitmapinfo.biSize = sizeof(BITMAPINFOHEADER);
    bitmapinfo.biWidth = desktopsize.right;
    bitmapinfo.biHeight = -desktopsize.bottom;
    bitmapinfo.biPlanes = 1;
    bitmapinfo.biBitCount = 32;
    bitmapinfo.biCompression = BI_RGB;
    bitmapinfo.biSizeImage = 0;
    bitmapinfo.biXPelsPerMeter = 0;
    bitmapinfo.biYPelsPerMeter = 0;
    bitmapinfo.biClrUsed = 0;
    bitmapinfo.biClrImportant = 0;

    //select each for use
    SelectObject(compatableDC, framebitmap);

}

int charsize = sizeof(char)*8;

void DesktopCaptureDirect::read(int scale = 0) {

    float realscale = (scale/100.0f); //calculate resolution downscale float
    int width = int(w*realscale);
    int height = int(h*realscale);

    screen_width=w;
    screen_height=h;

    w_s = width;
    h_s = height;

    if (oldscale != scale) { //make sure to change the size of the screen if needed
        oldscale = scale;

        std::cout << "making new bitmap cause scale changed" << std::endl;

        //create new bitmap
        DeleteObject(framebitmap);

        framebitmap = CreateCompatibleBitmap(desktopDC, (int)width, (int)height);

        bitmapinfo.biSize = sizeof(BITMAPINFOHEADER);
        bitmapinfo.biWidth = width;
        bitmapinfo.biHeight = -height;
        bitmapinfo.biPlanes = 1;
        bitmapinfo.biBitCount = 32;
        bitmapinfo.biCompression = BI_RGB;
        bitmapinfo.biSizeImage = 0;
        bitmapinfo.biXPelsPerMeter = 0;
        bitmapinfo.biYPelsPerMeter = 0;
        bitmapinfo.biClrUsed = 0;
        bitmapinfo.biClrImportant = 0;

        SelectObject(compatableDC, framebitmap);

    }

    BOOL success = StretchBlt(compatableDC,0,0,width,height, desktopDC, 0, 0, w, h, SRCCOPY);
    if (success == false) {
        std::cout << "StretchBlt failed! remaking everything..." << std::endl;
        oldscale = 0;
        oh = 0;
        ow = 0;
    }

    cursor = { sizeof(cursor) };
    GetCursorInfo(&cursor);

    info = { sizeof(info) };
    GetIconInfo(cursor.hCursor, &info);

    cursorimage = { sizeof(cursorimage) };
    GetObject(info.hbmColor, sizeof(cursorimage), &cursorimage);

    DrawIconEx(compatableDC, (cursor.ptScreenPos.x-info.xHotspot) * realscale, (cursor.ptScreenPos.y-info.yHotspot) * realscale, cursor.hCursor, cursorimage.bmHeight, cursorimage.bmWidth, 0, NULL, DI_NORMAL);
    
    if (oh != height && ow != width) {
    	oh = height;
    	ow = width;
        free(RGBframe);
        RGBframe = new unsigned int[width*height];
    	std::cout << "making new buffer" << std::endl;
    }

    RGBframesize = width*height;
 	
    GetDIBits(compatableDC, framebitmap, 0, height, RGBframe, (BITMAPINFO*)&bitmapinfo, DIB_RGB_COLORS);

    for (int i = 0; i < (width * height); i++) { // convert from BGR to RGB
        
        unsigned int orig = RGBframe[i];
        int swap = (((orig << 24) >> 24) << 16) | ((orig << 8) >> 24); 
        RGBframe[i] = (orig & 0xFF00FF00) | swap;
        
    }

}

DesktopCaptureDirect::~DesktopCaptureDirect() {
    DeleteObject(framebitmap);
    DeleteDC(compatableDC);
    ReleaseDC(0,desktopDC);
}

//

DesktopCaptureDirect direct;
InputSimulator inputsim;

//JAVASCRIPT BINDINGS BELOW

void TestFunc(const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();
  double value = 3.14;
  v8::Local<v8::Number> num = v8::Number::New(isolate, value);
  args.GetReturnValue().Set(num);
}

void GetFrame(const v8::FunctionCallbackInfo<v8::Value>& args) {

    v8::Isolate* isolate = args.GetIsolate();

    v8::Local<v8::Value> x = args[0];
    v8::Local<v8::Int32> conx = v8::Local<v8::Int32>::Cast(x);
    int scale = conx->Value();

    direct.read(scale);

    size_t length = direct.RGBframesize;
    size_t size = direct.RGBframesize * sizeof(int);

    v8::Local<v8::ArrayBuffer> framebuffer = v8::ArrayBuffer::New(isolate, size);

    memcpy(framebuffer->Data(), direct.RGBframe, size);
    v8::Local<v8::Int32Array> frame = v8::Int32Array::New(framebuffer, 0, length);

    args.GetReturnValue().Set(frame);

}

void MouseEvent(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();

    v8::Local<v8::Value> x = args[0];
    v8::Local<v8::Value> y = args[1];
    v8::Local<v8::Value> t = args[2];
    v8::Local<v8::Value> p = args[3];

    v8::Local<v8::Int32> conx = v8::Local<v8::Int32>::Cast(x);
    v8::Local<v8::Int32> cony = v8::Local<v8::Int32>::Cast(y);
    v8::Local<v8::Int32> cont = v8::Local<v8::Int32>::Cast(t);
    v8::Local<v8::Int32> conp = v8::Local<v8::Int32>::Cast(p);

    int c_x = conx->Value();
    int c_y = cony->Value();

    int c_type = cont->Value();
    int c_param = conp->Value();

    inputsim.MouseEvent(c_x, c_y, c_type, c_param);
}

void MoveMouse(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();

    v8::Local<v8::Value> x = args[0];
    v8::Local<v8::Value> y = args[1];
    v8::Local<v8::Value> abs = args[2];

    v8::Local<v8::Int32> conx = v8::Local<v8::Int32>::Cast(x);
    v8::Local<v8::Int32> cony = v8::Local<v8::Int32>::Cast(y);
    v8::Local<v8::Boolean> cona = v8::Local<v8::Boolean>::Cast(abs);

    int c_x = conx->Value();
    int c_y = cony->Value();
    int c_a = cona->Value();

    inputsim.MoveMouse(c_x, c_y, c_a);
}

void KeyPress(const v8::FunctionCallbackInfo<v8::Value>& args) {

    v8::Isolate* isolate = args.GetIsolate();

    v8::Local<v8::Value> character = args[0];
    v8::Local<v8::Value> up = args[1];

    v8::Local<v8::Int32> keycode1 = v8::Local<v8::Int32>::Cast(character);

    v8::Local<v8::Boolean> is_up = v8::Local<v8::Boolean>::Cast(up);

    bool c_up = is_up->Value();
    int keycode2 = keycode1->Value();

    inputsim.KeyPress((SHORT) keycode2, c_up);

}

void GetWidth(const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();
  double value = direct.w_s;
  v8::Local<v8::Number> num = v8::Number::New(isolate, value);
  args.GetReturnValue().Set(num);
}

void GetHeight(const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();
  double value = direct.h_s;
  v8::Local<v8::Number> num = v8::Number::New(isolate, value);
  args.GetReturnValue().Set(num);
}

void init(v8::Local<v8::Object> exports) {

    NODE_SET_METHOD(exports, "TestFunc", TestFunc);
    NODE_SET_METHOD(exports, "GetFrame", GetFrame);
    NODE_SET_METHOD(exports, "KeyPress", KeyPress);
    NODE_SET_METHOD(exports, "MoveMouse", MoveMouse);
    NODE_SET_METHOD(exports, "MouseEvent", MouseEvent);
    NODE_SET_METHOD(exports, "GetWidth", GetWidth);
    NODE_SET_METHOD(exports, "GetHeight", GetHeight);

}


NODE_MODULE(NODE_GYP_MODULE_NAME, init);