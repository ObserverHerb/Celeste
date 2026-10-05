#include "widgets/widgets.h"

QTableWidgetItem* ReadOnlyItem(const QString &text)
{
	QTableWidgetItem *item=new QTableWidgetItem(text);
	item->setFlags(item->flags() & ~Qt::ItemIsEditable);
	return item;
}

namespace UI
{
	namespace EventSubscriptions
	{
		const char *HEADER_ID="ID";

		const int Dialog::COLUMN_COUNT=4;

		Dialog::Dialog(QWidget *parent) : QDialog(parent),
			layout(this),
			list(0,COLUMN_COUNT,this),
			buttons(this),
			remove(Text::BUTTON_REMOVE,this),
			close(Text::BUTTON_CLOSE,this)
		{
			setLayout(&layout);

			list.setHorizontalHeaderLabels({u"Type"_s,u"Creation Date"_s,u"CallbackURL"_s,HEADER_ID});
			list.setSelectionBehavior(QAbstractItemView::SelectRows);
			list.setSelectionMode(QAbstractItemView::SingleSelection);
			list.setSortingEnabled(true);
			layout.addWidget(&list);

			buttons.addButton(&close,QDialogButtonBox::AcceptRole);
			buttons.addButton(&remove,QDialogButtonBox::ActionRole);
			connect(&buttons,&QDialogButtonBox::accepted,this,&QDialog::accept);
			connect(&buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
			connect(&remove,&QPushButton::clicked,this,&Dialog::Remove);
			layout.addWidget(&buttons);

			remove.setEnabled(false);

			setSizeGripEnabled(true);
		}

		void Dialog::showEvent(QShowEvent *event)
		{
			setMinimumWidth(ScreenWidthThird(this));
			emit RequestSubscriptionList();
			QDialog::showEvent(event);
		}

		void Dialog::Add(const QString &id,const QString &type,const QDateTime &creationDate,const QString &callbackURL)
		{
			list.setSortingEnabled(false);
			list.insertRow(list.rowCount());
			int row=list.rowCount()-1;
			list.setItem(row,0,ReadOnlyItem(type));
			list.setItem(row,1,ReadOnlyItem(creationDate.toString(Qt::RFC2822Date)));
			list.setItem(row,2,ReadOnlyItem(callbackURL));
			list.setItem(row,3,ReadOnlyItem(id));
			list.setSortingEnabled(true);
			list.resizeColumnsToContents();
			remove.setEnabled(true);
		}

		void Dialog::Remove()
		{
			// we get all of the items for the selected row
			// look for the item that has the correct column header (ID)
			// pass that item's text to EventSub as the one to be removed
			QList<QTableWidgetItem*> items=list.selectedItems();
			for (auto candidate=items.begin(); candidate != items.end(); candidate++)
			{
				if (list.horizontalHeaderItem((*candidate)->column())->text() == HEADER_ID) emit RemoveSubscription((*candidate)->text());
			}
			int rowCount=list.rowCount();
			if (rowCount < 1) remove.setEnabled(false);
		}

		void Dialog::Removed(const QString &id)
		{
			QList<QTableWidgetItem*> items=list.findItems(id,Qt::MatchFixedString|Qt::MatchCaseSensitive);
			for (auto candidate=items.begin(); candidate != items.end(); candidate++)
			{
				list.removeRow(list.row(*candidate));
			}
		}
	}

}