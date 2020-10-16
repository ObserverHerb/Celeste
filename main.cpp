#include "window.h"

#include <QApplication>
#include <globals.h>

int main(int argc,char *argv[])
{
	QApplication application(argc,argv);
	application.setOrganizationName(ORGANIZATION_NAME);
	application.setApplicationName(APPLICATION_NAME);
#ifdef Q_OS_WIN
	Win32Window window;
#else
	Window window;
#endif
	window.show();
	return application.exec();
}
