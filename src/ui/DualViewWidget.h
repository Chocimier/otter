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

#ifndef OTTER_DUALVIEWWIDGET_H
#define OTTER_DUALVIEWWIDGET_H

#include "ItemViewWidget.h"

#include <QtCore/QAbstractItemModel>
#include <QtWidgets/QWidget>

namespace Otter {

class DualViewWidget : public QWidget
{
	Q_OBJECT
public:
	enum ViewType {
		TreeViewType,
		SplitViewType,
		OneFolderViewType
	};

	explicit DualViewWidget(QWidget *parent = 0);

	QModelIndex currentIndex() const;
	QItemSelectionModel* selectionModel() const;
	bool eventFilter(QObject *object, QEvent *event);


public slots:
	void setDragDropMode(QAbstractItemView::DragDropMode behavior);
	void setExpanded(const QModelIndex &index, bool expanded);
	void setFilterRoles(const QSet<int> &roles);
	void setFilterString(const QString string);
	void setModel(QAbstractItemModel *model);
	void setViewType(ViewType type);

protected slots:
	void requestItemActivate(const QModelIndex &index);
	void showContextMenu(const QPoint &point);
	void treeIndexChanged();
	void updateWidgets();
	void widgetFocused(QWidget *oldWidget, QWidget *newWidget);

private:
	QSet<int> m_filterRoles;
	ItemViewWidget *m_currentView;
	ItemViewWidget *m_treeView;
	ItemViewWidget *m_listView;
	QAbstractItemModel *m_model;
	ViewType m_type;
	QAbstractItemView::DragDropMode m_dragDropMode;

signals:
	void needsActionsUpdate();
	void requestedItemActivate(const QModelIndex &index, QEvent *event);
	void requestedItemDelete(const QModelIndex &index);
	void customContextMenuRequested(const QModelIndex &index);
};

}

#endif
