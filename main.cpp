#include <QApplication>
#include <QMessageBox>
#include <QInputDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QGridLayout>
#include <QListWidget>
#include <QJsonDocument>
#include <QJsonObject>
#include "window.h"
#include "channel.h"
#include "bot.h"
#include "log.h"
#include "eventsub.h"
#include "server.h"
#include "globals.h"
#include "security.h"
#include "pulsar.h"

const char *ORGANIZATION_NAME="Sky-Meyg";
const char *APPLICATION_NAME="Celeste";

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

	Security security;
	if (!security.Administrator()) security.Administrator().Set(QInputDialog::getText(nullptr,"Administrator","Please provide the account name of the Twitch user who will serve as the bot's administrator.").toLower());
	if (!security.ClientID()) security.ClientID().Set(QInputDialog::getText(nullptr,"Client ID","A client ID has not yet been set. Please provide your bot's client ID.",QLineEdit::Password));
	if (!security.ClientSecret()) security.ClientSecret().Set(QInputDialog::getText(nullptr,"Client Secret","A client secret has not yet been set. Please provide your bot's client secret.",QLineEdit::Password));
	if (!security.CallbackURL()) security.CallbackURL().Set(QInputDialog::getText(nullptr,"Callback URL","EventSub requires the URL at which your bot's server can reached. Please provide a callback URL."));
	if (!security.Scope())
	{
		QDialog scopeDialog;
		QGridLayout *layout=new QGridLayout(&scopeDialog);
		scopeDialog.setLayout(layout);
		QListWidget *listBox=new QListWidget(&scopeDialog);
		listBox->setSelectionMode(QAbstractItemView::ExtendedSelection);
		listBox->addItems(Security::SCOPES);
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
			security.Scope().Set(scopes.join(" "));
		}
		security.OAuthToken().Unset();
	}

	try
	{
		Log log;
		IRCSocket socket;
		Channel *channel=new Channel(security,&socket);
		Server server;
		Bot celeste(security);
		Pulsar pulsar;
#ifdef Q_OS_WIN
		Win32Window window;
#else
		Window window;
#endif

		security.connect(&security,&Security::TokenRequestFailed,[]() {
			QMessageBox failureDialog;
			failureDialog.setWindowTitle("Re-Authentication Failed");
			failureDialog.setText("Attempt to obtain OAuth token failed.");
			failureDialog.setStandardButtons(QMessageBox::Ok);
			failureDialog.setDefaultButton(QMessageBox::Ok);
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
		celeste.connect(&celeste,&Bot::AnnounceTextWall,&window,&Window::AnnounceTextWall);
		celeste.connect(&celeste,&Bot::AnnounceHost,&window,&Window::AnnounceHost);
		celeste.connect(&celeste,&Bot::SetAgenda,&window,&Window::SetAgenda);
		celeste.connect(&celeste,&Bot::ShowPortraitVideo,&window,&Window::ShowPortraitVideo);
		celeste.connect(&celeste,&Bot::ShowCurrentSong,&window,&Window::ShowCurrentSong);
		celeste.connect(&celeste,&Bot::ShowCommandList,&window,&Window::ShowCommandList);
		celeste.connect(&celeste,&Bot::ShowFollowage,&window,&Window::ShowFollowage);
		celeste.connect(&celeste,&Bot::ShowUptime,&window,&Window::ShowUptime);
		celeste.connect(&celeste,&Bot::ShowTotalTime,&window,&Window::ShowUptime);
		celeste.connect(&celeste,&Bot::Shoutout,&window,&Window::Shoutout);
		celeste.connect(&celeste,&Bot::PlayVideo,&window,&Window::PlayVideo);
		celeste.connect(&celeste,&Bot::PlayAudio,&window,&Window::PlayAudio);
		celeste.connect(&celeste,&Bot::Panic,&window,&Window::ShowPanicText);
		celeste.connect(&celeste,&Bot::Pulse,&pulsar,&Pulsar::Pulse);
		pulsar.connect(&pulsar,&Pulsar::Print,&log,&Log::Write);
		channel->connect(channel,&Channel::Print,&log,&Log::Write);
		channel->connect(channel,&Channel::Dispatch,&celeste,&Bot::ParseChatMessage);
		channel->connect(channel,&Channel::Ping,&celeste,&Bot::Ping);
		channel->connect(channel,&Channel::Joined,&window,&Window::ShowChat);
		channel->connect(channel,&Channel::Joined,[&echo,&log,&celeste,&window]() {
			log.disconnect(echo);
			celeste.connect(&celeste,&Bot::Print,&window,&Window::Print);
		});
		channel->connect(channel,&Channel::Disconnected,[channel]() {
			QMessageBox retryDialog;
			retryDialog.setWindowTitle("Connection Failed");
			retryDialog.setText("Failed to connect to Twitch. Would you like to try again?");
			retryDialog.setStandardButtons(QMessageBox::Yes|QMessageBox::No);
			retryDialog.setDefaultButton(QMessageBox::Yes);
			if (retryDialog.exec() == QMessageBox::No) return;
			channel->Connect();
		});
		channel->connect(channel,&Channel::Connected,[&security,&server,&celeste,&log]() {
			Viewer::Remote *viewer=new Viewer::Remote(security,security.Administrator());
			viewer->connect(viewer,&Viewer::Remote::Recognized,viewer,[&security,&log,&server,&celeste](Viewer::Local viewer) {
				EventSub *eventSub=new EventSub(security,viewer);
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
		});
		channel->connect(channel,&Channel::Connected,&security,&Security::StartClocks);
		QMetaObject::Connection reauthorize;
		channel->connect(channel,&Channel::Denied,[&reauthorize,&security,&server]() {
			QMessageBox authenticateDialog;
			authenticateDialog.setWindowTitle("Authentication Failed");
			authenticateDialog.setText("Twitch did not accept Celeste's credentials. Would you like to obtain a new OAuth token and retry?");
			authenticateDialog.setStandardButtons(QMessageBox::Yes|QMessageBox::No);
			authenticateDialog.setDefaultButton(QMessageBox::Yes);
			if (authenticateDialog.exec() == QMessageBox::No) return;

			reauthorize=server.connect(&server,&Server::Dispatch,[&reauthorize,&security,&server](qintptr socketID,const QUrlQuery &query) {
				server.disconnect(reauthorize); // break any connection with security's slots so we don't conflict with EventSub later
				emit server.SocketWrite(socketID,QJsonDocument(QJsonObject({
					{QUERY_PARAMETER_CODE,query.queryItemValue(QUERY_PARAMETER_CODE)},
					{QUERY_PARAMETER_SCOPE,query.queryItemValue(QUERY_PARAMETER_SCOPE)}
				})).toJson(QJsonDocument::Compact));
				security.RequestToken(query.queryItemValue(QUERY_PARAMETER_CODE),query.queryItemValue(QUERY_PARAMETER_SCOPE));
			});
			security.AuthorizeUser();
		});
		server.connect(&server,&Server::Print,&log,&Log::Write);
		application.connect(&application,&QApplication::aboutToQuit,[&log,&socket,channel]() {
			socket.connect(&socket,&IRCSocket::disconnected,&log,&Log::Archive);
			channel->disconnect(); // stops attempting to reconnect by removing all connections to signals
			channel->deleteLater();
		});

		if (!log.Open()) throw std::runtime_error("Could not open log file!");
		if (!server.Listen()) throw std::runtime_error("Could not start server!");
		pulsar.LoadTriggers();
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
