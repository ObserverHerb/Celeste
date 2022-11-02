#include <QApplication>
#include <QCloseEvent>
#include <QMessageBox>
#include <QInputDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QGridLayout>
#include <QListWidget>
#include <QJsonDocument>
#include <QJsonObject>
#include "window.h"
#include "widgets.h"
#include "channel.h"
#include "bot.h"
#include "log.h"
#include "eventsub.h"
#include "server.h"
#include "globals.h"
#include "security.h"
#include "pulsar.h"

const char *ORGANIZATION_NAME="EngineeringDeck";
const char *APPLICATION_NAME="Celeste";
const char *SUBSYSTEM="initialization";

enum
{
	OK,
	NOT_CONFIGURED,
	FATAL_ERROR,
	UNKNOWN_ERROR
};

using ApplicationWindow=std::conditional<Platform::Windows(),Win32Window,Window>::type;

int main(int argc,char *argv[])
{
	if constexpr (Platform::Windows()) qputenv("QT_MULTIMEDIA_PREFERRED_PLUGINS", "windowsmediafoundation");

	QApplication application(argc,argv);
	application.setOrganizationName(ORGANIZATION_NAME);
	application.setApplicationName(APPLICATION_NAME);
	if constexpr (Platform::Windows()) application.setWindowIcon(QIcon(":/celeste.png"));

#ifdef DEVELOPER_MODE
	if (QMessageBox(QMessageBox::Warning,"DEVELOPER MODE","**WARNING** Celeste is currently in developer mode. Sensitive data will be displayed in the main window and written to the log. Only proceed if you know what you are doing. Continue?",QMessageBox::Yes|QMessageBox::No).exec() == QMessageBox::No) return OK;
#endif

	Security security;
	if (!security.Administrator() || static_cast<QString>(security.Administrator()).isEmpty())
	{
		QString administrator=QInputDialog::getText(nullptr,"Administrator","Please provide the account name of the Twitch user who will serve as the bot's administrator.").toLower();
		if (administrator.isEmpty()) return NOT_CONFIGURED;
		security.Administrator().Set(administrator);
	}
	if (!security.ClientID() || static_cast<QString>(security.ClientID()).isEmpty())
	{
		QString clientID=QInputDialog::getText(nullptr,"Client ID","A client ID has not yet been set. Please provide your bot's client ID from the Twitch developer console.",QLineEdit::Password);
		if (clientID.isEmpty()) return NOT_CONFIGURED;
		security.ClientID().Set(clientID);
	}
	if (!security.ClientSecret() || static_cast<QString>(security.ClientSecret()).isEmpty())
	{
		QString secret=QInputDialog::getText(nullptr,"Client Secret","A client secret has not yet been set. Please provide your bot's client secret from the Twitch developer console.",QLineEdit::Password);
		if (secret.isEmpty()) return NOT_CONFIGURED;
		security.ClientSecret().Set(secret);
	}
	if (!security.CallbackURL() || static_cast<QString>(security.CallbackURL()).isEmpty())
	{
		QString callbackURL=QInputDialog::getText(nullptr,"Callback URL","OAuth and EventSub require the URL at which your bot's server can be reached. Please provide a callback URL.");
		if (callbackURL.isEmpty()) return NOT_CONFIGURED;
		security.CallbackURL().Set(callbackURL);
	}
	if (!security.Scope() || static_cast<QStringList>(security.Scope()).isEmpty())
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
		ApplicationWindow window;

		UI::Options::Dialog configureOptions(&window);
		configureOptions.AddCategory(new UI::Options::Categories::Channel(&configureOptions,{
			.name=channel->Name(),
			.protection=channel->Protection()
		}));
		configureOptions.AddCategory(new UI::Options::Categories::Window(&configureOptions,{
			.backgroundColor=window.BackgroundColor(),
			.accentColor=window.AccentColor(),
			.dimensions=window.Dimensions()
		}));
		configureOptions.AddCategory(new UI::Options::Categories::Bot(&configureOptions,{
			.arrivalSound=celeste.ArrivalSound(),
			.portraitVideo=celeste.PortraitVideo()
		}));

		security.connect(&security,&Security::TokenRequestFailed,[]() {
			QMessageBox failureDialog;
			failureDialog.setWindowTitle("Re-Authentication Failed");
			failureDialog.setText("Attempt to obtain OAuth token failed.");
			failureDialog.setStandardButtons(QMessageBox::Ok);
			failureDialog.setDefaultButton(QMessageBox::Ok);
			failureDialog.exec();
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
		celeste.connect(&celeste,&Bot::AnnounceDeniedCommand,&window,&Window::AnnounceDeniedCommand);
		celeste.connect(&celeste,&Bot::SetAgenda,&window,&Window::SetAgenda);
		celeste.connect(&celeste,&Bot::ShowPortraitVideo,&window,&Window::ShowPortraitVideo);
		celeste.connect(&celeste,&Bot::ShowCurrentSong,&window,&Window::ShowCurrentSong);
		celeste.connect(&celeste,&Bot::ShowCommand,&window,&Window::ShowCommand);
		celeste.connect(&celeste,&Bot::ShowCommandList,&window,&Window::ShowCommandList);
		celeste.connect(&celeste,&Bot::ShowFollowage,&window,&Window::ShowFollowage);
		celeste.connect(&celeste,&Bot::ShowUptime,&window,&Window::ShowUptime);
		celeste.connect(&celeste,&Bot::ShowTotalTime,&window,&Window::ShowUptime);
		celeste.connect(&celeste,&Bot::ShowTimezone,&window,&Window::ShowTimezone);
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
			security.connect(&security,&Security::AdministratorProfileObtained,&security,[&security,&log,&server,&celeste]() {
				EventSub *eventSub=new EventSub(security);
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
			security.ObtainAdministratorProfile();
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
		window.connect(&window,&Window::SuppressMusic,&celeste,&Bot::SuppressMusic);
		window.connect(&window,&Window::RestoreMusic,&celeste,&Bot::RestoreMusic);
		window.connect(&window,&Window::ConfigureOptions,[&configureOptions]() {
			configureOptions.exec();
		});
		window.connect(&window,&Window::CloseRequested,[channel,&celeste](QCloseEvent *closeEvent) {
			if (channel->Protection()) {
				QMessageBox emoteOnlyDialog;
				emoteOnlyDialog.setWindowTitle("Channel Protection");
				emoteOnlyDialog.setText("Enable emote-only chat?");
				emoteOnlyDialog.setStandardButtons(QMessageBox::Yes|QMessageBox::No);
				emoteOnlyDialog.setDefaultButton(QMessageBox::No);
				if (emoteOnlyDialog.exec() == QMessageBox::Yes) celeste.EmoteOnly(true);
			}

			QMessageBox resetDialog;
			resetDialog.setWindowTitle("Reset Next Session");
			resetDialog.setText("Would you like to reset the session for the next stream?");
			resetDialog.setStandardButtons(QMessageBox::Yes|QMessageBox::No);
			resetDialog.setDefaultButton(QMessageBox::Yes);
			celeste.SaveViewerAttributes(resetDialog.exec() == QMessageBox::Yes);

			closeEvent->accept();
		});

		if (!log.Open()) throw std::runtime_error("Could not open log file!");
		if (!server.Listen()) throw std::runtime_error("Could not start server!");
		pulsar.LoadTriggers();
		window.show();

		UI::Commands::Dialog *configureCommands=nullptr;
		try
		{
			configureCommands=new UI::Commands::Dialog(celeste.DeserializeCommands(celeste.LoadDynamicCommands()),&window);
			configureCommands->connect(configureCommands,QOverload<const Command::Lookup&>::of(&UI::Commands::Dialog::Save),&celeste,[&window,&celeste,&log,configureCommands](const Command::Lookup &commands) {
				static const char *ERROR_COMMANDS_LIST_FILE="Something went wrong saving the commands list to disk.";
				if (!celeste.SaveDynamicCommands(celeste.SerializeCommands(commands)))
				{
					QMessageBox failureDialog(&window);
					failureDialog.setWindowTitle("Save Commands Failed");
					failureDialog.setText(ERROR_COMMANDS_LIST_FILE);
					failureDialog.setIcon(QMessageBox::Warning);
					failureDialog.setStandardButtons(QMessageBox::Ok);
					failureDialog.setDefaultButton(QMessageBox::Ok);
					failureDialog.exec();
					log.Write(ERROR_COMMANDS_LIST_FILE,"save commands",SUBSYSTEM);
				}
			});
			window.connect(&window,&Window::ConfigureCommands,[configureCommands]() {
				configureCommands->open();
			});

			channel->Connect();
		}

		catch (const std::runtime_error &exception)
		{
			log.Write(exception.what(),"load commands",SUBSYSTEM);
		}

		return application.exec();
	}

	catch (const std::exception &exception)
	{
		qFatal("%s",exception.what());
		return FATAL_ERROR;
	}

	catch (...)
	{
		qFatal("An unknown error occurred!");
		return UNKNOWN_ERROR;
	}
}
