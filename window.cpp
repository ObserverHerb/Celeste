#include <QGuiApplication>
#include <QScreen>
#include <QEvent>
#include <QCloseEvent>
#include <QTimer>
#include <QLayout>
#include <QGridLayout>
#include <QDateTime>
#include <QTimeZone>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <chrono>
#include <stdexcept>
#include "window.h"

const char *SETTINGS_CATEGORY_WINDOW="Window";

Window::Window() : QWidget(nullptr),
	visiblePane(nullptr),
	background(new QWidget(this)),
	settingWindowSize(SETTINGS_CATEGORY_WINDOW,"Size"),
	settingBackgroundColor(SETTINGS_CATEGORY_WINDOW,"BackgroundColor","#ff000000"),
	settingAccentColor(SETTINGS_CATEGORY_WINDOW,"AccentColor","#ff000000")
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
}

void Window::SwapPane(PersistentPane *pane)
{
	if (visiblePane) visiblePane->deleteLater();
	visiblePane=pane;
	background->layout()->addWidget(visiblePane);
	connect(this,&Window::Print,visiblePane,&PersistentPane::Print);
}

void Window::AnnounceArrival(const QString &name,QImage profileImage,const QString &audioPath)
{
	StageEphemeralPane(new MultimediaAnnouncePane({
		{"Please welcome<br>",1},
		{QString("%1<br>").arg(name),1.5},
		{"to the chat",1}
	},profileImage,audioPath));
}

void Window::AnnounceRedemption(const QString &viewer,const QString& rewardTitle,const QString& message)
{
	StageEphemeralPane(new AnnouncePane({
		{QString("%1<br>").arg(viewer),1.5},
		{"has redeemed<br>",1},
		{QString("%1<br>").arg(rewardTitle),1.5},
		{message,1}
	}));
}

void Window::AnnounceSubscription(const QString &viewer)
{
	/*StageEphemeralPane(new AudioAnnouncePane({
		{QString("%1<br>").arg(viewer),1.5},
		{"has subscribed!",1}
	},settingSubscriptionSound));*/
}

void Window::AnnounceRaid(const QString &viewer,const unsigned int viewers)
{
	/*StageEphemeralPane(new AudioAnnouncePane({
		{QString("%1<br>").arg(viewer),1.5},
		{"is raiding with<br>",1},
		{QString("%1<br>").arg(StringConvert::PositiveInteger(viewers)),1.5},
		{"viewers",1}
	},settingRaidSound));
	lastRaid=QDateTime::currentDateTime();*/
	// FIXME: finish this
}

void Window::ShowChat()
{
	/*ChatPane *chatPane=new ChatPane(this);
	SwapPane(chatPane);
	connect(this,&Window::Print,chatPane,&ChatPane::Print);
	connect(this,&Window::ChatMessage,chatPane,&ChatPane::Message);
	connect(this,&Window::RefreshChat,chatPane,&ChatPane::Refresh);
	connect(this,&Window::SetAgenda,chatPane,&ChatPane::SetAgenda);*/
}

void Window::PlayVideo(const QString &path)
{
	StageEphemeralPane(new VideoPane(path));
}

void Window::PlayAudio(const QString &viewer,const QString &message,const QString &path)
{
	StageEphemeralPane(new AudioAnnouncePane({
		{QString("%1<br>").arg(viewer),1.5},
		{message,1}
	},path));
}

void Window::ShowCommandList(std::vector<std::pair<QString,QString>> descriptions)
{
	AnnouncePane *pane;
	int duration=15000;
	int count=8; // TODO: this should be calculated based on text line height vs pixel height of window
	count=count*2; // a single command covers two lines
	for (int page=0; page < std::ceil(static_cast<double>(descriptions.size())/count); page++)
	{
		std::vector<std::pair<QString,double>> lines;
		for (const std::pair<QString,QString> &command : descriptions)
		{
			lines.push_back({QString("<div style='margin-bottom: 0;'>!%1</div>").arg(command.first),0.6});
			lines.push_back({QString("<div style='margin-bottom: 0.5em;'>%1</div>").arg(command.second),0.5});
		}
		int duration=15000;
		int count=8; // TODO: this should be calculated based on text line height vs pixel height of window
		count=count*2; // a single command covers two lines
		for (int page=0; page < std::ceil(static_cast<double>(lines.size())/count); page++)
		{
			std::vector<std::pair<QString,double>>::size_type start=page*count;
			std::vector<std::pair<QString,double>>::size_type end=start+count;
			pane=new AnnouncePane({&lines[start],&lines[std::min(lines.size(),end)]});
			pane->Duration(duration);
			StageEphemeralPane(pane);
		}
	}
}

void Window::ShowCommand(const QString &name,const QString &description)
{
	if (!ephemeralPanes.empty()) return;
	StageEphemeralPane(new AnnouncePane(QString("<h2>!%1</h2><br>%2").arg(
		name,
		description
	)));
}

void Window::ShowPanicText(const QString &text)
{
	SwapPane(new StatusPane(this));
	emit Print(text);
}

void Window::Shoutout(const QString &name,const QString &description,const QImage &profileImage)
{
	ImageAnnouncePane *pane=new ImageAnnouncePane({
		{"Drop a follow on<br>",1},
		{QString("%1<br>").arg(name),1.5},
		{description,0.5}
	},profileImage);
	pane->AccentColor(settingAccentColor);
	pane->Duration(10000); // TODO: change from hardcoded to configurable duration
	StageEphemeralPane(pane);
}

void Window::ShowTimezone(const QString &timezone)
{
	emit Print(QDateTime::currentDateTime().timeZone().displayName(QDateTime::currentDateTime().timeZone().isDaylightTime(QDateTime::currentDateTime()) ? QTimeZone::DaylightTime : QTimeZone::StandardTime,QTimeZone::LongName));
}

void Window::ShowUptime(std::chrono::hours hours,std::chrono::minutes minutes,std::chrono::seconds seconds)
{
	// TODO: I think I wanted this to show the ticks as they're ticking, and that was the idea behind the status context
	emit Print(QString("Connection to Twitch has been live for %1 hours, %2 minutes, and %3 seconds").arg(StringConvert::Integer(hours.count()),StringConvert::Integer(minutes.count()),StringConvert::Integer(seconds.count())));
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
	if (ephemeralPanes.empty()) throw std::logic_error("Ran out of ephemeral panes but messages still coming in to remove them"); // FIXME: this is undefined behavior since it's being thrown from a slot
	if (ephemeralPanes.front()->Expired()) ephemeralPanes.pop();
	if (ephemeralPanes.empty()) visiblePane->show();
}

void Window::ShowCurrentSong(const QString &song,const QString &album,const QString &artist,const QImage coverArt)
{
	ImageAnnouncePane *pane=new ImageAnnouncePane({
		{QString("Now playing %1").arg(song),1.5},
		{"by",0.5},
		{QString("%2").arg(artist),1},
		{"from the ablum",0.5},
		{QString("%3").arg(album),1}
	},coverArt);
	pane->AccentColor(settingAccentColor);
	StageEphemeralPane(pane);
}

const QSize Window::ScreenThird()
{
	QSize screenSize=QSize(QGuiApplication::primaryScreen()->geometry().size());
	int shortestSide=std::min(screenSize.width(),screenSize.height())/3;
	return {shortestSide,shortestSide};
}

