#include <QEvent>
#include <QTimer>
#include <QLayout>
#include <QGridLayout>
#include <QtWin>
#include <chrono>
#include <stdexcept>
#include "window.h"
#include "receivers.h"

Window::Window() : QWidget(nullptr),
	ircSocket(nullptr),
	visiblePane(nullptr),
	background(new QWidget(this)),
	vibeKeeper(new QMediaPlayer(this)),
	settingVibePlaylist(SETTINGS_CATEGORY_VIBE,"Playlist"),
	settingAdministrator(SETTINGS_CATEGORY_AUTHORIZATION,"Administrator"),
	settingOAuthToken(SETTINGS_CATEGORY_AUTHORIZATION,"Token"),
	settingJoinDelay(SETTINGS_CATEGORY_AUTHORIZATION,"JoinDelay",5),
	settingBackgroundColor(SETTINGS_CATEGORY_WINDOW,"BackgroundColor","#ff000000")
{
	setAttribute(Qt::WA_TranslucentBackground,true);
	setFixedSize(537,467);

#ifdef Q_OS_WIN
	QtWin::enableBlurBehindWindow(this);
#endif

	setLayout(new QGridLayout(this));
	layout()->setContentsMargins(0,0,0,0);
	background->setLayout(new QGridLayout(background)); // for translucency to work, there has to be a widget covering the window, otherwise the entire thing is clear
	background->layout()->setContentsMargins(0,0,0,0);
	QColor backgroundColor=settingBackgroundColor;
	background->setStyleSheet(QString("background-color: rgba(%1,%2,%3,%4);").arg(
		StringConvert::Integer(backgroundColor.red()),
		StringConvert::Integer(backgroundColor.green()),
		StringConvert::Integer(backgroundColor.blue()),
		StringConvert::Integer(backgroundColor.alpha())));
	layout()->addWidget(background);

	SwapPane(new StatusPane(this));
}

Window::~Window()
{
	ircSocket->disconnectFromHost();
}

bool Window::event(QEvent *event)
{
	if (event->type() == QEvent::Polish)
	{
		ircSocket=new QTcpSocket(this);
		connect(ircSocket,&QTcpSocket::connected,this,&Window::Connected);
		connect(ircSocket,&QTcpSocket::readyRead,this,&Window::DataAvailable);
		Authenticate();
	}

	return QWidget::event(event);
}

void Window::Connected()
{
	emit Print("Connected!");
	Print(QString(IRC_COMMAND_USER).arg(static_cast<QString>(settingAdministrator)));
	emit Print("Sending credentials...");
	ircSocket->write(QString(IRC_COMMAND_PASSWORD).arg(static_cast<QString>(settingOAuthToken)).toLocal8Bit());
	ircSocket->write(QString(IRC_COMMAND_USER).arg(static_cast<QString>(settingAdministrator)).toLocal8Bit());
}

void Window::Disconnected()
{
	emit Print("Disconnected");
}

void Window::DataAvailable()
{
	QString data=ircSocket->readAll();
	emit Dispatch(data);
}

void Window::SwapPane(Pane *pane)
{
	if (visiblePane) visiblePane->deleteLater();
	visiblePane=pane;
	background->layout()->addWidget(visiblePane);
	connect(this,&Window::Print,visiblePane,&Pane::Print);
}

void Window::Authenticate()
{
	AuthenticationReceiver *authenticationReceiver=new AuthenticationReceiver(this);
	connect(this,&Window::Dispatch,authenticationReceiver,&AuthenticationReceiver::Process);
	connect(authenticationReceiver,&AuthenticationReceiver::Print,visiblePane,&Pane::Print);
	connect(authenticationReceiver,&AuthenticationReceiver::Succeeded,[this]() {
		Print(QString("Joining stream in %1 seconds...").arg(TimeConvert::Interval(TimeConvert::Seconds(settingJoinDelay))));
		QTimer::singleShot(TimeConvert::Interval(settingJoinDelay),this,&Window::JoinStream);
	});
	ircSocket->connectToHost(TWITCH_HOST,TWITCH_PORT);
	emit Print("Connecting to IRC...");
}

void Window::JoinStream()
{
	ChannelJoinReceiver *channelJoinReceiver=new ChannelJoinReceiver(this);
	connect(this,&Window::Dispatch,channelJoinReceiver,&ChannelJoinReceiver::Process);
	connect(channelJoinReceiver,&ChannelJoinReceiver::Print,visiblePane,&Pane::Print);
	connect(channelJoinReceiver,&ChannelJoinReceiver::Succeeded,this,&Window::FollowChat);
	ircSocket->write(StringConvert::ByteArray(IRC_COMMAND_JOIN.arg(static_cast<QString>(settingAdministrator))));
}

void Window::FollowChat()
{
	ChatPane *chatPane=new ChatPane(this);
	SwapPane(chatPane);
	ChatMessageReceiver *chatMessageReceiver=new ChatMessageReceiver(this);
	connect(this,&Window::Dispatch,chatMessageReceiver,&ChatMessageReceiver::Process);
	connect(chatMessageReceiver,&ChatMessageReceiver::Print,visiblePane,&Pane::Print);
	connect(chatMessageReceiver,&ChatMessageReceiver::PlayVideo,[this](const QString path) {
		StageEphemeralPane(new VideoPane(path));
	});
}

void Window::StageEphemeralPane(EphemeralPane *pane)
{
	connect(pane,&EphemeralPane::Finished,this,&Window::ReleaseLiveEphemeralPane);
	background->layout()->addWidget(pane);
	if (ephemeralPanes.size() > 0)
	{
		connect(ephemeralPanes.back(),&EphemeralPane::Finished,pane,&EphemeralPane::Show);
		ephemeralPanes.push(pane);
	}
	else
	{
		ephemeralPanes.push(pane);
		visiblePane->hide();
		pane->Show();
	}
}

void Window::ReleaseLiveEphemeralPane()
{
	if (ephemeralPanes.empty()) throw std::logic_error("Ran out of ephemeral panes but messages still coming in to remove them");
	ephemeralPanes.pop();
	if (ephemeralPanes.empty()) visiblePane->show();
}
