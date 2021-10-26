#pragma once

#include <QObject>
#include <QString>
#include <QImage>

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
	const QString& DisplayName() const;
	const QImage& ProfileImage() const;
	const QString& Description() const;
protected:
	QString name;
	QString id;
	QString displayName;
	QString description;
	QImage profileImage;
	void DownloadProfileImage(const QString &url);
signals:
	void Error(const QString &message);
	void ProfileImageDownloaded();
	void Recognized(UserRecognizer *recognizer);
};
