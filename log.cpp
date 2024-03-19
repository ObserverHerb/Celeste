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
	Close();
}

bool Log::CreateDirectory()
{
	QDir directory(settingLogDirectory);
	const char *operation="creating log directory";
	Write({directory.absolutePath(),operation});
	if (!directory.exists() && !directory.mkpath(directory.absolutePath()))
	{
		Write({QString("Failed: %1").arg(directory.absolutePath()),operation});
		return false;
	}
	return true;
}

bool Log::Open()
{
	const char *operation="create log file";
	Write({file.fileName(),operation});
	if (!CreateDirectory()) return false;
	if (!file.open(QIODevice::ReadWrite|QIODevice::Truncate))
	{
		Write({"Failed",operation});
		return false;
	}
	return true;
}

void Log::Close()
{
	Write({file.fileName(),"close log file"});
	file.close();
}

void Log::Write(const Entry &entry)
{
	emit Print(entry);
	hold.push(entry);
	if (!file.isOpen()) return;
	while (!hold.empty())
	{
		if (file.write(hold.front()) < 0 || !file.flush())
		{
			hold.push({"Failed","write to log file"});
			return;
		}
		hold.pop();
	}
}

void Log::Receive(const QString &message,const QString &operation,const QString &subsystem)
{
	Write({message,operation,subsystem});
}

void Log::Archive()
{
	const char *operation="archive log";

	QFile datedFile(QDir(settingLogDirectory).absoluteFilePath(QDate::currentDate().toString("yyyyMMdd.log")));
	if (datedFile.exists())
	{
		if (!file.reset())
		{
			Write({"Failed to seek to beginning of file",operation});
			return;
		}

		if (!datedFile.open(QIODevice::WriteOnly|QIODevice::Append))
		{
			Write({"Could not open log file for today's date",operation});
			return;
		}
		while (!file.atEnd())
		{
			if (datedFile.write(file.read(4096)) < 0)
			{
				Write({"Could not append to archived log file",operation});
				return;
			}
		}
	}
	else
	{
		if (!file.rename(QFileInfo(datedFile).absoluteFilePath())) // rename will close the file for you, per Qt docs
		{
			Write({"Could not save log as archive file",operation});
			return;
		}
	}
}

ApplicationSetting& Log::Directory()
{
	return settingLogDirectory;
}