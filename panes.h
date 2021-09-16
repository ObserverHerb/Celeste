#pragma once

#include <QWidget>
#include <QLabel>
#include <QTextEdit>
#include <QStackedWidget>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsDropShadowEffect>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QTimer>
#include <queue>
#include <unordered_map>
#include "relay.h"
#include "settings.h"
#include "widgets.h"

class PersistentPane : public QWidget
{
	Q_OBJECT
public:
	PersistentPane(QWidget *parent=nullptr) : QWidget(parent) { }
public slots:
	virtual void Print(const QString &text)=0;
};

class StatusPane : public PersistentPane
{
	Q_OBJECT
public:
	StatusPane(QWidget *parent);
protected:
	QTextEdit *output;
	ApplicationSetting settingFont;
	ApplicationSetting settingFontSize;
	ApplicationSetting settingForegroundColor;
	ApplicationSetting settingBackgroundColor;
	static const QString SETTINGS_CATEGORY;
public slots:
	void Print(const QString &text) override;
};

class ChatPane : public PersistentPane
{
	Q_OBJECT
public:
	ChatPane(QWidget *parent);
	void SetAgenda(const QString &text);
protected:
	QLabel *agenda;
	ScrollingTextEdit *chat;
	QLabel *status;
	std::queue<Relay::Status::Package> statusUpdates;
	QTimer statusClock;
	ApplicationSetting settingFont;
	ApplicationSetting settingFontSize;
	ApplicationSetting settingForegroundColor;
	ApplicationSetting settingBackgroundColor;
	ApplicationSetting settingStatusInterval;
	static const QString SETTINGS_CATEGORY;
	void ResetStatusClock();
public slots:
	void Refresh();
	void Print(const QString &text) override;
	Relay::Status::Context* Alert(const QString &text);
protected slots:
	void DismissAlert();
};

class EphemeralPane : public QWidget
{
	Q_OBJECT
public:
	EphemeralPane(QWidget *parent=nullptr);
	virtual void Show()=0;
signals:
	void Finished();
	void Error(const QString &description);
};

class VideoPane : public EphemeralPane
{
	Q_OBJECT
public:
	VideoPane(const QString &path,QWidget *parent=nullptr);
	void Show() override;
protected:
	QMediaPlayer *videoPlayer;
	QVideoWidget *viewport;
};

class AnnouncePane : public EphemeralPane
{
	Q_OBJECT
public:
	AnnouncePane(const QString &text,QWidget *parent=nullptr);
	AnnouncePane(const std::vector<std::pair<QString,double>> &lines,QWidget *parent=nullptr);
	void Show() override;
	virtual void Polish();
	void AccentColor(const QColor &color) { accentColor=color; }
	void Duration(const int duration) { clock.setInterval(duration); }
	const QString BuildParagraph(const std::vector<std::pair<QString,double>> &lines);
protected:
	QColor accentColor;
	QLabel *output;
	QTimer clock;
	ApplicationSetting settingDuration;
	ApplicationSetting settingFont;
	ApplicationSetting settingFontSize;
	ApplicationSetting settingForegroundColor;
	ApplicationSetting settingBackgroundColor;
	static const QString SETTINGS_CATEGORY;
};

class AudioAnnouncePane : public AnnouncePane
{
	Q_OBJECT
public:
	AudioAnnouncePane(const QString &text,const StringConvert::Valid &path,QWidget *parent=nullptr);
	AudioAnnouncePane(const std::vector<std::pair<QString,double>> &lines,const StringConvert::Valid &path,QWidget *parent=nullptr);
	void Show() override;
protected:
	QMediaPlayer *audioPlayer;
signals:
	void DurationAvailable(qint64 duration);
};

class ImageAnnouncePane : public AnnouncePane
{
	Q_OBJECT
public:
	ImageAnnouncePane(const QString &text,const QImage &image,QWidget *parent=nullptr);
	ImageAnnouncePane(const std::vector<std::pair<QString,double>> &lines,const QImage &image,QWidget *parent=nullptr);
	void Polish() override;
protected:
	QGraphicsScene *scene;
	QGraphicsView *view;
	QStackedWidget *stack;
	QGraphicsDropShadowEffect *shadow;
	QImage image;
};

class MultimediaAnnouncePane : public AnnouncePane
{
	Q_OBJECT
public:
	MultimediaAnnouncePane(const QString &text,const QImage &image,const QString &path,QWidget *parent=nullptr);
	MultimediaAnnouncePane(const std::vector<std::pair<QString,double>> &lines,const QImage &image,const QString &path,QWidget *parent=nullptr);
	void Polish() override;
	void Show() override;
protected:
	MultimediaAnnouncePane(const QString &path,QWidget *parent=nullptr);
	AudioAnnouncePane *audioPane;
	ImageAnnouncePane *imagePane;
};
