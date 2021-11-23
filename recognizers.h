#pragma once

#include <QObject>
#include <QString>
#include <QImage>
#include <memory>
#include <unordered_map>

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

class ProfileImage : public QObject
{
	Q_OBJECT
public:
	ProfileImage(const QUrl &profileImageURL);
	operator QImage() const;
protected:
	QImage image;
signals:
	void Error(const QString &message);
	void ProfileImageDownloaded(const QImage &image);
};

class Viewer
{
public:
	Viewer(const QString &name,const QString &id,const QString &displayName,std::shared_ptr<class ProfileImage> profileImage,const QString &description) : name(name), id(id), displayName(displayName), profileImage(profileImage), description(description) { }
	const QString& Name() const;
	const QString& ID() const;
	const QString& DisplayName() const;
	std::shared_ptr<class ProfileImage> ProfileImage() const;
	const QString& Description() const;
protected:
	QString name;
	QString id;
	QString displayName;
	std::shared_ptr<class ProfileImage> profileImage;
	QString description;
};
using Viewers=std::unordered_map<QString,Viewer>;

class UserRecognizer : public QObject
{
	Q_OBJECT
public:
	UserRecognizer(const QString &username);
protected:
	QString name;
	void DownloadProfileImage(const QString &url);
signals:
	void Error(const QString &message);
	void Recognized(Viewer viewer);
};
