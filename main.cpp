#include <QApplication>
#include <QMessageBox>
#include "window.h"
#include "channel.h"
#include "bot.h"
#include "log.h"
#include "globals.h"
#include "subscribers.h"

enum
{
	OK,
	FATAL_ERROR,
	UNKNOWN_ERROR
};

int main(int argc,char *argv[])
{
	qputenv("QT_MULTIMEDIA_PREFERRED_PLUGINS", "windowsmediafoundation");

	QApplication application(argc,argv);
	application.setOrganizationName(ORGANIZATION_NAME);
	application.setApplicationName(APPLICATION_NAME);
	application.setWindowIcon(QIcon(":/celeste_256.png"));

	PrivateSetting settingAdministrator("Administrator");
	PrivateSetting settingClientID("ClientID");
	PrivateSetting settingOAuthToken("Token");

	try
	{
		Log log;
		IRCSocket socket;
		Channel *channel=new Channel(settingAdministrator,settingOAuthToken,&socket);
		Bot celeste(settingAdministrator,settingOAuthToken,settingClientID);
#ifdef Q_OS_WIN
		Win32Window window;
#else
		Window window;
#endif

		application.connect(&application,&QApplication::aboutToQuit,[&log,&socket,channel]() {
			socket.connect(&socket,&IRCSocket::disconnected,&log,&Log::Archive);
			channel->deleteLater();
		});
		QMetaObject::Connection echo=log.connect(&log,&Log::Print,&window,&Window::Print);
		celeste.connect(&celeste,&Bot::ChatMessage,&window,&Window::ChatMessage);
		celeste.connect(&celeste,&Bot::RefreshChat,&window,&Window::RefreshChat);
		celeste.connect(&celeste,&Bot::Print,&log,&Log::Write);
		celeste.connect(&celeste,&Bot::SetAgenda,&window,&Window::SetAgenda);
		celeste.connect(&celeste,&Bot::ShowCommandList,&window,&Window::ShowCommandList);
		celeste.connect(&celeste,&Bot::Shoutout,&window,&Window::Shoutout);
		celeste.connect(&celeste,&Bot::PlayVideo,&window,&Window::PlayVideo);
		celeste.connect(&celeste,&Bot::PlayAudio,&window,&Window::PlayAudio);
		channel->connect(channel,&Channel::Print,&log,&Log::Write);
		channel->connect(channel,&Channel::Dispatch,&celeste,&Bot::ParseChatMessage);
		channel->connect(channel,&Channel::Ping,&celeste,&Bot::Ping);
		channel->connect(channel,&Channel::Joined,&window,&Window::ShowChat);
		channel->connect(channel,&Channel::Joined,[&echo,&log,&celeste,&window]() {
			//QObject::disconnect(echo);
			//celeste.connect(&celeste,&Bot::Print,&window,&Window::Print);
		});

		PrivateSetting settingServerToken("ServerToken");
		/*EventSubscriber *eventSub=nullptr;
		Viewer::Remote *viewer=new Viewer::Remote(settingAdministrator,settingOAuthToken,settingClientID);
		viewer->connect(viewer,&Viewer::Remote::Recognized,viewer,[eventSub,&log,&window,&settingServerToken,&settingClientID](Viewer::Local viewer) mutable {
			eventSub=new EventSubscriber(viewer,settingServerToken,settingClientID);
			eventSub->Listen();
			eventSub->connect(eventSub,&EventSubscriber::Print,&log,&Log::Write);
			eventSub->connect(eventSub,&EventSubscriber::Redemption,&window,&Window::AnnounceRedemption);
			eventSub->connect(eventSub,&EventSubscriber::Raid,&window,&Window::AnnounceRaid);
			eventSub->connect(eventSub,&EventSubscriber::Subscription,&window,&Window::AnnounceSubscription);
			eventSub->Subscribe(SUBSCRIPTION_TYPE_REDEMPTION);
			eventSub->Subscribe(SUBSCRIPTION_TYPE_RAID);
			eventSub->Subscribe(SUBSCRIPTION_TYPE_SUBSCRIPTION);
		});*/

		if (!log.Open()) throw std::runtime_error("Could not open log file!");
		window.show();
		channel->Connect();

		return application.exec();
	}

	catch (const std::runtime_error &exception)
	{
		qFatal(exception.what());
		return FATAL_ERROR;
	}

	catch (...)
	{
		qFatal("An unknown error occurred!");
		return UNKNOWN_ERROR;
	}
}
