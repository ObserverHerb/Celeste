#include "window.h"

#include <QApplication>
#include <globals.h>

int main(int argc,char *argv[])
{
	qputenv("QT_MULTIMEDIA_PREFERRED_PLUGINS", "windowsmediafoundation");

	QApplication application(argc,argv);
	application.setOrganizationName(ORGANIZATION_NAME);
	application.setApplicationName(APPLICATION_NAME);
	application.setWindowIcon(QIcon(":/celeste_256.png"));
#ifdef Q_OS_WIN
	Win32Window window;
#else
	Window window;
#endif
	window.show();
	return application.exec();
}
