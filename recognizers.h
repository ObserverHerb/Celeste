#pragma once

#include <QObject>
#include <QString>

class FileRecognizer : public QObject
{
	Q_OBJECT
public:
	FileRecognizer(const QString &path);
	const QString File(const int index);
	const QString First();
	const QString Random();
	int RandomIndex();
protected:
	std::vector<QString> files;
};


class UserRecognizer : public QObject
{
	Q_OBJECT
public:
	UserRecognizer(const QString &username);
	const QString& Name() const;
	const QString& ID() const;
protected:
	QString name;
	QString id;
signals:
	void Error(const QString &message);
	void Recognized(UserRecognizer *recognizer);
};
