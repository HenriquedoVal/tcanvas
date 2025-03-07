#pragma once
#pragma comment(lib, "user32")

#include <assert.h>
#include <stdbool.h>

#include <windows.h>


static void (*ctrlc_callback)(void) = NULL;


static BOOL WINAPI _ctrl_handler(DWORD ctrl_type)
{
    switch (ctrl_type) {
        case CTRL_C_EVENT:
            if (ctrlc_callback != NULL)
                ctrlc_callback();
            ExitProcess(EXIT_SUCCESS);
        default:
            return FALSE;
    }
}


static bool register_ctrlc_handler(void (*callback)(void))
{
    ctrlc_callback = callback;
    return SetConsoleCtrlHandler(_ctrl_handler, TRUE);
}


bool get_terminal_size(int *width, int *height)
{
    HANDLE std_out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (std_out == INVALID_HANDLE_VALUE) return false;

    CONSOLE_SCREEN_BUFFER_INFO buf_info;
    if (!GetConsoleScreenBufferInfo(std_out, &buf_info)) return false;

    *width = (int)buf_info.dwSize.X;
    *height = (int)buf_info.dwSize.Y;

    return true;
}


bool kb_key_down(int key)
{
    return (GetKeyState(key) & 0x8000) != 0;
}


void flush_input_buffer(void)
{
    static HANDLE std_in = NULL;
    if (std_in == NULL) std_in = GetStdHandle(STD_INPUT_HANDLE);
    assert(std_in != INVALID_HANDLE_VALUE);

    BOOL ret = FlushConsoleInputBuffer(std_in);
    assert(ret);
}
