#ifndef DECK_H
#define DECK_H

#include "door.h"

#include <random>
#include <string>
#include <utility> // pair
#include <vector>

/*
I tried card_height = 5, but the cards looked a little too stretched out/tall.
3 looks good.

layout, rev2:

12345678901234567890123456789012345678901234567890123456789012345678901234567890

                    +---------------------------------+
                      Space Ace - Tri-Peaks Solitaire
                    +---------------------------------+
Cards start at 0, not 1!
             ░░1░░                   ░░2░░                   ░░3░░
             ░░░░░                   ░░░░░                   ░░░░░
         ▒▒4▒▒░░░▒▒5▒▒           ##6##░░░##7##           ##8##░░░##9##
         ▒▒▒▒▒   ▒▒▒▒▒           #####   #####           #####   #####
     ▓▓10▓▒▒▒▓▓11▓▒▒▒▓▓12▓   ##13#===##14#===##15#   ##16#===##17#===##18#
     ▓▓▓▓▓   ▓▓▓▓▓   ▓▓▓▓▓   #####   #####   #####   #####   #####   #####
 ██19█▓▓▓██20█▓▓▓██21█▓▓▓##22#===##23#===##24#===##25#===##26#===##27#===##28#
 █████   █████   █████   #####   #####   #####   #####   #####   #####   #####
 █████   █████   █████   #####   #####   #####   #####   #####   #####   #####

  Name:                          ##30#   --29-       Playing: December 31
  Score:                         #####   -----       Current Streak: nn
  Time used: xxx / XXX left      #####   -----       Longest Streak: nn
  Playing Hand X of X            Cards left XX
  1234567890123456789012345      123456789012345     12345678901234567890
 [4/<] Left [6/>] Right [Space] Play Card [Enter] Draw [Q]uit [R]edraw [H]elp

^ -- above is 20 lines from +-- to [4/<] < Left
  score_panel                    left_panel          streak_panel
                                 command_panel

                           #####
Player Information         #####        Time in: xx Time out: xx
Name:                      #####    Playing Day: November 3rd
Hand Score   :                            Current Streak: N
Todays Score :      XX Cards Remaining    Longest Streak: NN
Monthly Score:      Playing Hand X of X   Most Won: xxx Lost: xxx
 [4] Lf [6] Rt  [Space] Play Card [Enter] Draw [D]one [H]elp [R]edraw


layout, rev1:

         ░░░░░             ░░░░░             ░░░░░
         ░░░░░             ░░░░░             ░░░░░
      ▒▒▒▒▒░▒▒▒▒▒       #####░#####       #####░#####
      ▒▒▒▒▒ ▒▒▒▒▒       ##### #####       ##### #####
   ▓▓▓▓▓▒▓▓▓▓▓▒▓▓▓▓▓ #####=#####=##### #####=#####=#####
   ▓▓▓▓▓ ▓▓▓▓▓ ▓▓▓▓▓ ##### ##### ##### ##### ##### #####
█████▓█████▓█████▓#####=#####=#####=#####=#####=#####=#####
█████ █████ █████ ##### ##### ##### ##### ##### ##### #####
█████ █████ █████ ##### ##### ##### ##### ##### ##### #####

                           #####
Player Information         #####        Time in: xx Time out: xx
Name:                      #####    Playing Day: November 3rd
Hand Score   :                            Current Streak: N
Todays Score :      XX Cards Remaining    Longest Streak: NN
Monthly Score:      Playing Hand X of X   Most Won: xxx Lost: xxx
 [4] Lf [6] Rt  [Space] Play Card [Enter] Draw [D]one [H]elp [R]edraw



Spacing 1 or 3.  1 is what was used before, 3 looks better, takes up more
screenspace.  And I have plenty, even on 80x23.

TODO:  Have functions that gives me:
int deck(int c);  // which deck #
int suit(int c);  // suit
int rank(int c);  // rank
 */

typedef std::vector<int> cards;

class Deck {
private:
  door::ANSIColor cardback;
  vector<door::Panel *> cards;
  vector<door::Panel *> backs;
  vector<door::Panel *> mark;
  door::Panel *card_of(int c);
  std::string back_char(int level);
  door::Panel *back_of(int level);
  door::Panel *mark_of(int c);
  void init(void);
  char rank_symbol(int c);
  std::string suit_symbol(int c);
  int card_height;

public:
  enum SUIT { HEART, DIAMOND, CLUBS, SPADE };

  Deck(int size = 3);
  // Deck(const Deck &) = default;
  Deck(Deck &&);
  Deck &operator=(Deck &&);
  Deck(door::ANSIColor backcolor, int size = 3);
  ~Deck();

  int is_rank(int c);
  int is_suit(int c);
  int is_deck(int c);
  /**
   * @brief Can this rank play on this other rank?
   *
   * @param c1
   * @param c2
   * @return true
   * @return false
   */
  bool can_play(int c1, int c2);

  door::Panel *card(int c);
  /**
   * @brief Return panel for back of card.
   *
   * 0 = Blank
   * 1 = level 1 (furthest/darkest)
   * 2 = level 2
   * 3 = level 3
   * 4 = level 4 (closest/lightest)
   *
   * 5 = left (fills with left corner in place)
   * 6 = right (fills right corner)
   * 7 = both (fills both corners)
   *
   * @param level
   * @return door::Panel*
   */

  door::Panel *back(int level);
  /**
   * @brief Returns marker
   *
   * 0 = blank
   * 1 = [] symbol thing \xfe ■
   *
   * @param c
   * @return door::Panel*
   */
  door::Panel *marker(int c);
  void part(int x, int y, door::Door &d, int level, bool left);
  std::vector<int> unblocks(int c);
  const static std::array<std::pair<int, int>, 18> blocks;

  void remove_card(door::Door &door, int c, int off_x, int off_y, bool left,
                   bool right);
};

/**
 * @brief Given a position, space=3, height=3, return x,y and level.
 *
 * This is the older version that allows for space and h "height"
 * to be variable.  I'd rather have one that has them as constants.
 *
 * @param pos
 * @param space
 * @param h
 * @param x
 * @param y
 * @param level
 */
[[deprecated("Use cardgo(int pos, int &x, int &y, int &level")]] void
cardgo(int pos, int space, int h, int &x, int &y, int &level);

/**
 * @brief Where does this card go?
 *
 * This finds x, y, and the level (for the card background)
 *
 * @param pos
 * @param x
 * @param y
 * @param level
 */
void cardgo(int pos, int &x, int &y, int &level);

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
cards card_shuffle(std::seed_seq &seed, int decks = 1);

/**
 * @brief return vector of card states
 *
 * These are pre-initialized to 0.
 * Default to 1 deck (0-51), but this handles any number of decks.
 *
 * @param decks
 * @return cards
 */

cards card_states(int decks = 1);

/**
 * @brief Find the next card to move to.
 *
 * if (left) .. to the left, otherwise right
 * current is the current position we're on.
 *
 * return -1 failed to find anything.
 * @param left
 * @param states
 * @param current
 * @return int
 */
int find_next(bool left, const cards &states, int current);

/**
 * @brief Find the next closest card to move to.
 *
 * Given the card states, this finds the next closest card.
 * Uses current.
 *
 * return -1 there's no options to go to.  (END OF GAME)
 * @param states
 * @param current
 * @return int
 */
int find_next_closest(const cards &states, int current);

extern vector<std::string> deck_colors;
door::renderFunction makeColorRender(door::ANSIColor c1, door::ANSIColor c2,
                                     door::ANSIColor c3);
door::ANSIColor from_string(std::string colorCode);
std::string from_color_option(int opt);
void string_toupper(std::string &str);

#endif