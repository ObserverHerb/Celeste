#pragma once

#include <QMainWindow>
#include <QTcpSocket>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <queue>
#include <unordered_map>
#include "command.h"
#include "settings.h"
#include "panes.h"

const QString SETTINGS_CATEGORY_VIBE="Vibe";
const QString SETTINGS_CATEGORY_AUTHORIZATION="Authorization";
const QString SETTINGS_CATEGORY_WINDOW="Window";

enum class BuiltInCommands
{
	PING,
	SONG,
	VIBE
};
const Command PingCommand("ping","Let the Twitch servers know I'm still alive",CommandType::DISPATCH,true);
const Command SongCommand("song","Show the title, album, and artist of the song that is currently playing.",CommandType::DISPATCH);
const Command VibeCommand("vibe","Start the playlist of music for the stream",CommandType::DISPATCH,true);
using BuiltInCommandLookup=std::unordered_map<QString,BuiltInCommands>;
const BuiltInCommandLookup BUILT_IN_COMMANDS={
	{PingCommand.Name(),BuiltInCommands::PING},
	{SongCommand.Name(),BuiltInCommands::SONG},
	{VibeCommand.Name(),BuiltInCommands::VIBE}
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
	QWidget *background;
	QMediaPlayer *vibeKeeper;
	QMediaPlaylist vibeSources;
	QTimer helpClock;
	Setting settingHelpCooldown;
	Setting settingVibePlaylist;
	Setting settingAdministrator;
	Setting settingOAuthToken;
	Setting settingJoinDelay;
	Setting settingBackgroundColor;
	std::queue<EphemeralPane*> ephemeralPanes;
	void SwapPane(Pane *pane);
	void Authenticate();
	void StageEphemeralPane(EphemeralPane *pane);
	void ReleaseLiveEphemeralPane();
	const QString CurrentSong() const;
signals:
	void Print(const QString text);
	void Dispatch(const QString data);
protected slots:
	void JoinStream();
	void FollowChat();
};

