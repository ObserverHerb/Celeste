#include <QGuiApplication>
#include <QScreen>
#include <QEvent>
#include <QCloseEvent>
#include <QTimer>
#include <QLayout>
#include <QGridLayout>
#include <QDateTime>
#include <QTimeZone>
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
		{"Please welcome",1},
		{QString("%1").arg(name),1.5},
		{"to the chat",1}
	},profileImage,audioPath,this));
}

void Window::AnnounceRedemption(const QString &name,const QString& rewardTitle,const QString& message)
{
	StageEphemeralPane(new AnnouncePane({
		{QString("%1").arg(name),1.5},
		{"has redeemed",1},
		{QString("%1").arg(rewardTitle),1.5},
		{message,1}
	},this));
}

void Window::AnnounceSubscription(const QString &name,const QString &audioPath)
{
	AudioAnnouncePane *pane=new AudioAnnouncePane({
		{QString("%1").arg(name),1.5},
		{"has subscribed!",1}
	},audioPath,this);
	connect(pane,&AudioAnnouncePane::Print,this,&Window::Print);
	StageEphemeralPane(pane);
}

void Window::AnnounceRaid(const QString &name,const unsigned int viewers,const QString &audioPath)
{
	AudioAnnouncePane *pane=new AudioAnnouncePane({
		{QString("%1").arg(name),1.5},
		{"is raiding with",1},
		{QString("%1").arg(StringConvert::PositiveInteger(viewers)),1.5},
		{"viewers",1}
	},audioPath,this);
	connect(pane,&AudioAnnouncePane::Print,this,&Window::Print);
	StageEphemeralPane(pane);
}

void Window::AnnounceCheer(const QString &name,const unsigned int count,const QString &message,const QString &videoPath)
{
	VideoPane *pane=new VideoPane(videoPath,this);
	connect(pane,&VideoPane::Print,this,&Window::Print);
	StageEphemeralPane(pane);
	StageEphemeralPane(new AnnouncePane({
		{QString("%1 has cheered").arg(name),0.5},
		{QString("%1").arg(message),1.5},
		{QString("for %1 bits").arg(StringConvert::Integer(count)),0.5}
	},this));
}

void Window::AnnounceTextWall(const QString &message,const QString &audioPath)
{
	AudioAnnouncePane *pane=new AudioAnnouncePane({
		{message,0.5},
	},audioPath,this);
	connect(pane,&AudioAnnouncePane::Print,this,&Window::Print);
	StageEphemeralPane(pane);
}

void Window::AnnounceHost(const QString &hostingChannel,const QString &audioPath)
{
	StageEphemeralPane(new AudioAnnouncePane({
		{hostingChannel,1.5},
		{"is hosting the stream!",1}
	},audioPath,this));
}

void Window::ShowChat()
{
	ChatPane *chatPane=new ChatPane(this);
	SwapPane(chatPane);
	connect(this,&Window::Print,chatPane,&ChatPane::Print);
	connect(this,&Window::ChatMessage,chatPane,&ChatPane::Message);
	connect(this,&Window::RefreshChat,chatPane,&ChatPane::Refresh);
	connect(this,&Window::SetAgenda,chatPane,&ChatPane::SetAgenda);
}

void Window::PlayVideo(const QString &path)
{
	StageEphemeralPane(new VideoPane(path,this));
}

void Window::PlayAudio(const QString &viewer,const QString &message,const QString &path)
{
	StageEphemeralPane(new AudioAnnouncePane({
		{QString("%1").arg(viewer),1.5},
		{message,1}
	},path,this));
}

void Window::ShowPortraitVideo(const QString &path)
{
	StageEphemeralPane(new VideoPane(path,this));
}

void Window::ShowCommandList(std::vector<std::pair<QString,QString>> descriptions)
{
	QString text;
	for (const std::pair<QString,QString> &command : descriptions)
	{
		text.append(QString("<div class='name'>!%1<br><span class='description'> %2<br></div>").arg(command.first,command.second));
	}
	StageEphemeralPane(new ScrollingAnnouncePane(text,this));
}

void Window::ShowCommand(const QString &name,const QString &description)
{
	if (!ephemeralPanes.empty()) return;
	StageEphemeralPane(new AnnouncePane(QString("<h2>!%1</h2><br>%2").arg(
		name,
		description
	),this));
}

void Window::ShowPanicText(const QString &text)
{
	SwapPane(new StatusPane(this));
	emit Print(text);
}

void Window::Shoutout(const QString &name,const QString &description,const QImage &profileImage)
{
	ImageAnnouncePane *pane=new ImageAnnouncePane({
		{"Drop a follow on",1},
		{QString("%1").arg(name),1.5},
		{description,0.5}
	},profileImage,this);
	pane->AccentColor(settingAccentColor);
	pane->Duration(10000); // TODO: change from hardcoded to configurable duration
	StageEphemeralPane(pane);
}

void Window::ShowFollowage(const QString &name,std::chrono::years years,std::chrono::months months,std::chrono::days days)
{
	Lines lines;
	lines.push_back({QString("%1").arg(name),1.5});
	lines.push_back({"has been following the stream for",1});
	if (years.count() > 0) lines.push_back({QString("%1 %2").arg(StringConvert::Integer(years.count()),StringConvert::NumberAgreement("year","years",NumberConvert::Positive(years.count()))),1.5});
	QString finalLine;
	if (months.count() > 0)
	{
		if (days.count() < 1) finalLine.append("and ");
		finalLine.append(QString("%1 %2").arg(StringConvert::Integer(months.count()),StringConvert::NumberAgreement("month","months",NumberConvert::Positive(months.count()))));
	}
	if (days.count() > 0)
	{
		if (years.count() > 0 && months.count() > 0) finalLine.append(",");
		if (years.count() > 0 || months.count() > 0) finalLine.append(" and ");
		finalLine.append(QString("%1 %2").arg(StringConvert::Integer(days.count()),StringConvert::NumberAgreement("day","days",NumberConvert::Positive(days.count()))));
	}
	if (!finalLine.isEmpty()) lines.push_back({finalLine,years.count() > 0 ? 1 : 1.5});
	StageEphemeralPane(new AnnouncePane(lines,this));
}

void Window::ShowTimezone(const QString &timezone)
{
	emit Print(QDateTime::currentDateTime().timeZone().displayName(QDateTime::currentDateTime().timeZone().isDaylightTime(QDateTime::currentDateTime()) ? QTimeZone::DaylightTime : QTimeZone::StandardTime,QTimeZone::LongName));
}

void Window::ShowUptime(std::chrono::hours hours,std::chrono::minutes minutes,std::chrono::seconds seconds)
{
	Lines lines;
	lines.push_back({"Stream has been live for",1});
	if (hours.count() > 0) lines.push_back({QString("%1 %2").arg(StringConvert::Integer(hours.count()),StringConvert::NumberAgreement("hour","hours",NumberConvert::Positive(hours.count()))),1.5});
	QString finalLine;
	if (minutes.count() > 0)
	{
		if (seconds.count() < 1) finalLine.append("and ");
		finalLine.append(QString("%1 %2").arg(StringConvert::Integer(minutes.count()),StringConvert::NumberAgreement("minute","minutes",NumberConvert::Positive(minutes.count()))));
	}
	if (seconds.count() > 0)
	{
		if (hours.count() > 0 && minutes.count() > 0) finalLine.append(", ");
		if (hours.count() > 0 || minutes.count() > 0) finalLine.append(" and ");
		finalLine.append(QString("%1 %2").arg(StringConvert::Integer(seconds.count()),StringConvert::NumberAgreement("second","seconds",NumberConvert::Positive(seconds.count()))));
	}
	if (!finalLine.isEmpty()) lines.push_back({finalLine,hours.count() > 0 ? 1 : 1.5});
	StageEphemeralPane(new AnnouncePane(lines,this));
}

void Window::StageEphemeralPane(EphemeralPane *pane)
{
	connect(pane,&EphemeralPane::Expired,this,&Window::ReleaseLiveEphemeralPane);
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
	ephemeralPanes.pop();
	if (ephemeralPanes.empty()) visiblePane->show();
}

void Window::ShowCurrentSong(const QString &song,const QString &album,const QString &artist,const QImage coverArt)
{
	ImageAnnouncePane *pane=new ImageAnnouncePane({
		{QString("Now playing"),0.5},
		{QString("%1").arg(song),1.0},
		{"by",0.5},
		{QString("%2").arg(artist),0.75},
		{"from the ablum",0.5},
		{QString("%3").arg(album),0.75}
	},coverArt,this);
	pane->AccentColor(settingAccentColor);
	StageEphemeralPane(pane);
}

const QSize Window::ScreenThird()
{
	QSize screenSize=QSize(QGuiApplication::primaryScreen()->geometry().size());
	int shortestSide=std::min(screenSize.width(),screenSize.height())/3;
	return {shortestSide,shortestSide};
}

