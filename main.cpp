#include <QApplication>
#include <QMessageBox>
#include <QDesktopServices>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include "window.h"
#include "channel.h"
#include "bot.h"
#include "log.h"
#include "eventsub.h"
#include "server.h"
#include "globals.h"

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
	PrivateSetting settingClientSecret("ClientSecret");
	PrivateSetting settingOAuthToken("Token");
	PrivateSetting settingServerToken("ServerToken");
	PrivateSetting settingCallbackURL("CallbackURL");
	PrivateSetting settingScope("Permissions");

	try
	{
		Log log;
		IRCSocket socket;
		Channel *channel=new Channel(settingAdministrator,settingOAuthToken,&socket);
		Server server;
		Bot celeste(settingAdministrator,settingOAuthToken,settingClientID);
#ifdef Q_OS_WIN
		Win32Window window;
#else
		Window window;
#endif

		QMetaObject::Connection echo=log.connect(&log,&Log::Print,&window,&Window::Print);
		celeste.connect(&celeste,&Bot::ChatMessage,&window,&Window::ChatMessage);
		celeste.connect(&celeste,&Bot::RefreshChat,&window,&Window::RefreshChat);
		celeste.connect(&celeste,&Bot::Print,&log,&Log::Write);
		celeste.connect(&celeste,&Bot::AnnounceSubscription,&window,&Window::AnnounceSubscription);
		celeste.connect(&celeste,&Bot::AnnounceRaid,&window,&Window::AnnounceRaid);
		celeste.connect(&celeste,&Bot::AnnounceCheer,&window,&Window::AnnounceCheer);
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
			log.disconnect(echo);
			celeste.connect(&celeste,&Bot::Print,&window,&Window::Print);
		});
		QMetaObject::Connection retry=channel->connect(channel,&Channel::Disconnected,[channel]() {
			QMessageBox retryDialog;
			retryDialog.setWindowTitle("Connection Failed");
			retryDialog.setText("Failed to connect to Twitch. Would you like to try again?");
			retryDialog.setStandardButtons(QMessageBox::Yes|QMessageBox::No);
			retryDialog.setDefaultButton(QMessageBox::Yes);
			if (retryDialog.exec() == QMessageBox::No) return;
			channel->Connect();
		});
		channel->connect(channel,&Channel::Denied,[&settingClientID,&settingClientSecret,&settingOAuthToken,&settingCallbackURL,&settingScope,channel,&server]() {
			QMessageBox authenticateDialog;
			authenticateDialog.setWindowTitle("Authentication Failed");
			authenticateDialog.setText("Twitch did not accept Celeste's credentials. Would you like to obtain a new OAuth token and retry?");
			authenticateDialog.setStandardButtons(QMessageBox::Yes|QMessageBox::No);
			authenticateDialog.setDefaultButton(QMessageBox::Yes);
			if (authenticateDialog.exec() == QMessageBox::No) return;
			QMetaObject::Connection authenticate=server.connect(&server,&Server::Dispatch,[&settingClientID,&settingClientSecret,&settingOAuthToken,&settingCallbackURL,&settingScope,&server](qintptr socketID,const QUrlQuery &query) {
				const char *QUERY_KEY_CODE="code";
				const char *QUERY_KEY_SCOPE="scope";
				if (!query.hasQueryItem(QUERY_KEY_CODE) || !query.hasQueryItem(QUERY_KEY_SCOPE)) return;
				emit server.SocketWrite(socketID,QJsonDocument(QJsonObject({
					{QUERY_KEY_CODE,query.queryItemValue(QUERY_KEY_CODE)},
					{QUERY_KEY_SCOPE,query.queryItemValue(QUERY_KEY_SCOPE)}
				})).toJson(QJsonDocument::Compact));
				QNetworkAccessManager *manager=new QNetworkAccessManager();
				QNetworkRequest request({"https://id.twitch.tv/oauth2/token"});
				request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
				QUrlQuery payload({
					{QUERY_KEY_CODE,query.queryItemValue(QUERY_KEY_CODE)},
					{"client_id",settingClientID},
					{"client_secret",settingClientSecret},
					{"grant_type","authorization_code"},
					{"redirect_uri",settingCallbackURL}
				});
				QNetworkReply *reply=manager->post(request,StringConvert::ByteArray(payload.query()));
				manager->connect(reply,&QNetworkReply::finished,[manager,reply,&settingOAuthToken]() {
					reply->deleteLater();
					manager->deleteLater();
					QMessageBox failureDialog;
					failureDialog.setWindowTitle("Re-Authentication Failed");
					failureDialog.setText("Attempt to obtain OAuth token failed.");
					failureDialog.setStandardButtons(QMessageBox::Ok);
					failureDialog.setDefaultButton(QMessageBox::Ok);
					if (reply->error())
					{
						failureDialog.exec();
						return;
					}
					QJsonDocument json=QJsonDocument::fromJson(reply->readAll());
					if (json.isNull())
					{
						failureDialog.exec();
						return;
					}
					const char *JSON_KEY_ACCESS_TOKEN="access_token";
					if (!json.object().contains(JSON_KEY_ACCESS_TOKEN))
					{
						failureDialog.exec();
						return;
					}
					settingOAuthToken.Set(json.object().value(JSON_KEY_ACCESS_TOKEN).toString());
				});
			});
			QUrl request("https://id.twitch.tv/oauth2/authorize");
			request.setQuery(QUrlQuery({
				{"client_id",settingClientID},
				{"redirect_uri",settingCallbackURL},
				{"response_type","code"},
				{"scope",settingScope}
			 }));
			QDesktopServices::openUrl(request);
		});
		server.connect(&server,&Server::Print,&log,&Log::Write);
		application.connect(&application,&QApplication::aboutToQuit,[&log,&socket,channel,&retry]() {
			QObject::disconnect(retry);
			socket.connect(&socket,&IRCSocket::disconnected,&log,&Log::Archive);
			channel->deleteLater();
		});

		PrivateSetting settingServerToken("ServerToken");
		EventSub *eventSub=nullptr;
		Viewer::Remote *viewer=new Viewer::Remote(settingAdministrator,settingOAuthToken,settingClientID);
		viewer->connect(viewer,&Viewer::Remote::Recognized,viewer,[&log,&server,&window,&celeste,eventSub,&settingServerToken,&settingClientID,&settingCallbackURL](Viewer::Local viewer) mutable {
			eventSub=new EventSub(viewer,settingServerToken,settingClientID,settingCallbackURL);
			eventSub->connect(eventSub,&EventSub::Print,&log,&Log::Write);
			eventSub->connect(eventSub,&EventSub::Response,&server,&Server::SocketWrite);
			eventSub->connect(eventSub,&EventSub::Redemption,&window,&Window::AnnounceRedemption);
			eventSub->connect(eventSub,&EventSub::Subscription,&celeste,&Bot::Subscription);
			eventSub->connect(eventSub,&EventSub::Raid,&celeste,&Bot::Raid);
			eventSub->connect(eventSub,&EventSub::Cheer,&celeste,&Bot::Cheer);
			eventSub->Subscribe(SUBSCRIPTION_TYPE_REDEMPTION);
			eventSub->Subscribe(SUBSCRIPTION_TYPE_RAID);
			eventSub->Subscribe(SUBSCRIPTION_TYPE_SUBSCRIPTION);
			eventSub->Subscribe(SUBSCRIPTION_TYPE_CHEER);
			server.connect(&server,&Server::Dispatch,eventSub,&EventSub::ParseRequest);
		});

		if (!log.Open()) throw std::runtime_error("Could not open log file!");
		if (!server.Listen()) throw std::runtime_error("Could not start server!");
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
