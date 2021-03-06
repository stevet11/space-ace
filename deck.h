#ifndef DECK_H
#define DECK_H

#include "door.h"

#include <memory>
#include <random>
#include <string>
#include <utility> // pair
#include <vector>

/*
https://en.wikipedia.org/wiki/Code_page_437

using: \xb0, 0xb1, 0xb2, 0xdb
OR: \u2591, \u2592, \u2593, \u2588

Like so:

##### #####
##### #####
##### #####

Cards:  (Black on White, or Red on White)
8D### TH###
##D## ##H##
###D8 ###HT

D, H = Red, Clubs, Spades = Black.

^ Where D = Diamonds, H = Hearts

♥, ♦, ♣, ♠
\x03, \x04, \x05, \x06
\u2665, \u2666, \u2663, \u2660

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

  vector<shared_panel> cards;
  vector<shared_panel> backs;
  vector<shared_panel> mark;

  shared_panel cardOf(int c);
  std::string backSymbol(int level);
  shared_panel backOf(int level);
  shared_panel markOf(int c);
  char rankSymbol(int c);
  std::string suitSymbol(int c);
  const int card_height = 3;

public:
  enum SUIT { HEART, DIAMOND, CLUBS, SPADE };
  const static std::array<std::pair<int, int>, 18> blocks;

  Deck(door::ANSIColor backcolor = door::ANSIColor(door::COLOR::RED));
  Deck(Deck &&);
  Deck &operator=(Deck &&);

  ~Deck();

  void setBack(door::ANSIColor backcolor);
  int getRank(int c);
  int getSuit(int c);
  int getDeck(int c);
  bool canPlay(int card1, int card2);
  shared_panel card(int c);
  shared_panel back(int level);
  shared_panel marker(int c);
  std::vector<int> unblocks(int card);

  void removeCard(door::Door &door, int c, int off_x, int off_y, bool left,
                  bool right);
};

void cardPos(int pos, int &x, int &y);
void cardLevel(int pos, int &level);
void cardPosLevel(int pos, int &x, int &y, int &level);

cards shuffleCards(std::seed_seq &seed, int decks = 1);
cards makeCardStates(int decks = 1);
int findNextActiveCard(bool left, const cards &states, int current);
int findClosestActiveCard(const cards &states, int current);

extern vector<std::string> deck_colors;
door::renderFunction makeColorRender(door::ANSIColor c1, door::ANSIColor c2,
                                     door::ANSIColor c3);
door::ANSIColor stringToANSIColor(std::string colorCode);
std::string stringFromColorOptions(int opt);

door::Panel make_about(void);
door::Panel make_help(void);
void display_starfield(door::Door &door, std::mt19937 &rng);
void display_space_ace(door::Door &door);
void display_starfield_space_ace(door::Door &door, std::mt19937 &rng);

door::Panel make_timeout(int mx, int my);
door::Panel make_notime(int mx, int my);
door::Menu make_main_menu(void);
door::Menu make_config_menu(void);
door::Menu make_deck_menu(void);

door::Panel make_sysop_config(void);

#endif