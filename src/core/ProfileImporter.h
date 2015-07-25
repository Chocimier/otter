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

#ifndef OTTER_PROFILEIMPORTER_H
#define OTTER_PROFILEIMPORTER_H

#include "Importer.h"

namespace Otter
{

class ProfileImporterWidget;

class ProfileImporter : public Importer
{
	Q_OBJECT

public:
	enum ImporterState
	{
		WaitingState,
		WorkingState,
		SuccessState,
		NoFileState,
		ErrorState
	};

	explicit ProfileImporter(const QString &set, QObject *parent = 0);
	~ProfileImporter();

	QWidget* getOptionsWidget();
	QList<Importer*> getImporters();
	QString getTitle() const;
	QString getDescription() const;
	QString getVersion() const;
	QString getFileFilter() const;
	QString getSuggestedPath() const;
	QString getBrowser() const;
	ImportType getType() const;
	bool onlyDirectories() const;

signals:
	void stateChanged(int index, ImporterState state);

public slots:
	bool import();
	bool setPath(const QString &path);

private:
	ProfileImporterWidget *m_optionsWidget;
	QList<Importer*> m_importers;
};

}

#endif
