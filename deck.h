#include "door.h"

#include <string>
#include <vector>

class Deck {
private:
  door::ANSIColor cardback;
  vector<door::Panel *> cards;
  vector<door::Panel *> backs;
  door::Panel *card_of(int c);
  std::string back_char(int level);
  door::Panel *back_of(int level);
  int is_rank(int c);
  int is_suit(int c);
  void init(void);
  char rank_symbol(int c);
  std::string suit_symbol(int c);

public:
  enum SUIT { HEART, DIAMOND, CLUBS, SPADE };

  Deck();
  Deck(door::ANSIColor backcolor);
  ~Deck();

  door::Panel *card(int c);
  door::Panel *back(int level);
  void part(int x, int y, door::Door &d, int level, bool left);
};

void cardgo(int pos, int &x, int &y, int &level);
