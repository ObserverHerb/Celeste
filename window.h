#pragma once

#include <QMainWindow>
#include <QTcpSocket>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QtConcurrent>
#include <queue>
#include <unordered_map>
#include "globals.h"
#include "command.h"
#include "settings.h"
#include "volume.h"
#include "panes.h"

inline const char *SETTINGS_CATEGORY_VIBE="Vibe";
inline const char *SETTINGS_CATEGORY_WINDOW="Window";
inline const char *SETTINGS_CATEGORY_EVENTS="Events";

enum class BuiltInCommands
{
	AGENDA,
	PING,
	SHOUTOUT,
	SONG,
	THINK,
	TIMEZONE,
	UPDATE,
	UPTIME,
	VIBE,
	VOLUME
};

class BuiltInCommand : public Command
{
public:
	BuiltInCommand(const QString &name,const QString &description) : Command(name,description,CommandType::DISPATCH,false) { }
};
class ProtectedBuiltInCommand : public Command
{
public:
	ProtectedBuiltInCommand(const QString &name,const QString &description) : Command(name,description,CommandType::DISPATCH,true) { }
};

const ProtectedBuiltInCommand AgendaCommand("agenda","Set the agenda of the stream, displayed in the header of the chat window");
const ProtectedBuiltInCommand PingCommand("ping","Let the Twitch servers know I'm still alive");
const BuiltInCommand ShoutOutCommand("so","Call attention to another streamer's channel");
const BuiltInCommand SongCommand("song","Show the title, album, and artist of the song that is currently playing");
const BuiltInCommand ThinkCommand("think","Play some thinking music for when Herb is thinking (too hard)");
const BuiltInCommand TimezoneCommand("timezone","Display the timezone of the system the bot is running on");
const ProtectedBuiltInCommand UpdateCommand("update","Refresh database of content such as emotes");
const BuiltInCommand UptimeCommand("uptime","Show how long the bot has been connected");
const ProtectedBuiltInCommand VibeCommand("vibe","Start the playlist of music for the stream");
const ProtectedBuiltInCommand VolumeCommand("volume","Adjust the volume of the vibe keeper");

using BuiltInCommandLookup=std::unordered_map<QString,BuiltInCommands>;
const BuiltInCommandLookup BUILT_IN_COMMANDS={
	{AgendaCommand.Name(),BuiltInCommands::AGENDA},
	{PingCommand.Name(),BuiltInCommands::PING},
	{ShoutOutCommand.Name(),BuiltInCommands::SHOUTOUT},
	{SongCommand.Name(),BuiltInCommands::SONG},
	{ThinkCommand.Name(),BuiltInCommands::THINK},
	{TimezoneCommand.Name(),BuiltInCommands::TIMEZONE},
	{UpdateCommand.Name(),BuiltInCommands::UPDATE},
	{UptimeCommand.Name(),BuiltInCommands::UPTIME},
	{VibeCommand.Name(),BuiltInCommands::VIBE},
	{VolumeCommand.Name(),BuiltInCommands::VOLUME}
};

class Window : public QWidget
{
	Q_OBJECT
public:
	Window();
	~Window();
	bool event(QEvent *event) override;
	void closeEvent(QCloseEvent *event) override;
	void Connected();
	void Disconnected();
	void DataAvailable();
protected:
	QTcpSocket *ircSocket;
	PersistentPane *visiblePane;
	Volume::Fader *vibeFader;
	QWidget *background;
	QMediaPlayer *vibeKeeper;
	QMediaPlaylist vibeSources;
	QTimer helpClock;
	QFile logFile;
	Setting settingWindowSize;
	Setting settingHelpCooldown;
	Setting settingVibePlaylist;
	Setting settingClientID;
	Setting settingOAuthToken;
	Setting settingJoinDelay;
	Setting settingChannel;
	Setting settingBackgroundColor;
	Setting settingAccentColor;
	Setting settingArrivalSound;
	Setting settingThinkingSound;
	std::queue<EphemeralPane*> ephemeralPanes;
	QFuture<void> worker;
	static std::chrono::milliseconds uptime;
	void SwapPane(PersistentPane *pane);
	void Authenticate();
	void StageEmpeheralPane(EphemeralPane &&pane) { StageEphemeralPane(&pane); }
	void StageEphemeralPane(EphemeralPane *pane);
	void ReleaseLiveEphemeralPane();
	std::tuple<QString,QImage> CurrentSong() const;
	const QString Uptime() const;
	const QSize ScreenThird();
signals:
	void Print(const QString text);
	void Dispatch(const QString data);
	void Ponging();
protected slots:
	void JoinStream();
	void FollowChat();
	void Pong() const;
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
