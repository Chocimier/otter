/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "WebWidget.h"
#include "ContentsWidget.h"
#include "ReloadTimeDialog.h"
#include "../core/ActionsManager.h"
#include "../core/SearchesManager.h"
#include "../core/SettingsManager.h"

#include <QtCore/QUrl>
#include <QtGui/QGuiApplication>
#include <QtGui/QIcon>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QMenu>

namespace Otter
{

WebWidget::WebWidget(bool isPrivate, WebBackend *backend, ContentsWidget *parent) : QWidget(parent),
	m_backend(backend),
	m_contextMenu(new QMenu(this)),
	m_reloadTimeMenu(NULL),
	m_quickSearchMenu(NULL),
	m_reloadTime(-1),
	m_reloadTimer(0)
{
	Q_UNUSED(isPrivate)

	m_contextMenu->installEventFilter(this);

	connect(SearchesManager::getInstance(), SIGNAL(searchEnginesModified()), this, SLOT(updateQuickSearch()));
}

void WebWidget::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_reloadTimer)
	{
		killTimer(m_reloadTimer);

		m_reloadTimer = 0;

		if (!isLoading())
		{
			triggerAction(Action::ReloadAction);
		}
	}
}

void WebWidget::startReloadTimer()
{
	if (m_reloadTime >= 0)
	{
		triggerAction(Action::StopScheduledReloadAction);

		if (m_reloadTime > 0)
		{
			m_reloadTimer = startTimer(m_reloadTime * 1000);
		}
	}
}

void WebWidget::search(const QString &query, const QString &engine)
{
	Q_UNUSED(query)
	Q_UNUSED(engine)
}

void WebWidget::reloadTimeMenuAboutToShow()
{
	switch (getReloadTime())
	{
		case 1800:
			m_reloadTimeMenu->actions().at(0)->setChecked(true);

			break;
		case 3600:
			m_reloadTimeMenu->actions().at(1)->setChecked(true);

			break;
		case 7200:
			m_reloadTimeMenu->actions().at(2)->setChecked(true);

			break;
		case 21600:
			m_reloadTimeMenu->actions().at(3)->setChecked(true);

			break;
		case 0:
			m_reloadTimeMenu->actions().at(4)->setChecked(true);

			break;
		case -1:
			m_reloadTimeMenu->actions().at(7)->setChecked(true);

			break;
		default:
			m_reloadTimeMenu->actions().at(5)->setChecked(true);

			break;
	}
}

void WebWidget::quickSearch(QAction *action, OpenHints hints)
{
	const QString engine = ((!action || action->data().type() != QVariant::String) ? getQuickSearchEngine() : action->data().toString());

	if (SearchesManager::getSearchEngines().contains(engine))
	{
		if (engine != m_quickSearchEngine)
		{
			m_quickSearchEngine = engine;

			if (m_quickSearchMenu)
			{
				m_quickSearchMenu->clear();
			}

			emit quickSearchEngineChanged();
		}

		if (hints != DefaultOpen)
		{
			emit requestedSearch(getSelectedText(), m_quickSearchEngine, hints);
		}
		else if (QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier))
		{
			emit requestedSearch(getSelectedText(), m_quickSearchEngine, NewTabBackgroundOpen);
		}
		else if (QGuiApplication::keyboardModifiers().testFlag(Qt::ShiftModifier) || !SettingsManager::getValue(QLatin1String("Browser/ReuseCurrentTab")).toBool())
		{
			emit requestedSearch(getSelectedText(), m_quickSearchEngine, NewTabOpen);
		}
		else
		{
			search(getSelectedText(), m_quickSearchEngine);
		}
	}
}

void WebWidget::quickSearchMenuAboutToShow()
{
	if (m_quickSearchMenu && m_quickSearchMenu->isEmpty())
	{
		const QStringList engines = SearchesManager::getSearchEngines();

		for (int i = 0; i < engines.count(); ++i)
		{
			SearchInformation *engine = SearchesManager::getSearchEngine(engines.at(i));

			if (engine)
			{
				QAction *action = m_quickSearchMenu->addAction(engine->icon, engine->title);
				action->setData(engine->identifier);
				action->setToolTip(engine->description);
			}
		}

		m_quickSearchMenu->installEventFilter(this);
	}
}

void WebWidget::clearOptions()
{
	m_options.clear();
}

void WebWidget::showContextMenu(const QPoint &position, MenuFlags flags)
{
	m_contextMenu->clear();

	if (flags & StandardMenu)
	{
		m_contextMenu->addAction(getAction(Action::GoBackAction));
		m_contextMenu->addAction(getAction(Action::GoForwardAction));
		m_contextMenu->addAction(getAction(Action::RewindAction));
		m_contextMenu->addAction(getAction(Action::FastForwardAction));
		m_contextMenu->addSeparator();
		m_contextMenu->addAction(getAction(Action::ReloadAction));
		m_contextMenu->addAction(getAction(Action::ScheduleReloadAction));
		m_contextMenu->addSeparator();
		m_contextMenu->addAction(getAction(Action::AddBookmarkAction));
		m_contextMenu->addAction(getAction(Action::CopyAddressAction));
		m_contextMenu->addAction(getAction(Action::PrintAction));
		m_contextMenu->addSeparator();

		if (flags & FormMenu)
		{
			m_contextMenu->addAction(getAction(Action::CreateSearchAction));
			m_contextMenu->addSeparator();
		}

		m_contextMenu->addAction(getAction(Action::InspectElementAction));
		m_contextMenu->addAction(getAction(Action::ViewSourceAction));
		m_contextMenu->addAction(getAction(Action::ValidateAction));
		m_contextMenu->addSeparator();

		if (flags & FrameMenu)
		{
			QMenu *frameMenu = new QMenu(m_contextMenu);
			frameMenu->setTitle(tr("Frame"));
			frameMenu->addAction(getAction(Action::OpenFrameInCurrentTabAction));
			frameMenu->addAction(getAction(Action::OpenFrameInNewTabAction));
			frameMenu->addAction(getAction(Action::OpenFrameInNewTabBackgroundAction));
			frameMenu->addSeparator();
			frameMenu->addAction(getAction(Action::ViewFrameSourceAction));
			frameMenu->addAction(getAction(Action::ReloadFrameAction));
			frameMenu->addAction(getAction(Action::CopyFrameLinkToClipboardAction));

			m_contextMenu->addMenu(frameMenu);
			m_contextMenu->addSeparator();
		}

		m_contextMenu->addAction(ActionsManager::getAction(Action::ContentBlockingAction, this));
		m_contextMenu->addAction(getAction(Action::WebsitePreferencesAction));
		m_contextMenu->addSeparator();
		m_contextMenu->addAction(ActionsManager::getAction(Action::FullScreenAction, this));
	}
	else
	{
		if (flags & EditMenu)
		{
			m_contextMenu->addAction(getAction(Action::UndoAction));
			m_contextMenu->addAction(getAction(Action::RedoAction));
			m_contextMenu->addSeparator();
			m_contextMenu->addAction(getAction(Action::CutAction));
			m_contextMenu->addAction(getAction(Action::CopyAction));
			m_contextMenu->addAction(getAction(Action::PasteAction));
			m_contextMenu->addAction(getAction(Action::DeleteAction));
			m_contextMenu->addSeparator();
			m_contextMenu->addAction(getAction(Action::SelectAllAction));
			m_contextMenu->addAction(getAction(Action::ClearAllAction));
			m_contextMenu->addSeparator();

			if (flags & FormMenu)
			{
				m_contextMenu->addAction(getAction(Action::CreateSearchAction));
				m_contextMenu->addSeparator();
			}

			if (flags == EditMenu || flags == (EditMenu | FormMenu))
			{
				m_contextMenu->addAction(getAction(Action::InspectElementAction));
				m_contextMenu->addSeparator();
			}

			m_contextMenu->addAction(getAction(Action::CheckSpellingAction));
			m_contextMenu->addSeparator();
		}

		if (flags & SelectionMenu)
		{
			m_contextMenu->addAction(getAction(Action::SearchAction));
			m_contextMenu->addAction(getAction(Action::SearchMenuAction));
			m_contextMenu->addSeparator();

			if (!(flags & EditMenu))
			{
				m_contextMenu->addAction(getAction(Action::CopyAction));
				m_contextMenu->addSeparator();
			}

			m_contextMenu->addAction(getAction(Action::OpenSelectionAsLinkAction));
			m_contextMenu->addSeparator();
		}

		if (flags & MailMenu)
		{
			m_contextMenu->addAction(getAction(Action::OpenLinkInCurrentTabAction));
			m_contextMenu->addSeparator();
			m_contextMenu->addAction(getAction(Action::CopyLinkToClipboardAction));

			if (!(flags & ImageMenu))
			{
				m_contextMenu->addAction(getAction(Action::InspectElementAction));
			}

			m_contextMenu->addSeparator();
		}
		else if (flags & LinkMenu)
		{
			m_contextMenu->addAction(getAction(Action::OpenLinkAction));
			m_contextMenu->addAction(getAction(Action::OpenLinkInNewTabAction));
			m_contextMenu->addAction(getAction(Action::OpenLinkInNewTabBackgroundAction));
			m_contextMenu->addSeparator();
			m_contextMenu->addAction(getAction(Action::OpenLinkInNewWindowAction));
			m_contextMenu->addAction(getAction(Action::OpenLinkInNewWindowBackgroundAction));
			m_contextMenu->addSeparator();
			m_contextMenu->addAction(getAction(Action::BookmarkLinkAction));
			m_contextMenu->addAction(getAction(Action::CopyLinkToClipboardAction));
			m_contextMenu->addSeparator();
			m_contextMenu->addAction(getAction(Action::SaveLinkToDiskAction));
			m_contextMenu->addAction(getAction(Action::SaveLinkToDownloadsAction));

			if (!(flags & ImageMenu))
			{
				m_contextMenu->addAction(getAction(Action::InspectElementAction));
			}

			m_contextMenu->addSeparator();
		}

		if (flags & ImageMenu)
		{
			m_contextMenu->addAction(getAction(Action::OpenImageInNewTabAction));
			m_contextMenu->addAction(getAction(Action::ReloadImageAction));
			m_contextMenu->addAction(getAction(Action::CopyImageUrlToClipboardAction));
			m_contextMenu->addSeparator();
			m_contextMenu->addAction(getAction(Action::SaveImageToDiskAction));
			m_contextMenu->addAction(getAction(Action::CopyImageToClipboardAction));
			m_contextMenu->addSeparator();
			m_contextMenu->addAction(getAction(Action::InspectElementAction));
			m_contextMenu->addAction(getAction(Action::ImagePropertiesAction));
			m_contextMenu->addSeparator();
		}

		if (flags & MediaMenu)
		{
			m_contextMenu->addAction(getAction(Action::CopyMediaUrlToClipboardAction));
			m_contextMenu->addAction(getAction(Action::SaveMediaToDiskAction));
			m_contextMenu->addSeparator();
			m_contextMenu->addAction(getAction(Action::ToggleMediaPlayPauseAction));
			m_contextMenu->addAction(getAction(Action::ToggleMediaMuteAction));
			m_contextMenu->addAction(getAction(Action::ToggleMediaLoopAction));
			m_contextMenu->addAction(getAction(Action::ToggleMediaControlsAction));
			m_contextMenu->addSeparator();
			m_contextMenu->addAction(getAction(Action::InspectElementAction));
			m_contextMenu->addSeparator();
		}
	}

	m_contextMenu->exec(mapToGlobal(position));
}

void WebWidget::updateQuickSearch()
{
	if (m_quickSearchMenu && sender() == SearchesManager::getInstance())
	{
		m_quickSearchMenu->clear();
	}

	if (!SearchesManager::getSearchEngines().contains(m_quickSearchEngine))
	{
		const QString engine = SettingsManager::getValue(QLatin1String("Search/DefaultSearchEngine")).toString();

		if (engine != m_quickSearchEngine)
		{
			m_quickSearchEngine = engine;

			emit quickSearchEngineChanged();
		}
	}
}

void WebWidget::setOption(const QString &key, const QVariant &value)
{
	if (value.isNull())
	{
		m_options.remove(key);
	}
	else
	{
		m_options[key] = value;
	}
}

void WebWidget::setOptions(const QVariantHash &options)
{
	m_options = options;
}

void WebWidget::setRequestedUrl(const QUrl &url, bool typed, bool onlyUpdate)
{
	m_requestedUrl = url;

	if (!onlyUpdate)
	{
		setUrl(url, typed);
	}
}

void WebWidget::setReloadTime(int time)
{
	if (time != m_reloadTime)
	{
		m_reloadTime = time;

		if (m_reloadTimer != 0)
		{
			killTimer(m_reloadTimer);

			m_reloadTimer = 0;
		}

		if (time >= 0)
		{
			triggerAction(Action::StopScheduledReloadAction);

			if (time > 0)
			{
				m_reloadTimer = startTimer(time * 1000);
			}
		}
	}
}

void WebWidget::setReloadTime(QAction *action)
{
	const int reloadTime = action->data().toInt();

	if (reloadTime == -2)
	{
		ReloadTimeDialog dialog(qMax(0, getReloadTime()), this);

		if (dialog.exec() == QDialog::Accepted)
		{
			setReloadTime(dialog.getReloadTime());
		}
	}
	else
	{
		setReloadTime(reloadTime);
	}
}

void WebWidget::setStatusMessage(const QString &message, bool override)
{
	const QString oldMessage = getStatusMessage();

	if (override)
	{
		m_overridingStatusMessage = message;
	}
	else
	{
		m_javaScriptStatusMessage = message;
	}

	const QString newMessage = getStatusMessage();

	if (newMessage != oldMessage)
	{
		emit statusMessageChanged(newMessage);
	}
}

void WebWidget::setQuickSearchEngine(const QString &engine)
{
	if (engine != m_quickSearchEngine)
	{
		m_quickSearchEngine = engine;

		updateQuickSearch();

		emit quickSearchEngineChanged();
	}
}

WebBackend* WebWidget::getBackend()
{
	return m_backend;
}

QMenu* WebWidget::getReloadTimeMenu()
{
	if (!m_reloadTimeMenu)
	{
		m_reloadTimeMenu = new QMenu(this);
		m_reloadTimeMenu->addAction(tr("30 Minutes"))->setData(1800);
		m_reloadTimeMenu->addAction(tr("1 Hour"))->setData(3600);
		m_reloadTimeMenu->addAction(tr("2 Hours"))->setData(7200);
		m_reloadTimeMenu->addAction(tr("6 Hours"))->setData(21600);
		m_reloadTimeMenu->addAction(tr("Never"))->setData(0);
		m_reloadTimeMenu->addAction(tr("Custom..."))->setData(-2);
		m_reloadTimeMenu->addSeparator();
		m_reloadTimeMenu->addAction(tr("Page Default"))->setData(-1);

		QActionGroup *actionGroup = new QActionGroup(m_reloadTimeMenu);
		actionGroup->setExclusive(true);

		for (int i = 0; i < m_reloadTimeMenu->actions().count(); ++i)
		{
			m_reloadTimeMenu->actions().at(i)->setCheckable(true);

			actionGroup->addAction(m_reloadTimeMenu->actions().at(i));
		}

		connect(m_reloadTimeMenu, SIGNAL(aboutToShow()), this, SLOT(reloadTimeMenuAboutToShow()));
		connect(m_reloadTimeMenu, SIGNAL(triggered(QAction*)), this, SLOT(setReloadTime(QAction*)));
	}

	return m_reloadTimeMenu;
}

QMenu* WebWidget::getQuickSearchMenu()
{
	if (!m_quickSearchMenu)
	{
		m_quickSearchMenu = new QMenu(this);

		connect(m_quickSearchMenu, SIGNAL(aboutToShow()), this, SLOT(quickSearchMenuAboutToShow()));
	}

	return m_quickSearchMenu;
}

QString WebWidget::getQuickSearchEngine() const
{
	return (m_quickSearchEngine.isEmpty() ? SettingsManager::getValue(QLatin1String("Search/DefaultSearchEngine")).toString() : m_quickSearchEngine);
}

QString WebWidget::getSelectedText() const
{
	return QString();
}

QString WebWidget::getStatusMessage() const
{
	return (m_overridingStatusMessage.isEmpty() ? m_javaScriptStatusMessage : m_overridingStatusMessage);
}

QVariant WebWidget::getOption(const QString &key, const QUrl &url) const
{
	if (m_options.contains(key))
	{
		return m_options[key];
	}

	return SettingsManager::getValue(key, (url.isEmpty() ? getUrl() : url));
}

QUrl WebWidget::getRequestedUrl() const
{
	return ((getUrl().isEmpty() || isLoading()) ? m_requestedUrl : getUrl());
}

QVariantHash WebWidget::getOptions() const
{
	return m_options;
}

int WebWidget::getReloadTime() const
{
	return m_reloadTime;
}

bool WebWidget::hasOption(const QString &key) const
{
	return m_options.contains(key);
}

bool WebWidget::eventFilter(QObject *object, QEvent *event)
{
	if (event->type() == QEvent::MouseButtonRelease && (object == m_quickSearchMenu || object == m_contextMenu))
	{
		QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

		if (mouseEvent && (mouseEvent->button() == Qt::LeftButton || mouseEvent->button() == Qt::MiddleButton))
		{
			QAction *action = (qobject_cast<QMenu*>(object))->actionAt(mouseEvent->pos());

			if (object == m_contextMenu && action != getAction(Action::SearchAction))
			{
				return QWidget::eventFilter(object, event);
			}

			if (mouseEvent->button() == Qt::MiddleButton || mouseEvent->modifiers() & Qt::ControlModifier)
			{
				quickSearch(action, NewTabBackgroundOpen);
			}
			else if (mouseEvent->modifiers() & Qt::ShiftModifier || !SettingsManager::getValue(QLatin1String("Browser/ReuseCurrentTab")).toBool())
			{
				quickSearch(action, NewTabOpen);
			}
			else
			{
				quickSearch(action);
			}

			m_contextMenu->close();

			return true;
		}
	}

	return QWidget::eventFilter(object, event);
}

}
