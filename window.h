#pragma once

#include <QMainWindow>
#include <QtConcurrent>
#include <queue>
#include <unordered_map>
#include "globals.h"
#include "panes.h"
#include "settings.h"
#include "subscribers.h"

class Window : public QWidget
{
	Q_OBJECT
public:
	Window();
protected:
	PersistentPane *visiblePane;
	QWidget *background;
	ApplicationSetting settingWindowSize;
	ApplicationSetting settingBackgroundColor;
	ApplicationSetting settingAccentColor;
	std::queue<EphemeralPane*> ephemeralPanes;
	void SwapPane(PersistentPane *pane);
	void ReleaseLiveEphemeralPane();
	const QSize ScreenThird();
signals:
	void Print(const QString &message);
	void ChatMessage(const QString &name,const QString &message,const std::vector<Media::Emote> &emotes,const QColor color,bool action);
	void SetAgenda(const QString &agenda);
	void RefreshChat();
public slots:
	void ShowChat();
	void AnnounceArrival(const QString &name,QImage profileImage,const QString &audioPath);
	void AnnounceRedemption(const QString &viewer,const QString &rewardTitle,const QString &message);
	void AnnounceSubscription(const QString &viewer);
	void AnnounceRaid(const QString& viewer,const unsigned int viewers);
	void PlayVideo(const QString &path);
	void PlayAudio(const QString &viewer,const QString &message,const QString &path);
	void ShowCommandList(std::vector<std::pair<QString,QString>> descriptions);
	void ShowCommand(const QString &name,const QString &description);
	void ShowPanicText(const QString &text);
	void Shoutout(const QString &name,const QString &description,const QImage &profileImage);
	void ShowTimezone(const QString &timezone);
	void ShowUptime(std::chrono::hours hours,std::chrono::minutes minutes,std::chrono::seconds seconds);
protected slots:
	void StageEphemeralPane(EphemeralPane *pane);
	void ShowCurrentSong(const QString &song,const QString &album,const QString &artist,const QImage coverArt);
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
