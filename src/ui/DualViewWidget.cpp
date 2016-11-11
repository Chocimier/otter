/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2016 Piotr Wójcik <chocimier@tlen.pl>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#include "../core/Application.h"
#include "../core/Settings.h"
#include "DualViewWidget.h"

#include <QtCore/QMetaEnum>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QToolTip>

namespace Otter
{

DualViewWidget::DualViewWidget(QWidget *parent) : QWidget(parent),
	m_treeView(nullptr),
	m_listView(nullptr),
	m_model(nullptr),
	m_type(SplitViewType),
	m_dragDropMode(QAbstractItemView::NoDragDrop)
{
	setLayout(new QBoxLayout((QGuiApplication::isLeftToRight() ? QBoxLayout::LeftToRight : QBoxLayout::RightToLeft), this));

	connect(Application::getInstance(), SIGNAL(focusChanged(QWidget*,QWidget*)), this, SLOT(widgetFocused(QWidget*,QWidget*)));
	connect(this, SIGNAL(objectNameChanged(QString)), this, SLOT(loadState()));
	connect(this, SIGNAL(objectNameChanged(QString)), this, SLOT(updateWidgets()));
}

void DualViewWidget::setDragDropMode(QAbstractItemView::DragDropMode mode)
{
	m_dragDropMode = mode;

	updateWidgets();
}

void DualViewWidget::setExpanded(const QModelIndex &index, bool expanded)
{
	if (m_treeView)
	{
		m_treeView->setExpanded(index, expanded);
	}
}

void DualViewWidget::setFilterRoles(const QSet<int> &roles)
{
	m_filterRoles = roles;

	updateWidgets();
}

void DualViewWidget::setFilterString(const QString string)
{
	if (m_treeView)
	{
		m_treeView->setFilterString(string);

		if (m_type == SplitViewType)
		{
			m_treeView->setViewMode(ItemViewWidget::TreeViewMode);
			m_treeView->setViewFlags(ItemViewWidget::OnlyFoldersFlag);
		}
	}

	if (m_listView)
	{
		m_listView->setFilterString(string);
	}
}

void DualViewWidget::setModel(QAbstractItemModel *model)
{
	m_model = model;

	updateWidgets();
}

void DualViewWidget::setViewType(DualViewWidget::ViewType type)
{
	m_type = type;
	m_currentView = nullptr;

	if (m_treeView)
	{
		layout()->removeWidget(m_treeView);

		m_treeView->deleteLater();
		m_treeView = nullptr;
	}

	if (m_listView)
	{
		layout()->removeWidget(m_listView);

		m_listView->deleteLater();
		m_listView = nullptr;
	}

	switch (type)
	{
		case TreeViewType:
			m_treeView = new ItemViewWidget(this);
			m_treeView->setViewMode(ItemViewWidget::TreeViewMode);

			break;
		case SplitViewType:
			m_treeView = new ItemViewWidget(this);
			m_treeView->setViewMode(ItemViewWidget::TreeViewMode);
			m_treeView->setViewFlags(ItemViewWidget::OnlyFoldersFlag);

			m_listView = new ItemViewWidget(this);
			m_listView->setViewMode(ItemViewWidget::ListViewMode);
			m_listView->setViewFlags(ItemViewWidget::OneLevelFlag);

			connect(m_treeView, SIGNAL(needsActionsUpdate()), this, SLOT(treeIndexChanged()));

			break;
		case OneFolderViewType:
			m_listView = new ItemViewWidget(this);
			m_listView->setViewMode(ItemViewWidget::ListViewMode);
			m_listView->setViewFlags(ItemViewWidget::OneLevelFlag);

			break;
	}

	if (m_treeView)
	{
		m_treeView->installEventFilter(this);
		m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
		m_treeView->viewport()->installEventFilter(this);
		m_treeView->viewport()->setMouseTracking(true);

		layout()->addWidget(m_treeView);

		if (!m_listView)
		{
			m_currentView = m_treeView;
		}

		connect(m_treeView, SIGNAL(needsActionsUpdate()), this, SIGNAL(needsActionsUpdate()));
		connect(m_treeView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
		connect(m_treeView, SIGNAL(activated(QModelIndex)), this, SLOT(requestItemActivate(QModelIndex)));
	}

	if (m_listView)
	{
		m_listView->installEventFilter(this);
		m_listView->setContextMenuPolicy(Qt::CustomContextMenu);
		m_listView->setKeyboardNavigation(true);
		m_listView->viewport()->installEventFilter(this);
		m_listView->viewport()->setMouseTracking(true);

		layout()->addWidget(m_listView);

		m_currentView = m_listView;

		connect(m_listView, SIGNAL(needsActionsUpdate()), this, SIGNAL(needsActionsUpdate()));
		connect(m_listView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
		connect(m_listView, SIGNAL(activated(QModelIndex)), this, SLOT(requestItemActivate(QModelIndex)));
	}

	const QString suffix(QLatin1String("DualViewWidget"));
	const QString type(objectName().endsWith(suffix) ? objectName().left(objectName().size() - suffix.size()) : objectName());

	if (!type.isEmpty())
	{
		Settings settings(SessionsManager::getWritableDataPath(QLatin1String("views.ini")));
		settings.beginGroup(type);

		const QMetaEnum viewTypeEnum(metaObject()->enumerator(metaObject()->indexOfEnumerator(QLatin1String("ViewType").data())));

		settings.setValue(QLatin1String("viewType"), viewTypeEnum.valueToKey(m_type));
		settings.save();
	}

	updateWidgets();
}

void DualViewWidget::changeEvent(QEvent *event)
{
	if (event->type() == QEvent::LayoutDirectionChange)
	{
		QBoxLayout *boxLayout = dynamic_cast<QBoxLayout*>(layout());

		if (boxLayout)
		{
			boxLayout->setDirection(QGuiApplication::isLeftToRight() ? QBoxLayout::LeftToRight : QBoxLayout::RightToLeft);
		}
	}
}

void DualViewWidget::showContextMenu(const QPoint &point)
{
	ItemViewWidget *view(static_cast<ItemViewWidget*>(sender()));

	if (view)
	{
		emit customContextMenuRequested(view->indexAt(point));
	}
}

void DualViewWidget::requestItemActivate(const QModelIndex &index)
{
	emit requestedItemActivate(index, nullptr);
}

void DualViewWidget::treeIndexChanged()
{
	if (m_treeView && m_listView)
	{
		m_listView->displayFolder(m_treeView->currentIndex());
	}
}

void DualViewWidget::updateWidgets()
{
	const QString suffix(QLatin1String("DualViewWidget"));
	const QString prefix(objectName().endsWith(suffix) ? objectName().left(objectName().size() - suffix.size()) : objectName());

	if (m_treeView)
	{
		m_treeView->setObjectName(prefix + ((m_type == TreeViewType) ? QString() : QLatin1String("FolderTree")) + QLatin1String("ViewWidget"));
		m_treeView->setModel(m_model, true);
		m_treeView->setFilterRoles(m_filterRoles);
		m_treeView->setDragDropMode(m_dragDropMode);
	}

	if (m_listView)
	{
		m_listView->setObjectName(prefix + ((m_type == SplitViewType) ? QLatin1String("CurrentFolder") : QLatin1String("OneFolder")) + QLatin1String("ViewWidget"));
		m_listView->setModel(m_model, true);
		m_listView->setFilterRoles(m_filterRoles);
		m_listView->setDragDropMode(m_dragDropMode);
	}
}

void DualViewWidget::loadState()
{
	const QString suffix(QLatin1String("DualViewWidget"));
	const QString type(objectName().endsWith(suffix) ? objectName().left(objectName().size() - suffix.size()) : objectName());

	if (type.isEmpty())
	{
		return;
	}

	Settings settings(SessionsManager::getReadableDataPath(QLatin1String("views.ini")));
	settings.beginGroup(type);

	const QString viewTypeName(settings.getValue(QLatin1String("viewType")).toString());

	if (!viewTypeName.isEmpty())
	{
		const QMetaEnum viewTypeEnum(metaObject()->enumerator(metaObject()->indexOfEnumerator(QLatin1String("ViewType").data())));

		setViewType(static_cast<ViewType>(viewTypeEnum.keyToValue(viewTypeName.toLatin1().data())));
	}
}

void DualViewWidget::widgetFocused(QWidget *oldWidget, QWidget *newWidget)
{
	Q_UNUSED(oldWidget)

	if (newWidget == m_treeView || newWidget == m_listView)
	{
		ItemViewWidget *view(static_cast<ItemViewWidget*>(newWidget));

		m_currentView = view;
	}

	emit needsActionsUpdate();
}

QModelIndex DualViewWidget::currentIndex() const
{
	return (m_currentView ? m_currentView->currentIndex() : QModelIndex());
}

QItemSelectionModel* DualViewWidget::selectionModel() const
{
	return (m_currentView ? m_currentView->selectionModel() : nullptr);
}

DualViewWidget::ViewType DualViewWidget::viewType()
{
	return m_type;
}

bool DualViewWidget::eventFilter(QObject *object, QEvent *event)
{
	if (event->type() == QEvent::KeyPress && (object == m_treeView || object == m_listView))
	{
		QKeyEvent *keyEvent(static_cast<QKeyEvent*>(event));

		if (keyEvent && (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return))
		{
			emit requestedItemActivate(static_cast<ItemViewWidget*>(object)->currentIndex(), event);

			return true;
		}
		else if (keyEvent && keyEvent->key() == Qt::Key_Delete)
		{
			emit requestedItemDelete(static_cast<ItemViewWidget*>(object)->currentIndex());

			return true;
		}
	}
	else if ((event->type() == QEvent::MouseButtonRelease || event->type() == QEvent::MouseButtonDblClick) && ((m_treeView && object == m_treeView->viewport()) || (m_listView && object == m_listView->viewport())))
	{
		QMouseEvent *mouseEvent(static_cast<QMouseEvent*>(event));

		if (mouseEvent && (event->type() == QEvent::MouseButtonDblClick || (mouseEvent->button() == Qt::LeftButton && mouseEvent->modifiers() != Qt::NoModifier) || mouseEvent->button() == Qt::MiddleButton))
		{
			emit requestedItemActivate(static_cast<ItemViewWidget*>(object->parent())->indexAt(mouseEvent->pos()), event);

			return true;
		}
	}

	return QWidget::eventFilter(object, event);
}

}
