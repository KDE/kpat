/*
    SPDX-FileCopyrightText: 2010 Stefan Majewsky <majewsky@gmx.net>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#ifndef KGAMERENDERERCLIENT_H
#define KGAMERENDERERCLIENT_H

// own
#include "kdegames_export.h"
// Qt
#include <QPixmap>
// Std
#include <memory>

class KGameRendererClientPrivate;
class KGameRenderer;
class KGameRendererPrivate;

#ifndef KDEGAMES_QCOLOR_QHASH
#define KDEGAMES_QCOLOR_QHASH
inline uint qHash(const QColor &color)
{
    return color.rgba();
}
#endif // KDEGAMES_QCOLOR_QHASH

/**
 * @class KGameRendererClient kgamerendererclient.h <KGameRendererClient>
 * @short An object that receives pixmaps from a KGameRenderer.
 *
 * This class abstracts a sprite rendered by KGameRenderer. Given a sprite key,
 * render size and possibly a frame index, it returns the QPixmap for this
 * sprite (frame) once it becomes available. See the KGameRenderer class
 * documentation for details.
 *
 * Subclasses have to reimplement the receivePixmap() method.
 * @since 4.6
 */
class KDEGAMES_EXPORT KGameRendererClient
{
public:
    /// Creates a new client which receives pixmaps for the sprite with the
    /// given @a spriteKey as provided by the given @a renderer.
    KGameRendererClient(KGameRenderer *renderer, const QString &spriteKey);
    virtual ~KGameRendererClient();

    /// @return the renderer used by this client
    KGameRenderer *renderer() const;
    /// @return the frame count, or 0 for non-animated sprites, or -1 if the
    /// sprite does not exist at all
    /// @see KGameRenderer::frameCount()
    int frameCount() const;

    /// @return the key of the sprite currently rendered by this client
    QString spriteKey() const;
    /// Defines the key of the sprite which is rendered by this client.
    void setSpriteKey(const QString &spriteKey);
    /// @return the current frame number, or -1 for non-animated sprites
    int frame() const;
    /// For animated sprites, render another frame. The given frame number is
    /// normalized by taking the modulo of the frame count, so the following
    /// code works fine:
    /// @code
    ///     class MyClient : public KGameRendererClient { ... }
    ///     MyClient client;
    ///     client.setFrame(client.frame() + 1); //cycle to next frame
    ///     client.setFrame(KRandom::random());  //choose a random frame
    /// @endcode
    void setFrame(int frame);
    /// @return the size of the pixmap requested from KGameRenderer
    QSize renderSize() const;
    /// Defines the size of the pixmap that will be requested from
    /// KGameRenderer. For pixmaps rendered on the screen, you usually want
    /// to set this size such that the pixmap does not have to be scaled when
    /// it is rendered onto your primary view (for speed reasons).
    ///
    /// The default render size is very small (width = height = 3 pixels), so
    /// that you notice when you forget to set this. ;-)
    void setRenderSize(QSize renderSize);
    /// @return the custom color replacements for this client
    QHash<QColor, QColor> customColors() const;
    /// Defines the custom color replacements for this client. That is, for
    /// each entry in this has, the key color will be replaced by its value
    /// if it is encountered in the sprite.
    ///
    /// @note Custom colors increase the rendering time considerably, so use
    ///       this feature only if you really need its flexibility.
    void setCustomColors(const QHash<QColor, QColor> &customColors);

protected:
    /// This method is called when the KGameRenderer has provided a new
    /// pixmap for this client (esp. after theme changes and after calls to
    /// setFrame(), setRenderSize() and setSpriteKey()).
    virtual void receivePixmap(const QPixmap &pixmap) = 0;

private:
    friend class KGameRendererClientPrivate;
    friend class KGameRenderer;
    friend class KGameRendererPrivate;
    std::unique_ptr<KGameRendererClientPrivate> const d_ptr;
    Q_DECLARE_PRIVATE(KGameRendererClient)
};

#endif // KGAMERENDERERCLIENT_H
