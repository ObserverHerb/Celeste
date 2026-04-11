#include <QGuiApplication>
#include <QScreen>
#include <QEvent>
#include <QCloseEvent>
#include <QTimer>
#include <QLayout>
#include <QGridLayout>
#include <QMenu>
#include <QDateTime>
#include <QTimeZone>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <chrono>
#include <stdexcept>
#include "window.h"

const char *SETTINGS_CATEGORY_WINDOW="Window";

using PrintLog=QOverload<const QString&,const QString&,const QString&>;
using PrintUI=QOverload<const QString&>;

Window::Window() : QMainWindow(nullptr),
	background(nullptr),
	settingWindowSize(SETTINGS_CATEGORY_WINDOW,"Size",ScreenThird()),
	settingBackgroundColor(SETTINGS_CATEGORY_WINDOW,"BackgroundColor","#ff000000"),
	configureOptions("Options",this),
	configureCommands("Commands",this),
	configureEventSubscriptions("Event Subscriptions",this),
	metrics("Metrics",this),
	vibePlaylist("Vibe Playlist",this),
	status("Status",this)
{
	setAttribute(Qt::WA_TranslucentBackground,true);
	setFixedSize(settingWindowSize);
	layout()->setContentsMargins(0,0,0,0);

	QColor backgroundColor=settingBackgroundColor;
	background=new PaneHost(backgroundColor,this);
	background->setLayout(new QGridLayout(background)); // for translucency to work, there has to be a widget covering the window, otherwise the entire thing is clear
	background->layout()->setContentsMargins(0,0,0,0);
	setStyleSheet(QString("QMainWindow { background-color: rgba(%1,%2,%3,%4); }").arg(
		StringConvert::Integer(backgroundColor.red()),
		StringConvert::Integer(backgroundColor.green()),
		StringConvert::Integer(backgroundColor.blue()),
		StringConvert::Integer(backgroundColor.alpha()))
	);
	setCentralWidget(background);
	connect(this,PrintUI::of(&Window::Print),background,&PaneHost::Print);
	connect(this,&Window::FlushMedia,background,&PaneHost::Flush);
	connect(background,&PaneHost::HighPriority,this,&Window::HighPriority);
	connect(background,&PaneHost::LowPriority,this,&Window::LowPriority);

	connect(&configureOptions,&QAction::triggered,this,&Window::ConfigureOptions);
	connect(&configureCommands,&QAction::triggered,this,&Window::ConfigureCommands);
	connect(&configureEventSubscriptions,&QAction::triggered,this,&Window::ConfigureEventSubscriptions);
	connect(&metrics,&QAction::triggered,this,&Window::ShowMetrics);
	connect(&vibePlaylist,&QAction::triggered,this,&Window::ShowVibePlaylist);
	connect(&status,&QAction::triggered,this,&Window::ShowStatus);

	StatusPane *pane=new StatusPane(this);
	connect(pane,&StatusPane::ContextMenu,this,&Window::contextMenuEvent);
	background->ReplacePersistent(pane);
}

void Window::AnnounceArrival(const QString &name,std::shared_ptr<QImage> profileImage,const QString &audioPath)
{
	try
	{
		MultimediaAnnouncePane *pane=new MultimediaAnnouncePane({
			{"Please welcome",1},
			{name,1.5},
			{"to the chat",1}
		},*profileImage,audioPath,this,PANE_PRIORITY_IMMEDIATE);
		connect(pane,&MultimediaAnnouncePane::Print,this,PrintLog::of(&Window::Print));
		background->StageEphemeral(pane);
	}

	catch (const std::runtime_error &exception)
	{
		emit Print(exception.what(),"announce arrival");
	}
}

void Window::AnnounceRedemption(const QString &name,const QString& rewardTitle,const QString& message)
{
	AnnouncePane *pane=new AnnouncePane({
		{name,1.5},
		{"has redeemed",1},
		{rewardTitle,1.5},
		{message,1}
	},this,PANE_PRIORITY_DEFAULT);
	connect(pane,&AnnouncePane::Print,this,PrintLog::of(&Window::Print));
	background->StageEphemeral(pane);
}

void Window::AnnounceSubscription(const QString &name,const QString &audioPath)
{
	try
	{
		AudioAnnouncePane *pane=new AudioAnnouncePane({
			{name,1.5},
			{"has subscribed!",1}
		},audioPath,this,PANE_PRIORITY_IMMEDIATE);
		connect(pane,&AudioAnnouncePane::Print,this,PrintLog::of(&Window::Print));
		background->StageEphemeral(pane);
	}

	catch (const std::runtime_error &exception)
	{
		emit Print(exception.what(),"announce subscription");
	}
}

void Window::AnnounceRaid(const QString &name,const unsigned int viewers,const QString &audioPath)
{
	try
	{
		AudioAnnouncePane *pane=new AudioAnnouncePane({
			{name,1.5},
			{"is raiding with",1},
			{StringConvert::PositiveInteger(viewers),1.5},
			{"viewers",1}
		},audioPath,this,PANE_PRIORITY_IMMEDIATE);
		connect(pane,&AudioAnnouncePane::Print,this,PrintLog::of(&Window::Print));
		background->StageEphemeral(pane);
	}

	catch (const std::runtime_error &exception)
	{
		emit Print(exception.what(),"announce raid");
	}
}

void Window::AnnounceCheer(const QString &name,const unsigned int count,const QString &message,const QString &videoPath)
{
	try
	{
		VideoPane *pane=new VideoPane(videoPath,this,PANE_PRIORITY_IMMEDIATE);
		connect(pane,&VideoPane::Print,this,PrintLog::of(&Window::Print));
		background->StageEphemeral(pane);
		background->StageEphemeral(new AnnouncePane({
			{QString("%1 has cheered").arg(name),0.5},
			{message,1.5},
			{QString("for %1 bits").arg(StringConvert::Integer(count)),0.5}
		},this,PANE_PRIORITY_IMMEDIATE));
	}

	catch (const std::runtime_error &exception)
	{
		emit Print(exception.what(),"announce bits cheered");
	}
}

void Window::AnnounceTextWall(const QString &message,const QString &audioPath)
{
	try
	{
		AudioAnnouncePane *pane=new AudioAnnouncePane({
			{message,0.5},
		},audioPath,this,PANE_PRIORITY_DEFAULT-1);
		connect(pane,&AudioAnnouncePane::Print,this,PrintLog::of(&Window::Print));
		background->StageEphemeral(pane);
	}

	catch (const std::runtime_error &exception)
	{
		emit Print(exception.what(),"announce wall-of-text");
	}
}

void Window::AnnounceDeniedCommand(const QString &videoPath)
{
	try
	{
		VideoPane *pane=new VideoPane(videoPath,this,PANE_PRIORITY_IMMEDIATE-1);
		connect(pane,&VideoPane::Print,this,PrintLog::of(&Window::Print));
		background->StageEphemeral(pane);
	}

	catch (const std::runtime_error &exception)
	{
		emit Print(exception.what(),"play denial video");
	}
}

void Window::AnnounceHypeTrainProgress(int level,double progress)
{
	AnnouncePane *pane=new AnnouncePane({
		{u"Hype Train!"_s,0.5},
		{u"Level %1"_s.arg(level),2},
		{u"%1% of the way to level "_s.arg(progress*100,0,'f',2)+StringConvert::Integer(level+1),1}
	},this,PANE_PRIORITY_DEFAULT+1);
	background->StageEphemeral(pane);
}

void Window::AnnounceAdBreakStarting(const QString& videoPath)
{
	try
	{
		VideoPane *pane=new VideoPane(videoPath,this,PANE_PRIORITY_ROUTINE);
		connect(pane,&VideoPane::Print,this,PrintLog::of(&Window::Print));
		background->StageEphemeral(pane);
	}

	catch (const std::runtime_error& exception)
	{
		emit Print(exception.what(),"play ad break starting video");
	}
}

void Window::AnnounceAdBreakFinished(const QString& videoPath)
{
	try
	{
		VideoPane *pane=new VideoPane(videoPath,this,PANE_PRIORITY_ROUTINE);
		connect(pane,&VideoPane::Print,this,PrintLog::of(&Window::Print));
		background->StageEphemeral(pane);
	}

	catch (const std::runtime_error& exception)
	{
		emit Print(exception.what(),"play finished ad break video");
	}
}

void Window::ShowChat()
{
	ChatPane *chatPane=new ChatPane(this);
	background->ReplacePersistent(chatPane)->deleteLater();
	connect(this,&Window::ChatMessage,chatPane,&ChatPane::Message);
	connect(this,&Window::DeleteChatMessage,chatPane,&ChatPane::DeleteMessage);
	connect(this,&Window::RefreshChat,chatPane,&ChatPane::Refresh);
	connect(this,&Window::SetAgenda,chatPane,&ChatPane::SetAgenda);
	connect(chatPane,&ChatPane::ContextMenu,this,&Window::contextMenuEvent);
}

void Window::PlayVideo(const QString &path,bool chaosMode)
{
	try
	{
		VideoPane *pane=new VideoPane(path,this,PANE_PRIORITY_IMMEDIATE);
		connect(pane,&VideoPane::Print,this,PrintLog::of(&Window::Print));
		background->StageEphemeral(pane,chaosMode);
	}

	catch (const std::runtime_error &exception)
	{
		emit Print(exception.what(),"play video");
	}
}

void Window::PlayAudio(const QString &viewer,const QString &message,const QString &path)
{
	try
	{
		AudioAnnouncePane *pane=new AudioAnnouncePane({
			{viewer,1.5},
			{message,1}
		},path,this,PANE_PRIORITY_IMMEDIATE);
		connect(pane,&AudioAnnouncePane::Print,this,PrintLog::of(&Window::Print));
		background->StageEphemeral(pane);
	}

	catch (const std::runtime_error &exception)
	{
		emit Print(exception.what(),"play audio");
	}
}

void Window::ShowPortraitVideo(const QString &path)
{
	try
	{
		VideoPane *pane=new VideoPane(path,this,PANE_PRIORITY_INCONSEQUENTIAL);
		connect(pane,&VideoPane::Print,this,PrintLog::of(&Window::Print));
		background->StageEphemeral(pane);
	}

	catch (const std::runtime_error &exception)
	{
		emit Print(exception.what(),"show portrait video");
	}
}

void Window::ShowCommandList(std::vector<std::tuple<QString,QStringList,QString>> descriptions)
{
	QString text;
	for (std::tuple<QString,QStringList,QString> &command : descriptions)
	{
		// https://doc.qt.io/qt-6/qstring.html#prepend
		// This operation is typically very fast (constant time),
		// because QString preallocates extra space at the beginning
		// of the string data, so it can grow without reallocating
		// the entire string each time.
		text.append(QString("<div class='name'>!%1<br>").arg(std::get<0>(command)));
		if (QStringList aliases=std::get<1>(command); !aliases.empty()) text.append(QString("<span class='aliases'>%1<br></span>").arg("!"+std::get<1>(command).join(", !")));
		text.append(QString("<span class='description'>%1</span><br></div>").arg(std::get<2>(command)));
	}
	ScrollingPane *pane=new ScrollingPane(text,this);
	connect(pane,&ScrollingPane::Print,this,PrintLog::of(&Window::Print));
	background->StageEphemeral(pane);
}

void Window::ShowCommand(const QString &name,const QString &description)
{
	AnnouncePane *pane=new AnnouncePane({
		{u"!"_s+name,1.5},
		{description,1}
	},this,PANE_PRIORITY_INCONSEQUENTIAL);
	connect(pane,&AnnouncePane::Print,this,PrintLog::of(&Window::Print));
	background->StageEphemeral(pane);
}

void Window::ShowPanicText(const QString &text)
{
	background->ReplacePersistent(new StatusPane(this))->deleteLater();
	emit Print(text);
}

void Window::Shoutout(const QString &name,const QString &description,std::shared_ptr<QImage> profileImage)
{
	ImageAnnouncePane *pane=new ImageAnnouncePane({
		{"Drop a follow on",1},
		{name,1.5},
		{description,0.5}
	},*profileImage,this,PANE_PRIORITY_ROUTINE);
	connect(pane,&ImageAnnouncePane::Print,this,PrintLog::of(&Window::Print));
	pane->Duration(10000); // TODO: change from hardcoded to configurable duration
	background->StageEphemeral(pane);
}

void Window::ShowFollowage(const QString &name,std::chrono::years years,std::chrono::months months,std::chrono::days days)
{
	Lines lines;
	lines.emplace_back(name,1.5);
	lines.emplace_back("has been following the stream for",1);
	if (years.count() > 0) lines.emplace_back(QString("%1 %2").arg(StringConvert::Integer(years.count()),StringConvert::NumberAgreement("year","years",NumberConvert::Positive(years.count()))),1.5);
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
	if (!finalLine.isEmpty()) lines.emplace_back(finalLine,years.count() > 0 ? 1 : 1.5);
	AnnouncePane *pane=new AnnouncePane(lines,this,PANE_PRIORITY_ROUTINE);
	connect(pane,&AnnouncePane::Print,this,PrintLog::of(&Window::Print));
	background->StageEphemeral(pane);
}

void Window::ShowTimezone(const QString &timezone)
{
	emit Print(timezone);
}

void Window::ShowUptime(std::chrono::hours hours,std::chrono::minutes minutes,std::chrono::seconds seconds)
{
	Lines lines;
	lines.emplace_back("Stream has been live for",1);
	if (hours.count() > 0) lines.emplace_back(QString("%1 %2").arg(StringConvert::Integer(hours.count()),StringConvert::NumberAgreement("hour","hours",NumberConvert::Positive(hours.count()))),1.5);
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
	if (!finalLine.isEmpty()) lines.emplace_back(finalLine,hours.count() > 0 ? 1 : 1.5);
	AnnouncePane *pane=new AnnouncePane(lines,this,PANE_PRIORITY_ROUTINE);
	connect(pane,&AnnouncePane::Print,this,PrintLog::of(&Window::Print));
	background->StageEphemeral(pane);
}

void Window::ShowCurrentSong(const QString &song,const QString &album,const QString &artist,const QImage coverArt)
{
	ImageAnnouncePane *pane=new ImageAnnouncePane({
		{"Now playing",0.5},
		{song,1.0},
		{"by",0.5},
		{artist,0.75},
		{"from the ablum",0.5},
		{album,0.75}
	},coverArt,this,PANE_PRIORITY_INCONSEQUENTIAL);
	connect(pane,&ImageAnnouncePane::Print,this,PrintLog::of(&Window::Print));
	background->StageEphemeral(pane);
}

void Window::ShowCurrentSong(const QString &song,const QString &artist,const QImage coverArt)
{
	ImageAnnouncePane *pane=new ImageAnnouncePane({
		{"Now playing",0.5},
		{song,1.0},
		{"by",0.5},
		{artist,0.75}
	},coverArt,this,PANE_PRIORITY_INCONSEQUENTIAL);
	connect(pane,&ImageAnnouncePane::Print,this,PrintLog::of(&Window::Print));
	background->StageEphemeral(pane);
}

void Window::Resize(const QSize &dimensions)
{
	if (dimensions.isEmpty())
		setFixedSize(settingWindowSize);
	else
		setFixedSize(dimensions);
}

const QSize Window::ScreenThird()
{
	QSize screenSize=QSize(QGuiApplication::primaryScreen()->geometry().size());
	int shortestSide=std::min(screenSize.width(),screenSize.height())/3;
	return {shortestSide,shortestSide};
}

ApplicationSetting& Window::BackgroundColor()
{
	return settingBackgroundColor;
}

ApplicationSetting& Window::Dimensions()
{
	return settingWindowSize;
}

void Window::contextMenuEvent(QContextMenuEvent *event)
{
	QMenu menu(this);
	menu.addAction(&configureOptions);
	menu.addAction(&configureCommands);
	menu.addAction(&vibePlaylist);
	menu.addSeparator();
	menu.addAction(&configureEventSubscriptions);
	menu.addAction(&metrics);
	menu.addSeparator();
	menu.addAction(&status);
	menu.exec(event->globalPos());
	event->accept();
}

void Window::closeEvent(QCloseEvent *event)
{
	emit CloseRequested(event);
}
