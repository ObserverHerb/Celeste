#include "panes.h"

#include <QVBoxLayout>
#include <QStackedLayout>
#include <QLabel>
#include <QResizeEvent>

const QString StatusPane::SETTINGS_CATEGORY="StatusPane";

StatusPane::StatusPane(QWidget *parent) : PersistentPane(parent),
	settingFont(SETTINGS_CATEGORY,"Font","Droid Sans Mono"),
	settingFontSize(SETTINGS_CATEGORY,"FontSize",10),
	settingForegroundColor(SETTINGS_CATEGORY,"ForegroundColor","#ffffffff"),
	settingBackgroundColor(SETTINGS_CATEGORY,"BackgroundColor","#ff000000")
{
	output=new QTextEdit(this);
	output->setStyleSheet(StyleSheet::Colors(settingForegroundColor,settingBackgroundColor));
	output->setFontFamily(settingFont);
	output->setFontPointSize(settingFontSize);
	output->document()->setDocumentMargin(16);
	output->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	output->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	output->setFrameStyle(QFrame::NoFrame);

	setLayout(new QVBoxLayout(this));
	layout()->setContentsMargins(0,0,0,0);
	layout()->addWidget(output);
}

void StatusPane::Print(const QString &text)
{
	output->insertPlainText(text);
	output->insertPlainText("\n");
	output->ensureCursorVisible();
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
	agenda->setStyleSheet(StyleSheet::Colors(settingForegroundColor,settingBackgroundColor));
	agenda->setFont(QFont(settingFont,static_cast<qreal>(settingFontSize)*1.666,QFont::Bold));
	agenda->setMargin(16);
	agenda->setAlignment(Qt::AlignCenter);
	agenda->setWordWrap(true);
	agenda->hide();
	layout()->addWidget(agenda);

	chat=new PinnedTextEdit(this);
	chat->setStyleSheet(StyleSheet::Colors(settingForegroundColor,settingBackgroundColor));
	chat->setFontFamily(settingFont);
	chat->setFontPointSize(settingFontSize);
	chat->document()->setDefaultStyleSheet(QString("div.user { font-family: '%1'; font-size: %2pt; } div.message, span.message { font-family: '%1'; font-size: %3pt; }").arg(static_cast<QString>(settingFont),StringConvert::Integer(static_cast<int>(settingFontSize)*1.333),StringConvert::Integer(static_cast<int>(settingFontSize))));
	chat->document()->setDocumentMargin(static_cast<qreal>(settingFontSize)*1.333);
	chat->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	chat->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	chat->setFrameStyle(QFrame::NoFrame);
	chat->setCursorWidth(0);
	layout()->addWidget(chat);

	status=new QLabel(this);
	status->setStyleSheet(StyleSheet::Colors(settingForegroundColor,settingBackgroundColor));
	status->setFont(QFont(settingFont,static_cast<qreal>(settingFontSize)*0.833)); // QLabel doesn't have setFontFamily()
	status->setMargin(16);
	status->setAlignment(Qt::AlignCenter);
	status->setWordWrap(true);
	status->setTextFormat(Qt::RichText);
	status->hide();
	layout()->addWidget(status);

	statusClock.setInterval(TimeConvert::Interval(static_cast<std::chrono::milliseconds>(settingStatusInterval)));
	connect(&statusClock,&QTimer::timeout,this,&ChatPane::DismissStatus);
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

void ChatPane::Refresh()
{
	chat->viewport()->update();
}

void ChatPane::Message(const QString &name,const QString &message,const std::vector<Media::Emote> &emotes,const QStringList &badgeIcons,const QColor color,bool action) const
{
	QString badges;
	for (const QString &icon : badgeIcons) badges.append(QString("<img style='vertical-align: middle;' src='%1' /> ").arg(icon));

	QString emotedMessage;
	unsigned int position=0;
	for (const Media::Emote &emote : emotes)
	{
		if (position < emote.StartPosition()) emotedMessage+=message.midRef(position,emote.StartPosition()-position);
		emotedMessage+=QString("<img style='vertical-align: middle;' src='%1' />").arg(emote.Path());
		position=emote.EndPosition()+1;
	}
	if (position < static_cast<unsigned int>(message.size())) emotedMessage+=message.midRef(position,message.size()-position);

	if (action)
		chat->Append(QString("<div>%4</div><div class='user' style='color: %3;'>%1 <span class='message'>%2</span><br></div>").arg(name,emotedMessage,color.isValid() ? color.name() : settingForegroundColor,badges));
	else
		chat->Append(QString("<div>%4</div><div class='user' style='color: %3;'>%1</div><div class='message'>%2<br></div>").arg(name,emotedMessage,color.isValid() ? color.name() : settingForegroundColor,badges));
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

EphemeralPane::EphemeralPane(QWidget *parent) : QWidget(parent), expired(false)
{
	setVisible(false);
	connect(this,&EphemeralPane::Finished,this,&EphemeralPane::Expire);
}

void EphemeralPane::Expire()
{
	if (expired) return;
	expired=true;
	emit Expired();
	deleteLater();
}

VideoPane::VideoPane(const QString &path,QWidget *parent) : EphemeralPane(parent), videoPlayer(new QMediaPlayer(this)), viewport(new QVideoWidget(this))
{
	videoPlayer->setVideoOutput(viewport);
	videoPlayer->setMedia(QUrl::fromLocalFile(path));
	connect(videoPlayer,&QMediaPlayer::stateChanged,[this](QMediaPlayer::State state) {
		if (state == QMediaPlayer::StoppedState) emit Finished();
	});

	setLayout(new QVBoxLayout(this));
	layout()->setContentsMargins(0,0,0,0);
	layout()->addWidget(viewport);
}

void VideoPane::Show()
{
	show();
	videoPlayer->play();
}

const QString AnnouncePane::SETTINGS_CATEGORY="AnnouncePane";

AnnouncePane::AnnouncePane(const QString &text,QWidget *parent) : EphemeralPane(parent),
	output(new QLabel(text,this)),
	settingDuration(SETTINGS_CATEGORY,"Duration",5000),
	settingFont(SETTINGS_CATEGORY,"Font","Copperplate Gothic Bold"),
	settingFontSize(SETTINGS_CATEGORY,"FontSize",20),
	settingForegroundColor(SETTINGS_CATEGORY,"ForegroundColor","#ffffffff"),
	settingBackgroundColor(SETTINGS_CATEGORY,"BackgroundColor","#ff000000")
{
	setLayout(new QVBoxLayout(this));
	layout()->setContentsMargins(0,0,0,0);
	layout()->setSpacing(0);

	output->setStyleSheet(StyleSheet::Colors(settingForegroundColor,settingBackgroundColor));
	output->setFont(QFont(settingFont,settingFontSize,QFont::Bold));
	output->setMargin(16);
	output->setAlignment(Qt::AlignCenter);
	output->setWordWrap(true);
	output->setTextFormat(Qt::RichText);

	clock.setSingleShot(true);
	clock.setInterval(TimeConvert::Interval(static_cast<std::chrono::milliseconds>(settingDuration)));
	connect(&clock,&QTimer::timeout,this,&AnnouncePane::Finished);
}

AnnouncePane::AnnouncePane(const std::vector<std::pair<QString,double>> &lines,QWidget *parent) : AnnouncePane("",parent)
{
	output->setText(BuildParagraph(lines));
}

bool AnnouncePane::event(QEvent *event)
{
	if (event->type() == QEvent::Polish) Polish();
	return QWidget::event(event);
}

void AnnouncePane::Show()
{
	show();
	clock.start();
}

void AnnouncePane::Polish()
{
	layout()->addWidget(output);
}

const QString AnnouncePane::BuildParagraph(const std::vector<std::pair<QString,double>> &lines)
{
	QString paragraph;
	for (const std::pair<QString,double> &line : lines)
	{
		if (line.second == 1)
			paragraph.append(QString("%1").arg(line.first));
		else
			paragraph.append(QString(R"(<span style="font-size: %2pt;">%1</span>)").arg(line.first,StringConvert::Integer(static_cast<int>(settingFontSize)*line.second)));
		paragraph.append("<br>");
	}
	return QString(R"(<div style="line-height: 1.25;">%1</div>)").arg(paragraph);
}

ScrollingAnnouncePane::ScrollingAnnouncePane(const QString &text,QWidget *parent) : AnnouncePane(text,parent), commands(new ScrollingTextEdit(this))
{
	commands->setStyleSheet(StyleSheet::Colors(settingForegroundColor,settingBackgroundColor));
	commands->setFontFamily(settingFont);
	commands->setFontPointSize(settingFontSize);
	commands->document()->setDefaultStyleSheet(QString("div.user { font-family: '%1'; font-size: %2pt; } div.message, span.message { font-family: '%1'; font-size: %3pt; }").arg(static_cast<QString>(settingFont),StringConvert::Integer(static_cast<int>(settingFontSize)*1.333),StringConvert::Integer(static_cast<int>(settingFontSize))));
	commands->document()->setDocumentMargin(static_cast<qreal>(settingFontSize)*1.333);
	commands->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	commands->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	commands->setFrameStyle(QFrame::NoFrame);
	commands->setCursorWidth(0);

	connect(commands,&ScrollingTextEdit::Finished,this,&ScrollingAnnouncePane::Finished);
}

ScrollingAnnouncePane::ScrollingAnnouncePane(const Lines &lines,QWidget *parent) : ScrollingAnnouncePane("",parent)
{
	commands->setText(BuildParagraph(lines));
}

void ScrollingAnnouncePane::Show()
{
	show();
}

void ScrollingAnnouncePane::Polish()
{
	layout()->addWidget(commands);
}

AudioAnnouncePane::AudioAnnouncePane(const QString &text,const QString &path,QWidget *parent) : AnnouncePane(text,parent), audioPlayer(new QMediaPlayer(this))
{
	connect(audioPlayer,&QMediaPlayer::stateChanged,[this](QMediaPlayer::State state) {
		if (state == QMediaPlayer::StoppedState) emit Finished();
	});
	connect(audioPlayer,&QMediaPlayer::durationChanged,this,&AudioAnnouncePane::DurationAvailable);
	connect(audioPlayer,QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error),this,[this]() {
		emit Print(QString("Failed to load audio: %1").arg(audioPlayer->errorString()));
		emit Finished();
	});
	audioPlayer->setMedia(QUrl::fromLocalFile(path));
	output->setText(text);
}

AudioAnnouncePane::AudioAnnouncePane(const std::vector<std::pair<QString,double>> &lines,const QString &path,QWidget *parent) : AudioAnnouncePane("",path,parent)
{
	output->setText(BuildParagraph(lines));
}

void AudioAnnouncePane::Show()
{
	show();
	audioPlayer->play();
}

ImageAnnouncePane::ImageAnnouncePane(const QString &text,const QImage &image,QWidget *parent) : AnnouncePane(text,parent), view(nullptr), stack(nullptr), shadow(nullptr), image(image), pixmap(nullptr)
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

ImageAnnouncePane::ImageAnnouncePane(const std::vector<std::pair<QString,double>> &lines,const QImage &image,QWidget *parent) : ImageAnnouncePane("",image,parent)
{
	output->setText(BuildParagraph(lines));
}

void ImageAnnouncePane::resizeEvent(QResizeEvent *event)
{
	if (!expired)
	{
		int coverSize=std::max(event->size().width(),event->size().height());
		if (!image.isNull()) pixmap->setPixmap(QPixmap::fromImage(QImage(image).scaled(QSize(coverSize,coverSize))));
	}
}

void ImageAnnouncePane::Polish()
{
	shadow->setColor(accentColor);
	stack->addWidget(output);
	stack->addWidget(view);
	layout()->addWidget(stack);
}

MultimediaAnnouncePane::MultimediaAnnouncePane(const QString &path,QWidget *parent) : AnnouncePane("",parent), imagePane(nullptr)
{
	audioPane=new AudioAnnouncePane("",path,this);
	connect(audioPane,&AudioAnnouncePane::Finished,this,&MultimediaAnnouncePane::Finished);
}

MultimediaAnnouncePane::MultimediaAnnouncePane(const QString &text,const QImage &image,const QString &path,QWidget *parent) : MultimediaAnnouncePane(path,parent)
{
	imagePane=new ImageAnnouncePane(text,image,this);
}

MultimediaAnnouncePane::MultimediaAnnouncePane(const std::vector<std::pair<QString,double>> &lines,const QImage &image,const QString &path,QWidget *parent) : MultimediaAnnouncePane(path,parent)
{
	imagePane=new ImageAnnouncePane(lines,image,this);
}

void MultimediaAnnouncePane::Polish()
{
	layout()->addWidget(imagePane);
}

void MultimediaAnnouncePane::Show()
{
	imagePane->Show();
	audioPane->Show();
	show();
}
