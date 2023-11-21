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
const char *SUBSYSTEM_DIALOG="configuration dialog";
const char *SUBSYSTEM_AUTHORIZATION="authorization";

enum
{
	OK,
	NOT_CONFIGURED,
	AUTHENTICATION_ERROR,
	FATAL_ERROR,
	UNKNOWN_ERROR
};

using ApplicationWindow=std::conditional<Platform::Windows(),Win32Window,Window>::type;

int MessageBox(const QString &title,const QString &text,QMessageBox::Icon icon,QMessageBox::StandardButtons buttons,QMessageBox::StandardButton defaultButton,QWidget *parent=nullptr)
{
	QMessageBox box(parent);
	box.setWindowTitle(title);
	box.setText(text);
	box.setIcon(icon);
	box.setStandardButtons(buttons);
	box.setDefaultButton(defaultButton);
	return box.exec();
}

void ShowOptions(ApplicationWindow &window,Channel *channel,Bot &bot,Music::Player &musicPlayer,Log &log,Security &security)
{
	UI::Options::Dialog *configureOptions=new UI::Options::Dialog(&window);
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
		.suppressedVolume=musicPlayer.SuppressedVolume()
	}));
	UI::Options::Categories::Bot *optionsCategoryBot=new UI::Options::Categories::Bot(configureOptions,{
		.arrivalSound=bot.ArrivalSound(),
		.portraitVideo=bot.PortraitVideo(),
		.cheerVideo=bot.CheerVideo(),
		.subscriptionSound=bot.SubscriptionSound(),
		.raidSound=bot.RaidSound(),
		.inactivityCooldown=bot.InactivityCooldown(),
		.helpCooldown=bot.HelpCooldown(),
		.textWallThreshold=bot.TextWallThreshold(),
		.textWallSound=bot.TextWallSound()
	});
	configureOptions->AddCategory(optionsCategoryBot);
	configureOptions->AddCategory(new UI::Options::Categories::Log(configureOptions,{
		.directory=log.Directory()
	}));
	configureOptions->AddCategory(new UI::Options::Categories::Security(configureOptions,security));

	configureOptions->connect(optionsCategoryBot,QOverload<const QString&,QImage,const QString&>::of(&UI::Options::Categories::Bot::PlayArrivalSound),&window,&Window::AnnounceArrival);
	configureOptions->connect(optionsCategoryBot,QOverload<const QString&>::of(&UI::Options::Categories::Bot::PlayPortraitVideo),&window,&Window::ShowPortraitVideo);
	configureOptions->connect(optionsCategoryBot,QOverload<const QString&,const QString&>::of(&UI::Options::Categories::Bot::PlayTextWallSound),&window,&Window::AnnounceTextWall);
	configureOptions->connect(optionsCategoryBot,QOverload<const QString&,const unsigned int,const QString&,const QString&>::of(&UI::Options::Categories::Bot::PlayCheerVideo),&window,&Window::AnnounceCheer);
	configureOptions->connect(optionsCategoryBot,QOverload<const QString&,const QString&>::of(&UI::Options::Categories::Bot::PlaySubscriptionSound),&window,&Window::AnnounceSubscription);
	configureOptions->connect(optionsCategoryBot,QOverload<const QString&,const unsigned int,const QString&>::of(&UI::Options::Categories::Bot::PlayRaidSound),&window,&Window::AnnounceRaid);
	configureOptions->connect(configureOptions,&UI::Options::Dialog::Refresh,&window,&Window::RefreshChat);
	configureOptions->connect(configureOptions,&UI::Options::Dialog::finished,[configureOptions](int finished) {
		configureOptions->deleteLater();
	});

	configureOptions->open();
}

void ShowCommands(ApplicationWindow &window,Bot &bot,const Command::Lookup &commands,Log &log)
{
	UI::Commands::Dialog *configureCommands=new UI::Commands::Dialog(commands,&window);
	configureCommands->connect(configureCommands,QOverload<const Command::Lookup&>::of(&UI::Commands::Dialog::Save),&bot,[&window,&bot,&log,configureCommands](const Command::Lookup& commands) {
		static const char *ERROR_COMMANDS_LIST_FILE="Something went wrong saving the commands list to a file";
		if (!bot.SaveDynamicCommands(bot.SerializeCommands(commands)))
		{
			MessageBox(u"Save dynamic commands Failed"_s,ERROR_COMMANDS_LIST_FILE,QMessageBox::Warning,QMessageBox::Ok,QMessageBox::Ok,&window);
			log.Write(ERROR_COMMANDS_LIST_FILE,u"save dynamic commands"_s,SUBSYSTEM_DIALOG);
		}
	});
	configureCommands->connect(configureCommands,&UI::Options::Dialog::finished,[configureCommands](int finished) {
		configureCommands->deleteLater();
	});

	configureCommands->open();
}

void ShowEventSubscriptions(ApplicationWindow &window,EventSub *eventSub)
{
	UI::EventSubscriptions::Dialog *configureEventSubscriptions=new UI::EventSubscriptions::Dialog(&window);
	eventSub->connect(eventSub,&EventSub::EventSubscription,configureEventSubscriptions,&UI::EventSubscriptions::Dialog::Add,Qt::QueuedConnection);
	eventSub->connect(eventSub,&EventSub::EventSubscriptionRemoved,configureEventSubscriptions,&UI::EventSubscriptions::Dialog::Removed,Qt::QueuedConnection);
	configureEventSubscriptions->connect(configureEventSubscriptions,&UI::EventSubscriptions::Dialog::RequestSubscriptionList,eventSub,&EventSub::RequestEventSubscriptionList,Qt::QueuedConnection);
	configureEventSubscriptions->connect(configureEventSubscriptions,&UI::EventSubscriptions::Dialog::RemoveSubscription,eventSub,&EventSub::RemoveEventSubscription,Qt::QueuedConnection);
	configureEventSubscriptions->connect(configureEventSubscriptions,&UI::EventSubscriptions::Dialog::finished,configureEventSubscriptions,[configureEventSubscriptions](int finished) {
		configureEventSubscriptions->deleteLater();
	},Qt::QueuedConnection);
	configureEventSubscriptions->open();
}

void ShowPlaylist(const File::List &files,ApplicationWindow &window,Bot &bot,Log &log)
{
	UI::VibePlaylist::Dialog *configurePlaylist=new UI::VibePlaylist::Dialog(files,&window);
	configurePlaylist->connect(configurePlaylist,QOverload<const File::List&>::of(&UI::VibePlaylist::Dialog::Save),&bot,[&window,&bot,&log](const File::List &files) {
		static const char *ERROR_VIBE_PLAYLIST_FILE="Something went wrong saving the vibe playlist to a file";
		if (!bot.SaveVibePlaylist(bot.SerializeVibePlaylist(files)))
		{
			MessageBox(u"Save vibe playlist Failed"_s,ERROR_VIBE_PLAYLIST_FILE,QMessageBox::Warning,QMessageBox::Ok,QMessageBox::Ok,&window);
			log.Write(ERROR_VIBE_PLAYLIST_FILE,u"save playlist"_s,SUBSYSTEM_DIALOG);
		}
	});
	configurePlaylist->connect(configurePlaylist,&UI::VibePlaylist::Dialog::finished,[configurePlaylist](int finished) {
		configurePlaylist->deleteLater();
	});

	configurePlaylist->open();
}

int main(int argc,char *argv[])
{
	if constexpr (Platform::Windows()) qputenv("QT_MULTIMEDIA_PREFERRED_PLUGINS", "windowsmediafoundation");

	QApplication application(argc,argv);
	application.setOrganizationName(ORGANIZATION_NAME);
	application.setApplicationName(APPLICATION_NAME);
	if constexpr (!Platform::Windows()) application.setWindowIcon(QIcon(Resources::CELESTE));

#ifdef DEVELOPER_MODE
	if (MessageBox(u"DEVELOPER MODE"_s,u"**WARNING** Celeste is currently in developer mode. Sensitive data will be displayed in the main window and written to the log. Only proceed if you know what you are doing. Continue?"_s,QMessageBox::Warning,QMessageBox::Yes|QMessageBox::No,QMessageBox::No) == QMessageBox::No) return OK;
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
		UI::Security::Scopes scopes(nullptr);
		if (scopes.exec()) security.Scope().Set(scopes().join(" "));
		security.OAuthToken().Unset();
	}

	try
	{
		Log log;
		IRCSocket socket;
		Channel *channel=new Channel(security,&socket);
		Server server;
		Music::Player musicPlayer(true,0);
		Bot celeste(musicPlayer,security);
		const Command::Lookup &botCommands=celeste.DeserializeCommands(celeste.LoadDynamicCommands());
		const File::List &musicPlaylist=celeste.DeserializeVibePlaylist(celeste.LoadVibePlaylist());
		Pulsar pulsar;
		ApplicationWindow window;
		UI::Metrics::Dialog metrics(&window);

		security.connect(&security,&Security::TokenRequestFailed,[&application]() {
			MessageBox(u"Authentication Failed"_s,u"Attempt to obtain OAuth token failed."_s,QMessageBox::Warning,QMessageBox::Ok,QMessageBox::Ok);
			application.exit(AUTHENTICATION_ERROR);
		});
		security.connect(&security,&Security::TokenRefreshFailed,[&log]() {
			log.Write("Attempt to refresh OAuth token failed","refresh auth token",SUBSYSTEM_AUTHORIZATION);
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
		celeste.connect(&celeste,&Bot::AnnounceDeniedCommand,&window,&Window::AnnounceDeniedCommand);
		celeste.connect(&celeste,&Bot::SetAgenda,&window,&Window::SetAgenda);
		celeste.connect(&celeste,&Bot::ShowPortraitVideo,&window,&Window::ShowPortraitVideo);
		celeste.connect(&celeste,QOverload<const QString&,const QString&,const QString&,const QImage>::of(&Bot::ShowCurrentSong),&window,QOverload<const QString&,const QString&,const QString&,const QImage>::of(&Window::ShowCurrentSong));
		celeste.connect(&celeste,QOverload<const QString&,const QString&,const QImage>::of(&Bot::ShowCurrentSong),&window,QOverload<const QString&,const QString&,const QImage>::of(&Window::ShowCurrentSong));
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
		channel->connect(channel,&Channel::Disconnected,[channel,&window]() {
			qApp->alert(&window);
			qApp->beep();
			if (MessageBox(u"Connection Failed"_s,u"Failed to connect to Twitch. Would you like to try again?"_s,QMessageBox::Question,QMessageBox::Yes|QMessageBox::No,QMessageBox::Yes) == QMessageBox::No) return;
			channel->Connect();
		});
		QMetaObject::Connection events;
		QMetaObject::Connection administratorProfile; // have to do it this way because Qt::UniqueConnection doesn't work with lambdas (https://bugreports.qt.io/browse/QTBUG-52438)
		channel->connect(channel,&Channel::Connected,[&events,&administratorProfile,&security,&window,channel,&server,&celeste,&log]() {
			if (!administratorProfile)
			{
				administratorProfile=security.connect(&security,&Security::AdministratorProfileObtained,&security,[&events,&security,&window,channel,&log,&server,&celeste]() mutable {
					EventSub *eventSub=new EventSub(security);
					events=server.connect(&server,&Server::Dispatch,eventSub,&EventSub::ParseRequest);
					eventSub->connect(eventSub,&EventSub::Print,&log,&Log::Write);
					eventSub->connect(eventSub,&EventSub::Response,&server,QOverload<qintptr,const QString&>::of(&Server::SocketWrite));
					eventSub->connect(eventSub,&EventSub::Redemption,&celeste,&Bot::Redemption);
					eventSub->connect(eventSub,&EventSub::ChannelSubscription,&celeste,&Bot::Subscription);
					eventSub->connect(eventSub,&EventSub::Raid,&celeste,&Bot::Raid);
					eventSub->connect(eventSub,&EventSub::Cheer,&celeste,&Bot::Cheer);
					eventSub->connect(eventSub,&EventSub::HypeTrain,&window,&Window::AnnounceHypeTrainProgress);
					eventSub->connect(eventSub,&EventSub::ParseCommand,&celeste,QOverload<JSON::SignalPayload*,const QString&,const QString&>::of(&Bot::DispatchCommand),Qt::QueuedConnection);
					eventSub->connect(eventSub,&EventSub::EventSubscriptionFailed,eventSub,[eventSub](const QString &type) {
						MessageBox(u"EventSub Request Failed"_s,u"The attempt to subscribe to %1 failed."_s.arg(type),QMessageBox::Information,QMessageBox::Ok,QMessageBox::Ok);
					},Qt::QueuedConnection);
					eventSub->connect(eventSub,&EventSub::Unauthorized,eventSub,[eventSub,&events,&security,&server,channel]() {
						if (MessageBox(u"EventSub Authorization Failed"_s,u"There was a problem with authorization while subscribing to an event. Would you like to attempt to renew the server token?"_s,QMessageBox::Question,QMessageBox::Yes|QMessageBox::No,QMessageBox::Yes) == QMessageBox::Yes)
						{
							server.disconnect(events);
							security.AuthorizeServer();
							eventSub->deleteLater();
							channel->Disconnect();
						}
					},Qt::QueuedConnection);
					eventSub->connect(eventSub,&EventSub::RateLimitHit,eventSub,[eventSub]() {
						MessageBox(u"EventSub Rate Limit"_s,u"The maximum number of subscription requests has been hit. EventSub functionality may be limited."_s,QMessageBox::Information,QMessageBox::Ok,QMessageBox::Ok);
					},Qt::QueuedConnection);
					window.connect(&window,&Window::ConfigureEventSubscriptions,[&window,eventSub]() {
						ShowEventSubscriptions(window,eventSub);
					});

					eventSub->Subscribe();
				});
			}
			security.ObtainAdministratorProfile();
		});
		channel->connect(channel,&Channel::Connected,&security,&Security::StartClocks);
		QMetaObject::Connection reauthorize;
		channel->connect(channel,&Channel::Denied,[&reauthorize,&security,&server]() {
			if (MessageBox(u"Authentication Failed"_s,u"Twitch did not accept Celeste's credentials. Would you like to obtain a new OAuth token and retry?"_s,QMessageBox::Question,QMessageBox::Yes|QMessageBox::No,QMessageBox::Yes) == QMessageBox::No) return;
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
			if (channel->Protection())
			{
				if (MessageBox(u"Channel Protection"_s,u"Enable emote-only chat?"_s,QMessageBox::Question,QMessageBox::Yes|QMessageBox::No,QMessageBox::No) == QMessageBox::Yes) celeste.EmoteOnly(true);
			}
			celeste.SaveViewerAttributes(MessageBox(u"Reset Next Session"_s,u"Would you like to reset the session for the next stream?"_s,QMessageBox::Question,QMessageBox::Yes|QMessageBox::No,QMessageBox::Yes) == QMessageBox::Yes);
			closeEvent->accept();
		});
		window.connect(&window,&Window::ConfigureOptions,[&window,channel,&celeste,&musicPlayer,&log,&security]() {
			ShowOptions(window,channel,celeste,musicPlayer,log,security);
		});
		window.connect(&window,&Window::ConfigureCommands,[&window,&celeste,&botCommands,&log]() {
			ShowCommands(window,celeste,botCommands,log);
		});
		window.connect(&window,&Window::ShowVibePlaylist,[&musicPlaylist,&window,&celeste,&log]() {
			ShowPlaylist(musicPlaylist,window,celeste,log);
		});

		if (!log.Open()) MessageBox(u"Error Opening Log"_s,u"Failed to open log file. Log messages will not be saved to filesystem"_s,QMessageBox::Critical,QMessageBox::Ok,QMessageBox::Ok);
		if (!server.Listen()) MessageBox(u"Error Starting Web Server"_s,u"Unable to start local server. Events will not be received from Twitch."_s,QMessageBox::Critical,QMessageBox::Ok,QMessageBox::Ok);
		pulsar.LoadTriggers();
		window.show();
		channel->Connect();

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
