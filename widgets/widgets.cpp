#include <QScrollBar>
#include <QShowEvent>
#include <QEvent>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFileDialog>
#include <QFontDialog>
#include <QVideoWidget>
#include <QTableView>
#include <QHeaderView>
#include <QWindow>
#include <QScreen>
#include <QMessageBox>
#include <QInputDialog>
#include <QTextFrame>
#include <QColorSpace>
#include <algorithm>
#include "globals.h"
#include "widgets/widgets.h"

namespace StyleSheet
{
	const QString Colors(const QColor &foreground,const QColor &background)
	{
		return QString("color: rgba(%1,%2,%3,%4); background-color: rgba(%5,%6,%7,%8);").arg(
			StringConvert::Integer(foreground.red()),
			StringConvert::Integer(foreground.green()),
			StringConvert::Integer(foreground.blue()),
			StringConvert::Integer(foreground.alpha()),
			StringConvert::Integer(background.red()),
			StringConvert::Integer(background.green()),
			StringConvert::Integer(background.blue()),
			StringConvert::Integer(background.alpha())
		);
	}
}

namespace UI
{
	int ScreenWidthThird(QWidget *widget)
	{
		return widget->window()->windowHandle()->screen()->availableGeometry().width()/3;
	}

	std::optional<QString> OpenVideo(QWidget *parent,const QString &initialPath)
	{
		auto path=QFileDialog::getOpenFileName(parent,Text::DIALOG_TITLE_FILE,initialPath.isEmpty() ? Filesystem::HomePath().absolutePath() : initialPath,QString("Videos (*.%1)").arg(Text::FILE_TYPE_VIDEO));
		if (!QDir(path).exists()) return std::nullopt;
		return QDir::toNativeSeparators(path);
	}

	std::optional<QString> OpenAudio(QWidget *parent,const QString &initialPath)
	{
		auto path=QFileDialog::getOpenFileName(parent,Text::DIALOG_TITLE_FILE,initialPath.isEmpty() ? Filesystem::HomePath().absolutePath() : initialPath,QString("Audios (*.%1)").arg(Text::FILE_TYPE_AUDIO));
		if (!QDir(path).exists()) return std::nullopt;
		return QDir::toNativeSeparators(path);
	}

	std::optional<QString> OpenDirectory(QWidget *parent,const QString &initialPath)
	{
		auto path=QFileDialog::getExistingDirectory(parent,Text::DIALOG_TITLE_DIRECTORY,initialPath.isEmpty() ? Filesystem::DataPath().absolutePath() : initialPath);
		if (!QDir(path).exists()) return std::nullopt;
		return QDir::toNativeSeparators(path);
	}

	std::optional<QString> PickColor(QWidget *parent,const QString &initialColor)
	{
		auto color=QColorDialog::getColor(initialColor,parent,u"Choose a Color"_s,QColorDialog::ShowAlphaChannel);
		if (!color.isValid()) return std::nullopt;
		return color.name(QColor::HexArgb);
	}

	std::optional<std::tuple<QString,int>> PickFont(QWidget *parent,const QString &initialFamily,int initialPointSize)
	{
		bool ok=false;
		QFont candidate(initialFamily,initialPointSize);
		candidate=QFontDialog::getFont(&ok,candidate,parent,u"Choose Font"_s);
		if (!ok) return std::nullopt;
		return {{candidate.family(),candidate.pointSize()}};
	}

	namespace Feedback
	{
		Error::Error() : errors(0) { }

		void Error::SwapTrackingName(const QString &oldName,const QString &newName)
		{
			auto candidate=errors.extract(oldName);
			if (!candidate.empty())
			{
				candidate.value()=newName;
				errors.insert(std::move(candidate));
			}
			CompileErrorMessages();
		}

		void Error::StyleSource(bool valid,QWidget *widget)
		{
			if (valid)
				ValidStyle(widget);
			else
				InvalidStyle(widget);
		}

		void Error::ValidStyle(QWidget *widget)
		{
			widget->setStyleSheet("background-color: none;");
			errors.erase(widget->objectName());
			if (errors.size() == 0) emit Clear(true);
			CompileErrorMessages();
		}

		void Error::InvalidStyle(QWidget *widget)
		{
			auto backgroundColor=widget->palette().color(QPalette::Window);
			QColor invalidColor("LavenderBlush");
			auto CalculateLinearLuminance=[](const QColor &color)->float {
				return 0.2126f*color.redF()+0.7152f*color.greenF()+0.0722f*color.blueF();; // WCAG luminance coefficients
			};
			QColorSpace rgbSpace=QColorSpace(QColorSpace::SRgb);
			QColorSpace linearSpace(QColorSpace::SRgbLinear);
			QColorTransform toLinear=rgbSpace.transformationToColorSpace(linearSpace);
			QColorTransform toSRGB=linearSpace.transformationToColorSpace(rgbSpace);
			QColor invalidLinear=toLinear.map(invalidColor);
			float invalidLuminance=CalculateLinearLuminance(invalidLinear);
			QColor backgroundLinear=toLinear.map(backgroundColor);
			float backgroundLuminance=CalculateLinearLuminance(backgroundLinear);
			float scale=backgroundLuminance/invalidLuminance;
			widget->setStyleSheet(u"background-color: %0;"_s.arg(
				toSRGB.map(QColor::fromRgbF(
					std::clamp(invalidLinear.redF()*scale,0.0f,1.0f),
					std::clamp(invalidLinear.greenF()*scale,0.0f,1.0f),
					std::clamp(invalidLinear.blueF()*scale,0.0f,1.0f),
					invalidLinear.alphaF()
				)).name(QColor::NameFormat::HexArgb)
			));
			if (errors.insert(widget->objectName()).second) emit Clear(false);
			CompileErrorMessages();
		}

		void Error::CompileErrorMessages()
		{
			QString messages;

			for (const QString &error : errors) messages+=error+"\n";
			emit ReportProblem(messages.trimmed());
			emit Count(messages.size());
		}

		Help::Help(QWidget *parent) : QGroupBox(u"Help"_s,parent), text(this)
		{
			auto topLevelLayout=new QVBoxLayout(this);
			text.setEnabled(false);
			text.setSizePolicy(QSizePolicy(QSizePolicy::Preferred,QSizePolicy::MinimumExpanding));
			text.setStyleSheet(QStringLiteral("border: none; color: palette(window-text);"));
			auto textPalette=text.palette();
			textPalette.setColor(text.backgroundRole(),palette().color(backgroundRole()));
			text.setPalette(textPalette);
			text.setAutoFillBackground(true);
			topLevelLayout->addWidget(&text);

			connect(this,&UI::Feedback::Help::Message,&text,&QTextEdit::setText);
		}
	}

	ColorPreview::ColorPreview(QWidget *parent,const QString &color): QLabel(parent)
	{
		Set(color);
		setText(QStringLiteral("preview"));
	}

	void ColorPreview::Set(const QString &color)
	{
		setStyleSheet(QString("border: 1px solid black; color: %1; background-color: %1;").arg(color));
	}

	ColorEdit::ColorEdit(QWidget *parent): QLineEdit(parent)
	{
		connect(this,&QLineEdit::textChanged,this,&ColorEdit::Validate);
	}

	void ColorEdit::Validate(const QString &color)
	{
		bool valid=QColor(color).isValid();
		emit NeedValidatedStyle(valid,this);
		emit Valid(valid);
	}

	RequiredEdit::RequiredEdit(QWidget *parent): QLineEdit(parent)
	{
		connect(this,&QLineEdit::textChanged,this,&RequiredEdit::Validate);
	}

	void RequiredEdit::Validate(const QString &text)
	{
		bool valid=!text.isEmpty();
		emit NeedValidatedStyle(valid,this);
		emit Valid(valid);
	}

	PathEdit::PathEdit(const QStringList &acceptableFileExtensions,QWidget *parent,bool allowDirectories): QLineEdit(parent), acceptableFileExtensions(acceptableFileExtensions), allowDirectories(allowDirectories)
	{
		connect(this,&QLineEdit::textChanged,this,&PathEdit::Validate);
	}

	void PathEdit::Validate(const QString &path)
	{
		QFileInfo candidate(path);
		bool valid=candidate.exists() && (allowDirectories ? candidate.isDir() : false || acceptableFileExtensions.contains(candidate.suffix()));
		emit NeedValidatedStyle(valid,this);
		emit Valid(valid);
	}

	DirectoryEdit::DirectoryEdit(QWidget *parent): QLineEdit(parent)
	{
		connect(this,&QLineEdit::textChanged,this,&DirectoryEdit::Validate);
	}

	void DirectoryEdit::Validate(const QString &path)
	{
		bool valid=!path.isEmpty() && QDir(path).exists(); // QDir falls back to root on empty string
		emit NeedValidatedStyle(valid,this);
		emit Valid(valid);
	}

	FontEdit::FontEdit(QSpinBox *pointSize,QWidget *parent): QLineEdit(parent), pointSize(pointSize)
	{
		connect(this,&QLineEdit::textChanged,this,QOverload<const QString&>::of(&FontEdit::Validate));
		connect(pointSize,&QSpinBox::valueChanged,this,QOverload<int>::of(&FontEdit::Validate));
	}

	void FontEdit::Validate(const QString &family,int pointSize)
	{
		bool valid=false;
		if (QFontDatabase::families().contains(family,Qt::CaseInsensitive))
		{
			auto availableSizes=QFontDatabase::pointSizes(family);
			if (availableSizes.isEmpty() || availableSizes.contains(pointSize)) valid=true;// empty means scalable font so all point sizes valid
		}
		emit NeedValidatedStyle(valid,this);
		emit Valid(valid);
	}

	void FontEdit::Validate(const QString &family)
	{
		Validate(family,pointSize->value());
	}

	void FontEdit::Validate(int pointSize)
	{
		Validate(text(),pointSize);
	}

	WebAddressEdit::WebAddressEdit(QWidget *parent): QLineEdit(parent)
	{
		connect(this,&QLineEdit::textChanged,this,&WebAddressEdit::Validate);
	}

	void WebAddressEdit::Validate(const QString &address)
	{
		bool valid=QUrl(address,QUrl::StrictMode).isValid();
		emit NeedValidatedStyle(valid,this);
		emit Valid(valid);
	}

	namespace Security
	{
		Scopes::Scopes(QWidget *parent) : QDialog(parent),
			layout(this),
			list(this),
			scopes({"chat:read"})
		{
			setLayout(&layout);
			setSizeGripEnabled(true);

			list.setSelectionMode(QAbstractItemView::ExtendedSelection);
			list.addItems(::Security::SCOPES);
			layout.addWidget(&list);

			auto buttons=new QDialogButtonBox(this);
			auto okay=buttons->addButton(QDialogButtonBox::Ok);
			okay->setDefault(true);
			layout.addWidget(buttons);

			connect(buttons,&QDialogButtonBox::accepted,this,&QDialog::accept);
			connect(this,&QDialog::accepted,this,&Scopes::Save);
		}

		QStringList Scopes::operator()()
		{
			return scopes;
		}

		void Scopes::Save()
		{
			const auto selectedItems=list.selectedItems();
			for (QListWidgetItem *item : selectedItems) scopes.append(item->text());
		}
	}

	namespace Metrics
	{
		Dialog::Dialog(QWidget *parent) : QDialog(parent,Qt::Dialog|Qt::CustomizeWindowHint|Qt::WindowTitleHint|Qt::WindowCloseButtonHint),
			layout(this),
			users(this)
		{
			layout.addWidget(&users);
			setModal(false);
			setSizeGripEnabled(true);
		}

		void Dialog::Joined(const QString &user)
		{
			QListWidgetItem *item=new QListWidgetItem(user);
			users.addItem(item);
			item->setForeground(palette().mid());
			UpdateTitle();
		}

		void Dialog::Acknowledged(const QString &name)
		{
			QList<QListWidgetItem*> items=users.findItems(name,Qt::MatchExactly);
			if (items.isEmpty()) return;
			QListWidgetItem *item=items.at(0);
			item->setForeground(palette().text());
		}

		void Dialog::Parted(const QString &user)
		{
			const auto items=users.findItems(user,Qt::MatchExactly);
			for (QListWidgetItem *item : items) delete users.takeItem(users.row(item));
			UpdateTitle();
		}

		void Dialog::UpdateTitle()
		{
			setWindowTitle(QStringLiteral("Metrics (%1)").arg(StringConvert::Integer(users.count())));
		}
	}
}
