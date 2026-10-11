// Stub de DECLARACOES apenas (para g++ -fsyntax-only sem SDL2-dev). Nao faz parte do projeto.
#pragma once
#include <cstdint>
#include <cstddef>
typedef uint8_t Uint8; typedef uint16_t Uint16; typedef uint32_t Uint32; typedef uint64_t Uint64; typedef int32_t Sint32; typedef int64_t Sint64; typedef int16_t Sint16;
typedef int32_t SDL_Keycode; typedef int SDL_bool; enum { SDL_FALSE = 0, SDL_TRUE = 1 };
struct SDL_Window; typedef void* SDL_GLContext;
enum { SDL_INIT_VIDEO = 0x20, SDL_INIT_GAMECONTROLLER = 0x2000, SDL_WINDOW_OPENGL = 2, SDL_WINDOW_RESIZABLE = 32, SDL_WINDOW_FULLSCREEN_DESKTOP = 4097, SDL_WINDOWPOS_CENTERED = 0x2FFF0000, SDL_ENABLE = 1, SDL_GL_DEPTH_SIZE = 6, SDL_GL_STENCIL_SIZE = 7, SDL_TOUCH_MOUSEID = -1 };
enum { SDL_QUIT = 0x100, SDL_WINDOWEVENT = 0x200, SDL_KEYDOWN = 0x300, SDL_KEYUP = 0x301, SDL_TEXTINPUT = 0x303, SDL_MOUSEMOTION = 0x400, SDL_MOUSEBUTTONDOWN, SDL_MOUSEBUTTONUP, SDL_MOUSEWHEEL, SDL_FINGERDOWN = 0x700, SDL_FINGERUP, SDL_FINGERMOTION, SDL_DROPFILE = 0x1000, SDL_CONTROLLERDEVICEADDED = 0x653, SDL_CONTROLLERDEVICEREMOVED = 0x654 };
enum { SDL_WINDOWEVENT_FOCUS_LOST = 13 };
enum { SDL_BUTTON_LEFT = 1, SDL_BUTTON_MIDDLE = 2, SDL_BUTTON_RIGHT = 3 };
#define SDL_BUTTON(X) (1 << ((X)-1))
enum { SDL_SCANCODE_A = 4, SDL_SCANCODE_D = 7, SDL_SCANCODE_S = 22, SDL_SCANCODE_W = 26, SDL_SCANCODE_SPACE = 44, SDL_SCANCODE_LCTRL = 224, SDL_SCANCODE_LSHIFT = 225, SDL_SCANCODE_RCTRL = 228, SDL_SCANCODE_RSHIFT = 229 };
struct SDL_Keysym { int scancode; SDL_Keycode sym; Uint16 mod; Uint32 unused; };
struct SDL_KeyboardEvent { Uint32 type; Uint32 timestamp; Uint32 windowID; Uint8 state; Uint8 repeat; SDL_Keysym keysym; };
struct SDL_MouseMotionEvent { Uint32 type; Uint32 timestamp; Uint32 windowID; Uint32 which; Uint32 state; Sint32 x, y, xrel, yrel; };
struct SDL_MouseButtonEvent { Uint32 type; Uint32 timestamp; Uint32 windowID; Uint32 which; Uint8 button; Uint8 state; Uint8 clicks; Sint32 x, y; };
struct SDL_MouseWheelEvent { Uint32 type; Uint32 timestamp; Uint32 windowID; Uint32 which; Sint32 x, y; Uint32 direction; float preciseX, preciseY; };
struct SDL_TextInputEvent { Uint32 type; Uint32 timestamp; Uint32 windowID; char text[32]; };
struct SDL_WindowEvent { Uint32 type; Uint32 timestamp; Uint32 windowID; Uint8 event; Sint32 data1, data2; };
struct SDL_TouchFingerEvent { Uint32 type; Uint32 timestamp; Sint64 touchId; Sint64 fingerId; float x, y, dx, dy, pressure; };
struct SDL_ControllerDeviceEvent { Uint32 type; Uint32 timestamp; Sint32 which; };
struct SDL_DropEvent { Uint32 type; Uint32 timestamp; char* file; };
union SDL_Event { Uint32 type; SDL_KeyboardEvent key; SDL_MouseMotionEvent motion; SDL_MouseButtonEvent button; SDL_MouseWheelEvent wheel; SDL_TextInputEvent text; SDL_WindowEvent window; SDL_TouchFingerEvent tfinger; SDL_DropEvent drop; SDL_ControllerDeviceEvent cdevice; };
struct SDL_version { Uint8 major, minor, patch; };
int SDL_Init(Uint32); int SDL_InitSubSystem(Uint32); void SDL_Quit(); const char* SDL_GetError();
SDL_Window* SDL_CreateWindow(const char*, int, int, int, int, Uint32); void SDL_DestroyWindow(SDL_Window*);
void SDL_GetWindowSize(SDL_Window*, int*, int*); void SDL_SetWindowTitle(SDL_Window*, const char*); void SDL_WarpMouseInWindow(SDL_Window*, int, int);
int SDL_GL_SetAttribute(int, int); SDL_GLContext SDL_GL_CreateContext(SDL_Window*); void SDL_GL_DeleteContext(SDL_GLContext); void* SDL_GL_GetProcAddress(const char*);
int SDL_GL_SetSwapInterval(int); void SDL_GL_SwapWindow(SDL_Window*);
int SDL_PollEvent(SDL_Event*); Uint8 SDL_EventState(Uint32, int); const Uint8* SDL_GetKeyboardState(int*); Uint16 SDL_GetModState();
Uint32 SDL_GetMouseState(int*, int*); int SDL_SetRelativeMouseMode(SDL_bool); void SDL_StartTextInput();
Uint32 SDL_GetTicks(); void SDL_Delay(Uint32); Uint32 SDL_GetWindowFlags(SDL_Window*); int SDL_SetWindowFullscreen(SDL_Window*, Uint32); Uint64 SDL_GetPerformanceCounter(); Uint64 SDL_GetPerformanceFrequency();
int SDL_GetCPUCount(); int SDL_GetSystemRAM(); const char* SDL_GetCurrentVideoDriver(); void SDL_GetVersion(SDL_version*);
typedef struct SDL_mutex SDL_mutex; typedef struct SDL_Thread SDL_Thread; SDL_mutex* SDL_CreateMutex(); int SDL_LockMutex(SDL_mutex*); int SDL_UnlockMutex(SDL_mutex*); SDL_Thread* SDL_CreateThread(int (*)(void*), const char*, void*); void SDL_DetachThread(SDL_Thread*); int SDL_OpenURL(const char*); void SDL_StopTextInput(); char* SDL_GetClipboardText(); void SDL_free(void*); void* SDL_LoadObject(const char*); void* SDL_LoadFunction(void*, const char*);
enum : int {
 SDLK_F10 = 1099, SDLK_F12 = 1098, SDLK_F11 = 1100, KMOD_ALT = 1101,
 KMOD_CTRL = 1001,
 KMOD_SHIFT = 1002,
 SDLK_1 = 1003,
 SDLK_9 = 1004,
 SDLK_BACKSPACE = 1005,
 SDLK_DOWN = 1006,
 SDLK_ESCAPE = 1007,
 SDLK_F3 = 1008,
 SDLK_F4 = 1009,
 SDLK_F5 = 1010,
 SDLK_F6 = 1011,
 SDLK_F7 = 1011, SDLK_F8 = 1012, SDLK_F9 = 1013,
 SDLK_KP_ENTER = 1013,
 SDLK_PAGEDOWN = 1014,
 SDLK_PAGEUP = 1015,
 SDLK_RETURN = 1016,
 SDLK_SLASH = 1017,
 SDLK_UP = 1018,
 SDLK_e = 1019,
 SDLK_q = 1020,
 SDLK_t = 1021,
 SDLK_v = 1022, SDLK_n = 1023, SDLK_m = 1024,
};

#ifndef BL_STUB_CURSOR
#define BL_STUB_CURSOR
typedef struct SDL_Cursor SDL_Cursor; enum { SDL_SYSTEM_CURSOR_ARROW = 0, SDL_SYSTEM_CURSOR_HAND = 11 };
inline SDL_Cursor* SDL_CreateSystemCursor(int) { return nullptr; } inline void SDL_SetCursor(SDL_Cursor*) {}
#endif

// ---- game controller (so declaracoes) ----
struct _SDL_GameController; typedef struct _SDL_GameController SDL_GameController; typedef int32_t SDL_JoystickID; struct _SDL_Joystick; typedef struct _SDL_Joystick SDL_Joystick;
typedef int SDL_GameControllerButton; typedef int SDL_GameControllerAxis;
enum { SDL_CONTROLLER_AXIS_LEFTX = 0, SDL_CONTROLLER_AXIS_LEFTY, SDL_CONTROLLER_AXIS_RIGHTX, SDL_CONTROLLER_AXIS_RIGHTY, SDL_CONTROLLER_AXIS_TRIGGERLEFT, SDL_CONTROLLER_AXIS_TRIGGERRIGHT };
SDL_bool SDL_IsGameController(int); SDL_GameController* SDL_GameControllerOpen(int); void SDL_GameControllerClose(SDL_GameController*);
SDL_Joystick* SDL_GameControllerGetJoystick(SDL_GameController*); SDL_JoystickID SDL_JoystickInstanceID(SDL_Joystick*); const char* SDL_GameControllerName(SDL_GameController*);
Uint8 SDL_GameControllerGetButton(SDL_GameController*, SDL_GameControllerButton); Sint16 SDL_GameControllerGetAxis(SDL_GameController*, SDL_GameControllerAxis);
int SDL_PushEvent(SDL_Event*);
enum : int { SDLK_f = 1200, SDLK_F1 = 1201, SDLK_F2 = 1202 };
enum { SDL_SCANCODE_F3 = 60 };
