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
#include <exception>
#include "window.h"
#include "widgets.h"
#include "channel.h"
#include "bot.h"
#include "log.h"
#include "eventsub.h"
#include "globals.h"
#include "security.h"
#include "pulsar.h"

const char *ORGANIZATION_NAME="EngineeringDeck";
const char *APPLICATION_NAME="Celeste";
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

void ShowOptions(ApplicationWindow &window,Channel *channel,Bot &bot,Pulsar &pulsar,Music::Player &musicPlayer,Log &log,Security &security)
{
	std::shared_ptr<UI::Feedback::Error> errorReport=std::make_shared<UI::Feedback::Error>();
	UI::Options::Dialog *configureOptions=new UI::Options::Dialog(&window);
	configureOptions->AddCategory(new UI::Options::Categories::Channel({
		.name=channel->Name(),
		.protection=channel->Protection()
	},errorReport,configureOptions));
	configureOptions->AddCategory(new UI::Options::Categories::Window({
		.backgroundColor=window.BackgroundColor(),
		.dimensions=window.Dimensions()
	},configureOptions));
	StatusPane statusPane(&window);
	configureOptions->AddCategory(new UI::Options::Categories::Status({
		.font=statusPane.Font(),
		.fontSize=statusPane.FontSize(),
		.foregroundColor=statusPane.ForegroundColor(),
		.backgroundColor=statusPane.BackgroundColor()
	},errorReport,configureOptions));
	ChatPane chatPane(&window);
	configureOptions->AddCategory(new UI::Options::Categories::Chat({
		.font=chatPane.Font(),
		.fontSize=chatPane.FontSize(),
		.foregroundColor=chatPane.ForegroundColor(),
		.backgroundColor=chatPane.BackgroundColor(),
		.statusInterval=chatPane.StatusInterval()
	},errorReport,configureOptions));
	AnnouncePane announcePane(QString{},&window);
	configureOptions->AddCategory(new UI::Options::Categories::Pane({
		.font=announcePane.Font(),
		.fontSize=announcePane.FontSize(),
		.foregroundColor=announcePane.ForegroundColor(),
		.backgroundColor=announcePane.BackgroundColor(),
		.accentColor=announcePane.AccentColor(),
		.duration=announcePane.Duration()
	},errorReport,configureOptions));
	configureOptions->AddCategory(new UI::Options::Categories::Music({
		.suppressedVolume=musicPlayer.SuppressedVolume()
	},configureOptions));
	UI::Options::Categories::Bot *optionsCategoryBot=new UI::Options::Categories::Bot({
		.arrivalSound=bot.ArrivalSound(),
		.portraitVideo=bot.PortraitVideo(),
		.cheerVideo=bot.CheerVideo(),
		.subscriptionSound=bot.SubscriptionSound(),
		.raidSound=bot.RaidSound(),
		.inactivityCooldown=bot.InactivityCooldown(),
		.helpCooldown=bot.HelpCooldown(),
		.textWallThreshold=bot.TextWallThreshold(),
		.textWallSound=bot.TextWallSound(),
		.pulsarEnabled=pulsar.Enabled()
	},errorReport,configureOptions);
	configureOptions->AddCategory(optionsCategoryBot);
	configureOptions->AddCategory(new UI::Options::Categories::Log({
		.directory=log.Directory()
	},errorReport,configureOptions));
	configureOptions->AddCategory(new UI::Options::Categories::Security(security,errorReport,configureOptions));

	configureOptions->connect(optionsCategoryBot,QOverload<const QString&,std::shared_ptr<QImage>,const QString&>::of(&UI::Options::Categories::Bot::PlayArrivalSound),&window,&Window::AnnounceArrival);
	configureOptions->connect(optionsCategoryBot,QOverload<const QString&>::of(&UI::Options::Categories::Bot::PlayPortraitVideo),&window,&Window::ShowPortraitVideo);
	configureOptions->connect(optionsCategoryBot,QOverload<const QString&,const QString&>::of(&UI::Options::Categories::Bot::PlayTextWallSound),&window,&Window::AnnounceTextWall);
	configureOptions->connect(optionsCategoryBot,QOverload<const QString&,const unsigned int,const QString&,const QString&>::of(&UI::Options::Categories::Bot::PlayCheerVideo),&window,&Window::AnnounceCheer);
	configureOptions->connect(optionsCategoryBot,QOverload<const QString&,const QString&>::of(&UI::Options::Categories::Bot::PlaySubscriptionSound),&window,&Window::AnnounceSubscription);
	configureOptions->connect(optionsCategoryBot,QOverload<const QString&,const unsigned int,const QString&>::of(&UI::Options::Categories::Bot::PlayRaidSound),&window,&Window::AnnounceRaid);
	configureOptions->connect(configureOptions,&UI::Options::Dialog::Refresh,&window,&Window::RefreshChat);
	configureOptions->connect(configureOptions,&UI::Options::Dialog::finished,[configureOptions](int result) {
		Q_UNUSED(result)
		configureOptions->deleteLater();
	});

	configureOptions->open();
}

void ShowCommands(ApplicationWindow &window,Bot &bot,const Command::Lookup &commands)
{
	UI::Commands::Dialog *configureCommands=new UI::Commands::Dialog(commands,&window);
	configureCommands->connect(configureCommands,QOverload<const Command::Lookup&>::of(&UI::Commands::Dialog::Save),&bot,[&window,&bot](const Command::Lookup& commands) {
		if (!bot.SaveDynamicCommands(bot.SerializeCommands(commands)))
		{
			MessageBox(u"Save dynamic commands Failed"_s,u"Something went wrong saving the commands list to a file"_s,QMessageBox::Warning,QMessageBox::Ok,QMessageBox::Ok,&window);
		}
	});
	configureCommands->connect(configureCommands,&UI::Options::Dialog::finished,[configureCommands](int result) {
		Q_UNUSED(result)
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
		Q_UNUSED(finished)
		configureEventSubscriptions->deleteLater();
	},Qt::QueuedConnection);
	configureEventSubscriptions->open();
}

void ShowPlaylist(const File::List &files,ApplicationWindow &window,Bot &bot,Music::Player &player)
{
	UI::VibePlaylist::Dialog *configurePlaylist=new UI::VibePlaylist::Dialog(files,player.Filename(),&window);
	configurePlaylist->connect(configurePlaylist,QOverload<const File::List&>::of(&UI::VibePlaylist::Dialog::Save),&bot,[&window,&bot](const File::List &files) {
		if (!bot.SaveVibePlaylist(bot.SerializeVibePlaylist(files)))
		{
			MessageBox(u"Save vibe playlist Failed"_s,u"Something went wrong saving the vibe playlist to a file"_s,QMessageBox::Warning,QMessageBox::Ok,QMessageBox::Ok,&window);
			return;
		}
		bot.SetVibePlaylist(files);
	});
	configurePlaylist->connect(configurePlaylist,&UI::VibePlaylist::Dialog::finished,[configurePlaylist](int result) {
		Q_UNUSED(result)
		configurePlaylist->deleteLater();
	});
	configurePlaylist->connect(configurePlaylist,QOverload<QUrl>::of(&UI::VibePlaylist::Dialog::Play),&player,QOverload<QUrl>::of(&Music::Player::Start));
	configurePlaylist->connect(configurePlaylist,&UI::VibePlaylist::Dialog::Stop,&player,&Music::Player::Stop);
	configurePlaylist->connect(configurePlaylist,&UI::VibePlaylist::Dialog::Volume,&player,QOverload<int>::of(&Music::Player::Volume));

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
	if (!security.CallbackURL() || static_cast<QString>(security.CallbackURL()).isEmpty())
	{
		QString callbackURL=QInputDialog::getText(nullptr,"Callback URL","OAuth and EventSub require the URL at which your bot's server can be reached. Please provide a callback URL.");
		if (callbackURL.isEmpty()) return NOT_CONFIGURED;
		security.CallbackURL().Set(callbackURL);
	}
	if (!security.Scope() || static_cast<QString>(security.Scope()).isEmpty()) // just test against QString since we're just looking to see if something is there (not worth the conversion overhead of splitting)
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
		Music::Player musicPlayer(true,0);
		Bot celeste(musicPlayer,security);
		const Command::Lookup &botCommands=celeste.DeserializeCommands(celeste.LoadDynamicCommands());
		const File::List &musicPlaylist=celeste.SetVibePlaylist(celeste.DeserializeVibePlaylist(celeste.LoadVibePlaylist()));
		Pulsar pulsar;
		EventSub *eventSub=nullptr;
		ApplicationWindow window;
		UI::Metrics::Dialog metrics(&window);
		UI::Status::Window<StatusPane> status(&window);

		security.connect(&security,&Security::TokenRequestFailed,&security,[&application]() {
			MessageBox(u"Authentication Failed"_s,u"Attempt to obtain OAuth token failed."_s,QMessageBox::Warning,QMessageBox::Ok,QMessageBox::Ok);
			application.exit(AUTHENTICATION_ERROR);
		});
		QMetaObject::Connection echo=log.connect(&log,&Log::Print,&window,QOverload<const QString&>::of(&Window::Print));
		log.connect(&log,&Log::Print,&status.Pane(),&StatusPane::Print);
		celeste.connect(&celeste,&Bot::ChatMessage,&window,&Window::ChatMessage);
		celeste.connect(&celeste,&Bot::DeleteChatMessage,&window,&Window::DeleteChatMessage);
		celeste.connect(&celeste,&Bot::RefreshChat,&window,&Window::RefreshChat);
		celeste.connect(&celeste,&Bot::Print,&log,&Log::Receive);
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
		celeste.connect(&celeste,&Bot::Pulse,&pulsar,QOverload<const QString&,const QString&>::of(&Pulsar::Pulse));
		celeste.connect(&celeste,&Bot::Welcomed,&metrics,&UI::Metrics::Dialog::Acknowledged);
		celeste.connect(&celeste,&Bot::Panic,&window,&Window::ShowPanicText);
		celeste.connect(&celeste,&Bot::Panic,&celeste,[&celeste]() {
			celeste.disconnect();
		});
		pulsar.connect(&pulsar,&Pulsar::Print,&log,&Log::Receive);
		pulsar.connect(&pulsar,&Pulsar::Dimensions,&window,&Window::Resize);
		channel->connect(channel,&Channel::Print,&log,&Log::Receive);
		channel->connect(channel,&Channel::Dispatch,&celeste,&Bot::ParseChatMessage);
		channel->connect(channel,&Channel::Deleted,&celeste,&Bot::ParseChatMessageDeletion);
		channel->connect(channel,&Channel::Ping,&celeste,&Bot::Ping);
		channel->connect(channel,QOverload<const QString&>::of(&Channel::Joined),&metrics,&UI::Metrics::Dialog::Joined);
		channel->connect(channel,QOverload<const QString&>::of(&Channel::Parted),&metrics,&UI::Metrics::Dialog::Parted);
		channel->connect(channel,QOverload<>::of(&Channel::Joined),&window,[&echo,&log,&celeste,&pulsar,&window]() {
			log.disconnect(echo);
			window.connect(&window,QOverload<const QString&,const QString&,const QString&>::of(&Window::Print),&window,QOverload<const QString&>::of(&Window::Print));
			window.connect(&window,QOverload<const QString&,const QString&,const QString&>::of(&Window::Print),&log,&Log::Receive);
			celeste.connect(&celeste,&Bot::Print,&window,QOverload<const QString&>::of(&Window::Print));
			pulsar.connect(&pulsar,&Pulsar::Print,&window,QOverload<const QString&>::of(&Window::Print));
			window.ShowChat();
		});
		channel->connect(channel,&Channel::Disconnected,&window,[channel,&window]() {
			qApp->alert(&window);
			qApp->beep();
			if (MessageBox(u"Connection Failed"_s,u"Failed to connect to Twitch. Would you like to try again?"_s,QMessageBox::Question,QMessageBox::Yes|QMessageBox::No,QMessageBox::Yes) == QMessageBox::No) return;
			channel->Connect();
		});
		channel->connect(channel,&Channel::Connected,eventSub,[&security,&window,&celeste,&log,&application,eventSub]() mutable {
			if (eventSub) eventSub->deleteLater();
			eventSub=new EventSub(security);

			eventSub->connect(eventSub,&EventSub::Print,&log,&Log::Receive);
			eventSub->connect(eventSub,&EventSub::Redemption,&celeste,&Bot::Redemption);
			eventSub->connect(eventSub,&EventSub::ChannelSubscription,&celeste,&Bot::Subscription);
			eventSub->connect(eventSub,&EventSub::Raid,&celeste,&Bot::Raid);
			eventSub->connect(eventSub,&EventSub::Cheer,&celeste,&Bot::Cheer);
			eventSub->connect(eventSub,&EventSub::HypeTrain,&window,&Window::AnnounceHypeTrainProgress);
			eventSub->connect(eventSub,&EventSub::ParseCommand,&celeste,QOverload<JSON::SignalPayload*,const QString&,const QString&>::of(&Bot::DispatchCommandViaSubsystem),Qt::QueuedConnection);
			eventSub->connect(eventSub,&EventSub::EventSubscriptionFailed,eventSub,[](const QString &type) {
				MessageBox(u"EventSub Request Failed"_s,u"The attempt to subscribe to %1 failed."_s.arg(type),QMessageBox::Information,QMessageBox::Ok,QMessageBox::Ok);
			},Qt::QueuedConnection);
			eventSub->connect(eventSub,&EventSub::Unauthorized,&security,&Security::AuthorizeUser,Qt::QueuedConnection);
			eventSub->connect(eventSub,&EventSub::RateLimitHit,eventSub,[]() {
				MessageBox(u"EventSub Rate Limit"_s,u"The maximum number of subscription requests has been hit. EventSub functionality may be limited."_s,QMessageBox::Information,QMessageBox::Ok,QMessageBox::Ok);
			},Qt::QueuedConnection);
			eventSub->connect(eventSub,&EventSub::Connected,eventSub,QOverload<>::of(&EventSub::Subscribe),Qt::QueuedConnection);
			window.connect(&window,&Window::ConfigureEventSubscriptions,&window,[&window,eventSub]() {
				ShowEventSubscriptions(window,eventSub);
			});
			celeste.connect(&celeste,&Bot::Panic,eventSub,&EventSub::deleteLater,Qt::QueuedConnection);
			application.connect(&application,&QApplication::aboutToQuit,eventSub,&EventSub::deleteLater,Qt::DirectConnection);
		});
		channel->connect(channel,&Channel::Denied,&security,&Security::AuthorizeUser);
		security.connect(&security,&Security::Initialized,channel,&Channel::Connect);
		security.connect(&security,&Security::Print,&log,&Log::Receive);
		application.connect(&application,&QApplication::aboutToQuit,&application,[&log,&socket,channel]() {
			socket.connect(&socket,&IRCSocket::disconnected,&log,&Log::Archive);
			channel->disconnect(); // stops attempting to reconnect by removing all connections to signals
			channel->deleteLater();
		});
		window.connect(&window,&Window::SuppressMusic,&celeste,&Bot::SuppressMusic);
		window.connect(&window,&Window::RestoreMusic,&celeste,&Bot::RestoreMusic);
		window.connect(&window,&Window::ShowMetrics,&metrics,&QDialog::show);
		window.connect(&window,&Window::CloseRequested,&window,[channel,&celeste](QCloseEvent *closeEvent) {
			if (channel->Protection())
			{
				if (MessageBox(u"Channel Protection"_s,u"Enable emote-only chat?"_s,QMessageBox::Question,QMessageBox::Yes|QMessageBox::No,QMessageBox::No) == QMessageBox::Yes) celeste.EmoteOnly(true);
			}
			celeste.SaveViewerAttributes(MessageBox(u"Reset Next Session"_s,u"Would you like to reset the session for the next stream?"_s,QMessageBox::Question,QMessageBox::Yes|QMessageBox::No,QMessageBox::Yes) == QMessageBox::Yes);
			closeEvent->accept();
		});
		window.connect(&window,&Window::ConfigureOptions,&window,[&window,channel,&celeste,&pulsar,&musicPlayer,&log,&security]() {
			ShowOptions(window,channel,celeste,pulsar,musicPlayer,log,security);
		});
		window.connect(&window,&Window::ConfigureCommands,&window,[&window,&celeste,&botCommands]() {
			ShowCommands(window,celeste,botCommands);
		});
		window.connect(&window,&Window::ShowVibePlaylist,&window,[&musicPlaylist,&window,&celeste,&musicPlayer]() {
			ShowPlaylist(musicPlaylist,window,celeste,musicPlayer);
		});
		window.connect(&window,&Window::ShowStatus,[&status]() {
			status.Open();
		});

		if (!log.Open()) MessageBox(u"Error Opening Log"_s,u"Failed to open log file. Log messages will not be saved to filesystem"_s,QMessageBox::Critical,QMessageBox::Ok,QMessageBox::Ok);
		pulsar.Connect();
		pulsar.LoadTriggers();
		window.show();
		security.Listen();

		return application.exec();
	}

	catch (const std::runtime_error &exception)
	{
		MessageBox(u"Critial Error"_s,QString(exception.what())+u"\n\nApplication will exit."_s,QMessageBox::Critical,QMessageBox::Ok,QMessageBox::Ok);
		return FATAL_ERROR;
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
