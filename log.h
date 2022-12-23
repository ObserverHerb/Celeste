#pragma once

#include <QString>
#include <QFile>
#include "globals.h"
#include "settings.h"

class Entry
{
public:
	Entry(const QString &message,const QString &operation,const QString &subsystem="logger")
	{
		data=QString("== %1").arg(subsystem.toUpper());
		if (!operation.isEmpty()) data.append(QString(" (%1)").arg(operation.toUpper()));
		data.append(QString("\n%1").arg(message));
		if (data.back() != '\n') data.append("\n");
	}
	operator QString() const { return data; }
	operator QByteArray() const { return StringConvert::ByteArray(data); }
protected:
	QString data;
};

class Log : public QObject
{
	Q_OBJECT
public:
	Log(QObject *parent=nullptr);
	~Log();
	ApplicationSetting& Directory();
protected:
	ApplicationSetting settingLogDirectory;
	QFile file;
	bool CreateDirectory();
signals:
	void Print(const Entry &entry);
public slots:
	bool Open();
	void Write(const QString &message,const QString &operation,const QString &subsystem);
	void Archive();
};
