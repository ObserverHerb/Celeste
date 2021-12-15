#pragma once

#include <QWidget>
#include <QLabel>
#include <QTextEdit>
#include <QStackedWidget>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsPixmapItem>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QTimer>
#include <QEvent>
#include <queue>
#include <unordered_map>
#include "settings.h"
#include "widgets.h"
#include "entities.h"

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
	QTimer statusClock;
	std::queue<QString> statuses;
	ApplicationSetting settingFont;
	ApplicationSetting settingFontSize;
	ApplicationSetting settingForegroundColor;
	ApplicationSetting settingBackgroundColor;
	ApplicationSetting settingStatusInterval;
	static const QString SETTINGS_CATEGORY;
public slots:
	void Refresh();
	void Print(const QString &text) override;
	void Message(const QString &name,const QString &message,const std::vector<Media::Emote> &emotes,const QStringList &badgeIcons,const QColor color,bool action) const;
protected slots:
	void DismissStatus();
};

class EphemeralPane : public QWidget
{
	Q_OBJECT
public:
	EphemeralPane(QWidget *parent=nullptr);
	virtual void Show()=0;
	bool Expired();
protected:
	bool expired;
signals:
	void Finished();
	void Print(const QString &text);
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
	bool event(QEvent *event) override;
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
	AudioAnnouncePane(const QString &text,const QString &path,QWidget *parent=nullptr);
	AudioAnnouncePane(const std::vector<std::pair<QString,double>> &lines,const QString &path,QWidget *parent=nullptr);
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
	void resizeEvent(QResizeEvent *event) override;
protected:
	QGraphicsScene *scene;
	QGraphicsView *view;
	QStackedWidget *stack;
	QGraphicsDropShadowEffect *shadow;
	QImage image;
	QGraphicsPixmapItem *pixmap;
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
