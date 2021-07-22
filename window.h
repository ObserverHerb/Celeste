#pragma once

#include <QMainWindow>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QtConcurrent>
#include <queue>
#include <unordered_map>
#include "channel.h"
#include "command.h"
#include "globals.h"
#include "panes.h"
#include "settings.h"
#include "subscribers.h"
#include "viewers.h"
#include "volume.h"

enum class Commands
{
	AGENDA,
	COMMANDS,
	PANIC,
	SHOUTOUT,
	SONG,
	TIMEZONE,
	UPDATE,
	UPTIME,
	VIBE,
	VOLUME
};

class Window : public QWidget
{
	Q_OBJECT
public:
	Window();
	~Window();
	bool event(QEvent *event) override;
	void closeEvent(QCloseEvent *event) override;
protected:
	Channel *channel;
	PersistentPane *visiblePane;
	ChatPane *chatPane;
	Volume::Fader *vibeFader;
	QWidget *background;
	QMediaPlayer *vibeKeeper;
	QMediaPlaylist vibeSources;
	QTimer helpClock;
	QFile logFile;
	Setting settingWindowSize;
	Setting settingHelpCooldown;
	Setting settingVibePlaylist;
	Setting settingBackgroundColor;
	Setting settingAccentColor;
	Setting settingArrivalSound;
	Setting settingSubscriptionSound;
	Setting settingRaidSound;
	std::queue<EphemeralPane*> ephemeralPanes;
	QFuture<void> worker;
	static std::chrono::milliseconds uptime;
	static const char *SETTINGS_CATEGORY_VIBE;
	static const char *SETTINGS_CATEGORY_WINDOW;
	static const char *SETTINGS_CATEGORY_EVENTS;
	void BuildEventSubscriber();
	void SwapPane(PersistentPane *pane);
	void ReleaseLiveEphemeralPane();
	std::tuple<QString,QImage> CurrentSong() const;
	const QString Uptime() const;
	const QSize ScreenThird();
	virtual EventSubscriber* CreateEventSubscriber(const QString &administratorID);
	virtual const QString ArrivalSound() const;
signals:
	void Print(const QString text);
	void RequestEphemeralPane(AudioAnnouncePane *pane);
public slots:
	void AnnounceArrival(const Viewer &viewer);
protected slots:
	void StageEphemeralPane(EphemeralPane *pane);
	void FollowChat(ChatMessageReceiver &chatMessageReceiver);
	void Log(const QString &text);
};

#ifdef Q_OS_WIN
#include <QtWin>
class Win32Window : public Window
{
	Q_OBJECT
public:
	Win32Window()
	{
		QtWin::enableBlurBehindWindow(this);
	}
};
#endif
