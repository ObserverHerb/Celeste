#include <QDate>
#include "log.h"

const char *SETTINGS_CATEGORY_LOGGING="Logging";

Log::Log(QObject *parent) : QObject(parent),
	settingLogDirectory(SETTINGS_CATEGORY_LOGGING,"Directory",Filesystem::DataPath().absoluteFilePath("logs"))
{
	file.setFileName(Directory().absoluteFilePath("current"));
}

Log::~Log()
{
	file.close();
}

bool Log::CreateDirectory()
{
	const char *operation="creating log directory";
	emit Print({Directory().absolutePath(),operation});
	if (!Directory().exists() && !Directory().mkpath(Directory().absolutePath()))
	{
		emit Print({QString("Failed: %1").arg(Directory().absolutePath()),operation});
		return false;
	}
	return true;
}

QDir Log::Directory()
{
	return QDir(settingLogDirectory);
}

bool Log::Open()
{
	const char *operation="create log file";
	emit Print({Directory().absolutePath(),operation});
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

	QFile datedFile(Directory().absoluteFilePath(QDate::currentDate().toString("yyyyMMdd.log")));
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
