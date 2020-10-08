#include "window.h"

#include <QApplication>
#include <globals.h>

int main(int argc,char *argv[])
{
	QApplication application(argc,argv);
	application.setOrganizationName(ORGANIZATION_NAME);
	application.setApplicationName(APPLICATION_NAME);
	Window window;
	window.show();
	return application.exec();
}
