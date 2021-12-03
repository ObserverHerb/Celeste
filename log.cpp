#include <QDate>
#include "log.h"
#include "globals.h"

const char *SETTINGS_CATEGORY_LOGGING="Logging";

Log::Log(QObject *parent) : QObject(parent),
	settingLogDirectory(SETTINGS_CATEGORY_LOGGING,"Directory",Filesystem::DataPath().absoluteFilePath("logs"))
{
	file.setFileName(Directory().absoluteFilePath("current"));
	connect(this,&Log::Error,this,[this]() {
		file.close();
	});
}

Log::~Log()
{
	file.close();
}

bool Log::CreateDirectory()
{
	QDir logDirectory(settingLogDirectory);
	emit Print(QString("Creating log directory: %1").arg(logDirectory.absolutePath()));
	if (!logDirectory.exists() && !logDirectory.mkpath(logDirectory.absolutePath()))
	{
		emit Error(QString("Failed to create log directory: %1").arg(logDirectory.absolutePath()));
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
	emit Print("Creating log file.");
	if (!file.open(QIODevice::ReadWrite))
	{
		emit Error("Failed to create log file");
		 return false;
	}
	return true;
}

bool Log::Write(const QString &entry)
{
	if (file.write(StringConvert::ByteArray(QString("%1\n").arg(entry))) < 0 || !file.flush())
	{
		emit Error("Failed writing to log file.");
		return false;
	}
	return true;
}

bool Log::Archive()
{
	if (!file.reset())
	{
		emit Error("Failed to archive log file.");
		return false;
	}
	
	QFile datedFile(Directory().absoluteFilePath(QDate::currentDate().toString("yyyyMMdd.log")));
	if (datedFile.exists())
	{
		const QString appendingError="Failed appending to archived log file.";
		if (!datedFile.open(QIODevice::WriteOnly|QIODevice::Append))
		{
			emit Error(appendingError);
			return false;
		}
		while (!file.atEnd())
		{
			if (datedFile.write(file.read(4096)) < 0)
			{
				emit Error(appendingError);
				return false;
			}
		}
	}
	else
	{
		if (!file.rename(QFileInfo(datedFile).absoluteFilePath()))
		{
			emit Error("Failed to archive log file.");
			return false;
		}
	}
	file.close();
	return true;
}
