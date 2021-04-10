#ifndef DECK_H
#define DECK_H

#include "door.h"

#include <memory>
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

typedef std::vector<int> cards; // or a "deck"
typedef std::shared_ptr<door::Panel> shared_panel;

class Deck {
private:
  // We assume for this game that there's only one deck back color.
  door::ANSIColor card_back_color;
  // shared_ptr<door::Panel> for the win?
  vector<door::Panel *> cards;
  vector<door::Panel *> backs;
  vector<door::Panel *> mark;
  door::Panel *cardOf(int c);
  std::string backSymbol(int level);
  door::Panel *backOf(int level);
  door::Panel *markOf(int c);
  char rankSymbol(int c);
  std::string suitSymbol(int c);
  const int card_height = 3;

public:
  enum SUIT { HEART, DIAMOND, CLUBS, SPADE };

  Deck(door::ANSIColor backcolor = door::ANSIColor(door::COLOR::RED));
  Deck(Deck &&);
  Deck &operator=(Deck &&);

  ~Deck();

  int getRank(int c);
  int getSuit(int c);
  int getDeck(int c);

  bool canPlay(int card1, int card2);
  door::Panel *card(int c);
  door::Panel *back(int level);
  door::Panel *marker(int c);

  void part(int x, int y, door::Door &d, int level, bool left);
  std::vector<int> unblocks(int card);
  const static std::array<std::pair<int, int>, 18> blocks;

  void removeCard(door::Door &door, int c, int off_x, int off_y, bool left,
                  bool right);
};

void cardgo(int pos, int &x, int &y, int &level);
cards shuffleCards(std::seed_seq &seed, int decks = 1);
cards makeCardStates(int decks = 1);
int findNextActiveCard(bool left, const cards &states, int current);
int findClosestActiveCard(const cards &states, int current);

extern vector<std::string> deck_colors;
door::renderFunction makeColorRender(door::ANSIColor c1, door::ANSIColor c2,
                                     door::ANSIColor c3);
door::ANSIColor stringToANSIColor(std::string colorCode);
std::string stringFromColorOptions(int opt);

void string_toupper(std::string &str);

#endif