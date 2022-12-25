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
#include <string>
#include <exception>
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

class InitializationError : public std::runtime_error
{
public:
	InitializationError(const std::string &operation,const std::string &message) : std::runtime_error(message), operation(operation) { }
	const char* Operation() const { return operation.c_str(); }
protected:
	std::string operation;
};

enum
{
	OK,
	NOT_CONFIGURED,
	AUTHENTICATION_ERROR,
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
	if constexpr (!Platform::Windows()) application.setWindowIcon(QIcon(Resources::CELESTE));

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
		scopeDialog.setSizeGripEnabled(true);
		QListWidget *listBox=new QListWidget(&scopeDialog);
		listBox->setSelectionMode(QAbstractItemView::ExtendedSelection);
		listBox->addItems(Security::SCOPES);
		scopeDialog.layout()->addWidget(listBox);
		QDialogButtonBox *buttons=new QDialogButtonBox(&scopeDialog);
		QPushButton *okay=buttons->addButton(QDialogButtonBox::Ok);
		okay->setDefault(true);
		okay->connect(okay,&QPushButton::clicked,&scopeDialog,&QDialog::accept);
		scopeDialog.layout()->addWidget(buttons);
		if (scopeDialog.exec())
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
		UI::Metrics::Dialog metrics(&window);

		security.connect(&security,&Security::TokenRequestFailed,[&application]() {
			QMessageBox failureDialog;
			failureDialog.setWindowTitle("Authentication Failed");
			failureDialog.setText("Attempt to obtain OAuth token failed.");
			failureDialog.setStandardButtons(QMessageBox::Ok);
			failureDialog.setDefaultButton(QMessageBox::Ok);
			failureDialog.exec();
			application.exit(AUTHENTICATION_ERROR);
		});
		security.connect(&security,&Security::TokenRefreshFailed,[&log]() {
			log.Write("Attempt to refresh OAuth token failed","refresh auth token",SUBSYSTEM);
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
		celeste.connect(&celeste,&Bot::Welcomed,&metrics,&UI::Metrics::Dialog::Acknowledged);
		pulsar.connect(&pulsar,&Pulsar::Print,&log,&Log::Write);
		channel->connect(channel,&Channel::Print,&log,&Log::Write);
		channel->connect(channel,&Channel::Dispatch,&celeste,&Bot::ParseChatMessage);
		channel->connect(channel,&Channel::Ping,&celeste,&Bot::Ping);
		channel->connect(channel,QOverload<const QString&>::of(&Channel::Joined),&metrics,&UI::Metrics::Dialog::Joined);
		channel->connect(channel,QOverload<const QString&>::of(&Channel::Parted),&metrics,&UI::Metrics::Dialog::Parted);
		channel->connect(channel,QOverload<>::of(&Channel::Joined),&window,&Window::ShowChat);
		channel->connect(channel,QOverload<>::of(&Channel::Joined),[&echo,&log,&celeste,&window]() {
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
				eventSub->connect(eventSub,&EventSub::Response,&server,QOverload<qintptr,const QString&>::of(&Server::SocketWrite));
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

			reauthorize=server.connect(&server,&Server::Dispatch,[&reauthorize,&security,&server](qintptr socketID,const QUrlQuery& query) {
				server.disconnect(reauthorize); // break any connection with security's slots so we don't conflict with EventSub later
				server.SocketWrite(socketID,QStringLiteral(R"(<html><body><h1>Authorization Successful!</h1><br><b>Token:</b> %1<br><b>Scope:</b> %2</body></html>)").arg(QString(query.queryItemValue(QUERY_PARAMETER_CODE).size(),'*'),query.queryItemValue(QUERY_PARAMETER_SCOPE).replace('+',", ")),Network::CONTENT_TYPE_HTML);
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
		window.connect(&window,&Window::ShowMetrics,&metrics,&QDialog::show);
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
		UI::Options::Dialog *configureOptions=nullptr;
		try
		{
			configureOptions=new UI::Options::Dialog(&window);
			configureOptions->AddCategory(new UI::Options::Categories::Channel(configureOptions,{
				.name=channel->Name(),
				.protection=channel->Protection()
			}));
			configureOptions->AddCategory(new UI::Options::Categories::Window(configureOptions,{
				.backgroundColor=window.BackgroundColor(),
				.dimensions=window.Dimensions()
			}));
			StatusPane statusPane(&window);
			configureOptions->AddCategory(new UI::Options::Categories::Status(configureOptions,{
				.font=statusPane.Font(),
				.fontSize=statusPane.FontSize(),
				.foregroundColor=statusPane.ForegroundColor(),
				.backgroundColor=statusPane.BackgroundColor()
			}));
			ChatPane chatPane(&window);
			configureOptions->AddCategory(new UI::Options::Categories::Chat(configureOptions,{
				.font=chatPane.Font(),
				.fontSize=chatPane.FontSize(),
				.foregroundColor=chatPane.ForegroundColor(),
				.backgroundColor=chatPane.BackgroundColor(),
				.statusInterval=chatPane.StatusInterval()
			}));
			AnnouncePane announcePane(QString{},&window);
			configureOptions->AddCategory(new UI::Options::Categories::Pane(configureOptions,{
				.font=announcePane.Font(),
				.fontSize=announcePane.FontSize(),
				.foregroundColor=announcePane.ForegroundColor(),
				.backgroundColor=announcePane.BackgroundColor(),
				.accentColor=announcePane.AccentColor(),
				.duration=announcePane.Duration()
			}));
			configureOptions->AddCategory(new UI::Options::Categories::Music(configureOptions,{
				.suppressedVolume=Music::Player(&window,false,0).SuppressedVolume()
			}));
			UI::Options::Categories::Bot *optionsCategoryBot=new UI::Options::Categories::Bot(configureOptions,{
				.arrivalSound=celeste.ArrivalSound(),
				.portraitVideo=celeste.PortraitVideo(),
				.cheerVideo=celeste.CheerVideo(),
				.subscriptionSound=celeste.SubscriptionSound(),
				.raidSound=celeste.RaidSound(),
				.inactivityCooldown=celeste.InactivityCooldown(),
				.helpCooldown=celeste.HelpCooldown(),
				.textWallThreshold=celeste.TextWallThreshold(),
				.textWallSound=celeste.TextWallSound()
			});
			configureOptions->AddCategory(optionsCategoryBot);
			configureOptions->AddCategory(new UI::Options::Categories::Log(configureOptions,{
				.directory=log.Directory()
			}));

			configureOptions->connect(optionsCategoryBot,QOverload<const QString&,QImage,const QString&>::of(&UI::Options::Categories::Bot::PlayArrivalSound),&window,&Window::AnnounceArrival);
			configureOptions->connect(optionsCategoryBot,QOverload<const QString&>::of(&UI::Options::Categories::Bot::PlayPortraitVideo),&window,&Window::ShowPortraitVideo);
			configureOptions->connect(optionsCategoryBot,QOverload<const QString&,const QString&>::of(&UI::Options::Categories::Bot::PlayTextWallSound),&window,&Window::AnnounceTextWall);
			configureOptions->connect(optionsCategoryBot,QOverload<const QString&,const unsigned int,const QString&,const QString&>::of(&UI::Options::Categories::Bot::PlayCheerVideo),&window,&Window::AnnounceCheer);
			configureOptions->connect(optionsCategoryBot,QOverload<const QString&,const QString&>::of(&UI::Options::Categories::Bot::PlaySubscriptionSound),&window,&Window::AnnounceSubscription);
			configureOptions->connect(optionsCategoryBot,QOverload<const QString&,const unsigned int,const QString&>::of(&UI::Options::Categories::Bot::PlayRaidSound),&window,&Window::AnnounceRaid);
			configureOptions->connect(configureOptions,&UI::Options::Dialog::Refresh,&window,&Window::RefreshChat);
			window.connect(&window,&Window::ConfigureOptions,configureOptions,[configureOptions]() {
				configureOptions->open();
			});

			configureCommands=new UI::Commands::Dialog(celeste.DeserializeCommands(celeste.LoadDynamicCommands()),&window);
			configureCommands->connect(configureCommands,QOverload<const Command::Lookup&>::of(&UI::Commands::Dialog::Save),&celeste,[&window,&celeste,&log,configureCommands](const Command::Lookup &commands) {
				static const char *ERROR_COMMANDS_LIST_FILE="Something went wrong saving the commands list to a file";
				if (!celeste.SaveDynamicCommands(celeste.SerializeCommands(commands)))
				{
					QMessageBox failureDialog(&window);
					failureDialog.setWindowTitle("Save dynamic commands Failed");
					failureDialog.setText(ERROR_COMMANDS_LIST_FILE);
					failureDialog.setIcon(QMessageBox::Warning);
					failureDialog.setStandardButtons(QMessageBox::Ok);
					failureDialog.setDefaultButton(QMessageBox::Ok);
					failureDialog.exec();
					log.Write(ERROR_COMMANDS_LIST_FILE,"save dynamic commands",SUBSYSTEM);
				}
			});
			window.connect(&window,&Window::ConfigureCommands,[configureCommands]() {
				configureCommands->open();
			});

			UI::VibePlaylist::Dialog *configurePlaylist=nullptr;
			try
			{
				configurePlaylist=new UI::VibePlaylist::Dialog(celeste.DeserializeVibePlaylist(celeste.LoadVibePlaylist()),&window);
				configurePlaylist->connect(configurePlaylist,QOverload<const File::List&>::of(&UI::VibePlaylist::Dialog::Save),&celeste,[&window,&celeste,&log](const File::List &files) {
					static const char *ERROR_VIBE_PLAYLIST_FILE="Something went wrong saving the vibe playlist to a file";
					if (!celeste.SaveVibePlaylist(celeste.SerializeVibePlaylist(files)))
					{
						QMessageBox failureDialog(&window);
						failureDialog.setWindowTitle("Save vibe playlist Failed");
						failureDialog.setText(ERROR_VIBE_PLAYLIST_FILE);
						failureDialog.setIcon(QMessageBox::Warning);
						failureDialog.setStandardButtons(QMessageBox::Ok);
						failureDialog.setDefaultButton(QMessageBox::Ok);
						failureDialog.exec();
					}
				});
				window.connect(&window,&Window::ShowVibePlaylist,[configurePlaylist]() {
					configurePlaylist->open();
				});
			}
			catch (const std::runtime_error &exception)
			{
				throw InitializationError("load playlists",exception.what());
			}

			channel->Connect();
		}

		catch (const InitializationError &exception)
		{
			log.Write(exception.what(),exception.Operation(),SUBSYSTEM);
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
