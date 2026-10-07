/*
    SPDX-FileCopyrightText: 2026 KPatience Android port contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kgamesound.h"

// Qt
#include <QUrl>
#ifdef KPAT_HAVE_QTMULTIMEDIA
#include <QAudioOutput>
#include <QMediaPlayer>
#endif

KGameSound::KGameSound(const QString &file, QObject *parent)
    : QObject(parent)
{
#ifdef KPAT_HAVE_QTMULTIMEDIA
    if (file.isEmpty()) {
        return;
    }
    m_output = new QAudioOutput(this);
    m_player = new QMediaPlayer(this);
    m_player->setAudioOutput(m_output);
    m_player->setSource(file.startsWith(QLatin1Char(':')) ? QUrl(QLatin1String("qrc") + file) : QUrl::fromLocalFile(file));
#else
    Q_UNUSED(file)
#endif
}

KGameSound::~KGameSound() = default;

bool KGameSound::isValid() const
{
#ifdef KPAT_HAVE_QTMULTIMEDIA
    return m_player && m_player->error() == QMediaPlayer::NoError;
#else
    return false;
#endif
}

void KGameSound::start()
{
#ifdef KPAT_HAVE_QTMULTIMEDIA
    if (!m_player) {
        return;
    }
    // restart from the beginning if the previous playback is still running
    m_player->setPosition(0);
    m_player->play();
#endif
}

void KGameSound::stop()
{
#ifdef KPAT_HAVE_QTMULTIMEDIA
    if (m_player) {
        m_player->stop();
    }
#endif
}

#include "moc_kgamesound.cpp"
