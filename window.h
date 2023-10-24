#pragma once

#include <QMainWindow>
#include <QAction>
#include <queue>
#include <unordered_map>
#include "globals.h"
#include "panes.h"
#include "settings.h"
#include "widgets.h"

class Window : public QMainWindow
{
	Q_OBJECT
public:
	Window();
	ApplicationSetting& BackgroundColor();
	ApplicationSetting& Dimensions();
protected:
	QWidget *background;
	PersistentPane *livePersistentPane;
	std::queue<EphemeralPane*> highPriorityEphemeralPanes;
	std::queue<EphemeralPane*> lowPriorityEphemeralPanes;
	ApplicationSetting settingWindowSize;
	ApplicationSetting settingBackgroundColor;
	QAction configureOptions;
	QAction configureCommands;
	QAction configureEventSubscriptions;
	QAction metrics;
	QAction vibePlaylist;
	void SwapPersistentPane(PersistentPane *pane);
	void ReleaseLiveEphemeralPane();
	const QSize ScreenThird();
	void contextMenuEvent(QContextMenuEvent *event) override;
	void closeEvent(QCloseEvent *event) override;
signals:
	void Print(const QString &message);
	void ChatMessage(const Chat::Message &message);
	void SetAgenda(const QString &agenda);
	void RefreshChat();
	void SuppressMusic();
	void RestoreMusic();
	void ConfigureOptions();
	void ConfigureCommands();
	void ConfigureEventSubscriptions();
	void ShowMetrics();
	void ShowVibePlaylist();
	void CloseRequested(QCloseEvent *event);
public slots:
	void ShowChat();
	void AnnounceArrival(const QString &name,QImage profileImage,const QString &audioPath);
	void AnnounceRedemption(const QString &name,const QString &rewardTitle,const QString &message);
	void AnnounceSubscription(const QString &name,const QString &audioPath);
	void AnnounceRaid(const QString &name,const unsigned int viewers,const QString &audioPath);
	void AnnounceCheer(const QString &name,const unsigned int count,const QString &message,const QString &videoPath);
	void AnnounceTextWall(const QString &message,const QString &audioPath);
	void AnnounceDeniedCommand(const QString &videoPath);
	void AnnounceHypeTrainProgress(int level,double progress);
	void PlayVideo(const QString &path);
	void PlayAudio(const QString &viewer,const QString &message,const QString &path);
	void ShowPortraitVideo(const QString &path);
	void ShowCurrentSong(const QString &song,const QString &album,const QString &artist,const QImage coverArt);
	void ShowCommandList(std::vector<std::tuple<QString,QStringList,QString>> descriptions);
	void ShowCommand(const QString &name,const QString &description);
	void ShowPanicText(const QString &text);
	void Shoutout(const QString &name,const QString &description,const QImage &profileImage);
	void ShowFollowage(const QString &name,std::chrono::years years,std::chrono::months months,std::chrono::days days);
	void ShowTimezone(const QString &timezone);
	void ShowUptime(std::chrono::hours hours,std::chrono::minutes minutes,std::chrono::seconds seconds);
protected slots:
	void StageEphemeralPane(EphemeralPane *pane);
};

class Win32Window : public Window
{
public:
	Win32Window();
};
