/*
    SPDX-FileCopyrightText: 2026 KPatience Android port contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KGAMESOUND_H
#define KGAMESOUND_H

// Qt
#include <QObject>

class QAudioOutput;
class QMediaPlayer;

/**
 * Minimal stand-in for libkdegames' KGameSound, used where libkdegames (and
 * its OpenAL/libsndfile audio backend) is not available, i.e. on Android.
 * Playback goes through Qt Multimedia; if KPat is built without it, the
 * sound is silently ignored.
 */
class KGameSound : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(KGameSound)

public:
    /// @a file may be a local file path or a Qt resource path (":/...")
    explicit KGameSound(const QString &file, QObject *parent = nullptr);
    ~KGameSound() override;

    bool isValid() const;

public Q_SLOTS:
    void start();
    void stop();

private:
    QMediaPlayer *m_player = nullptr;
    QAudioOutput *m_output = nullptr;
};

#endif // KGAMESOUND_H
