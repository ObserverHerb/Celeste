#pragma once

#include <QString>
#include <QWidget>
#include "globals.h"

namespace UI
{
	template <typename TWidget> concept WidgetHasViewport=Concept::Widget<TWidget> && requires (TWidget member)
	{
		{ member.viewport() }->std::same_as<QWidget*>;
	};

	template <typename TWidget> concept WidgetIsBool=Concept::Widget<TWidget> && requires(TWidget member,bool checked)
	{
		{ member.setChecked(checked) }->std::same_as<void>;
	};

	template <typename TWidget> concept WidgetIsText=Concept::Widget<TWidget> && requires(TWidget member,const QString &text)
	{
		requires !WidgetIsBool<TWidget>;
		{ member.setText(text) }->std::same_as<void>;
	};

	template <typename TWidget> concept WidgetIsRichText=Concept::Widget<TWidget> && requires(TWidget member,const QString &text)
	{
		requires !WidgetIsBool<TWidget>;
		{ member.setPlainText(text) }->std::same_as<void>;
	};

	template <typename TWidget> concept WidgetIsList=Concept::Widget<TWidget> && requires(TWidget member,int index)
	{
		{ member.setCurrentIndex(index) }->std::same_as<void>;
	};

	// lazy widget interface provides a class that always exists
	// while containing a widget that may or may not exist
	template <Concept::Widget TWidget>
	class LazyWidgetInterface
	{
	public:
		void Show()
		{
			if (widget) return;
			TWidget *candidate=new TWidget(parent);
			setupNeeded(candidate);
			widget=candidate;
			widget->setObjectName(name);
		}

		void Hide()
		{
			if (!widget) return;
			widget->disconnect();
			widget->deleteLater();
			widget=nullptr;
		}

		void Name(const QString &name)
		{
			this->name=name;
			if (widget) widget->setObjectName(name);
		}

		const QString& Name() const { return name; }
		TWidget* operator*() { return Widget(); }
		void Enable(bool enabled) { widget->setEnabled(enabled); }
		void Visible(bool visible) { widget->setVisible(visible); }
		std::optional<QWidget*> Viewport() requires WidgetHasViewport<TWidget> { return widget ? std::make_optional(widget->viewport()) : std::nullopt; }
		bool operator==(const QObject *other) const { return widget==other; }
	protected:
		QString name;
		QWidget *parent;
		TWidget *widget;
		std::function<void(TWidget*)> setupNeeded;
		LazyWidgetInterface(const QString &name,std::function<void(TWidget*)> setupNeeded,QWidget *parent) : name(name), parent(parent), widget(nullptr), setupNeeded(setupNeeded) { } // protected constructor to prevent instantiating base class outside of derived class

		TWidget* Widget()
		{
			if (!widget) Show();
			return widget;
		}
	};

	// lazy widgets hold the value their assigned QWidget holds,
	// but can track and provide that value whether the QWidget has
	// been instantiated yet or not, pairing stable, persistent data
	// with an unstable, ephemeral widget
	template <Concept::Widget TWidget>
	class LazyWidget : public LazyWidgetInterface<TWidget>
	{
	public:
		LazyWidget(const QString &name,std::function<void(TWidget*)> setupNeeded,QWidget *parent) : LazyWidgetInterface<TWidget>(name,setupNeeded,parent) { }
		void operator=(const QString &text) requires WidgetIsText<TWidget> { value=text; }
		void operator=(bool checked) requires WidgetIsBool<TWidget> { value=checked; }
		void operator=(int index) requires WidgetIsList<TWidget> { value=index; }
		operator QString() const requires WidgetIsText<TWidget> { return this->widget ? Text() : value; }
		operator bool() const requires WidgetIsBool<TWidget> { return this->widget ? this->widget->isChecked() : value; }
		operator int() const requires WidgetIsList<TWidget> { return this->widget ? this->widget->currentIndex() : value; }
		void RevertValue() requires WidgetIsText<TWidget> { if (this->widget) Text(value); }
		void RevertValue() requires WidgetIsBool<TWidget> { if (this->widget) this->widget->setChecked(value); }
		void RevertValue() requires WidgetIsList<TWidget> { if (this->widget) this->widget->setCurrentIndex(value); }
		void CacheValue() requires WidgetIsText<TWidget> { if (this->widget) value=Text(); }
		void CacheValue() requires WidgetIsBool<TWidget> { if (this->widget) value=this->widget->isChecked(); }
		void CacheValue() requires WidgetIsList<TWidget> { if (this->widget) value=this->widget->currentIndex(); }
		const QString& CachedValue() const requires WidgetIsText<TWidget> { return value; }
		bool CachedValue() const requires WidgetIsBool<TWidget> { return value; }
		int CachedValue() const requires WidgetIsList<TWidget> { return value; }

	protected:
		std::conditional<WidgetIsText<TWidget>,QString,typename std::conditional<WidgetIsList<TWidget>,int,bool>::type>::type value;
		QString Text() const requires (WidgetIsText<TWidget> && !WidgetIsRichText<TWidget>) { return this->widget->text(); }
		QString Text() const requires WidgetIsRichText<TWidget> { return this->widget->toPlainText(); }
		void Text(const QString &value) requires (WidgetIsText<TWidget> && !WidgetIsRichText<TWidget>) { if (this->widget) this->widget->setText(value); }
		void Text(const QString &value) requires WidgetIsRichText<TWidget> { if (this->widget) this->widget->setPlainText(value); }
	};

	// this version of lazy widget pairs data with a QWidget
	// that doesn't normally have its own data member, such as
	// a push button
	template <Concept::Widget TWidget,typename TValue>
	class GraftedLazyWidget : public LazyWidgetInterface<TWidget>
	{
	public:
		GraftedLazyWidget(const QString &name,std::function<void(TWidget*)> setupNeeded,QWidget *parent) : LazyWidgetInterface<TWidget>(name,setupNeeded,parent) { }
		void operator=(const TValue &value) { this->value=value; }
		TValue Value() const { return value; }

	protected:
		TValue value;
	};
}
