#ifndef DEMO_H
#define DEMO_H

class GameBubble;

#include <QWidget>


class DemoBubbles : public QWidget
{
    Q_OBJECT

public:
    DemoBubbles( QWidget *parent);
    ~DemoBubbles();
    virtual void paintEvent ( QPaintEvent * event );
    virtual void mouseMoveEvent ( QMouseEvent * event );
    virtual void mousePressEvent ( QMouseEvent * event );
    virtual void resizeEvent ( QResizeEvent * event );

signals:
    void gameSelected( int i );

private:
    int games;
    int bubble_text_height, bubble_width, bubble_height;

    GameBubble *m_bubbles;
};

#endif // DEMO_H
