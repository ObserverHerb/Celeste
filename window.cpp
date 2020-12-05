#include <QGuiApplication>
#include <QScreen>
#include <QEvent>
#include <QTimer>
#include <QLayout>
#include <QGridLayout>
#include <QDateTime>
#include <QTimeZone>
#include <chrono>
#include <stdexcept>
#include "window.h"
#include "receivers.h"

std::chrono::milliseconds Window::uptime=TimeConvert::Now();

/*!
 * \class Window
 * \brief The top most and only window of the application.
 *
 * This isn't meant to contain any visible content other than a sequence
 * of panes.
 */

/*!
 * \fn Window::Window
 * \brief Constructs the main window, initializing any settings, media players,
 * and timers needed for built-in commands.
 */
Window::Window() : QWidget(nullptr),
	ircSocket(nullptr),
	visiblePane(nullptr),
	vibeFader(nullptr),
	background(new QWidget(this)),
	vibeKeeper(new QMediaPlayer(this)),
	settingWindowSize(SETTINGS_CATEGORY_WINDOW,"Size"),
	settingHelpCooldown(SETTINGS_CATEGORY_WINDOW,"HelpCooldown",300000),
	settingVibePlaylist(SETTINGS_CATEGORY_VIBE,"Playlist"),
	settingOAuthToken(SETTINGS_CATEGORY_AUTHORIZATION,"Token"),
	settingJoinDelay(SETTINGS_CATEGORY_AUTHORIZATION,"JoinDelay",5),
	settingChannel(SETTINGS_CATEGORY_AUTHORIZATION,"Channel",""),
	settingBackgroundColor(SETTINGS_CATEGORY_WINDOW,"BackgroundColor","#ff000000"),
	settingAccentColor(SETTINGS_CATEGORY_WINDOW,"AccentColor","#ff000000"),
	settingArrivalSound(SETTINGS_CATEGORY_EVENTS,"Arrival"),
	settingThinkingSound(SETTINGS_CATEGORY_EVENTS,"Thinking")
{
	setAttribute(Qt::WA_TranslucentBackground,true);
	if (settingWindowSize)
		setFixedSize(settingWindowSize);
	else
		setFixedSize(ScreenThird());

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

	helpClock.setInterval(TimeConvert::Interval(std::chrono::milliseconds(settingHelpCooldown)));
	vibeKeeper->setVolume(0);

	connect(this,&Window::Ponging,this,&Window::Pong);
}

/*!
 * \fn Window::~Window
 * \brief Deallocate the main window.
 *
 * This will close the connection to IRC.
 */
Window::~Window()
{
	ircSocket->disconnectFromHost();
}

/*!
 * \fn Window::event
 * \brief Handles events the window receives from the operating system.
 *
 * This is where the process of connecting to IRC is started. Last minute
 * initialization is also performed here.
 *
 * \param event Pointer to the QEvent that has been occurred
 * \return bool Whether or not the event was successfully processed
 */
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

/*!
 * \fn Window::Connected
 * \brief Slot that is called when a successful connection to IRC is made
 *
 * This will start the process of authenticating with Twitch.
 */
void Window::Connected()
{
	emit Print("Connected!");
	if (!settingAdministrator)
	{
		Print("Please set the Administrator under the Authorization section in your settings file");
		return;
	}
	Print(QString(IRC_COMMAND_USER).arg(static_cast<QString>(settingAdministrator)));
	emit Print("Sending credentials...");
	ircSocket->write(StringConvert::ByteArray(QString(IRC_COMMAND_PASSWORD).arg(static_cast<QString>(settingOAuthToken))));
	ircSocket->write(StringConvert::ByteArray(QString(IRC_COMMAND_USER).arg(static_cast<QString>(settingAdministrator))));
}

/*!
 * \fn Window::Disconnected
 * \brief Slot that is called when the connection to IRC is dropped.
 */
void Window::Disconnected()
{
	emit Print("Disconnected");
}

/*!
 * \fn Window::DataAvailable
 * \brief Slot that is called when data is received from the IRC server.
 *
 * Messages that are to be processed by the bot are passed to the
 * Window::Dispatch() function.
 */
void Window::DataAvailable()
{
	QString data=ircSocket->readAll();
	if (data.size() > 4 && data.left(4) == "PING")
	{
		emit Ponging();
		return;
	}
	emit Dispatch(data);
}

/*!
 * \fn Window::SwapPane
 * \brief Changes the visible pane to a new pane
 *
 * This will also deallocate the previously visible pane
 *
 * \param pane The pane to make visible
 */
void Window::SwapPane(PersistentPane *pane)
{
	if (visiblePane) visiblePane->deleteLater();
	visiblePane=pane;
	background->layout()->addWidget(visiblePane);
	connect(this,&Window::Print,visiblePane,&PersistentPane::Print);
}

/*!
 * \fn Window::Authenticate
 * \brief Attempts to connect to the IRC server
 *
 * If authentication is successful, an attempt to join the administrator's
 * channel is triggered after a short delay.
 */
void Window::Authenticate()
{
	AuthenticationReceiver *authenticationReceiver=new AuthenticationReceiver(this);
	connect(this,&Window::Dispatch,authenticationReceiver,&AuthenticationReceiver::Process);
	connect(authenticationReceiver,&AuthenticationReceiver::Print,visiblePane,&PersistentPane::Print);
	connect(authenticationReceiver,&AuthenticationReceiver::Succeeded,[this]() {
		Print(QString("Joining stream in %1 seconds...").arg(TimeConvert::Interval(TimeConvert::Seconds(settingJoinDelay))));
		QTimer::singleShot(TimeConvert::Interval(static_cast<std::chrono::milliseconds>(settingJoinDelay)),this,&Window::JoinStream);
	});
	ircSocket->connectToHost(TWITCH_HOST,TWITCH_PORT);
	emit Print("Connecting to IRC...");
}

/*!
 * \fn Window::JoinStream
 * \brief Attempts to the administrator's IRC channel
 *
 * If successful, switch to watching for chat messages.
 */
void Window::JoinStream()
{
	ChannelJoinReceiver *channelJoinReceiver=new ChannelJoinReceiver(this);
	connect(this,&Window::Dispatch,channelJoinReceiver,&ChannelJoinReceiver::Process);
	connect(channelJoinReceiver,&ChannelJoinReceiver::Print,visiblePane,&PersistentPane::Print);
	connect(channelJoinReceiver,&ChannelJoinReceiver::Succeeded,this,&Window::FollowChat);
	ircSocket->write(StringConvert::ByteArray(QString(IRC_COMMAND_JOIN).arg(settingChannel ? static_cast<QString>(settingChannel) : static_cast<QString>(settingAdministrator))));
}

/*!
 * \fn Window::FollowChat
 * \brief Watch for and process chat messages
 *
 * This is where the bot will spend most of its time. When a message is
 * received, it is processed and sent to the correct subsystem if
 * recognized. This is also where any persistent operations, such as
 * periodically displaying command hints, are started.
 */
void Window::FollowChat()
{
	ChatMessageReceiver *chatMessageReceiver=nullptr;
	try
	{
		chatMessageReceiver=new ChatMessageReceiver({
			AgendaCommand,PingCommand,SongCommand,ThinkCommand,TimezoneCommand,UptimeCommand,VibeCommand,VolumeCommand
		},this);
	}
	catch (const std::runtime_error &exception)
	{
		visiblePane->Print(QString("There was a problem starting chat\n%1").arg(exception.what()));
		return;
	}

	ChatPane *chatPane=new ChatPane(this);
	connect(this,&Window::Ponging,[chatPane]() {
		chatPane->Alert("Twitch is asking if we're still here<br>Letting Twitch server know we're still here");
	});
	connect(chatMessageReceiver,&ChatMessageReceiver::Alert,chatPane,&ChatPane::Alert);
	SwapPane(chatPane);

	connect(this,&Window::Dispatch,chatMessageReceiver,&ChatMessageReceiver::Process);
	connect(chatMessageReceiver,&ChatMessageReceiver::Print,visiblePane,&PersistentPane::Print);
	if (settingArrivalSound)
	{
		connect(chatMessageReceiver,&ChatMessageReceiver::ArrivalConfirmed,this,[this](const Viewer &viewer) {
			if (settingAdministrator != viewer.Name()) StageEphemeralPane(new AudioAnnouncePane({
				{"Please welcome<br>",1},
				{QString("%1<br>").arg(viewer.Name()),1.5},
				{"to the chat",1}
			},settingArrivalSound));
		});
	}
	connect(chatMessageReceiver,&ChatMessageReceiver::PlayVideo,[this](const QString &path) {
		StageEphemeralPane(new VideoPane(path));
	});
	connect(chatMessageReceiver,&ChatMessageReceiver::PlayAudio,[this](const QString &user,const QString &message,const QString &path) {
		StageEphemeralPane(new AudioAnnouncePane(QString("**%1**\n\n%2").arg(user,message),path));
	});

	connect(chatMessageReceiver,&ChatMessageReceiver::DispatchCommand,[this,chatPane](const Command &command) {
		try
		{
			switch (BUILT_IN_COMMANDS.at(command.Name()))
			{
			case BuiltInCommands::AGENDA:
				chatPane->SetAgenda(command.Message());
				break;
			case BuiltInCommands::PING:
				ircSocket->write(StringConvert::ByteArray(TWITCH_PING));
				break;
			case BuiltInCommands::SONG:
			{
				ImageAnnouncePane *pane=Tuple::New<ImageAnnouncePane,QString,QImage>(CurrentSong());
				pane->AccentColor(settingAccentColor);
				StageEphemeralPane(pane);
				break;
			}
			case BuiltInCommands::THINK:
				StageEphemeralPane(new AudioAnnouncePane("Shh... Herb is thinking...",settingThinkingSound));
				break;
			case BuiltInCommands::TIMEZONE:
				chatPane->Alert(QDateTime::currentDateTime().timeZone().displayName(QDateTime::currentDateTime().timeZone().isDaylightTime(QDateTime::currentDateTime()) ? QTimeZone::DaylightTime : QTimeZone::StandardTime,QTimeZone::LongName));
				break;
			case BuiltInCommands::UPTIME:
			{
				Relay::Status::Context *statusContext=chatPane->Alert(Uptime());
				connect(statusContext,&Relay::Status::Context::UpdateRequested,[this,statusContext]() {
					emit statusContext->Updated(Uptime());
				});
				break;
			}
			case BuiltInCommands::VIBE:
				if (vibeKeeper->state() == QMediaPlayer::PlayingState)
				{
					chatPane->Alert("Pausing the vibes...");
					vibeKeeper->pause();
					break;
				}
				vibeKeeper->play();
				break;
			case BuiltInCommands::VOLUME:
				if (command.Message().isEmpty())
				{
					if (vibeFader)
					{
						vibeFader->Abort();
						chatPane->Alert("Aborting volume change...");
					}
					break;
				}
				try
				{
					vibeFader=new Volume::Fader(vibeKeeper,command.Message(),this);
					connect(vibeFader,&Volume::Fader::Feedback,chatPane,&ChatPane::Alert);
					vibeFader->Start();
				}
				catch (const std::range_error &exception)
				{
					chatPane->Alert(QString("%1\n\n%2").arg("Failed to adjust volume!",exception.what()));
				}
				break;
			}
		}
		catch (const std::out_of_range &exception)
		{
			chatPane->Alert(QString(R"(Command "%1" not fully implemented!)").arg(command.Name()));
		}
	});

	if (settingVibePlaylist)
	{
		connect(&vibeSources,&QMediaPlaylist::loadFailed,[this,chatPane]() {
			chatPane->Alert(QString("<b>Failed to load vibe playlist</b><br>%2").arg(vibeSources.errorString()));
		});
		connect(&vibeSources,&QMediaPlaylist::loaded,[this,chatPane]() {
			vibeSources.shuffle();
			vibeSources.setCurrentIndex(Random::Bounded(0,vibeSources.mediaCount()));
			vibeKeeper->setPlaylist(&vibeSources);
			chatPane->Alert("Vibe playlist loaded!");
		});
		vibeSources.load(QUrl::fromLocalFile(settingVibePlaylist));
		connect(vibeKeeper,QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error),[this,chatPane](QMediaPlayer::Error error) {
			chatPane->Alert(QString("<b>Vibe Keeper failed to start</b><br>%2").arg(vibeKeeper->errorString()));
		});
		connect(vibeKeeper,&QMediaPlayer::stateChanged,[this,chatPane](QMediaPlayer::State state) {
			if (state == QMediaPlayer::PlayingState) chatPane->Alert(std::get<QString>(CurrentSong()));
		});
	}

	connect(&helpClock,&QTimer::timeout,[this,chatMessageReceiver]() {
		if (!ephemeralPanes.empty()) return;
		try
		{
			const Command &command=chatMessageReceiver->RandomCommand();
			StageEphemeralPane(new AnnouncePane(QString("<h2>!%1</h2><br>%2").arg(
				command.Name(),
				command.Description()
			)));
		}
		catch (const std::range_error &exception)
		{
			chatMessageReceiver->Alert(QString("Failed to show hint for random command<br>%1").arg(exception.what()));
		}
	});
	helpClock.start();
}

/*!
 * \fn Window::StageEphemeralPane
 * \brief Add an ephemeral pane to the queue of panes to be displayed
 *
 * If this is the first ephemeral pane, it will be shown immediately, otherwise
 * it will be shown when the pane before it is finished. The currently visible
 * persistent pane is hidden while the queue contains ephemeral panes.
 *
 * \param pane The pane to be shown
 */
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

/*!
 * \fn Window::ReleaseLiveEphemeralPane
 * \brief Removes visible emphemeral pane from the queue
 *
 * When the queue becomes empty, this will instead show the persistent pane that
 * was visible before processing of the queue of ephemeral panes was started.
 */
void Window::ReleaseLiveEphemeralPane()
{
	if (ephemeralPanes.empty()) throw std::logic_error("Ran out of ephemeral panes but messages still coming in to remove them"); // FIXME: this is undefined behavior since it's being thrown from a slot
	ephemeralPanes.pop();
	if (ephemeralPanes.empty()) visiblePane->show();
}

std::tuple<QString,QImage> Window::CurrentSong() const
{
	return {
		QString("Now playing %1 by %2\n\nfrom the ablum %3").arg(
			vibeKeeper->metaData("Title").toString(),
			vibeKeeper->metaData("AlbumArtist").toString(),
			vibeKeeper->metaData("AlbumTitle").toString()
		),
		vibeKeeper->metaData("CoverArtImage").value<QImage>()
	};
}

/*!
 * \fn Window::Pong
 * \brief Responds to a Twitch PING with a PONG
 *
 * This is transparent to viewers. It will not be displayed on a pane.
 */
void Window::Pong() const
{
	ircSocket->write(StringConvert::ByteArray(TWITCH_PONG));
}

/*!
 * \fn Window::Uptime
 * \brief Calculates the difference between now and when the bot was started
 * \return A text representation of the difference
 */
const QString Window::Uptime() const
{
	std::chrono::milliseconds duration=TimeConvert::Now()-uptime;
	std::chrono::hours hours=std::chrono::duration_cast<std::chrono::hours>(duration);
	std::chrono::minutes minutes=std::chrono::duration_cast<std::chrono::minutes>(duration-hours);
	std::chrono::seconds seconds=std::chrono::duration_cast<std::chrono::seconds>(duration-minutes);
	return QString("Connection to Twitch has been live for %1 hours, %2 minutes, and %3 seconds").arg(StringConvert::Integer(hours.count()),StringConvert::Integer(minutes.count()),StringConvert::Integer(seconds.count()));
}

/*!
 * \fn Window::ScreenThird
 * \brief Calculates a third of the user's screen size
 * \return A square QSize the size of 1/3 the shortes side of the screen
 */
const QSize Window::ScreenThird()
{
	QSize screenSize=QSize(QGuiApplication::primaryScreen()->geometry().size());
	int shortestSide=std::min(screenSize.width(),screenSize.height())/3;
	return {shortestSide,shortestSide};
}
