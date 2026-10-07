/*
    SPDX-FileCopyrightText: 2010 Stefan Majewsky <majewsky@gmx.net>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "colorproxy_p.h"

// Qt
#include <QPainterPath>

// BEGIN QPaintDeviceColorProxy

QPaintDeviceColorProxy::QPaintDeviceColorProxy(QPaintDevice *proxiedDevice, const QHash<QColor, QColor> &replacements)
    : m_proxiedDevice(proxiedDevice)
    , m_engine(new QPaintEngineColorProxy)
    , m_replacements(replacements)
{
}

QPaintDeviceColorProxy::~QPaintDeviceColorProxy()
{
    delete m_engine;
}

QPaintDevice *QPaintDeviceColorProxy::proxiedDevice() const
{
    return m_proxiedDevice;
}

QPaintEngine *QPaintDeviceColorProxy::paintEngine() const
{
    return m_engine;
}

int QPaintDeviceColorProxy::metric(QPaintDevice::PaintDeviceMetric metric) const
{
    // return m_proxiedDevice->metric(metric);
    // This does not work because m_proxiedDevice->QPaintDevice::metric() is
    // protected, so we have to do the monkey dance instead.

    typedef int (QPaintDevice::*MetricFunction)(void) const;
    static const MetricFunction metricFunctions[] = {nullptr,
                                                     &QPaintDevice::width, // QPaintDevice::PdmWidth == 1
                                                     &QPaintDevice::height,
                                                     &QPaintDevice::widthMM,
                                                     &QPaintDevice::heightMM,
                                                     &QPaintDevice::colorCount,
                                                     &QPaintDevice::depth,
                                                     &QPaintDevice::logicalDpiX,
                                                     &QPaintDevice::logicalDpiY,
                                                     &QPaintDevice::physicalDpiX,
                                                     &QPaintDevice::physicalDpiY};
    return (m_proxiedDevice->*metricFunctions[metric])();
}

QBrush QPaintDeviceColorProxy::map(const QBrush &brush) const
{
    const Qt::BrushStyle style = brush.style();
    const bool isGradient = style == Qt::LinearGradientPattern || style == Qt::RadialGradientPattern || style == Qt::ConicalGradientPattern;
    // copy the given brush and map the contained color
    if (!isGradient) {
        QBrush result(brush);
        result.setColor(map(brush.color()));
        return result;
    }
    // for gradients, map colors of gradient stops
    QGradient newGradient(*brush.gradient());
    QGradientStops stops = newGradient.stops();
    for (QGradientStop &stop : stops) {
        stop.second = map(stop.second);
    }
    newGradient.setStops(stops);
    // there is no QBrush::setGradient(), so we have to clone the instance by hand
    QBrush result(newGradient);
    result.setTransform(brush.transform());
    return result;
}

QColor QPaintDeviceColorProxy::map(const QColor &color) const
{
    return m_replacements.value(color, color);
}

QPen QPaintDeviceColorProxy::map(const QPen &pen) const
{
    QPen result(pen);
    result.setBrush(map(pen.brush()));
    return result;
}

// END QPaintDeviceColorProxy

// BEGIN QPaintEngineColorProxy

QPaintEngineColorProxy::QPaintEngineColorProxy()
    : m_proxy(nullptr)
    , m_painter(new QPainter)
{
}

QPaintEngineColorProxy::~QPaintEngineColorProxy()
{
    if (m_proxy) {
        end();
    }
    delete m_painter;
}

bool QPaintEngineColorProxy::begin(QPaintDevice *device)
{
    // previous operation not ended?
    if (m_proxy) {
        return false;
    }
    // allow only proxy devices
    QPaintDeviceColorProxy *proxyDevice = dynamic_cast<QPaintDeviceColorProxy *>(device);
    if (!proxyDevice) {
        return false;
    }
    // check proxied device
    QPaintDevice *proxiedDevice = proxyDevice->proxiedDevice();
    if (!proxiedDevice) {
        return false;
    }
    // start to paint on proxied device
    m_painter = new QPainter;
    if (!m_painter->begin(proxiedDevice)) {
        return false;
    }
    // success
    m_proxy = proxyDevice;
    return true;
}

bool QPaintEngineColorProxy::end()
{
    if (!m_proxy) {
        return false;
    }
    m_proxy = nullptr;
    return m_painter->end();
}

void QPaintEngineColorProxy::drawEllipse(const QRectF &rect)
{
    if (m_proxy) {
        m_painter->drawEllipse(rect);
    }
}

void QPaintEngineColorProxy::drawEllipse(const QRect &rect)
{
    if (m_proxy) {
        m_painter->drawEllipse(rect);
    }
}

void QPaintEngineColorProxy::drawImage(const QRectF &rectangle, const QImage &image, const QRectF &sr, Qt::ImageConversionFlags flags)
{
    if (m_proxy) {
        m_painter->drawImage(rectangle, image, sr, flags);
    }
}

void QPaintEngineColorProxy::drawLines(const QLineF *lines, int lineCount)
{
    if (m_proxy) {
        m_painter->drawLines(lines, lineCount);
    }
}

void QPaintEngineColorProxy::drawLines(const QLine *lines, int lineCount)
{
    if (m_proxy) {
        m_painter->drawLines(lines, lineCount);
    }
}

void QPaintEngineColorProxy::drawPath(const QPainterPath &path)
{
    if (m_proxy) {
        m_painter->drawPath(path);
    }
}

void QPaintEngineColorProxy::drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr)
{
    if (m_proxy) {
        m_painter->drawPixmap(r, pm, sr);
    }
}

void QPaintEngineColorProxy::drawPoints(const QPointF *points, int pointCount)
{
    if (m_proxy) {
        m_painter->drawPoints(points, pointCount);
    }
}

void QPaintEngineColorProxy::drawPoints(const QPoint *points, int pointCount)
{
    if (m_proxy) {
        m_painter->drawPoints(points, pointCount);
    }
}

void QPaintEngineColorProxy::drawPolygon(const QPointF *points, int pointCount, QPaintEngine::PolygonDrawMode mode)
{
    if (m_proxy) {
        m_painter->drawPolygon(points, pointCount, mode == WindingMode ? Qt::WindingFill : Qt::OddEvenFill);
    }
}

void QPaintEngineColorProxy::drawPolygon(const QPoint *points, int pointCount, QPaintEngine::PolygonDrawMode mode)
{
    if (m_proxy) {
        m_painter->drawPolygon(points, pointCount, mode == WindingMode ? Qt::WindingFill : Qt::OddEvenFill);
    }
}

void QPaintEngineColorProxy::drawRects(const QRectF *rects, int rectCount)
{
    if (m_proxy) {
        m_painter->drawRects(rects, rectCount);
    }
}

void QPaintEngineColorProxy::drawRects(const QRect *rects, int rectCount)
{
    if (m_proxy) {
        m_painter->drawRects(rects, rectCount);
    }
}

void QPaintEngineColorProxy::drawTextItem(const QPointF &p, const QTextItem &textItem)
{
    if (m_proxy) {
        m_painter->drawTextItem(p, textItem);
    }
}

void QPaintEngineColorProxy::drawTiledPixmap(const QRectF &rect, const QPixmap &pixmap, const QPointF &p)
{
    if (m_proxy) {
        m_painter->drawTiledPixmap(rect, pixmap, p);
    }
}

QPaintEngine::Type QPaintEngineColorProxy::type() const
{
    return QPaintEngine::User;
}

void QPaintEngineColorProxy::updateState(const QPaintEngineState &state)
{
    if (!m_proxy) {
        return;
    }
    const QPaintEngine::DirtyFlags flags = state.state();
    if (flags & QPaintEngine::DirtyBackground) {
        const QBrush brush = state.backgroundBrush();
        QBrush mappedBrush = m_proxy->map(brush);
        if (mappedBrush != brush)
            // pass changed brush back to own painter
            painter()->setBackground(mappedBrush);
        m_painter->setBackground(mappedBrush);
    }
    if (flags & QPaintEngine::DirtyBackgroundMode) {
        m_painter->setBackgroundMode(state.backgroundMode());
    }
    if (flags & QPaintEngine::DirtyBrush) {
        const QBrush brush = state.brush();
        QBrush mappedBrush = m_proxy->map(brush);
        if (mappedBrush != brush)
            // pass changed brush back to own painter
            painter()->setBrush(mappedBrush);
        m_painter->setBrush(mappedBrush);
    }
    if (flags & QPaintEngine::DirtyBrushOrigin) {
        m_painter->setBrushOrigin(state.brushOrigin());
    }
    if (flags & QPaintEngine::DirtyClipEnabled) {
        m_painter->setClipping(state.isClipEnabled());
    }
    if (flags & QPaintEngine::DirtyClipPath) {
        m_painter->setClipPath(state.clipPath(), state.clipOperation());
    }
    if (flags & QPaintEngine::DirtyClipRegion) {
        m_painter->setClipRegion(state.clipRegion(), state.clipOperation());
    }
    if (flags & QPaintEngine::DirtyCompositionMode) {
        m_painter->setCompositionMode(state.compositionMode());
    }
    if (flags & QPaintEngine::DirtyFont) {
        m_painter->setFont(state.font());
    }
    if (flags & QPaintEngine::DirtyHints) {
        m_painter->setRenderHints(state.renderHints());
    }
    if (flags & QPaintEngine::DirtyPen) {
        const QPen pen = state.pen();
        QPen mappedPen = m_proxy->map(pen);
        if (mappedPen != pen)
            // pass changed pen back to own painter
            painter()->setPen(mappedPen);
        m_painter->setPen(mappedPen);
    }
    if (flags & QPaintEngine::DirtyTransform) {
        m_painter->setTransform(state.transform());
    }
}

// END QPaintEngineColorProxy
