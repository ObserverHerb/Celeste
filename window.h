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
#include "volume.h"

enum class Commands
{
	AGENDA,
	COMMANDS,
	EMOTE,
	PANIC,
	SHOUTOUT,
	SONG,
	TIMEZONE,
	UPDATE,
	UPTIME,
	VIBE,
	VOLUME
};

enum class SettingState
{
	ENABLE,
	DISABLE,
	TOGGLE
};

class Window : public QWidget
{
	Q_OBJECT
public:
	Window();
	bool event(QEvent *event) override;
protected:
	Channel *channel;
	PersistentPane *visiblePane;
	ChatPane *chatPane;
	Volume::Fader *vibeFader;
	QWidget *background;
	QMediaPlayer *vibeKeeper;
	QMediaPlaylist vibeSources;
	QMediaPlayer *roaster;
	QMediaPlaylist roastSources;
	QTimer helpClock;
	QTimer inactivityClock;
	QDateTime lastRaid;
	ApplicationSetting settingWindowSize;
	ApplicationSetting settingHelpCooldown;
	ApplicationSetting settingInactivityCooldown;
	ApplicationSetting settingVibePlaylist;
	ApplicationSetting settingRoasts;
	ApplicationSetting settingBackgroundColor;
	ApplicationSetting settingAccentColor;
	ApplicationSetting settingArrivalSound;
	ApplicationSetting settingSubscriptionSound;
	ApplicationSetting settingRaidSound;
	ApplicationSetting settingRaidInterruptDuration;
	ApplicationSetting settingPortraitVideo;
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
	void EmoteOnly(SettingState state);
signals:
	void Log(const QString &text);
	void Print(const QString text);
	void RequestEphemeralPane(EphemeralPane *pane);
public slots:
	void AnnounceArrival(Viewer viewer);
protected slots:
	void StageEphemeralPane(EphemeralPane *pane);
	void FollowChat(ChatMessageReceiver *chatMessageReceiver);
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
