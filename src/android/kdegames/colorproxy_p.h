/*
    SPDX-FileCopyrightText: 2010 Stefan Majewsky <majewsky@gmx.net>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#ifndef LIBKDEGAMES_COLORPROXY_P_H
#define LIBKDEGAMES_COLORPROXY_P_H

// Qt
#include <QHash>
#include <QPaintDevice>
#include <QPaintEngine>

class QPaintEngineColorProxy;

#ifndef KDEGAMES_QCOLOR_QHASH
#define KDEGAMES_QCOLOR_QHASH
inline uint qHash(const QColor &color)
{
    return color.rgba();
}
#endif // KDEGAMES_QCOLOR_QHASH

/**
 * This QPaintDevice forwards all painting operations performed on it to the
 * contained QPaintDevice. The only modification is that certain colors are
 * replaced by others in all painting operations (except for drawImage and
 * drawPixmap).
 *
 * @note Using this class results in a non-negligible performance penalty
 * (~10-20% longer runtime), so use it only if the set of color replacements is
 * non-empty.
 */
class QPaintDeviceColorProxy : public QPaintDevice
{
public:
    /// @warning Replacement loops (e.g. color1 -> color2 -> color3 -> color1) lead to infinite loops.
    /// @warning You should not interact with the @a proxiedDevice during the lifetime of this instance.
    QPaintDeviceColorProxy(QPaintDevice *proxiedDevice, const QHash<QColor, QColor> &replacements);
    ~QPaintDeviceColorProxy() override;

    QPaintDevice *proxiedDevice() const;
    QPaintEngine *paintEngine() const override;

    QBrush map(const QBrush &brush) const;
    inline QColor map(const QColor &color) const;
    QPen map(const QPen &pen) const;

protected:
    int metric(PaintDeviceMetric metric) const override;

private:
    QPaintDevice *m_proxiedDevice;
    QPaintEngine *m_engine;
    QHash<QColor, QColor> m_replacements;
};

class QPaintEngineColorProxy : public QPaintEngine
{
public:
    QPaintEngineColorProxy();
    ~QPaintEngineColorProxy() override;

    bool begin(QPaintDevice *device) override;
    bool end() override;
    void drawEllipse(const QRectF &rect) override;
    void drawEllipse(const QRect &rect) override;
    void drawImage(const QRectF &rectangle, const QImage &image, const QRectF &sr, Qt::ImageConversionFlags flags = Qt::AutoColor) override;
    void drawLines(const QLineF *lines, int lineCount) override;
    void drawLines(const QLine *lines, int lineCount) override;
    void drawPath(const QPainterPath &path) override;
    void drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr) override;
    void drawPoints(const QPointF *points, int pointCount) override;
    void drawPoints(const QPoint *points, int pointCount) override;
    void drawPolygon(const QPointF *points, int pointCount, PolygonDrawMode mode) override;
    void drawPolygon(const QPoint *points, int pointCount, PolygonDrawMode mode) override;
    void drawRects(const QRectF *rects, int rectCount) override;
    void drawRects(const QRect *rects, int rectCount) override;
    void drawTextItem(const QPointF &p, const QTextItem &textItem) override;
    void drawTiledPixmap(const QRectF &rect, const QPixmap &pixmap, const QPointF &p) override;
    Type type() const override;
    void updateState(const QPaintEngineState &state) override;

private:
    QPaintDeviceColorProxy *m_proxy;
    QPainter *m_painter;
};

#endif // LIBKDEGAMES_COLORPROXY_P_H
