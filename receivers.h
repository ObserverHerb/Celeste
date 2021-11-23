#pragma once

#include <QObject>
#include <QTimer>
#include <QNetworkAccessManager>
#include <vector>
#include <unordered_set>
#include "command.h"
#include "emote.h"
#include "recognizers.h"

class MessageReceiver : public QObject
{
	Q_OBJECT
public:
	MessageReceiver(QObject *parent=nullptr) noexcept;
signals:
	void Succeeded();
	void Failed();
	void Print(const QString text);
public slots:
	virtual void Process(const QString data)=0;
};

class StatusReceiver : public MessageReceiver
{
	Q_OBJECT
public:
	StatusReceiver(QObject *parent=nullptr) noexcept : MessageReceiver(parent) { }
protected:
	void Interpret(const QString text);
};

class AuthenticationReceiver : public StatusReceiver
{
	Q_OBJECT
public:
	AuthenticationReceiver(QObject *parent=nullptr) noexcept : StatusReceiver(parent) { }
public slots:
	void Process(const QString data) override;
};


class ChannelJoinReceiver : public StatusReceiver
{
	Q_OBJECT
public:
	ChannelJoinReceiver(QObject *parent=nullptr) noexcept;
protected:
	QTimer failureDelay;
public slots:
	void Process(const QString data) override;
protected slots:
	void Fail();
};

class ChatMessageReceiver : public MessageReceiver // must catch std::runtime_error and std::out_of_range
{
	Q_OBJECT
	using TagMap=std::unordered_map<QString,QString>;
	using CommandMap=std::unordered_map<QString,Command>;
public:
	ChatMessageReceiver(QObject *parent=nullptr);
	void AttachCommand(const Command &command);
	const Command RandomCommand() const;
	const CommandMap& Commands() const { return commands; }
protected:
	CommandMap commands;
	std::unordered_map<QString,std::reference_wrapper<Command>> commandAliases;
	std::vector<Command> userCommands;
	std::unordered_set<QString> viewers;
	QNetworkAccessManager *downloadManager;
	void IdentifyViewer(const QString &name);
	std::optional<Command> FindCommand(const CommandCandidate &candidate);
	TagMap ParseTags(const QString &tags);
	QString ParseHostmask(const QString &mask);
	CommandCandidate ParseCommandCandidate(const QString &message);
	const QString DownloadEmote(const Emote &emote);
	void Fail(const QString &reason);
signals:
	void Refresh();
	void Alert(const QString &text);
	void ArrivalConfirmed(const Viewer viewer);
	void PlayVideo(const QString &path);
	void PlayAudio(const QString &user,const QString &message,const QString &path);
	void Speak(const QString sentence);
	void ShowVoices();
	void ForwardCommand(const Command command);
	void MessageProcessed();
public slots:
	void Process(const QString data) override;
};
