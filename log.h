#pragma once

#include <QString>
#include <QFile>
#include "globals.h"
#include "settings.h"

class Log : public QObject
{
	Q_OBJECT
public:
	Log(QObject *parent=nullptr);
	~Log();
protected:
	ApplicationSetting settingLogDirectory;
	QFile file;
	bool CreateDirectory();
	QDir Directory();
signals:
	void Print(const QString &message);
	void Error(const QString &message);
public slots:
	bool Open();
	bool Write(const QString &entry);
	bool Archive();
};
