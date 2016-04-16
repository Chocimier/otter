/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 - 2016 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2014 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "OperaStartPageImporter.h"
#include "../../../core/BookmarksManager.h"
#include "../../../core/Settings.h"
#include "../../../core/SettingsManager.h"

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include <QtWidgets/QFormLayout>

namespace Otter
{

OperaStartPageImporter::OperaStartPageImporter(QObject *parent): Importer(parent),
	m_removeCheckBox(NULL),
	m_backgroundCheckBox(NULL),
	m_optionsWidget(NULL)
{
}

OperaStartPageImporter::~OperaStartPageImporter()
{
	if (m_optionsWidget)
	{
		m_optionsWidget->deleteLater();
	}
}

QWidget* OperaStartPageImporter::getOptionsWidget()
{
	if (!m_optionsWidget)
	{
		m_optionsWidget = new QWidget();

		QFormLayout *layout(new QFormLayout(m_optionsWidget));

		m_optionsWidget->setLayout(layout);

		m_removeCheckBox = new QCheckBox(tr("Remove existing tiles"), m_optionsWidget);

		layout->addRow(m_removeCheckBox);

		m_backgroundCheckBox = new QCheckBox(tr("Import background image"), m_optionsWidget);
		m_backgroundCheckBox->setChecked(true);

		layout->addRow(m_backgroundCheckBox);
	}

	return m_optionsWidget;
}

QString OperaStartPageImporter::getTitle() const
{
	return QString(tr("Opera Speed Dial"));
}

QString OperaStartPageImporter::getDescription() const
{
	return QString(tr("Imports Speed Dial page from Opera Browser version 12 or earlier"));
}

QString OperaStartPageImporter::getVersion() const
{
	return QLatin1String("0.9");
}

QString OperaStartPageImporter::getFileFilter() const
{
	return tr("Opera speed dial file (speeddial.ini)");
}

QString OperaStartPageImporter::getSuggestedPath(const QString &path) const
{
	if (!path.isEmpty())
	{
		if (QFileInfo(path).isDir())
		{
			return QDir(path).filePath(QLatin1String("speeddial.ini"));
		}
		else
		{
			return path;
		}
	}
#if !defined(Q_OS_MAC) && defined(Q_OS_UNIX)
	const QString homePath(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).value(0));

	if (!homePath.isEmpty())
	{
		return QDir(homePath).filePath(QLatin1String(".opera/speeddial.ini"));
	}
#endif

	return path;
}

QString OperaStartPageImporter::getBrowser() const
{
	return QLatin1String("opera");
}

QUrl OperaStartPageImporter::getHomePage() const
{
	return QUrl(QLatin1String("http://otter-browser.org/"));
}

QIcon OperaStartPageImporter::getIcon() const
{
	return QIcon();
}

ImportType OperaStartPageImporter::getType() const
{
	return StartPageImport;
}

bool OperaStartPageImporter::import(const QString &path)
{
	const BookmarksModel *model(BookmarksManager::getModel());

	if (!model)
	{
		return false;
	}

	BookmarksItem *folder(model->getItem(SettingsManager::getValue(QLatin1String("StartPage/BookmarksFolder")).toString()));

	if (!folder)
	{
		return false;
	}

	Settings settings(getSuggestedPath(path), this);
	const QStringList groups(settings.getGroups());

//	qDebug() << settings.hasError();

	if (settings.hasError())
	{
		return false;
	}

	if (m_removeCheckBox && m_removeCheckBox->isChecked())
	{
		folder->removeRows(0, folder->rowCount());
	}

	settings.beginGroup(QLatin1String("Background"));

	if (m_backgroundCheckBox && m_backgroundCheckBox->isChecked() && settings.getValue(QLatin1String("Enabled")).toInt() == 1)
	{
		QString mode(settings.getValue(QLatin1String("Layout")).toString().toLower());

		if (mode == QLatin1String("crop"))
		{
			mode = QLatin1String("bestFit");
		}

		SettingsManager::setValue(QLatin1String("StartPage/BackgroundMode"), mode);
		SettingsManager::setValue(QLatin1String("StartPage/BackgroundPath"), settings.getValue(QLatin1String("Filename")));
	}

	for (int i = 1; groups.contains(QStringLiteral("Speed Dial %1").arg(i)); ++i)
	{
		settings.beginGroup(QStringLiteral("Speed Dial %1").arg(i));

		BookmarksManager::addBookmark(BookmarksModel::UrlBookmark, QUrl(settings.getValue(QLatin1String("Url")).toString()), settings.getValue(QLatin1String("Title")).toString(), folder);
	}

	return true;
}

}
