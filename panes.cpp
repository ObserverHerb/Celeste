#include "panes.h"

#include <QVBoxLayout>
#include <QStackedLayout>
#include <QLabel>

StatusPane::StatusPane(QWidget *parent) : Pane(parent)
{
	output=new QTextEdit(this);
	output->setStyleSheet("background-color: rgba(0,0,0,0); color: white;");
	output->setFontFamily("Droid Sans Mono");
	output->setFontPointSize(10);
	output->document()->setDocumentMargin(16);
	output->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	output->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	setLayout(new QVBoxLayout(this));
	layout()->setContentsMargins(0,0,0,0);
	layout()->addWidget(output);
}

void StatusPane::Print(const QString text)
{
	output->insertPlainText(text);
	output->insertPlainText("\n");
	output->moveCursor(QTextCursor::End);
}

const QString ChatPane::SETTINGS_CATEGORY="ChatPane";
ChatPane::ChatPane(QWidget *parent) : Pane(parent), agenda(nullptr), chat(nullptr), status(nullptr), settingStatusInterval(SETTINGS_CATEGORY,"StatusInterval",5000)
{
	setLayout(new QVBoxLayout(this));
	layout()->setContentsMargins(0,0,0,0);
	layout()->setSpacing(0);

	agenda=new QLabel(this);
	agenda->setStyleSheet("background-color: rgba(0,0,0,0); color: white;");
	agenda->setFont(QFont("Copperplate Gothic Bold",20,QFont::Bold));
	agenda->setMargin(16);
	agenda->setAlignment(Qt::AlignCenter);
	agenda->setWordWrap(true);
	layout()->addWidget(agenda);

	chat=new QTextEdit(this);
	chat->setStyleSheet("background-color: rgba(0,0,0,0); color: white;");
	chat->setFontFamily("Copperplate Gothic Bold");
	chat->setFontPointSize(16);
	chat->document()->setDefaultStyleSheet("div.user { font-family: 'Copperplate Gothic Bold'; font-size: 16pt; } div.message { font-family: 'Copperplate Gothic Bold'; font-size: 12pt; }");
	chat->document()->setDocumentMargin(16);
	chat->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	chat->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	chat->setFrameStyle(QFrame::NoFrame);
	chat->setCursorWidth(0);
	layout()->addWidget(chat);

	status=new QLabel(this);
	status->setStyleSheet("background-color: rgba(0,0,0,0); color: white;");
	status->setFont(QFont("Copperplate Gothic Bold",10));
	status->setMargin(16);
	status->setAlignment(Qt::AlignCenter);
	status->setWordWrap(true);
	status->setTextFormat(Qt::MarkdownText);
	layout()->addWidget(status);

	statusClock.setInterval(TimeConvert::Interval(static_cast<std::chrono::milliseconds>(settingStatusInterval)));
	connect(&statusClock,&QTimer::timeout,this,&ChatPane::DismissAlert);
}

void ChatPane::SetAgenda(const QString &text)
{
	agenda->setText(text);
}

void ChatPane::Print(const QString text)
{
	chat->insertHtml(text);
	chat->insertPlainText("\n");
	chat->moveCursor(QTextCursor::End);
}

Relay::Status::Context* ChatPane::Alert(const QString &text)
{
	Relay::Status::Context *statusUpdate=new Relay::Status::Context(text,this);
	connect(statusUpdate,&Relay::Status::Context::Updated,status,&QLabel::setText);
	statusUpdates.push(Relay::Status::Package(statusUpdate,&Relay::Status::Dismiss));
	if (statusUpdates.size() == 1)
	{
		statusClock.start();
		statusUpdate->Trigger();
	}
	return statusUpdate;
}

void ChatPane::DismissAlert()
{
	statusUpdates.pop();
	if (statusUpdates.empty())
	{
		status->clear();
		statusClock.stop();
		return;
	}
	statusUpdates.front()->Trigger();
}

EphemeralPane::EphemeralPane(QWidget *parent) : QWidget(parent)
{
	setVisible(false);
	connect(this,&EphemeralPane::Finished,this,&EphemeralPane::deleteLater);
}

VideoPane::VideoPane(const QString &path,QWidget *parent) : EphemeralPane(parent), viewport(new QVideoWidget(this)), videoPlayer(new QMediaPlayer(this))
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
AnnouncePane::AnnouncePane(const QString &text,QWidget *parent) : EphemeralPane(parent), output(new QLabel(text,this)), settingDuration(SETTINGS_CATEGORY,"Duration",5000)
{
	setLayout(new QVBoxLayout(this));
	layout()->setContentsMargins(0,0,0,0);
	layout()->setSpacing(0);

	output->setStyleSheet("background-color: rgba(0,0,0,0); color: white;");
	output->setFont(QFont("Copperplate Gothic Bold",20,QFont::Bold));
	output->setMargin(16);
	output->setAlignment(Qt::AlignCenter);
	output->setWordWrap(true);
	output->setTextFormat(Qt::MarkdownText);

	clock.setSingleShot(true);
	clock.setInterval(TimeConvert::Interval(static_cast<std::chrono::milliseconds>(settingDuration)));
	connect(&clock,&QTimer::timeout,this,&AnnouncePane::Finished);
}

void AnnouncePane::Show()
{
	Polish();
	show();
	clock.start();
}

void AnnouncePane::Polish()
{
	layout()->addWidget(output);
}

AudioAnnouncePane::AudioAnnouncePane(const QString &text,const QString &path,QWidget *parent) : AnnouncePane(text,parent), audioPlayer(new QMediaPlayer())
{
	audioPlayer->setMedia(QUrl::fromLocalFile(path));
	connect(audioPlayer,&QMediaPlayer::stateChanged,[this](QMediaPlayer::State state) {
		if (state == QMediaPlayer::StoppedState) emit Finished();
	}); // TODO: report loading failures to status section of chat pane
}

void AudioAnnouncePane::Show()
{
	Polish();
	show();
	audioPlayer->play(); // FIXME: catch errors above and don't play if the file failed to load
}

ImageAnnouncePane::ImageAnnouncePane(const QString &text,const QImage &image,QWidget *parent) : AnnouncePane(text,parent),  view(nullptr), stack(nullptr), shadow(nullptr), image(image)
{
	stack=new QStackedWidget(this);
	dynamic_cast<QStackedLayout*>(stack->layout())->setStackingMode(QStackedLayout::StackAll);

	scene=new QGraphicsScene(this);
	int coverSize=std::max(size().width(),size().height());
	scene->addPixmap(QPixmap::fromImage(QImage(image).scaled(QSize(coverSize,coverSize))));
	view=new QGraphicsView(scene);
	view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	shadow=new QGraphicsDropShadowEffect();
	shadow->setBlurRadius(10);
	shadow->setOffset(0,0);
	output->setGraphicsEffect(shadow);
}

void ImageAnnouncePane::Polish()
{
	shadow->setColor(accentColor);
	stack->addWidget(output);
	stack->addWidget(view);
	layout()->addWidget(stack);
}
