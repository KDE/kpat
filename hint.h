#ifndef HINT_H
#define HINT_H

class Card;
class Pile;

class MoveHint
{
public:
    MoveHint(Card *c, Pile *_to, bool d=true);
    bool dropIfTarget() const { return _dropiftarget; }
    Card *card() const { return _card; }
    Pile *pile() const { return to; }

private:
    Card *_card;
    Pile *to;
    bool _dropiftarget;
};

typedef QValueList<MoveHint*> HintList;

#endif
