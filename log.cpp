#include <QDate>
#include "log.h"

const char *SETTINGS_CATEGORY_LOGGING="Logging";

Log::Log(QObject *parent) : QObject(parent),
	settingLogDirectory(SETTINGS_CATEGORY_LOGGING,"Directory",Filesystem::DataPath().absoluteFilePath("logs"))
{
	file.setFileName(QDir(settingLogDirectory).absoluteFilePath("current"));
}

Log::~Log()
{
	file.close();
}

bool Log::CreateDirectory()
{
	QDir directory(settingLogDirectory);
	const char *operation="creating log directory";
	emit Print({directory.absolutePath(),operation});
	if (!directory.exists() && !directory.mkpath(directory.absolutePath()))
	{
		emit Print({QString("Failed: %1").arg(directory.absolutePath()),operation});
		return false;
	}
	return true;
}

bool Log::Open()
{
	const char *operation="create log file";
	emit Print({QDir(settingLogDirectory).absolutePath(),operation});
	if (!CreateDirectory()) return false;
	if (!file.open(QIODevice::ReadWrite))
	{
		emit Print({"Failed",operation});
		return false;
	}
	return true;
}

void Log::Write(const QString &message,const QString &operation,const QString &subsystem)
{
	Entry entry(message,operation,subsystem);
	emit Print(entry);
	if (file.write(entry) < 0 || !file.flush()) emit Print({"Failed","write to log file"});
}

void Log::Archive()
{
	const char *operation="archive log";
	if (!file.reset())
	{
		emit Print({"Failed to seek to beginning of file",operation});
		return;
	}

	QFile datedFile(QDir(settingLogDirectory).absoluteFilePath(QDate::currentDate().toString("yyyyMMdd.log")));
	if (datedFile.exists())
	{
		if (!datedFile.open(QIODevice::WriteOnly|QIODevice::Append))
		{
			emit Print({"Could not open log file for today's date",operation});
			return;
		}
		while (!file.atEnd())
		{
			if (datedFile.write(file.read(4096)) < 0)
			{
				emit Print({"Could not append to archived log file",operation});
				return;
			}
		}
	}
	else
	{
		if (!file.rename(QFileInfo(datedFile).absoluteFilePath()))
		{
			emit Print({"Could not save log as archive file",operation});
			return;
		}
	}
	file.close();
}

ApplicationSetting& Log::Directory()
{
	return settingLogDirectory;
}