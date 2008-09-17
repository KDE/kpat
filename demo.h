#include <QWidget>

#include <QPixmap>

class DemoBubbles : public QWidget
{
public:
    DemoBubbles( QWidget *parent);
    virtual void resizeEvent( QResizeEvent *e );
    virtual void paintEvent ( QPaintEvent * event );

private:
    QPixmap backPix, bubblePix;
    int games;
    int bubble_text_height;

};
