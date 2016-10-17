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

#include "EncryptionDevice.h"

namespace Otter
{

char EncryptionDevice::m_header[]{'E', 'F', '1'};
int EncryptionDevice::m_PKCSIterationCount = 100;
int EncryptionDevice::m_PKCSSaltSize = 128;
int EncryptionDevice::m_PKCSResultSize = 256;
int EncryptionDevice::m_ctrMode = CTR_COUNTER_LITTLE_ENDIAN;

EncryptionDevice::EncryptionDevice(QIODevice *targetDevice, QObject *parent) : QIODevice(parent),
	m_device(targetDevice),
	m_readingBuffered(0),
	m_writingBuffered(0),
	m_hasPlainKey(false),
	m_isValid(false),
	m_readAll(false)
{
	register_cipher(&aes_desc);
	register_hash(&sha256_desc);

	m_cipherIndex = find_cipher("aes");
	m_hashIndex = find_hash("sha256");

	if (m_cipherIndex == -1  || m_hashIndex == -1 || register_prng(&fortuna_desc) == -1 || register_prng(&sprng_desc) == -1)
	{
		return;
	}

	m_keySize = hash_descriptor[m_hashIndex].hashsize;
	m_blockSize = cipher_descriptor[m_cipherIndex].block_length;
	m_initializationVectorSize = cipher_descriptor[m_cipherIndex].block_length;

	if (cipher_descriptor[m_cipherIndex].keysize(&m_keySize) != CRYPT_OK)
	{
		return;
	}

	m_isValid = true;
}

void EncryptionDevice::close()
{
	if (openMode().testFlag(QIODevice::WriteOnly))
	{
		writeBufferEncrypted();

		m_device->close();

		setOpenMode(m_device->openMode());
	}

	m_isValid = false;
}

bool EncryptionDevice::isSequential() const
{
	return true;
}

bool EncryptionDevice::open(QIODevice::OpenMode mode)
{
	if (isOpen())
	{
		return true;
	}

	if (!m_isValid || !m_hasPlainKey || mode.testFlag(QIODevice::ReadWrite) || mode.testFlag(QIODevice::Append) || mode.testFlag(QIODevice::Text) || !m_device->open(mode))
	{
		return false;
	}

	setOpenMode(m_device->openMode());

	if (mode.testFlag(QIODevice::ReadOnly))
	{
		initReading();
	}
	else if (mode.testFlag(QIODevice::WriteOnly))
	{
		initWriting();
	}

	if (!m_isValid)
	{
		m_device->close();

		setOpenMode(m_device->openMode());

		return false;
	}

	return true;
}

void EncryptionDevice::setKey(const QByteArray &plainKey)
{
	m_plainKey = plainKey;
	m_hasPlainKey = true;
}

qint64 EncryptionDevice::readData(char *data, qint64 length)
{
	unsigned char ciphertext[MAXBLOCKSIZE]{};
	qint64 position(0);

	if (!m_isValid || (m_readAll && m_readingBuffered == 0))
	{
		return -1;
	}

	if (m_readingBuffered)
	{
		const qint64 bytesToCopy(qMin(m_readingBuffered, length));

		memcpy(data, &m_readingBuffer[m_blockSize - m_readingBuffered], bytesToCopy);

		position += bytesToCopy;
		m_readingBuffered -= bytesToCopy;
	}

	while (position < length && !m_readAll)
	{
		const qint64 bytesRead(m_device->read(reinterpret_cast<char*>(ciphertext), m_blockSize));

		if (bytesRead == -1)
		{
			m_isValid = false;

			return -1;
		}

		if (ctr_decrypt(ciphertext, m_readingBuffer, bytesRead, &m_ctr) != CRYPT_OK)
		{
			m_isValid = false;

			return -1;
		}

		const qint64 bytesToCopy(qMin(bytesRead, (length - position)));

		if (bytesToCopy < bytesRead)
		{
			m_readingBuffered = bytesRead - bytesToCopy;
		}

		memcpy(&data[position], m_readingBuffer, bytesToCopy);

		position += bytesToCopy;

		if (bytesRead < m_blockSize)
		{
			m_readAll = true;
		}
	}

	return position;
}

qint64 EncryptionDevice::writeData(const char *data, qint64 length)
{
	if (!m_isValid)
	{
		return -1;
	}

	qint64 position(0);

	while (position < length)
	{
		const qint64 bytesToCopy(qMin(qint64(m_blockSize - m_writingBuffered), (length - position)));

		memcpy(&m_writingBuffer[m_writingBuffered], &data[position], bytesToCopy);

		m_writingBuffered += bytesToCopy;
		position += bytesToCopy;

		if (m_writingBuffered < m_blockSize)
		{
			break;
		}

		if (!writeBufferEncrypted())
		{
			return -1;
		}
	}

	return length;
}

bool EncryptionDevice::writeBufferEncrypted()
{
	unsigned char ciphertext[MAXBLOCKSIZE]{};

	if (ctr_encrypt(m_writingBuffer, ciphertext, m_writingBuffered, &m_ctr) != CRYPT_OK)
	{
		m_isValid = false;

		return false;
	}

	if (m_device->write(reinterpret_cast<const char*>(ciphertext), m_writingBuffered) != m_writingBuffered)
	{
		m_isValid = false;

		return false;
	}

	if (m_writingBuffered != m_blockSize)
	{
		m_isValid = false;
	}

	m_writingBuffered = 0;

	return true;
}

void EncryptionDevice::initReading()
{
	char header[sizeof m_header]{};
	unsigned char salt[m_PKCSSaltSize]{};

	if ((m_device->read(header, sizeof m_header) != sizeof m_header) || memcmp(m_header, header, sizeof m_header))
	{
		m_isValid = false;

		return;
	}

	if (m_device->read(reinterpret_cast<char*>(salt), sizeof salt) != sizeof salt)
	{
		m_isValid = false;

		return;
	}

	applyPKCS(salt);

	if (ctr_start(m_cipherIndex, m_initializationVector, m_key, m_keySize, 0, m_ctrMode, &m_ctr) != CRYPT_OK)
	{
		m_isValid = false;

		return;
	}
}

void EncryptionDevice::initWriting()
{
	unsigned char salt[m_PKCSSaltSize]{};
	prng_state prng;

	rng_make_prng(128, find_prng("fortuna"), &prng, NULL);

	const int randomBytesRead(fortuna_read(salt, sizeof salt, &prng));

	if (randomBytesRead != sizeof salt)
	{
		m_isValid = false;

		return;
	}

	if (!applyPKCS(salt))
	{
		m_isValid = false;

		return;
	}

	if (ctr_start(m_cipherIndex, m_initializationVector, m_key, m_keySize, 0, m_ctrMode, &m_ctr) != CRYPT_OK)
	{
		m_isValid = false;

		return;
	}

	if (m_device->write(m_header, sizeof m_header) != sizeof m_header)
	{
		m_isValid = false;

		return;
	}

	if (m_device->write(reinterpret_cast<const char*>(salt), sizeof salt) != sizeof salt)
	{
		m_isValid = false;

		return;
	}
}

bool EncryptionDevice::applyPKCS(const unsigned char *salt)
{
	unsigned char PKCSResult[m_PKCSResultSize]{};
	unsigned long outputLength(sizeof PKCSResult);

	if (pkcs_5_alg2(reinterpret_cast<const unsigned char*>(m_plainKey.constData()), m_plainKey.size(), salt, m_PKCSSaltSize, m_PKCSIterationCount, m_hashIndex, PKCSResult, &outputLength) != CRYPT_OK)
	{
		return false;
	}

	memcpy(m_key, PKCSResult, m_keySize);
	memcpy(m_initializationVector, (PKCSResult + m_keySize), m_initializationVectorSize);

	return true;

}

}
