#include <QtWin>
#include "window.h"

Win32Window::Win32Window() : Window()
{
	QtWin::enableBlurBehindWindow(this);
}
