#pragma once

#include <QString>
#include <unordered_map>

enum class CommandType
{
	DISPATCH,
	VIDEO
};
using CommandTypeLookup=std::unordered_map<QString,CommandType>;
const CommandTypeLookup COMMAND_TYPES={
	{"dispatch",CommandType::DISPATCH},
	{"video",CommandType::VIDEO}
};

class Command
{
public:
	Command() : Command(QString(),QString(),CommandType::DISPATCH,false,QString()) { }
	Command(const QString &name,const QString &description,const CommandType &type,bool protect=false) : Command(name,description,type,false,QString(),protect) { }
	Command(const QString &name,const QString &description,const CommandType &type,bool random,const QString &path,bool protect=false) : name(name), description(description), type(type), random(random), path(path), protect(protect) { }
	const QString& Name() const { return name; }
	const QString& Description() const { return description; }
	CommandType Type() const { return type; }
	bool Random() const { return random; }
	bool Protect() const { return protect; }
	const QString& Path() { return path; }
protected:
	QString name;
	QString description;
	CommandType type;
	bool random;
	bool protect;
	QString path;
};

const QString JSON_KEY_COMMAND_NAME("command");
const QString JSON_KEY_COMMAND_DESCRIPTION("description");
const QString JSON_KEY_COMMAND_TYPE("type");
const QString JSON_KEY_COMMAND_RANDOM_PATH("random");
const QString JSON_KEY_COMMAND_PATH("path");
