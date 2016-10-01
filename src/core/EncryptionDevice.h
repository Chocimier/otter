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

#ifndef OTTER_ENCRYPTIONDEVICE_H
#define OTTER_ENCRYPTIONDEVICE_H

#include <QtCore/QIODevice>

#include <tomcrypt.h>

namespace Otter
{

class EncryptionDevice : public QIODevice
{
	Q_OBJECT

public:
	explicit EncryptionDevice(QIODevice *targetDevice, QObject *parent = 0);

	void close();
	bool isSequential() const;
	bool open(OpenMode mode);
	void setKey(const QByteArray &plainKey);

protected:
	qint64 readData(char *data, qint64 length);
	qint64 writeData(const char *data, qint64 length);
	bool writeBufferEncrypted();
	void initReading();
	void initWriting();
	bool applyPKCS(const unsigned char *salt);

private:
	QIODevice *m_device;
	QByteArray m_plainKey;
	symmetric_CTR m_ctr;
	unsigned char m_key[MAXBLOCKSIZE];
	unsigned char m_initializationVector[MAXBLOCKSIZE];
	unsigned char m_readingBuffer[MAXBLOCKSIZE];
	unsigned char m_writingBuffer[MAXBLOCKSIZE];
	qint64 m_readingBuffered;
	int m_blockSize;
	int m_cipherIndex;
	int m_hashIndex;
	int m_initializationVectorSize;
	int m_keySize;
	int m_writingBuffered;
	bool m_hasPlainKey;
	bool m_isValid;
	bool m_readAll;

	static char m_header[];
	static int m_PKCSIterationCount;
	static int m_PKCSSaltSize;
	static int m_PKCSResultSize;
	static int m_ctrMode;
};

}

#endif
