#pragma once

#include <QMainWindow>
#include <QTcpSocket>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <queue>
#include <unordered_map>
#include "globals.h"
#include "command.h"
#include "settings.h"
#include "volume.h"
#include "panes.h"

const QString SETTINGS_CATEGORY_VIBE="Vibe";
const QString SETTINGS_CATEGORY_WINDOW="Window";
const QString SETTINGS_CATEGORY_EVENTS="Events";

enum class BuiltInCommands
{
	AGENDA,
	PING,
	SONG,
	THINK,
	TIMEZONE,
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
const BuiltInCommand SongCommand("song","Show the title, album, and artist of the song that is currently playing");
const BuiltInCommand ThinkCommand("think","Play some thinking music for when Herb is thinking (too hard)");
const BuiltInCommand TimezoneCommand("timezone","Display the timezone of the system the bot is running on");
const BuiltInCommand UptimeCommand("uptime","Show how long the bot has been connected");
const ProtectedBuiltInCommand VibeCommand("vibe","Start the playlist of music for the stream");
const ProtectedBuiltInCommand VolumeCommand("volume","Adjust the volume of the vibe keeper");

using BuiltInCommandLookup=std::unordered_map<QString,BuiltInCommands>;
const BuiltInCommandLookup BUILT_IN_COMMANDS={
	{AgendaCommand.Name(),BuiltInCommands::AGENDA},
	{PingCommand.Name(),BuiltInCommands::PING},
	{SongCommand.Name(),BuiltInCommands::SONG},
	{ThinkCommand.Name(),BuiltInCommands::THINK},
	{TimezoneCommand.Name(),BuiltInCommands::TIMEZONE},
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
	void Connected();
	void Disconnected();
	void DataAvailable();
protected:
	QTcpSocket *ircSocket;
	Pane *visiblePane;
	Volume::Fader *vibeFader;
	QWidget *background;
	QMediaPlayer *vibeKeeper;
	QMediaPlaylist vibeSources;
	QTimer helpClock;
	Setting settingHelpCooldown;
	Setting settingVibePlaylist;
	Setting settingOAuthToken;
	Setting settingJoinDelay;
	Setting settingBackgroundColor;
	Setting settingAccentColor;
	Setting settingArrivalSound;
	Setting settingThinkingSound;
	std::queue<EphemeralPane*> ephemeralPanes;
	static std::chrono::milliseconds uptime;
	void SwapPane(Pane *pane);
	void Authenticate();
	void StageEmpeheralPane(EphemeralPane &&pane) { StageEphemeralPane(&pane); }
	void StageEphemeralPane(EphemeralPane *pane);
	void ReleaseLiveEphemeralPane();
	std::tuple<QString,QImage> CurrentSong() const;
	const QString Uptime() const;
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
