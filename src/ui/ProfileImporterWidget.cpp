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

#include "ProfileImporterWidget.h"
#include "../core/Utils.h"

#include "ui_ProfileImporterWidget.h"

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QToolButton>

namespace Otter
{

ProfileImporterWidget::ProfileImporterWidget(ProfileImporter *importer, QWidget *parent) : QWidget(parent),
	m_importer(importer),
	m_ui(new Ui::ProfileImporterWidget)
{
	m_ui->setupUi(this);
	m_ui->gridLayout->setColumnStretch(1, 1);

	QList<Importer*> parts = m_importer->getImporters();

	for(int i = 0; i < parts.count(); ++i)
	{
		m_ui->gridLayout->addWidget(new QLabel(this), i, 0);
		m_ui->gridLayout->addWidget(new QLabel(parts[i]->getTitle(), this), i, 1);

		QToolButton *optionsButton = new QToolButton(this);
		QAction *optionsAction = new QAction(Utils::getIcon(QLatin1String("configure")), QString(), optionsButton);

		optionsButton->setDefaultAction(optionsAction);
		optionsAction->setData(i);

		m_ui->gridLayout->addWidget(optionsButton, i, 2);

		QCheckBox *importCheckBox = new QCheckBox(tr("Import"), this);
		importCheckBox->setChecked(true);

		m_ui->gridLayout->addWidget(importCheckBox, i, 3);

		stateChanged(i, ProfileImporter::WaitingState);

		connect(optionsAction, SIGNAL(triggered(bool)), this, SLOT(showOptionsDialog()));
	}
}

ProfileImporterWidget::~ProfileImporterWidget()
{
	delete m_ui;
}

bool ProfileImporterWidget::selectedToImport(int index)
{
	QLayoutItem *item = m_ui->gridLayout->itemAtPosition(index, 3);

	if (!item)
	{
		return false;
	}

	QCheckBox *checkBox = qobject_cast<QCheckBox*>(item->widget());

	if (!checkBox)
	{
		return false;
	}

	return checkBox->checkState();
}

void ProfileImporterWidget::changeEvent(QEvent *e)
{
	QWidget::changeEvent(e);

	switch (e->type())
	{
		case QEvent::LanguageChange:
			m_ui->retranslateUi(this);

			break;
		default:
			break;
	}
}

void ProfileImporterWidget::stateChanged(int index, ProfileImporter::ImporterState state)
{
	QIcon icon;

	switch (state)
	{
		case ProfileImporter::WaitingState:
			icon = Utils::getIcon(QLatin1String("configure"));
			break;

		case ProfileImporter::WorkingState:
			icon = Utils::getIcon(QLatin1String("task-ongoing"));
			break;

		case ProfileImporter::SuccessState:
			icon = Utils::getIcon(QLatin1String("task-complete"));
			break;

		case ProfileImporter::NoFileState:
			icon = Utils::getIcon(QLatin1String("dialog-warning"));
			break;

		case ProfileImporter::ErrorState:
			icon = Utils::getIcon(QLatin1String("task-reject"));
			break;
	}

	QLayoutItem *item = m_ui->gridLayout->itemAtPosition(index, 0);

	if (!item)
	{
		return;
	}

	QLabel *label = qobject_cast<QLabel*>(item->widget());

	if (!label)
	{
		return;
	}

	label->setPixmap(icon.pixmap(20, 20));
}

void ProfileImporterWidget::showOptionsDialog()
{
	QAction *action = qobject_cast<QAction*>(sender());

	if (action)
	{
		QDialog *dialog = new QDialog(this);
		dialog->setLayout(new QVBoxLayout(dialog));
		dialog->layout()->addWidget(m_importer->getImporters()[action->data().toInt()]->getOptionsWidget());

		QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok, dialog);
		connect(box, SIGNAL(accepted()), dialog, SLOT(accept()));

		dialog->layout()->addWidget(box);
		dialog->setModal(true);
		dialog->exec();
	}
}

}
