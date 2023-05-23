#ifndef BATTERYTHING_WINDOWFOCUS_HPP
#define BATTERYTHING_WINDOWFOCUS_HPP

#include <X11/Xlib.h>
#include <X11/Xutil.h>

class WindowFocusDetector
{
public:
    WindowFocusDetector();
    ~WindowFocusDetector();

    unsigned long getActiveWindowPID();

private:
    Display *display;
    static int handle_error(Display *display, XErrorEvent *event);
};

#endif // BATTERYTHING_WINDOWFOCUS_HPP