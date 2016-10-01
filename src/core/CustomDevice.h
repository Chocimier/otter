/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
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

#ifndef OTTER_CUSTOMDEVICE_H
#define OTTER_CUSTOMDEVICE_H

#include <QtCore/QIODevice>

namespace Otter
{

class CustomDevice : public QIODevice
{
	Q_OBJECT

public:
	enum class Feature : unsigned char
	{
		Encryption = 1
	};

	explicit CustomDevice(QIODevice *device, QObject *parent = 0);

	/**
	 * @param device Underlying device, where processed data is wrote or from which comes data to be read and processed.
	 * @param features When write to CustomDevice, data is first processed by first feature in the list.
	 * When read from CustomDevice, data is first processed by last feature in the list.
	 * It means that same list should be passed when both writing and reading.
	 */
	explicit CustomDevice(QIODevice *device, const QList<Feature> &features, QObject *parent = 0);

	void close();
	bool isSequential() const;
	bool open(OpenMode mode);

	QIODevice* getChainDevice(int index);

protected:
	qint64 readData(char *data, qint64 length);
	qint64 writeData(const char *data, qint64 length);

private:
	QIODevice *m_targetDevice;
	QList<QIODevice*> m_chainDevices;
};

}

#endif
