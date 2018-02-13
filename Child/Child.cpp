#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

#include <string>
#include <sstream>

#include <SDL.h>
#include <SDL_syswm.h>
#include <SDL_opengl.h>

static bool running = true;
static SDL_Window* window = nullptr;
static SDL_GLContext context;
static SDL_SysWMinfo sysinfo;

static const std::wstring WindowTitle = L"Child";
static const std::wstring WindowClass = L"Child";

std::wstring LastErrorString()
{
    DWORD id = GetLastError();
    if (id == 0) return std::wstring();

    LPTSTR buffer = nullptr;
    size_t size = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                FORMAT_MESSAGE_FROM_SYSTEM |
                                FORMAT_MESSAGE_IGNORE_INSERTS,
                                nullptr, id,
                                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                (LPTSTR)&buffer, 0, nullptr);

    std::wstring message(buffer, size);
    LocalFree(buffer);
    return message;
}

void LastErrorMessageBox()
{
    std::wstring err = LastErrorString();
    MessageBox(nullptr, err.c_str(), L"ERROR", MB_ICONEXCLAMATION | MB_OK);
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    // Allocate new console
    if (AllocConsole() == 0)
    {
        LastErrorMessageBox();
        return 1;
    }

    // Redirect standard output and error to console
    FILE *out = nullptr, *err = nullptr;
    freopen_s(&out, "CONOUT$", "w", stdout);
    freopen_s(&err, "CONOUT$", "w", stderr);

    DWORD ppid = 0;

    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLine(), &argc);

    if (argv == NULL || argc == 0)
    {
        MessageBox(NULL, L"Unable to parse command line", L"Error", MB_OK);
        return 1;
    }

    ppid = wcstol(argv[0], '\0', 10);
    wprintf(L"%d\n", ppid);
    LocalFree(argv);

    if (ppid == 0)
    {
        const char* err = "Unable to find parent PID";
        MessageBoxA(nullptr, err, "ERROR", MB_ICONEXCLAMATION | MB_OK);
        return 1;
    }

    // Find parent window handle
    HWND hwndParent = FindWindowEx(NULL, NULL, L"Parent", NULL);
    DWORD pid; GetWindowThreadProcessId(hwndParent, &pid);

    if (pid != ppid)
    {
        std::stringstream ss;
        ss << "Unable to find parent window with PID " << ppid;
        MessageBoxA(nullptr, ss.str().c_str(), "ERROR", MB_ICONEXCLAMATION | MB_OK);
        return 1;
    }

    RECT rect;
    if (GetWindowRect(hwndParent, &rect) == 0)
    {
        LastErrorMessageBox();
        return 1;
    }

    // Initialize SDL
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        const char* err = SDL_GetError();
        MessageBoxA(nullptr, err, "ERROR", MB_ICONEXCLAMATION | MB_OK);
        SDL_Log("Failed to initialize SDL: %s", err);
        return 1;
    }

    // Initialize SDL window with OpenGL context
    window = SDL_CreateWindow("Child", 0, 0, rect.right - rect.left, rect.bottom - rect.top, SDL_WINDOW_OPENGL | SDL_WINDOW_BORDERLESS);
    context = SDL_GL_CreateContext(window);

    // Get SDL window handle
    SDL_VERSION(&sysinfo.version);

    HWND hwndChild = nullptr;
    if (SDL_GetWindowWMInfo(window, &sysinfo))
    {
        if (sysinfo.subsystem == SDL_SYSWM_WINDOWS)
        {
            hwndChild = sysinfo.info.win.window;
        }
    }

    if (!hwndChild)
    {
        MessageBoxA(nullptr, "Unable to retrieve handle for created SDL window", "ERROR", MB_ICONEXCLAMATION | MB_OK);
        return 1;
    }

    // Set SDL window as child to native parent window
    if (!SetParent(hwndChild, hwndParent))
    {
        LastErrorMessageBox();
        return 1;
    }

    GLfloat color[4] = { 0.2f, 0.4f, 0.1f, 1.0f };

    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = false;
            }
            else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE)
            {
                GLfloat red = color[0];
                color[0] = color[1];
                color[1] = red;
            }
        }

        SDL_GL_MakeCurrent(window, context);

        glClearColor(color[0], color[1], color[2], color[3]);
        glClear(GL_COLOR_BUFFER_BIT);

        SDL_GL_SwapWindow(window);
        SDL_Delay(1);
    }

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
