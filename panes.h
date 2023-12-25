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

using Line=std::pair<QString,double>;
using Lines=std::vector<Line>;

class PersistentPane : public QWidget
{
	Q_OBJECT
public:
	PersistentPane(QWidget *parent) : QWidget(parent) { }
public slots:
	virtual void Print(const QString &text)=0;
};

class StatusPane : public PersistentPane
{
	Q_OBJECT
public:
	StatusPane(QWidget *parent);
	ApplicationSetting& Font();
	ApplicationSetting& FontSize();
	ApplicationSetting& ForegroundColor();
	ApplicationSetting& BackgroundColor();
protected:
	StaticTextEdit output;
	ApplicationSetting settingFont;
	ApplicationSetting settingFontSize;
	ApplicationSetting settingForegroundColor;
	ApplicationSetting settingBackgroundColor;
	static const QString SETTINGS_CATEGORY;
signals:
	void ContextMenu(QContextMenuEvent *event);
public slots:
	void Print(const QString &text) override;
};

class ChatPane : public PersistentPane
{
	Q_OBJECT
public:
	ChatPane(QWidget *parent);
	void SetAgenda(const QString &text);
	ApplicationSetting& Font();
	ApplicationSetting& FontSize();
	ApplicationSetting& ForegroundColor();
	ApplicationSetting& BackgroundColor();
	ApplicationSetting& StatusInterval();
protected:
	QLabel *agenda;
	PinnedTextEdit *chat;
	QLabel *status;
	QTimer statusClock;
	std::queue<QString> statuses;
	ApplicationSetting settingFont;
	ApplicationSetting settingFontSize;
	ApplicationSetting settingForegroundColor;
	ApplicationSetting settingBackgroundColor;
	ApplicationSetting settingStatusInterval;
	static const QString SETTINGS_CATEGORY;
	void Format();
signals:
	void ContextMenu(QContextMenuEvent *event);
public slots:
	void Refresh();
	void Print(const QString &text) override;
	void Message(const Chat::Message &message) const;
protected slots:
	void DismissStatus();
};

class EphemeralPane : public QWidget
{
	Q_OBJECT
public:
	EphemeralPane(QWidget *parent,bool highPriority=true);
	void LowerPriority();
	bool HighPriority() const;
protected:
	bool expired;
	bool highPriority;
	void Expire();
signals:
	void Finished();
	void Expired();
	void Print(const QString &text);
};

class VideoPane : public EphemeralPane
{
	Q_OBJECT
public:
	VideoPane(const QString &path,QWidget *parent) noexcept(false);
protected:
	QMediaPlayer *videoPlayer;
	QVideoWidget *viewport;
	void showEvent(QShowEvent *event) override;
	void hideEvent(QHideEvent *event) override;
};

class ScrollingPane : public EphemeralPane
{
	Q_OBJECT
public:
	ScrollingPane(const QString &text,QWidget *parent);
protected:
	ScrollingTextEdit *commands;
	ApplicationSetting settingFont;
	ApplicationSetting settingFontSize;
	ApplicationSetting settingForegroundColor;
	ApplicationSetting settingBackgroundColor;
	static const QString SETTINGS_CATEGORY;
};

class AnnouncePane : public EphemeralPane
{
	Q_OBJECT
public:
	AnnouncePane(const Lines &lines,QWidget *parent);
	AnnouncePane(const QString &text,QWidget *parent);
	void Duration(const int duration) { clock.setInterval(duration); }
	ApplicationSetting& Font();
	ApplicationSetting& FontSize();
	ApplicationSetting& ForegroundColor();
	ApplicationSetting& BackgroundColor();
	ApplicationSetting& AccentColor();
	ApplicationSetting& Duration();
protected:
	Lines lines;
	QLabel *output;
	QTimer clock;
	ApplicationSetting settingDuration;
	ApplicationSetting settingFont;
	ApplicationSetting settingFontSize;
	ApplicationSetting settingForegroundColor;
	ApplicationSetting settingBackgroundColor;
	ApplicationSetting settingAccentColor;
	static const QString SETTINGS_CATEGORY;
	virtual void Polish();
	QString BuildParagraph(int width);
	void SingleLine(const QString &text);
	bool event(QEvent *event) override;
	void showEvent(QShowEvent *event) override;
	void hideEvent(QHideEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;
signals:
	void Resized(int width);
protected slots:
	void AdjustText(int width);
};

class AudioAnnouncePane : public AnnouncePane
{
	Q_OBJECT
public:
	AudioAnnouncePane(const Lines &lines,const QString &path,QWidget *parent);
	AudioAnnouncePane(const QString &text,const QString &path,QWidget *parent);
protected:
	QMediaPlayer *audioPlayer;
	QString path;
	void showEvent(QShowEvent *event) override;
	void hideEvent(QHideEvent *event) override;
signals:
	void DurationAvailable(qint64 duration);
};

class ImageAnnouncePane : public AnnouncePane
{
	Q_OBJECT
public:
	ImageAnnouncePane(const Lines &lines,const QImage &image,QWidget *parent);
	ImageAnnouncePane(const QString &text,const QImage &image,QWidget *parent);
protected:
	QGraphicsScene *scene;
	QGraphicsView *view;
	QStackedWidget *stack;
	QGraphicsDropShadowEffect *shadow;
	QImage image;
	QGraphicsPixmapItem *pixmap;
	void Polish() override;
	void resizeEvent(QResizeEvent *event) override;
};

class MultimediaAnnouncePane : public AnnouncePane
{
	Q_OBJECT
public:
	MultimediaAnnouncePane(const Lines &lines,const QImage &image,const QString &path,QWidget *parent);
	MultimediaAnnouncePane(const QString &text,const QImage &image,const QString &path,QWidget *parent);
protected:
	MultimediaAnnouncePane(const QString &path,QWidget *parent);
	AudioAnnouncePane *audioPane;
	ImageAnnouncePane *imagePane;
	void Polish() override;
	void showEvent(QShowEvent *event) override;
	void hideEvent(QHideEvent *event) override;
};
