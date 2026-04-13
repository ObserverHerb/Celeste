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
#include "settings.h"
#include "widgets.h"
#include "entities.h"

static constexpr int PANE_PRIORITY_IMMEDIATE=std::numeric_limits<int>::max();
static constexpr int PANE_PRIORITY_DEFAULT=PANE_PRIORITY_IMMEDIATE/2;
static constexpr int PANE_PRIORITY_ROUTINE=PANE_PRIORITY_DEFAULT/2;
static constexpr int PANE_PRIORITY_INCONSEQUENTIAL=0;

struct Line
{
	QString text;
	double size;
};
using Lines=std::vector<Line>;

class PersistentPane: public QWidget
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
	void EnableScrollBar();
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

class ChatPane: public PersistentPane
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
	void Message(std::shared_ptr<Chat::Message> message) const;
	void DeleteMessage(const QString &id);
protected slots:
	void DismissStatus();
};

class EphemeralPane: public QWidget
{
	Q_OBJECT
public:
	EphemeralPane(QWidget *parent,int priority=PANE_PRIORITY_DEFAULT);
	bool operator<(const EphemeralPane &other) const;
	int Priority() const;
protected:
	bool expired;
	int priority;
	void Expire();
	virtual QString Subsystem()=0;
	virtual void Interrupt()=0;
	virtual void showEvent(QShowEvent *event) override;
	virtual void hideEvent(QHideEvent *event) override;
	bool event(QEvent *event) override;
signals:
	void Presented();
	void Finished();
	void Expired();
	void Print(const QString &message,const QString &operation,const QString &subsystem);
};

class PaneHost: public QWidget
{
	Q_OBJECT
public:
	PaneHost(const QColor &backgroundColor,QWidget *parent);
	PersistentPane* ReplacePersistent(PersistentPane *candidate);
	void StageEphemeral(EphemeralPane *candidate,bool bypassQueue=false);
	void PresentNextEphemeral();
protected:
	EphemeralPane *livePane;
	PersistentPane *persistentPane;
	std::priority_queue<EphemeralPane*,std::vector<EphemeralPane*>,decltype([](const EphemeralPane *left,const EphemeralPane *right)->bool {
		return left->Priority() < right->Priority();
	})> ephemeralPanes;
	static const int HIGH_PRIORITY_THRESHOLD;
	bool Chaos();
signals:
	void HighPriority();
	void LowPriority();
	void Print(const QString &message);
public slots:
	void Flush();
protected slots:
	void RestoreOrder();
	void ReflowLayout();
};

class VideoPane: public EphemeralPane
{
	Q_OBJECT
public:
	VideoPane(const QString &path,QWidget *parent,int priority) noexcept(false);
protected:
	QMediaPlayer *videoPlayer;
	QVideoWidget *viewport;
	void showEvent(QShowEvent *event) override;
	void Interrupt() override;
	QString Subsystem() override;
};

class ScrollingPane: public EphemeralPane
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
	QString Subsystem() override;
	void Interrupt() override { } // pausing happens in the ScrollingTextEdit
};

class AnnouncePane: public EphemeralPane
{
	Q_OBJECT
public:
	AnnouncePane(const Lines &lines,QWidget *parent,int priority);
	AnnouncePane(const QString &text,QWidget *parent,int priority);
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
	void Interrupt() override;
	void resizeEvent(QResizeEvent *event) override;
	QString Subsystem() override;
signals:
	void Resized(int width);
protected slots:
	void AdjustText(int width);
};

class AudioAnnouncePane: public AnnouncePane
{
	Q_OBJECT
public:
	AudioAnnouncePane(const Lines &lines,const QString &path,QWidget *parent,int priority);
	AudioAnnouncePane(const QString &text,const QString &path,QWidget *parent,int priority);
protected:
	QMediaPlayer *audioPlayer;
	QString path;
	void showEvent(QShowEvent *event) override;
	void Interrupt() override;
	QString Subsystem() override;
signals:
	void DurationAvailable(qint64 duration);
};

class ImageAnnouncePane: public AnnouncePane
{
	Q_OBJECT
public:
	ImageAnnouncePane(const Lines &lines,const QImage &image,QWidget *parent,int priority);
	ImageAnnouncePane(const QString &text,const QImage &image,QWidget *parent,int priority);
protected:
	QGraphicsScene *scene;
	QGraphicsView *view;
	QStackedWidget *stack;
	QGraphicsDropShadowEffect *shadow;
	QImage image;
	QGraphicsPixmapItem *pixmap;
	void Polish() override;
	void Interrupt() override { };
	void resizeEvent(QResizeEvent *event) override;
	QString Subsystem() override;
};

class MultimediaAnnouncePane: public AnnouncePane
{
	Q_OBJECT
public:
	MultimediaAnnouncePane(const Lines &lines,const QImage &image,const QString &path,QWidget *parent,int priority);
	MultimediaAnnouncePane(const QString &text,const QImage &image,const QString &path,QWidget *parent,int priority);
protected:
	MultimediaAnnouncePane(const QString &path,QWidget *parent,int priority);
	AudioAnnouncePane *audioPane;
	ImageAnnouncePane *imagePane;
	void Polish() override;
	void Interrupt() override;
	void showEvent(QShowEvent *event) override;
	QString Subsystem() override;
};
