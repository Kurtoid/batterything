#include "windowfocus.hpp"
// recieve window focus events from X11
#include <iostream>
#include "X11/Xlib.h"
#include "X11/Xatom.h"
#include "X11/Xutil.h"
// for thread sleep
#include <chrono>
#include <thread>
bool xerror = false;
int WindowFocusDetector::handle_error(Display *display, XErrorEvent *error)
{
    printf("ERROR: X11 error\n");
    xerror = true;
    return 1;
}

WindowFocusDetector::WindowFocusDetector()
{
    display = XOpenDisplay(":0");
    if (display == NULL)
    {
        std::cout << "Error: could not open display" << std::endl;
        return;
    }
    XSetErrorHandler(handle_error);
}

WindowFocusDetector::~WindowFocusDetector()
{
    XCloseDisplay(display);
}

unsigned long WindowFocusDetector::getActiveWindowPID()
{
    unsigned long nitems, bytes_after;
    unsigned char *prop;
    int actual_format;
    Atom actual_type;
    Atom net_active_window = XInternAtom(display, "_NET_ACTIVE_WINDOW", False);
    int status = XGetWindowProperty(display, DefaultRootWindow(display), net_active_window, 0, 1, False, XA_WINDOW, &actual_type, &actual_format, &nitems, &bytes_after, &prop);
    if (status != Success)
    {
        std::cout << "Error: could not get property" << std::endl;
        return 0;
    }
    Window active_window = *((Window *)prop);
    XFree(prop);
    // std::cout << "Active window: " << active_window << std::endl;

    // // get _NET_WM_NAME
    // Atom net_wm_name = XInternAtom(display, "_NET_WM_NAME", False);
    // status = XGetWindowProperty(display, active_window, net_wm_name, 0, 100, False, AnyPropertyType, &actual_type, &actual_format, &nitems, &bytes_after, &prop);
    // if (status != Success)
    // {
    //     std::cout << "Error: could not get property" << std::endl;
    //     return 1;
    // }
    // std::cout << "Window name: " << (char *)prop << std::endl;
    // XFree(prop);

    // get _NET_WM_PID
    Atom net_wm_pid = XInternAtom(display, "_NET_WM_PID", False);
    status = XGetWindowProperty(display, active_window, net_wm_pid, 0, 1, False, XA_CARDINAL, &actual_type, &actual_format, &nitems, &bytes_after, &prop);
    if (status != Success)
    {
        std::cout << "Error: could not get property" << std::endl;
        return 1;
    }
    unsigned long pid = *((unsigned long *)prop);
    XFree(prop);
    // std::cout << "Window PID: " << pid << std::endl;
    return pid;
}

int windowfocus_main()
{
    std::cout << "window focus demo" << std::endl;
    WindowFocusDetector detector;

    while (true)
    {
        unsigned long pid = detector.getActiveWindowPID();
        std::cout << "Active window PID: " << pid << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    return 0;
}