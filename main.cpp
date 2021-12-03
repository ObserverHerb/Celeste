#include "window.h"

#include <QApplication>
#include <QMessageBox>
#include "globals.h"
#include "log.h"

enum
{
	OK,
	FATAL_ERROR,
	UNKNOWN_ERROR
};

void DisplayError(const QString &message)
{
	QMessageBox messageBox;
	messageBox.setText(message);
	messageBox.exec();
}

int main(int argc,char *argv[])
{
	qputenv("QT_MULTIMEDIA_PREFERRED_PLUGINS", "windowsmediafoundation");

	QApplication application(argc,argv);
	application.setOrganizationName(ORGANIZATION_NAME);
	application.setApplicationName(APPLICATION_NAME);
	application.setWindowIcon(QIcon(":/celeste_256.png"));

	try
	{
		Log log;
#ifdef Q_OS_WIN
		Win32Window window;
#else
		Window window;
#endif
		window.connect(&window,&Window::Log,&log,&Log::Write);
		application.connect(&application,&QApplication::aboutToQuit,&log,&Log::Archive);
		log.connect(&log,&Log::Error,&window,&Window::close);
		log.connect(&log,&Log::Error,[](const QString &message) {
			DisplayError(message);
		});
		log.Open();
		window.show();
		return application.exec();
	}

	catch (const std::runtime_error &exception)
	{
		qFatal(exception.what());
		return FATAL_ERROR;
	}

	catch (...)
	{
		qFatal("An unknown error occurred!");
		return UNKNOWN_ERROR;
	}
}
