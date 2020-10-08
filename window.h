#pragma once

#include <QMainWindow>
#include <QTcpSocket>
#include <QTextToSpeech>
#include <queue>
#include "settings.h"
#include "panes.h"

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
	QTextToSpeech *vocalizer;
	SettingCategory categoryAuthorization; // TODO: these are good candidates for constexpr std::string (C++20)
	Setting settingAdministrator;
	Setting settingOAuthToken;
	Setting settingJoinDelay;
	SettingCategory categoryWindow;
	Setting settingBackgroundColor;
	Settings settings;
	std::queue<EphemeralPane*> ephemeralPanes;
	void SwapPane(Pane *pane);
	void Authenticate();
	void StageEphemeralPane(EphemeralPane *pane);
	void ReleaseLiveEphemeralPane();
signals:
	void Print(const QString text);
	void Dispatch(const QString data);
protected slots:
	void JoinStream();
	void FollowChat();
};

