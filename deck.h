#include "door.h"

#include <random>
#include <string>
#include <vector>

/*
I tried card_height = 5, but the cards looked a little too stretched out/tall.
3 looks good.

Spacing 1 or 3.  1 is what was used before, 3 looks better, takes up more
screenspace.  And I have plenty, even on 80x23.

 */

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
  int card_height;

public:
  enum SUIT { HEART, DIAMOND, CLUBS, SPADE };

  Deck(int size = 3);
  Deck(door::ANSIColor backcolor, int size = 3);
  ~Deck();

  door::Panel *card(int c);
  door::Panel *back(int level);
  void part(int x, int y, door::Door &d, int level, bool left);
};

void cardgo(int pos, int space, int h, int &x, int &y, int &level);

vector<int> card_shuffle(std::seed_seq &seed, int decks = 1);