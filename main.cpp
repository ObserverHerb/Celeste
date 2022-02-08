#include <QApplication>
#include <QMessageBox>
#include <QInputDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QGridLayout>
#include <QListWidget>
#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonObject>
#include "window.h"
#include "channel.h"
#include "bot.h"
#include "log.h"
#include "eventsub.h"
#include "server.h"
#include "globals.h"

const char *ORGANIZATION_NAME="Sky-Meyg";
const char *APPLICATION_NAME="Celeste";
const char *JSON_KEY_ACCESS_TOKEN="access_token";
const char *JSON_KEY_REFRESH_TOKEN="refresh_token";

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

#ifdef DEVELOPER_MODE
	if (QMessageBox(QMessageBox::Warning,"DEVELOPER MODE","**WARNING** Celeste is currently in developer mode. Sensitive data will be displayed in the main window and written to the log. Only proceed if you know what you are doing. Continue?",QMessageBox::Yes|QMessageBox::No).exec() == QMessageBox::No) return OK;
#endif

	PrivateSetting settingAdministrator("Administrator");
	if (!settingAdministrator) settingAdministrator.Set(QInputDialog::getText(nullptr,"Administrator","Please provide the account name of the Twitch user who will serve as the bot's administrator.").toLower());
	PrivateSetting settingClientID("ClientID");
	if (!settingClientID) settingClientID.Set(QInputDialog::getText(nullptr,"Client ID","A client ID has not yet been set. Please provide your bot's client ID.",QLineEdit::Password));
	PrivateSetting settingClientSecret("ClientSecret");
	if (!settingClientSecret) settingClientSecret.Set(QInputDialog::getText(nullptr,"Client Secret","A client secret has not yet been set. Please provide your bot's client secret.",QLineEdit::Password));
	PrivateSetting settingOAuthToken("Token");
	PrivateSetting settingRefreshToken("RefreshToken");
	PrivateSetting settingServerToken("ServerToken");
	PrivateSetting settingCallbackURL("CallbackURL");
	if (!settingCallbackURL) settingCallbackURL.Set(QInputDialog::getText(nullptr,"Callback URL","EventSub requires the URL at which your bot's server can reached. Please provide a callback URL."));
	PrivateSetting settingScope("Permissions");
	if (!settingScope)
	{
		QDialog scopeDialog;
		QGridLayout *layout=new QGridLayout(&scopeDialog);
		scopeDialog.setLayout(layout);
		QListWidget *listBox=new QListWidget(&scopeDialog);
		listBox->setSelectionMode(QAbstractItemView::ExtendedSelection);
		listBox->addItems({"analytics:read:extensions","analytics:read:games","bits:read","channel:edit:commercial","channel:manage:broadcast","channel:manage:extensions","channel:manage:polls","channel:manage:predictions","channel:manage:redemptions","channel:manage:schedule","channel:manage:videos","channel:moderate","channel:read:editors","channel:read:goals","channel:read:hype_train","channel:read:polls","channel:read:predictions","channel:read:redemptions","channel:read:stream_key","channel:read:subscriptions","chat:edit","clips:edit","moderation:read","moderator:manage:banned_users","moderator:read:blocked_terms","moderator:manage:blocked_terms","moderator:manage:automod","moderator:read:automod_settings","moderator:manage:automod_settings","moderator:read:chat_settings","moderator:manage:chat_settings","user:edit","user:edit:follows","user:manage:blocked_users","user:read:blocked_users","user:read:broadcast","user:read:email","user:read:follows","user:read:subscriptions","whispers:edit","whispers:read"});
		scopeDialog.layout()->addWidget(listBox);
		QDialogButtonBox *buttons=new QDialogButtonBox(&scopeDialog);
		QPushButton *okay=buttons->addButton(QDialogButtonBox::Ok);
		okay->setDefault(true);
		okay->connect(okay,&QPushButton::clicked,&scopeDialog,&QDialog::accept);
		QPushButton *cancel=buttons->addButton(QDialogButtonBox::Cancel);
		okay->connect(cancel,&QPushButton::clicked,&scopeDialog,&QDialog::reject);
		scopeDialog.layout()->addWidget(buttons);
		if (scopeDialog.exec() == QDialog::Accepted)
		{
			QStringList scopes({"chat:read"});
			for (QListWidgetItem *item : listBox->selectedItems()) scopes.append(item->text());
			settingScope.Set(scopes.join(" "));
		}
		settingOAuthToken.Unset();
	}

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

		QTimer tokenTimer;
		tokenTimer.setInterval(3600000); // 1 hour
		tokenTimer.connect(&tokenTimer,&QTimer::timeout,[&settingRefreshToken,&settingOAuthToken,&settingClientID,&settingClientSecret]() {
			Network::Request({"https://id.twitch.tv/oauth2/token"},Network::Method::POST,[&settingRefreshToken,&settingOAuthToken,&settingClientID,&settingClientSecret](QNetworkReply *reply) {
				// TODO: can I implement more intelligent error handling here?
				if (reply->error()) return;
				QString uhoh=reply->readAll();
				QJsonDocument json=QJsonDocument::fromJson(StringConvert::ByteArray(uhoh));
				if (json.isNull()) return;
				if (!json.object().contains(JSON_KEY_REFRESH_TOKEN) || !json.object().contains(JSON_KEY_ACCESS_TOKEN)) return;
				settingOAuthToken.Set(json.object().value(JSON_KEY_ACCESS_TOKEN).toString());
				settingRefreshToken.Set(json.object().value(JSON_KEY_REFRESH_TOKEN).toString());
			},{
				{"client_id",settingClientID},
				{"client_secret",settingClientSecret},
				{"grant_type","refresh_token"},
				{"refresh_token",settingRefreshToken}
			},{
				{"Content-Type","application/x-www-form-urlencoded"}
			});
		});

		QMetaObject::Connection echo=log.connect(&log,&Log::Print,&window,&Window::Print);
		celeste.connect(&celeste,&Bot::ChatMessage,&window,&Window::ChatMessage);
		celeste.connect(&celeste,&Bot::RefreshChat,&window,&Window::RefreshChat);
		celeste.connect(&celeste,&Bot::Print,&log,&Log::Write);
		celeste.connect(&celeste,&Bot::AnnounceArrival,&window,&Window::AnnounceArrival);
		celeste.connect(&celeste,&Bot::AnnounceRedemption,&window,&Window::AnnounceRedemption);
		celeste.connect(&celeste,&Bot::AnnounceSubscription,&window,&Window::AnnounceSubscription);
		celeste.connect(&celeste,&Bot::AnnounceRaid,&window,&Window::AnnounceRaid);
		celeste.connect(&celeste,&Bot::AnnounceCheer,&window,&Window::AnnounceCheer);
		celeste.connect(&celeste,&Bot::SetAgenda,&window,&Window::SetAgenda);
		celeste.connect(&celeste,&Bot::ShowPortraitVideo,&window,&Window::ShowPortraitVideo);
		celeste.connect(&celeste,&Bot::ShowCurrentSong,&window,&Window::ShowCurrentSong);
		celeste.connect(&celeste,&Bot::ShowCommandList,&window,&Window::ShowCommandList);
		celeste.connect(&celeste,&Bot::ShowUptime,&window,&Window::ShowUptime);
		celeste.connect(&celeste,&Bot::Shoutout,&window,&Window::Shoutout);
		celeste.connect(&celeste,&Bot::PlayVideo,&window,&Window::PlayVideo);
		celeste.connect(&celeste,&Bot::PlayAudio,&window,&Window::PlayAudio);
		celeste.connect(&celeste,&Bot::Panic,&window,&Window::ShowPanicText);
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
		channel->connect(channel,&Channel::Connected,[&server,&window,&celeste,&log,&settingAdministrator,&settingOAuthToken,&settingServerToken,&settingClientID,&settingCallbackURL,&tokenTimer]() {
			EventSub *eventSub=nullptr;
			Viewer::Remote *viewer=new Viewer::Remote(settingAdministrator,settingOAuthToken,settingClientID);
			viewer->connect(viewer,&Viewer::Remote::Recognized,viewer,[&log,&server,&window,&celeste,eventSub,&settingServerToken,&settingClientID,&settingCallbackURL](Viewer::Local viewer) mutable {
				eventSub=new EventSub(viewer,settingServerToken,settingClientID,settingCallbackURL);
				eventSub->connect(eventSub,&EventSub::Print,&log,&Log::Write);
				eventSub->connect(eventSub,&EventSub::Response,&server,&Server::SocketWrite);
				eventSub->connect(eventSub,&EventSub::Redemption,&celeste,&Bot::Redemption);
				eventSub->connect(eventSub,&EventSub::Subscription,&celeste,&Bot::Subscription);
				eventSub->connect(eventSub,&EventSub::Raid,&celeste,&Bot::Raid);
				eventSub->connect(eventSub,&EventSub::Cheer,&celeste,&Bot::Cheer);
				eventSub->Subscribe(SUBSCRIPTION_TYPE_REDEMPTION);
				eventSub->Subscribe(SUBSCRIPTION_TYPE_RAID);
				eventSub->Subscribe(SUBSCRIPTION_TYPE_SUBSCRIPTION);
				eventSub->Subscribe(SUBSCRIPTION_TYPE_CHEER);
				server.connect(&server,&Server::Dispatch,eventSub,&EventSub::ParseRequest);
			});
			tokenTimer.start();
		});
		channel->connect(channel,&Channel::Denied,[&settingClientID,&settingClientSecret,&settingOAuthToken,&settingRefreshToken,&settingCallbackURL,&settingScope,channel,&server]() {
			QMessageBox authenticateDialog;
			authenticateDialog.setWindowTitle("Authentication Failed");
			authenticateDialog.setText("Twitch did not accept Celeste's credentials. Would you like to obtain a new OAuth token and retry?");
			authenticateDialog.setStandardButtons(QMessageBox::Yes|QMessageBox::No);
			authenticateDialog.setDefaultButton(QMessageBox::Yes);
			if (authenticateDialog.exec() == QMessageBox::No) return;
			QMetaObject::Connection authenticate=server.connect(&server,&Server::Dispatch,[&settingClientID,&settingClientSecret,&settingOAuthToken,&settingRefreshToken,&settingCallbackURL,&settingScope,&server](qintptr socketID,const QUrlQuery &query) {
				const char *QUERY_KEY_CODE="code";
				const char *QUERY_KEY_SCOPE="scope";
				if (!query.hasQueryItem(QUERY_KEY_CODE) || !query.hasQueryItem(QUERY_KEY_SCOPE)) return;
				emit server.SocketWrite(socketID,QJsonDocument(QJsonObject({
					{QUERY_KEY_CODE,query.queryItemValue(QUERY_KEY_CODE)},
					{QUERY_KEY_SCOPE,query.queryItemValue(QUERY_KEY_SCOPE)}
				})).toJson(QJsonDocument::Compact));
				Network::Request({"https://id.twitch.tv/oauth2/token"},Network::Method::POST,[&settingOAuthToken,&settingRefreshToken](QNetworkReply *reply) {
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
					if (!json.object().contains(JSON_KEY_ACCESS_TOKEN) || !json.object().contains(JSON_KEY_REFRESH_TOKEN))
					{
						failureDialog.exec();
						return;
					}
					settingOAuthToken.Set(json.object().value(JSON_KEY_ACCESS_TOKEN).toString());
					settingRefreshToken.Set(json.object().value(JSON_KEY_REFRESH_TOKEN).toString());
				},{
					{QUERY_KEY_CODE,query.queryItemValue(QUERY_KEY_CODE)},
					{"client_id",settingClientID},
					{"client_secret",settingClientSecret},
					{"grant_type","authorization_code"},
					{"redirect_uri",settingCallbackURL}
				},{
					{"Content-Type","application/x-www-form-urlencoded"}
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
