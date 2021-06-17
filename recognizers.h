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


/*class UserRecognizer
{
public:
	UserRecognizer();
};*/
