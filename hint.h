#ifndef HINT_H
#define HINT_H


class Card;
class Pile;


class MoveHint
{
public:
    MoveHint(Card *card, Pile *to, bool d=true);

    bool   dropIfTarget() const { return m_dropiftarget; }
    Card  *card() const         { return m_card;         }
    Pile  *pile() const         { return m_to;           }

private:
    Card  *m_card;
    Pile  *m_to;
    bool   m_dropiftarget;
};


typedef QValueList<MoveHint*>  HintList;


#endif
