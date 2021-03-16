#include "door.h"

#include <random>
#include <string>
#include <utility> // pair
#include <vector>

/*
I tried card_height = 5, but the cards looked a little too stretched out/tall.
3 looks good.

Spacing 1 or 3.  1 is what was used before, 3 looks better, takes up more
screenspace.  And I have plenty, even on 80x23.

TODO:  Have functions that gives me:
int deck(int c);  // which deck #
int suit(int c);  // suit
int rank(int c);  // rank
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
  int unblocks(int c);
  const static std::array<std::pair<int, int>, 18> blocks;
};

/**
 * @brief Given a position, space=3, height=3, return x,y and level.
 *
 * @param pos
 * @param space
 * @param h
 * @param x
 * @param y
 * @param level
 */
void cardgo(int pos, int space, int h, int &x, int &y, int &level);

/**
 * @brief shuffle deck of cards
 *
 * example of seeding the deck for a given date 2/27/2021 game 1
 * std::seed_seq s1{2021, 2, 27, 1};
 * vector<int> deck1 = card_shuffle(s1, 1);
 * @param seed
 * @param decks
 * @return vector<int>
 */
std::vector<int> card_shuffle(std::seed_seq &seed, int decks = 1);

std::vector<int> card_states(int decks = 1);
