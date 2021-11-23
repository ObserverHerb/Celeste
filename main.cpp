#include "window.h"
#include "command.h"

#include <QApplication>
#include <globals.h>

enum
{
	OK,
	FATAL_ERROR,
	UNKNOWN_ERROR
};

int main(int argc,char *argv[])
{
	qputenv("QT_MULTIMEDIA_PREFERRED_PLUGINS", "windowsmediafoundation");

	QApplication application(argc,argv);
	application.setOrganizationName(ORGANIZATION_NAME);
	application.setApplicationName(APPLICATION_NAME);
	application.setWindowIcon(QIcon(":/celeste_256.png"));

	ChatCommands::caseSensitive=ApplicationSetting("Commands","CaseSensitive",true).Enabled();

	try
	{
		if (!Filesystem::LogPath().exists() && !Filesystem::LogPath().mkpath(Filesystem::LogPath().absolutePath())) throw std::runtime_error("Failed to create log directory");
		QFile logFile(Filesystem::LogPath().absoluteFilePath("current"));
		if (!logFile.open(QIODevice::ReadWrite)) throw std::runtime_error("Failed to open log file");

#ifdef Q_OS_WIN
		Win32Window window;
#else
		Window window;
#endif

		QObject::connect(&window,&Window::Log,[&logFile](const QString &text) {
			logFile.write(StringConvert::ByteArray(QString("%1\n").arg(text)));
			logFile.flush();
		});
		QObject::connect(&application,&QApplication::aboutToQuit,[&logFile]() {
			if (logFile.exists())
			{
				logFile.reset();
				QFile datedFile(Filesystem::LogPath().absoluteFilePath(QDate::currentDate().toString("yyyyMMdd.log")));
				if (datedFile.exists())
				{
					if (datedFile.open(QIODevice::WriteOnly|QIODevice::Append))
					{
						while (!logFile.atEnd()) datedFile.write(logFile.read(4096));
					}
				}
				else
				{
					logFile.rename(QFileInfo(datedFile).absoluteFilePath());
				}
				logFile.close();
			}
		});

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
