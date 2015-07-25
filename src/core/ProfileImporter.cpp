/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Piotr Wójcik <chocimier@tlen.pl>
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

#include "ProfileImporter.h"
#include "../modules/importers/opera/OperaBookmarksImporter.h"
#include "../modules/importers/opera/OperaNotesImporter.h"
#include "../ui/ProfileImporterWidget.h"

namespace Otter
{

ProfileImporter::ProfileImporter(const QString &set, QObject *parent) : Importer(parent),
	m_optionsWidget(NULL)
{
	if (set == QLatin1String("opera"))
	{
		if (m_importers.isEmpty())
		{
			m_importers.append(new OperaBookmarksImporter(this));
			m_importers.append(new OperaNotesImporter(this));
		}
	}
}

ProfileImporter::~ProfileImporter()
{
	if (m_optionsWidget)
	{
		m_optionsWidget->deleteLater();
	}
}

QList<Importer*> ProfileImporter::getImporters()
{
	return m_importers;
}

QWidget *ProfileImporter::getOptionsWidget()
{
	if (!m_optionsWidget)
	{
		m_optionsWidget = new ProfileImporterWidget(this);
	}

	return m_optionsWidget;
}

QString ProfileImporter::getTitle() const
{
	return QString();
}

QString ProfileImporter::getDescription() const
{
	return QString();
}

QString ProfileImporter::getVersion() const
{
	return QString();
}

QString ProfileImporter::getFileFilter() const
{
	return QString();
}

QString ProfileImporter::getSuggestedPath() const
{
	return QString();
}

QString ProfileImporter::getBrowser() const
{
	return QString();
}

ImportType ProfileImporter::getType() const
{
	return FullImport;
}

bool ProfileImporter::onlyDirectories() const
{
	return true;
}

bool ProfileImporter::setPath(const QString &path)
{
	bool result = false;

	for (int i = 0; i < m_importers.count(); ++i)
	{
		bool fileExist = m_importers[i]->setPath(path);

		if (fileExist)
		{
			result = true;
		}
		else
		{
			stateChanged(i, NoFileState);
		}
	}

	return result;
}

bool ProfileImporter::import()
{
	bool result = true;

	for (int i = 0; i < m_importers.count(); ++i)
	{
		if (!m_optionsWidget->selectedToImport(i))
		{
			continue;
		}

		stateChanged(i, WorkingState);

		bool success = m_importers[i]->import();

		if (success)
		{
			stateChanged(i, SuccessState);
		}
		else
		{
			result = false;

			stateChanged(i, ErrorState);
		}
	}

	return result;
}

}
