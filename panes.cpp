#include "panes.h"

#include <QVBoxLayout>
#include <QStackedLayout>
#include <QLabel>
#include <QResizeEvent>

const QString StatusPane::SETTINGS_CATEGORY="StatusPane";

StatusPane::StatusPane(QWidget *parent) : PersistentPane(parent),
	output(this),
	settingFont(SETTINGS_CATEGORY,"Font","Droid Sans Mono"),
	settingFontSize(SETTINGS_CATEGORY,"FontSize",10),
	settingForegroundColor(SETTINGS_CATEGORY,"ForegroundColor","#ffffffff"),
	settingBackgroundColor(SETTINGS_CATEGORY,"BackgroundColor","#ff000000")
{
	output.setStyleSheet(StyleSheet::Colors<StaticTextEdit>(settingForegroundColor,settingBackgroundColor));
	output.setFontFamily(settingFont);
	output.setFontPointSize(settingFontSize);
	output.document()->setDocumentMargin(16);
	output.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	output.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	output.setFrameStyle(QFrame::NoFrame);

	connect(&output,&StaticTextEdit::ContextMenu,this,&StatusPane::ContextMenu);

	setLayout(new QVBoxLayout(this));
	layout()->setContentsMargins(0,0,0,0);
	layout()->addWidget(&output);
}

void StatusPane::Print(const QString &text)
{
	output.insertPlainText(text);
	output.insertPlainText("\n");
	output.ensureCursorVisible();
}

ApplicationSetting& StatusPane::Font()
{
	return settingFont;
}

ApplicationSetting& StatusPane::FontSize()
{
	return settingFontSize;
}

ApplicationSetting& StatusPane::ForegroundColor()
{
	return settingForegroundColor;
}

ApplicationSetting& StatusPane::BackgroundColor()
{
	return settingBackgroundColor;
}

const QString ChatPane::SETTINGS_CATEGORY="ChatPane";

ChatPane::ChatPane(QWidget *parent) : PersistentPane(parent),
	agenda(nullptr),
	chat(nullptr),
	status(nullptr),
	settingFont(SETTINGS_CATEGORY,"Font","Copperplate Gothic Bold"),
	settingFontSize(SETTINGS_CATEGORY,"FontSize",12),
	settingForegroundColor(SETTINGS_CATEGORY,"ForegroundColor","#ffffffff"),
	settingBackgroundColor(SETTINGS_CATEGORY,"BackgroundColor","#ff000000"),
	settingStatusInterval(SETTINGS_CATEGORY,"StatusInterval",5000)
{
	setLayout(new QVBoxLayout(this));
	layout()->setContentsMargins(0,0,0,0);
	layout()->setSpacing(0);

	agenda=new QLabel(this);
	agenda->setStyleSheet(StyleSheet::Colors<QLabel>(settingForegroundColor,settingBackgroundColor));
	agenda->setFont(QFont(settingFont,static_cast<qreal>(settingFontSize)*1.666,QFont::Bold));
	agenda->setMargin(16);
	agenda->setAlignment(Qt::AlignCenter);
	agenda->setWordWrap(true);
	agenda->hide();
	layout()->addWidget(agenda);

	chat=new PinnedTextEdit(this);
	chat->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	chat->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	chat->setFrameStyle(QFrame::NoFrame);
	chat->setCursorWidth(0);
	layout()->addWidget(chat);
	connect(chat,&PinnedTextEdit::ContextMenu,this,&ChatPane::ContextMenu);

	status=new QLabel(this);
	status->setMargin(16);
	status->setAlignment(Qt::AlignCenter);
	status->setWordWrap(true);
	status->setTextFormat(Qt::RichText);
	status->hide();
	layout()->addWidget(status);

	statusClock.setInterval(TimeConvert::Interval(static_cast<std::chrono::milliseconds>(settingStatusInterval)));
	connect(&statusClock,&QTimer::timeout,this,&ChatPane::DismissStatus);

	Format();
}

void ChatPane::SetAgenda(const QString &text)
{
	if (text.isEmpty())
	{
		agenda->clear();
		agenda->hide();
	}
	else
	{
		agenda->setText(text);
		agenda->show();
	}
}

void ChatPane::Format()
{
	chat->setStyleSheet(StyleSheet::Colors<PinnedTextEdit>(settingForegroundColor,settingBackgroundColor));
	chat->setFontFamily(settingFont);
	chat->setFontPointSize(settingFontSize);
	chat->document()->setDefaultStyleSheet(QString("div.user { font-family: '%1'; font-size: %2pt; } div.message, span.message { font-family: '%1'; font-size: %3pt; }").arg(static_cast<QString>(settingFont),StringConvert::Integer(static_cast<int>(settingFontSize)*1.333),StringConvert::Integer(static_cast<int>(settingFontSize))));
	chat->document()->setDocumentMargin(static_cast<qreal>(settingFontSize)*1.333);
	status->setStyleSheet(StyleSheet::Colors<QLabel>(settingForegroundColor,settingBackgroundColor));
	status->setFont(QFont(settingFont,static_cast<qreal>(settingFontSize)*0.833)); // QLabel doesn't have setFontFamily()
}

void ChatPane::Refresh()
{
	Format();
	chat->viewport()->update();
}

void ChatPane::Message(const Chat::Message &message) const
{
	QString badges;
	for (const QString &icon : message.badges) badges.append(QString{"<img style='vertical-align: middle;' src='%1' /> "}.arg(icon));

	QString emotedMessage;
	unsigned int position=0;
	for (const Chat::Emote &emote : message.emotes)
	{
		if (position < emote.start) emotedMessage+=QStringView{message.text}.mid(position,emote.start-position);
		emotedMessage+=QString(R"(<img style="vertical-align: middle;" src="%1" />)").arg(emote.path);
		position=emote.end+1;
	}
	if (position < static_cast<unsigned int>(message.text.size())) emotedMessage+=QStringView{message.text}.mid(position,message.text.size()-position);

	if (message.action)
		chat->Append(QString("<div>%4</div><div class='user' style='color: %3;'>%1 <span class='message'>%2</span><br></div>").arg(message.displayName,emotedMessage,message.color.isValid() ? message.color.name() : settingForegroundColor,badges));
	else
		chat->Append(QString("<div>%4</div><div class='user' style='color: %3;'>%1</div><div class='message'>%2<br></div>").arg(message.displayName,emotedMessage,message.color.isValid() ? message.color.name() : settingForegroundColor,badges));
}

void ChatPane::Print(const QString &text)
{
	statuses.push(text);
	status->setText(statuses.front());
	status->show();
	if (!statusClock.isActive()) statusClock.start();
}

void ChatPane::DismissStatus()
{
	statuses.pop();
	if (!statuses.empty())
	{
		status->setText(statuses.front());
		return;
	}
	status->clear();
	status->hide();
	statusClock.stop();
}

ApplicationSetting& ChatPane::Font()
{
	return settingFont;
}

ApplicationSetting& ChatPane::FontSize()
{
	return settingFontSize;
}

ApplicationSetting& ChatPane::ForegroundColor()
{
	return settingForegroundColor;
}

ApplicationSetting& ChatPane::BackgroundColor()
{
	return settingBackgroundColor;
}

ApplicationSetting& ChatPane::StatusInterval()
{
	return settingStatusInterval;
}

EphemeralPane::EphemeralPane(QWidget *parent,bool highPriority) : QWidget(parent), expired(false), highPriority(highPriority)
{
	setVisible(false);
	connect(this,&EphemeralPane::Finished,this,&EphemeralPane::Expire);
}

void EphemeralPane::LowerPriority()
{
	highPriority=false;
}

bool EphemeralPane::HighPriority() const
{
	return highPriority;
}

void EphemeralPane::Expire()
{
	if (expired) return;
	expired=true;
	emit Expired();
	deleteLater();
}

VideoPane::VideoPane(const QString &path,QWidget *parent) noexcept(false) : EphemeralPane(parent), videoPlayer(Multimedia::Player(this,1)), viewport(new QVideoWidget(this))
{
	if (!QFile(path).exists()) throw std::runtime_error(QString{"Video doesn't exist ("+path+")"}.toStdString());
	videoPlayer->setVideoOutput(viewport);
	videoPlayer->setSource(QUrl::fromLocalFile(path));
	connect(videoPlayer,&QMediaPlayer::playbackStateChanged,[this](QMediaPlayer::PlaybackState state) {
		if (state == QMediaPlayer::StoppedState) emit Finished();
	});

	setLayout(new QVBoxLayout(this));
	layout()->setContentsMargins(0,0,0,0);
	layout()->addWidget(viewport);
}

void VideoPane::showEvent(QShowEvent *event)
{
	videoPlayer->play();
	QWidget::showEvent(event);
}

void VideoPane::hideEvent(QHideEvent *event)
{
	videoPlayer->pause();
	QWidget::hideEvent(event);
}

QString VideoPane::Subsystem()
{
	return u"video pane"_s;
}

const QString ScrollingPane::SETTINGS_CATEGORY="ScrollingPane";

ScrollingPane::ScrollingPane(const QString &text,QWidget *parent) : EphemeralPane(parent,false),
	commands(new ScrollingTextEdit(this)),
	settingFont(SETTINGS_CATEGORY,"Font","Copperplate Gothic Bold"),
	settingFontSize(SETTINGS_CATEGORY,"FontSize",20),
	settingForegroundColor(SETTINGS_CATEGORY,"ForegroundColor","#ffffffff"),
	settingBackgroundColor(SETTINGS_CATEGORY,"BackgroundColor","#ff000000")
{
	QGridLayout *gridLayout=new QGridLayout(this);
	setLayout(gridLayout);
	gridLayout->setContentsMargins(0,0,0,0);
	gridLayout->setSpacing(0);

	commands->setStyleSheet(StyleSheet::Colors<ScrollingTextEdit>(settingForegroundColor,settingBackgroundColor));
	commands->setFontFamily(settingFont);
	commands->setFontPointSize(settingFontSize);
	commands->document()->setDefaultStyleSheet(QString("div.name { font-family: '%1'; font-size: %2pt; } span.description, span.aliases { font-family: '%1'; font-size: %3pt; }").arg(static_cast<QString>(settingFont),StringConvert::Integer(static_cast<int>(settingFontSize)),StringConvert::Integer(static_cast<int>(settingFontSize)*0.7)));
	commands->document()->setDocumentMargin(static_cast<qreal>(settingFontSize)*1.333);
	commands->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	commands->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	commands->setFrameStyle(QFrame::NoFrame);
	commands->setCursorWidth(0);
	commands->setText(text);
	gridLayout->addWidget(commands);

	connect(commands,&ScrollingTextEdit::Finished,this,&ScrollingPane::Finished);
}

QString ScrollingPane::Subsystem()
{
	return u"scrolling pane"_s;
}

const QString AnnouncePane::SETTINGS_CATEGORY="AnnouncePane";

AnnouncePane::AnnouncePane(const Lines &lines,QWidget *parent) : EphemeralPane(parent),
	lines(lines),
	output(new QLabel(this)),
	settingDuration(SETTINGS_CATEGORY,"Duration",5000),
	settingFont(SETTINGS_CATEGORY,"Font","Copperplate Gothic Bold"),
	settingFontSize(SETTINGS_CATEGORY,"FontSize",20),
	settingForegroundColor(SETTINGS_CATEGORY,"ForegroundColor","#ffffffff"),
	settingBackgroundColor(SETTINGS_CATEGORY,"BackgroundColor","#ff000000"),
	settingAccentColor(SETTINGS_CATEGORY,"AccentColor","#ff000000")
{
	QVBoxLayout *verticalLayout=new QVBoxLayout(this);
	setLayout(verticalLayout);
	verticalLayout->setContentsMargins(0,0,0,0);
	verticalLayout->setSpacing(0);

	output->setStyleSheet(StyleSheet::Colors<QLabel>(settingForegroundColor,settingBackgroundColor));
	output->setFont(QFont(settingFont,settingFontSize,QFont::Bold));
	output->setMargin(16);
	output->setAlignment(Qt::AlignCenter);
	output->setWordWrap(true);
	output->setTextFormat(Qt::RichText);

	clock.setSingleShot(true);
	connect(&clock,&QTimer::timeout,this,&AnnouncePane::Finished);
	connect(this,&AnnouncePane::Resized,this,&AnnouncePane::AdjustText,Qt::QueuedConnection);
}

AnnouncePane::AnnouncePane(const QString &text,QWidget *parent) : AnnouncePane(Lines{},parent)
{
	SingleLine(text);
}

void AnnouncePane::SingleLine(const QString &text)
{
	lines.emplace_back(Line{text,1});
}

bool AnnouncePane::event(QEvent *event)
{
	if (event->type() == QEvent::Polish) Polish();
	return QWidget::event(event);
}

void AnnouncePane::showEvent(QShowEvent *event)
{
	clock.setInterval(TimeConvert::Interval(static_cast<std::chrono::milliseconds>(settingDuration)));
	clock.start();
	QWidget::showEvent(event);
}

void AnnouncePane::hideEvent(QHideEvent *event)
{
	clock.stop();
	QWidget::hideEvent(event);
}

void AnnouncePane::resizeEvent(QResizeEvent *event)
{
	emit Resized(event->size().width());
	QWidget::resizeEvent(event);
}

void AnnouncePane::AdjustText(int width)
{
	output->setText(BuildParagraph(width));
}

void AnnouncePane::Polish()
{
	layout()->addWidget(output);
}

QString AnnouncePane::BuildParagraph(int width)
{
	QString paragraph;
	QFont font=output->font();
	for (const Line &line : lines)
	{
		font.setPointSizeF(output->font().pointSizeF()*line.second);
		int pointSize=line.first.contains(QChar{32}) ? font.pointSize() : StringConvert::RestrictFontWidth(font,line.first,width-output->margin()*2);
		if (line.second == 1 && font.pointSizeF() == output->font().pointSizeF())
			paragraph.append(QString("%1").arg(line.first));
		else
			paragraph.append(QString(R"(<span style="font-size: %2pt;">%1</span>)").arg(line.first,StringConvert::Integer(pointSize)));
		paragraph.append("<br>");
	}
	return QString(R"(<div style="line-height: 1.25;">%1</div>)").arg(paragraph);
}

ApplicationSetting& AnnouncePane::Font()
{
	return settingFont;
}

ApplicationSetting& AnnouncePane::FontSize()
{
	return settingFontSize;
}

ApplicationSetting& AnnouncePane::ForegroundColor()
{
	return settingForegroundColor;
}

ApplicationSetting& AnnouncePane::BackgroundColor()
{
	return settingBackgroundColor;
}

ApplicationSetting& AnnouncePane::AccentColor()
{
	return settingAccentColor;
}

ApplicationSetting& AnnouncePane::Duration()
{
	return settingDuration;
}

QString AnnouncePane::Subsystem()
{
	return u"announce pane"_s;
}

AudioAnnouncePane::AudioAnnouncePane(const Lines &lines,const QString &path,QWidget *parent) : AnnouncePane(lines,parent), audioPlayer(Multimedia::Player(this,1)), path(path)
{
	if (!QFile(path).exists()) throw std::runtime_error(QString{"Audio doesn't exist ("+path+")"}.toStdString());
	connect(audioPlayer,&QMediaPlayer::playbackStateChanged,this,[this](QMediaPlayer::PlaybackState state) {
		if (state == QMediaPlayer::StoppedState) emit Finished();
	});
	connect(audioPlayer,&QMediaPlayer::durationChanged,this,&AudioAnnouncePane::DurationAvailable);
	connect(audioPlayer,&QMediaPlayer::errorOccurred,this,[this](QMediaPlayer::Error error,const QString &errorString) {
		Q_UNUSED(error)
		emit Print(QString("Failed to play audio: %1").arg(errorString),"play audio",Subsystem());
		emit Finished();
	});
}

AudioAnnouncePane::AudioAnnouncePane(const QString &text,const QString &path,QWidget *parent) : AudioAnnouncePane(Lines{},path,parent)
{
	SingleLine(text);
}

void AudioAnnouncePane::showEvent(QShowEvent *event)
{
	connect(audioPlayer,&QMediaPlayer::mediaStatusChanged,this,[this,event](QMediaPlayer::MediaStatus status) {
		if (status == QMediaPlayer::LoadedMedia)
		{
			audioPlayer->play();
			QWidget::showEvent(event);
			return;
		}
		if (status == QMediaPlayer::InvalidMedia)
		{
			emit Print(QString("Failed to load audio: %1").arg(audioPlayer->errorString()),"load audio",Subsystem());
			emit Finished();
		}
		event->ignore();
	});
	audioPlayer->setSource(QUrl::fromLocalFile(path));
}

void AudioAnnouncePane::hideEvent(QHideEvent *event)
{
	audioPlayer->pause();
	QWidget::hideEvent(event);
}

QString AudioAnnouncePane::Subsystem()
{
	return u"audible announce pane"_s;
}

ImageAnnouncePane::ImageAnnouncePane(const Lines &lines,const QImage &image,QWidget *parent) : AnnouncePane(lines,parent), view(nullptr), stack(nullptr), shadow(nullptr), image(image), pixmap(nullptr)
{
	stack=new QStackedWidget(this);
	qobject_cast<QStackedLayout*>(stack->layout())->setStackingMode(QStackedLayout::StackAll);

	scene=new QGraphicsScene(this);
	view=new QGraphicsView(scene);
	view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	pixmap=new QGraphicsPixmapItem(); // the scene takes ownership of the pixmap pointer
	scene->addItem(pixmap);

	shadow=new QGraphicsDropShadowEffect();
	shadow->setBlurRadius(50); // TODO: abstract this out to a setting
	shadow->setOffset(0,0);
	output->setGraphicsEffect(shadow);
}

ImageAnnouncePane::ImageAnnouncePane(const QString &text,const QImage &image,QWidget *parent) : ImageAnnouncePane(Lines{},image,parent)
{
	SingleLine(text);
}

void ImageAnnouncePane::resizeEvent(QResizeEvent *event)
{
	if (!expired)
	{
		int coverSize=std::max(event->size().width(),event->size().height());
		if (!image.isNull()) pixmap->setPixmap(QPixmap::fromImage(QImage(image).scaled(QSize(coverSize,coverSize))));
	}
	AnnouncePane::resizeEvent(event);
}

void ImageAnnouncePane::Polish()
{
	shadow->setColor(settingAccentColor);
	stack->addWidget(output);
	stack->addWidget(view);
	layout()->addWidget(stack);
}

QString ImageAnnouncePane::Subsystem()
{
	return u"visual announcement pane"_s;
}

MultimediaAnnouncePane::MultimediaAnnouncePane(const QString &path,QWidget *parent) : AnnouncePane(Lines{},parent), imagePane(nullptr)
{
	audioPane=new AudioAnnouncePane(Lines{},path,this);
	connect(audioPane,&AudioAnnouncePane::Finished,this,&MultimediaAnnouncePane::Finished);
	connect(audioPane,&AudioAnnouncePane::Print,this,&MultimediaAnnouncePane::Print);
	connect(imagePane,&ImageAnnouncePane::Print,this,&MultimediaAnnouncePane::Print);
}

MultimediaAnnouncePane::MultimediaAnnouncePane(const Lines &lines,const QImage &image,const QString &path,QWidget *parent) : MultimediaAnnouncePane(path,parent)
{
	imagePane=new ImageAnnouncePane(lines,image,this);
}

MultimediaAnnouncePane::MultimediaAnnouncePane(const QString &text,const QImage &image,const QString &path,QWidget *parent) : MultimediaAnnouncePane(path,parent)
{
	imagePane=new ImageAnnouncePane(text,image,this);
}

void MultimediaAnnouncePane::Polish()
{
	layout()->addWidget(imagePane);
}

void MultimediaAnnouncePane::showEvent(QShowEvent *event)
{
	audioPane->show();
	imagePane->show();
	QWidget::showEvent(event); // don't call AnnouncePane's show event because that will make it do AnnouncePane things which will conflict with the AudioAnnouncePane's timing
}

void MultimediaAnnouncePane::hideEvent(QHideEvent *event)
{
	audioPane->hide();
	imagePane->hide();
	QWidget::hideEvent(event);  // don't call AnnouncePane's hide event because that will make it do AnnouncePane things which will conflict with the AudioAnnouncePane's timing
}

QString MultimediaAnnouncePane::Subsystem()
{
	return u"audible and visual announcement pane"_s;
}