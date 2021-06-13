#include "deck.h"

#include "space.h"
#include "utils.h"
#include "version.h"
#include <algorithm>
#include <iomanip> // setw
#include <map>
#include <sstream>

Deck::Deck(door::ANSIColor backcolor) : card_back_color{backcolor} {
  for (int i = 0; i < 52; ++i) {
    cards.push_back(cardOf(i));
  }
  // 0 = BLANK, 1-4 levels
  for (int i = 0; i < 5; ++i) {
    backs.push_back(backOf(i));
  }
  mark.push_back(markOf(0));
  mark.push_back(markOf(1));
}

Deck::~Deck() {}

Deck::Deck(Deck &&ref) {
  card_back_color = ref.card_back_color;
  /*
  for (auto c : cards)
    delete c;

  cards.clear();
  */
  cards = ref.cards;
  ref.cards.clear();
  /*
  for (auto b : backs)
    delete b;
  backs.clear();
  */
  backs = ref.backs;
  ref.backs.clear();
  /*
  for (auto m : mark)
    delete m;
  mark.clear();
  */
  mark = ref.mark;

  ref.mark.clear();
  // card_height = ref.card_height;
};

Deck &Deck::operator=(Deck &&ref) {
  card_back_color = ref.card_back_color;
  /*
  for (auto c : cards)
    delete c;
  cards.clear();
  */
  cards = ref.cards;
  ref.cards.clear();
  /*
  for (auto b : backs)
    delete b;
  backs.clear();
  */
  backs = ref.backs;
  ref.backs.clear();
  /*
  for (auto m : mark)
    delete m;
  mark.clear();
  */
  mark = ref.mark;
  ref.mark.clear();
  // card_height = ref.card_height;
  return *this;
}

int Deck::getDeck(int c) { return c / 52; }
int Deck::getSuit(int c) { return (c % 52) / 13; }
int Deck::getRank(int c) { return (c % 52) % 13; }

char Deck::rankSymbol(int c) {
  const char symbols[] = "A23456789TJQK";
  return symbols[c];
}

std::string Deck::suitSymbol(int c) {
  // unicode
  if (door::unicode) {
    switch (c) {
    case 0:
      return std::string("\u2665");
    case 1:
      return std::string("\u2666");
    case 2:
      return std::string("\u2663");
    case 3:
      return std::string("\u2660");
    }
  } else {
    if (door::full_cp437) {
      switch (c) {
      case 0:
        return std::string(1, '\x03');
      case 1:
        return std::string(1, '\x04');
      case 2:
        return std::string(1, '\x05');
      case 3:
        return std::string(1, '\x06');
      }
    } else {
      // These look horrible!
      switch (c) {
      case 0:
        return std::string(1, '*'); // H
      case 1:
        return std::string(1, '^'); // D
      case 2:
        return std::string(1, '%'); // C
      case 3:
        return std::string(1, '$'); // S
      }
    }
  }
  return std::string("!", 1);
}

shared_panel Deck::cardOf(int c) {
  int suit = getSuit(c);
  int rank = getRank(c);
  bool is_red = (suit < 2);
  door::ANSIColor color;

  if (is_red) {
    color = door::ANSIColor(door::COLOR::RED, door::COLOR::WHITE);
  } else {
    color = door::ANSIColor(door::COLOR::BLACK, door::COLOR::WHITE);
  }
  shared_panel p = std::make_shared<door::Panel>(0, 0, 5);
  // setColor sets border_color.  NOT WHAT I WANT.
  // p->setColor(color);
  char r = rankSymbol(rank);
  std::string s = suitSymbol(suit);

  // build lines
  std::ostringstream oss;
  oss << r << s << "   ";
  std::string str = oss.str();
  p->addLine(std::make_unique<door::Line>(str, 5, color));
  oss.str(std::string());
  oss.clear();

  if (card_height == 5)
    p->addLine(std::make_unique<door::Line>("     ", 5, color));

  oss << "  " << s << "  ";
  str = oss.str();
  p->addLine(std::make_unique<door::Line>(str, 5, color));
  oss.str(std::string());
  oss.clear();

  if (card_height == 5)
    p->addLine(std::make_unique<door::Line>("     ", 5, color));

  oss << "   " << s << r;
  str = oss.str();
  p->addLine(std::make_unique<door::Line>(str, 5, color));
  oss.str(std::string());
  oss.clear();

  return p;
}

std::string Deck::backSymbol(int level) {
  std::string c;
  if (level == 0) {
    c = ' ';
    return c;
  }
  if (door::unicode) {
    switch (level) {
    case 1:
      c = "\u2591";
      break;
    case 2:
      c = "\u2592";
      break;
    case 3:
      c = "\u2593";
      break;
    case 4:
      c = "\u2588";
      break;
    }
  } else {
    switch (level) {
    case 1:
      c = "\xb0";
      break;
    case 2:
      c = "\xb1";
      break;
    case 3:
      c = "\xb2";
      break;
    case 4:
      c = "\xdb";
      break;
    }
  }
  return c;
}

void Deck::setBack(door::ANSIColor backColor) {
  for (auto &back : backs) {
    back->lineSetBack(backColor);
  }
  card_back_color = backColor;
}

shared_panel Deck::backOf(int level) {
  // using: \xb0, 0xb1, 0xb2, 0xdb
  // OR:    \u2591, \u2592, \u2593, \u2588

  // door::ANSIColor color(door::COLOR::RED, door::COLOR::BLACK);
  shared_panel p = std::make_shared<door::Panel>(0, 0, 5);
  std::string c = backSymbol(level);
  std::string l = c + c + c + c + c;
  for (int x = 0; x < card_height; ++x) {
    p->addLine(std::make_unique<door::Line>(l, 5, card_back_color));
  };
  // p->addLine(std::make_unique<door::Line>(l, 5, card_back_color));
  // p->addLine(std::make_unique<door::Line>(l, 5, card_back_color));
  return p;
}

shared_panel Deck::markOf(int c) {
  shared_panel p = std::make_shared<door::Panel>(1);
  door::ANSIColor color = door::ANSIColor(
      door::COLOR::BLUE, door::COLOR::WHITE); // , door::ATTR::BOLD);
  std::string m;
  if (c == 0)
    m = " ";
  else {
    if (door::unicode) {
      m = "\u25a0";
    } else {
      m = "\xfe";
    }
  }
  p->addLine(std::make_unique<door::Line>(m, 1, color));
  return p;
}

shared_panel Deck::card(int c) { return cards[c]; }

/**
 * @brief Return panel for back of card.
 *
 * - 0 = Blank
 * - 1 = level 1 (furthest/darkest)
 * - 2 = level 2
 * - 3 = level 3
 * - 4 = level 4 (closest/lightest)
 *
 * @param level
 * @return door::Panel*
 */
shared_panel Deck::back(int level) { return backs[level]; }

const std::array<std::pair<int, int>, 18> Deck::blocks = {
    make_pair(3, 4),   make_pair(5, 6),   make_pair(7, 8), // end row 1
    make_pair(9, 10),  make_pair(10, 11), make_pair(12, 13),
    make_pair(13, 14), make_pair(15, 16), make_pair(16, 17),
    make_pair(18, 19), // end row 2
    make_pair(19, 20), make_pair(20, 21), make_pair(21, 22),
    make_pair(22, 23), make_pair(23, 24), make_pair(24, 25),
    make_pair(25, 26), make_pair(26, 27) // 27
};

/**
 * @brief Which card(s) are unblocked by this card?
 *
 * @param card
 * @return * int
 */
std::vector<int> Deck::unblocks(int card) {
  std::vector<int> result;
  for (size_t i = 0; i < blocks.size(); ++i) {
    if ((blocks.at(i).first == card) || (blocks.at(i).second == card)) {
      result.push_back(i);
    }
  }
  return result;
}

/**
 * @brief Can this card play on this other card?
 *
 * @param card1
 * @param card2
 * @return true
 * @return false
 */
bool Deck::canPlay(int card1, int card2) {
  int rank1, rank2;
  rank1 = getRank(card1);
  rank2 = getRank(card2);

  // this works %13 handles wrap-around for us.
  if ((rank1 + 1) % 13 == rank2)
    return true;

  if (rank1 == 0) {
    rank1 += 13;
  }
  if (rank1 - 1 == rank2)
    return true;
  return false;
}

/**
 * @brief Returns marker
 *
 * 0 = blank
 * 1 = [] symbol thing \xfe ■
 *
 * @param c
 * @return door::Panel*
 */
shared_panel Deck::marker(int c) { return mark[c]; }

/**
 * @brief removeCard
 *
 * This removes a card at a given position (c).
 * It needs to know if there are cards underneath
 * to the left or right.  (If so, we restore those missing parts.)
 *
 * @param door
 * @param c
 * @param off_x
 * @param off_y
 * @param left
 * @param right
 */
void Deck::removeCard(door::Door &door, int c, int off_x, int off_y, bool left,
                      bool right) {
  int cx, cy, level;
  cardPosLevel(c, cx, cy, level);
  if (level > 1)
    --level;
  std::string cstr = backSymbol(level);
  door::Goto g(cx + off_x, cy + off_y);
  door << g << card_back_color;
  if (left)
    door << cstr;
  else
    door << " ";
  door << "   ";
  if (right)
    door << cstr;
  else
    door << " ";
  g.set(cx + off_x, cy + off_y + 1);
  door << g << "     ";
  g.set(cx + off_x, cy + off_y + 2);
  door << g << "     ";
}

/*
Layout spacing 1:

         1         2         3         4         5         6
123456789012345678901234567890123456789012345678901234567890
         ░░░░░             ░░░░░             ░░░░░
         ░░░░░             ░░░░░             ░░░░░
      ▒▒▒▒▒░▒▒▒▒▒       #####░#####       #####░#####
      ▒▒▒▒▒ ▒▒▒▒▒       ##### #####       ##### #####
   ▓▓▓▓▓▒▓▓▓▓▓▒▓▓▓▓▓ #####=#####=##### #####=#####=#####
   ▓▓▓▓▓ ▓▓▓▓▓ ▓▓▓▓▓ ##### ##### ##### ##### ##### #####
█████▓█████▓█████▓#####=#####=#####=#####=#####=#####=#####
█████ █████ █████ ##### ##### ##### ##### ##### ##### #####
█████ █████ █████ ##### ##### ##### ##### ##### ##### #####

width = 5 * 10 + (1*9) = 59   OK!

Layout with spacing = 2:

            EEEEE
        ZZZZZ
    yyyyyZZZyyyyy
    yyyyy   yyyyy
XXXXXyyyXXXXXyyyXXXXX
XXXXX   XXXXX   XXXXX

width = 5 * 10 + (2 * 9) = 50+18 = 68   !  I could do that!
*/

#ifdef NO
/**
 * @brief Where does this card go in relation to everything else?
 *
 * This function is deprecated, see the other cardgo.
 *
 * @param pos
 * @param space
 * @param h
 * @param x
 * @param y
 * @param level
 */
void cardgo(int pos, int space, int h, int &x, int &y, int &level) {
  // special cases here
  if (pos == 28) {
    // cardgo(23, space, h, x, y, level);
    cardgo(23, x, y, level);
    y += h + 1;
    --level;
    return;
  } else {
    if (pos == 29) {
      // cardgo(22, space, h, x, y, level);
      cardgo(22, x, y, level);
      y += h + 1;
      --level;
      return;
    }
  }

  const int CARD_WIDTH = 5;
  int HALF_WIDTH = 3;
  // space = 1 or 3
  // int space = 1;
  // space = 3;
  HALF_WIDTH += space / 2;

  /*
int levels[4] = {3, 6, 9, 10};

  for (level = 0; level < 4; ++level) {
      if (pos < levels[level]) {
          level++;
          // we're here
          y = (level -1) * 2 + 1;
      } else {
          pos -= levels[level];
      }
  }
*/
  int between = CARD_WIDTH + space;

  if (pos < 3) {
    // top
    level = 1;
    y = (level - 1) * (h - 1) + 1;
    x = pos * (between * 3) + between + HALF_WIDTH + space; // 10
    return;
  } else {
    pos -= 3;
  }
  if (pos < 6) {
    level = 2;
    y = (level - 1) * (h - 1) + 1;
    int group = (pos) / 2;
    x = pos * between + (group * between) + CARD_WIDTH + space * 2;
    return;
  } else {
    pos -= 6;
  }
  if (pos < 9) {
    level = 3;
    y = (level - 1) * (h - 1) + 1;
    x = pos * between + HALF_WIDTH + space;
    return;
  } else {
    pos -= 9;
  }
  if (pos < 10) {
    level = 4;
    y = (level - 1) * (h - 1) + 1;
    x = (pos)*between + space;
    return;
  } else {
    // something is wrong.
    y = -1;
    x = -1;
    level = -1;
  }
}
#endif

void cardPos(int pos, int &x, int &y) {
  const int space = 3;
  const int height = 3;

  // special cases here
  if (pos == 28) {
    cardPos(23, x, y);
    y += height + 1;
    return;
  } else {
    if (pos == 29) {
      cardPos(22, x, y);
      y += height + 1;
      return;
    }
  }

  const int CARD_WIDTH = 5;
  int HALF_WIDTH = 3;
  HALF_WIDTH += space / 2;

  int between = CARD_WIDTH + space;
  int level; // I still need level in my calculations

  if (pos < 3) {
    // top
    level = 1;
    y = (level - 1) * (height - 1) + 1;
    x = pos * (between * 3) + between + HALF_WIDTH + space; // 10
    return;
  } else {
    pos -= 3;
  }
  if (pos < 6) {
    level = 2;
    y = (level - 1) * (height - 1) + 1;
    int group = (pos) / 2;
    x = pos * between + (group * between) + CARD_WIDTH + space * 2;
    return;
  } else {
    pos -= 6;
  }
  if (pos < 9) {
    level = 3;
    y = (level - 1) * (height - 1) + 1;
    x = pos * between + HALF_WIDTH + space;
    return;
  } else {
    pos -= 9;
  }
  if (pos < 10) {
    level = 4;
    y = (level - 1) * (height - 1) + 1;
    x = (pos)*between + space;
    return;
  } else {
    // something is wrong.
    y = -1;
    x = -1;
    level = -1;
  }
}

void cardLevel(int pos, int &level) {
  /*
  const int space = 3;
  const int height = 3;
  */

  // special cases here
  if (pos == 28) {
    cardLevel(23, level);
    --level;
    return;
  } else {
    if (pos == 29) {
      cardLevel(22, level);
      --level;
      return;
    }
  }

  /*
  const int CARD_WIDTH = 5;
  int HALF_WIDTH = 3;
  HALF_WIDTH += space / 2;
  int between = CARD_WIDTH + space;
  */

  if (pos < 3) {
    // top
    level = 1;
    return;
  } else {
    pos -= 3;
  }

  if (pos < 6) {
    level = 2;
    return;
  } else {
    pos -= 6;
  }

  if (pos < 9) {
    level = 3;
    return;
  } else {
    pos -= 9;
  }
  if (pos < 10) {
    level = 4;
    return;
  } else {
    // something is wrong.
    level = -1;
  }
}

/**
 * @brief Given card pos, calculate x, y, and level values.
 *
 * level is used to determine the card background gradient.
 *
 * @param pos
 * @param x
 * @param y
 * @param level
 */
void cardPosLevel(int pos, int &x, int &y, int &level) {
  const int space = 3;
  const int height = 3;

  // special cases here
  if (pos == 28) {
    cardPosLevel(23, x, y, level);
    y += height + 1;
    --level;
    return;
  } else {
    if (pos == 29) {
      cardPosLevel(22, x, y, level);
      y += height + 1;
      --level;
      return;
    }
  }

  const int CARD_WIDTH = 5;
  int HALF_WIDTH = 3;
  HALF_WIDTH += space / 2;

  int between = CARD_WIDTH + space;

  if (pos < 3) {
    // top
    level = 1;
    y = (level - 1) * (height - 1) + 1;
    x = pos * (between * 3) + between + HALF_WIDTH + space; // 10
    return;
  } else {
    pos -= 3;
  }
  if (pos < 6) {
    level = 2;
    y = (level - 1) * (height - 1) + 1;
    int group = (pos) / 2;
    x = pos * between + (group * between) + CARD_WIDTH + space * 2;
    return;
  } else {
    pos -= 6;
  }
  if (pos < 9) {
    level = 3;
    y = (level - 1) * (height - 1) + 1;
    x = pos * between + HALF_WIDTH + space;
    return;
  } else {
    pos -= 9;
  }
  if (pos < 10) {
    level = 4;
    y = (level - 1) * (height - 1) + 1;
    x = (pos)*between + space;
    return;
  } else {
    // something is wrong.
    y = -1;
    x = -1;
    level = -1;
  }
}

/**
 * @brief shuffle deck of cards
 *
 * example of seeding the deck for a given date 2/27/2021 game 1
 * std::seed_seq s1{2021, 2, 27, 1};
 * vector<int> deck1 = shuffleCards(s1, 1);
 * @param seed
 * @param decks
 * @return vector<int>
 */
cards shuffleCards(std::seed_seq &seed, int decks) {
  std::mt19937 gen;

  // build deck of cards
  int size = decks * 52;
  std::vector<int> deck;
  deck.reserve(size);
  for (int x = 0; x < size; ++x) {
    deck.push_back(x);
  }

  // repeatable, but random
  gen.seed(seed);
  std::shuffle(deck.begin(), deck.end(), gen);
  return deck;
}

/**
 * @brief generate a vector of ints to track card states.
 *
 * This initializes everything to 0.
 *
 * @param decks
 * @return cards
 */
cards makeCardStates(int decks) {
  // auto states = std::unique_ptr<std::vector<int>>(); // (decks * 52, 0)>;
  std::vector<int> states;
  states.assign(decks * 52, 0);
  return states;
}

/**
 * @brief Find the next card we can move the marker to.
 *
 * if left, look in the left - direction, otherwise the right + direction.
 * current is the current active card.
 * states is the card states (0 = down, 1 = in play, 2 = removed)
 *
 * updated: If we can't go any further left (or right), then
 * roll around to the other side.
 *
 * @param left
 * @param states
 * @param current
 * @return int
 */
int findNextActiveCard(bool left, const cards &states, int current) {
  int cx, cy;
  int current_x;
  cardPos(current, cx, cy);
  current_x = cx;
  int x;
  int pos = -1;
  int pos_x;

  int max_pos = -1;
  int max_x = -1;
  int min_pos = -1;
  int min_x = 100;

  if (left)
    pos_x = 0;
  else
    pos_x = 100;

  for (x = 0; x < 28; x++) {
    if (states.at(x) == 1) {
      // possible location
      if (x == current)
        continue;
      cardPos(x, cx, cy);
      // find max and min while we're iterating here
      if (cx < min_x) {
        min_pos = x;
        min_x = cx;
      }
      if (cx > max_x) {
        max_pos = x;
        max_x = cx;
      }
      if (left) {
        if ((cx < current_x) and (cx > pos_x)) {
          pos_x = cx;
          pos = x;
        }
      } else {
        if ((cx > current_x) and (cx < pos_x)) {
          pos_x = cx;
          pos = x;
        }
      }
    }
  }
  if (pos == -1) {
    // we couldn't find one
    if (left) {
      // use max -- roll around to the right
      pos = max_pos;
    } else {
      // use min -- roll around to the left
      pos = min_pos;
    }
  }
  return pos;
}

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
int findClosestActiveCard(const cards &states, int current) {
  int cx, cy;
  int current_x;
  cardPos(current, cx, cy);
  current_x = cx;
  int x;
  int pos = -1;
  int pos_x = -1;

  for (x = 0; x < 28; x++) {
    if (states.at(x) == 1) {
      // possible location
      if (x == current)
        continue;
      cardPos(x, cx, cy);
      if (pos == -1) {
        pos = x;
        pos_x = cx;
      } else {
        if (abs(current_x - cx) < abs(current_x - pos_x)) {
          pos = x;
          pos_x = cx;
        }
      }
    }
  }
  return pos;
}

vector<std::string> deck_colors = {std::string("All"),   std::string("Blue"),
                                   std::string("Brown"), std::string("Cyan"),
                                   std::string("Green"), std::string("Magenta"),
                                   std::string("Red"),   std::string("White")};
/**
 * @brief menu render that sets the text color based on the color found in the
 * text itself.  This only finds one color.
 *
 * @param c1 [] brackets
 * @param c2 text within brackets
 * @param c3 base color give (we set the FG, we use the BG)
 * @return door::renderFunction
 */
door::renderFunction makeColorRender(door::ANSIColor c1, door::ANSIColor c2,
                                     door::ANSIColor c3) {
  door::renderFunction render = [c1, c2,
                                 c3](const std::string &txt) -> door::Render {
    door::Render r(txt);

    bool option = true;
    door::ColorOutput co;
    // I need this mutable
    door::ANSIColor textColor = c3;

    // Color update:
    {
      std::string found;

      for (auto &dc : deck_colors) {
        if (txt.find(dc) != string::npos) {
          found = dc;
          break;
        }
      }

      if (!found.empty()) {
        if (found == "All") {
          // handle this some other way.
          textColor.setFg(door::COLOR::WHITE);
        } else {
          door::ANSIColor c = stringToANSIColor(found);
          textColor.setFg(c.getFg());
        }
      }
    }
    co.pos = 0;
    co.len = 0;
    co.c = c1;

    int tpos = 0;
    for (char const &c : txt) {
      if (option) {
        if (c == '[' or c == ']') {
          if (co.c != c1)
            if (co.len != 0) {
              r.outputs.push_back(co);
              co.reset();
              co.pos = tpos;
            }
          co.c = c1;
          if (c == ']')
            option = false;
        } else {
          if (co.c != c2)
            if (co.len != 0) {
              r.outputs.push_back(co);
              co.reset();
              co.pos = tpos;
            }
          co.c = c2;
        }
      } else {
        if (co.c != textColor)
          if (co.len != 0) {
            r.outputs.push_back(co);
            co.reset();
            co.pos = tpos;
          }
        co.c = textColor;
      }
      co.len++;
      tpos++;
    }
    if (co.len != 0) {
      r.outputs.push_back(co);
    }
    return r;
  };
  return render;
}

/**
 * @brief menu render that sets the text color based on the color found in the
 * text itself.  This finds all colors.
 *
 * @param c1 [] brackets
 * @param c2 text within brackets
 * @param c3 base color give (we set the FG, we use the BG)
 * @return door::renderFunction
 */
door::renderFunction makeColorsRender(door::ANSIColor c1, door::ANSIColor c2,
                                      door::ANSIColor c3) {
  door::renderFunction render = [c1, c2,
                                 c3](const std::string &txt) -> door::Render {
    door::Render r(txt);

    // find all of the words here
    std::vector<std::pair<int, int>> words = find_words(txt);
    auto words_it = words.begin();

    // option is "[?]" part of the string
    bool option = true;

    // I need this mutable
    door::ANSIColor textColor = c3;
    door::ANSIColor normal = c3;

    normal.setFg(door::COLOR::WHITE);
    bool color_word = false;
    std::pair<int, int> word_pair;

#ifdef DEBUG_OUTPUT
    if (get_logger) {
      get_logger() << "makeColorsRender() " << txt << std::endl;
      for (auto word : words) {
        get_logger() << word.first << "," << word.second << std::endl;
      }
    }
#endif

    int tpos = 0;
    for (char const &c : txt) {
      if (option) {
        if (c == '[' or c == ']') {
          r.append(c1);
          if (c == ']')
            option = false;
        } else {
          r.append(c2);
        }
      } else {
        // Ok, we are out of the options now.

        if (color_word) {
          // we're in a COLOR word.
          if (tpos < word_pair.first + word_pair.second)
            r.append(textColor);
          else {
            color_word = false;
            r.append(normal);
          }
        } else {
          // look for COLOR word.
          while ((words_it != words.end()) and (words_it->first < tpos)) {
#ifdef DEBUG_OUTPUT
            if (get_logger) {
              get_logger() << "tpos " << tpos << "(next words_it)" << std::endl;
            }
#endif
            ++words_it;
          }

          if (words_it == words.end()) {
            // we're out.
            r.append(normal);
          } else {
            if (words_it->first == tpos) {
              // start of word -- check it!
              std::string color = txt.substr(words_it->first, words_it->second);
              bool found = false;

              if (!iequals(color, std::string("ALL")))
                for (auto &dc : deck_colors) {
                  if (color.find(dc) != string::npos) {
                    found = true;
                    break;
                  }
                }

#ifdef DEBUG_OUTPUT
              if (get_logger) {
                get_logger() << "word: [" << color << "] : deck_colors "
                             << found << " pos: " << tpos
                             << " word_start: " << words_it->first << std::endl;
              }
#endif

              if (found) {
                door::ANSIColor c = stringToANSIColor(color);
                textColor.setFg(c.getFg());
                // check
                if (textColor.getFg() == textColor.getBg()) {
                  // problem detected!
                  textColor.setBg(door::COLOR::WHITE);
                }
                word_pair = *words_it;
                r.append(textColor);
                color_word = true;
              } else {
                r.append(normal);
              }
            } else {
              r.append(normal);
            }
          }
        }
      }
      ++tpos;
    }
    return r;
  };
  return render;
}

// convert a string to an option
// an option to the string to store
// This needs to be updated to use deck_colors.
door::ANSIColor stringToANSIColor(std::string colorCode) {
  std::map<std::string, door::ANSIColor> codeMap = {
      {std::string("BLUE"), door::ANSIColor(door::COLOR::BLUE)},
      {std::string("BROWN"), door::ANSIColor(door::COLOR::BROWN)},
      {std::string("RED"), door::ANSIColor(door::COLOR::RED)},
      {std::string("CYAN"), door::ANSIColor(door::COLOR::CYAN)},
      {std::string("GREEN"), door::ANSIColor(door::COLOR::GREEN)},
      {std::string("MAGENTA"), door::ANSIColor(door::COLOR::MAGENTA)},
      {std::string("WHITE"), door::ANSIColor(door::COLOR::WHITE)}};

  std::string code = colorCode;
  string_toupper(code);

  auto iter = codeMap.find(code);
  if (iter != codeMap.end()) {
    return iter->second;
  }

  // And if it doesn't match, and isn't ALL ... ?
  // if (code.compare("ALL") == 0) {
  std::random_device dev;
  std::mt19937_64 rng(dev());

  std::uniform_int_distribution<size_t> idDist(0, codeMap.size() - 1);
  iter = codeMap.begin();
  std::advance(iter, idDist(rng));

  return iter->second;
  // }
}

std::string stringFromColorOptions(int opt) { return deck_colors[opt]; }

door::Panel make_about(void) {
  const int W = 60;
  door::Panel about(W);
  about.setStyle(door::BorderStyle::SINGLE_DOUBLE);
  about.setColor(door::ANSIColor(door::COLOR::YELLOW, door::COLOR::BLUE,
                                 door::ATTR::BOLD));

  about.addLine(std::make_unique<door::Line>(
      "About This Door", W,
      door::ANSIColor(door::COLOR::CYAN, door::COLOR::BLUE, door::ATTR::BOLD)));

  std::unique_ptr<door::Line> spacer = about.spacer_line(true); // false);
  about.addLine(std::move(spacer));

  /*
  123456789012345678901234567890123456789012345678901234567890-60
  This door was written by Bugz.

  It is written in c++, supports CP437 and unicode, and can
  adjust to any size screen, only supports Linux, and replaces
  opendoors.

  It's written in c++, and replaces the outdated opendoors
  library.

   */
  door::ANSIColor c_color =
      door::ANSIColor(door::COLOR::WHITE, door::COLOR::BLUE, door::ATTR::BOLD);
  about.addLine(
      std::make_unique<door::Line>(SPACEACE " v" SPACEACE_VERSION, W, c_color));
  std::string copyright = SPACEACE_COPYRIGHT;
  if (door::unicode) {
    replace(copyright, "(C)", "\u00a9");
  }

  about.addLine(std::make_unique<door::Line>(copyright, W, c_color));
  about.addLine(std::make_unique<door::Line>("", W));
  about.addLine(
      std::make_unique<door::Line>("This door was written by Bugz.", W));
  about.addLine(std::make_unique<door::Line>("", W));
  about.addLine(std::make_unique<door::Line>(
      "It is written in door++ using c++, understands CP437 and", W));
  about.addLine(std::make_unique<door::Line>(
      "unicode, adapts to screen sizes, and runs on Linux.", W));
  return about;
}

door::Panel make_help(void) {
  const int W = 60;
  door::Panel help(W);
  help.setStyle(door::BorderStyle::DOUBLE_SINGLE);
  help.setColor(door::ANSIColor(door::COLOR::YELLOW, door::COLOR::BLUE,
                                door::ATTR::BOLD));
  help.addLine(std::make_unique<door::Line>(
      "Help", W,
      door::ANSIColor(door::COLOR::CYAN, door::COLOR::BLUE, door::ATTR::BOLD)));
  std::unique_ptr<door::Line> spacer = help.spacer_line(false);
  help.addLine(std::move(spacer));

  // help.addLine(p.spacer_line)
  /*
  123456789012345678901234567890123456789012345678901234567890-60

  Use Left/Right arrow keys, or 4/6 keys to move marker.
  Select card to play with Space or 5.  Enter draws another card.
  A card can play if it is higher or lower in rank by 1.

  Example: A Jack could select either a Ten or a Queen.
  Example: A King could select either an Ace or a Queen.


  The more cards in your streak, the more points you earn.

   */

  door::ANSIColor c_color =
      door::ANSIColor(door::COLOR::WHITE, door::COLOR::BLUE, door::ATTR::BOLD);

  help.addLine(
      std::make_unique<door::Line>(SPACEACE " v" SPACEACE_VERSION, W, c_color));
  std::string copyright = SPACEACE_COPYRIGHT;
  if (door::unicode) {
    replace(copyright, "(C)", "\u00a9");
  }

  help.addLine(std::make_unique<door::Line>(copyright, W, c_color));
  help.addLine(std::make_unique<door::Line>("", W));
  help.addLine(std::make_unique<door::Line>(
      "Use Left/Right arrow keys, or 4/6 keys to move marker.", W));
  help.addLine(std::make_unique<door::Line>(
      "The marker wraps around the sides of the screen.", W));
  help.addLine(std::make_unique<door::Line>("", W));
  help.addLine(
      std::make_unique<door::Line>("Select card to play with Space or 5.", W));

  help.addLine(std::make_unique<door::Line>(
      "A card can play if it is higher or lower in rank by 1.", W));
  help.addLine(std::make_unique<door::Line>("", W));
  help.addLine(std::make_unique<door::Line>("Enter draws another card.", W));
  help.addLine(std::make_unique<door::Line>("", W));
  help.addLine(std::make_unique<door::Line>(
      "Example: A Jack could select either a Ten or a Queen.", W));
  help.addLine(std::make_unique<door::Line>(
      "Example: A King could select either an Ace or a Queen.", W));
  help.addLine(std::make_unique<door::Line>("", W));
  help.addLine(std::make_unique<door::Line>(
      "The more cards in your streak, the more points you earn.", W));
  return help;
}

/**
 * @brief Display starfield
 *
 * This displays stars placed randomly on a blank screen.
 * There are two chars for stars, and there are two colors for them.
 *
 * This has been updated, the stars are stored in a set.  The starfield output
 * is optimized.  If another star is on the same row, we use spaces or ANSI
 * Cursor Forward to set the next position.
 *
 * @param door
 * @param rng
 */
void display_starfield(door::Door &door, std::mt19937 &rng) {
  door << door::reset << door::cls;

  int mx = door.width;
  int my = door.height;

  // display starfield
  const char *stars[2];

  stars[0] = ".";
  if (door::unicode) {
    stars[1] = "\u2219"; // "\u00b7";

  } else {
    stars[1] = "\xf9"; // "\xfa";
  };

  struct star_pos {
    int x;
    int y;

    /**
     * @brief Provide less than operator.
     *
     * This will allow the star_pos to be stored sorted (top->bottom,
     * left->right) in a set.
     *
     * @param rhs
     * @return true
     * @return false
     */
    bool operator<(const star_pos rhs) const {
      if (rhs.y > y)
        return true;
      if (rhs.y == y) {
        if (rhs.x > x)
          return true;
        return false;
      }
      return false;
    }
  };

  {
    // Make uniform random distribution between 1 and MAX screen size X/Y
    std::uniform_int_distribution<int> uni_x(1, mx);
    std::uniform_int_distribution<int> uni_y(1, my);

    door::ANSIColor white(door::COLOR::WHITE);
    door::ANSIColor dark(door::COLOR::BLACK, door::ATTR::BRIGHT);

    // 10 is too many, 100 is too few. 40 looks ok.
    int MAX_STARS = ((mx * my) / 40);
    // door.log() << "Generating starmap using " << mx << "," << my << " : "
    //           << MAX_STARS << " stars." << std::endl;

    std::set<star_pos> sky;

    for (int i = 0; i < MAX_STARS; i++) {
      star_pos pos;
      bool valid;
      do {
        pos.x = uni_x(rng);
        pos.y = uni_y(rng);
        auto ret = sky.insert(pos);
        // was insert a success?  (not a duplicate)
        valid = ret.second;
      } while (!valid);
    }

    int i = 0;
    star_pos last_pos;

    for (auto &pos : sky) {
      bool use_goto = true;

      if (i != 0) {
        // check last_pos to current position
        if (pos.y == last_pos.y) {
          // Ok, same row -- try some optimizations
          int dx = pos.x - last_pos.x;
          if (dx == 0) {
            use_goto = false;
          } else {
            if (dx < 5) {
              door << std::string(dx, ' ');
              use_goto = false;
            } else {
              // Use ANSI Cursor Forward
              door << "\x1b[" << dx << "C";
              use_goto = false;
            }
          }
        }
      }

      if (use_goto) {
        door::Goto star_at(pos.x, pos.y);
        door << star_at;
      }

      if (i % 5 < 2)
        door << dark;
      else
        door << white;

      if (i % 2 == 0)
        door << stars[0];
      else
        door << stars[1];

      ++i;
      last_pos = pos;
      last_pos.x++; // star output moves us by one.
    }
  }
}

void display_space_ace(door::Door &door) {
  int mx = door.width;
  int my = door.height;

  // space_ace is 72 chars wide, 6 high
  int sa_x = (mx - 72) / 2;
  int sa_y = (my - 6) / 2;

  // output the SpaceAce logo -- centered!
  for (const auto s : space) {
    door::Goto sa_at(sa_x, sa_y);
    door << sa_at;
    if (door::unicode) {
      std::string unicode;
      door::cp437toUnicode(s, unicode);
      door << unicode; // << door::nl;
    } else
      door << s; // << door::nl;
    sa_y++;
  }
  // pause 5 seconds so they can enjoy our awesome logo -- if they want.
  door.sleep_key(5);
}

void display_starfield_space_ace(door::Door &door, std::mt19937 &rng) {
  // mx = door.width;
  // my = door.height;

  display_starfield(door, rng);
  display_space_ace(door);
  door << door::reset;
}

door::Panel make_timeout(int mx, int my) {
  door::ANSIColor yellowred =
      door::ANSIColor(door::COLOR::YELLOW, door::COLOR::RED, door::ATTR::BOLD);

  std::string line_text("Sorry, you've been inactive for too long.");
  int msgWidth = line_text.length() + (2 * 3); // + padding * 2
  door::Panel timeout((mx - (msgWidth)) / 2, my / 2 + 4, msgWidth);
  // place.setTitle(std::make_unique<door::Line>(title), 1);
  timeout.setStyle(door::BorderStyle::DOUBLE);
  timeout.setColor(yellowred);

  door::Line base(line_text);
  base.setColor(yellowred);
  std::string pad1(3, ' ');

  /*
      std::string pad1(3, '\xb0');
      if (door::unicode) {
        std::string unicode;
        door::cp437toUnicode(pad1.c_str(), unicode);
        pad1 = unicode;
      }
  */

  base.setPadding(pad1, yellowred);
  // base.setColor(door::ANSIColor(door::COLOR::GREEN, door::COLOR::BLACK));
  std::unique_ptr<door::Line> stuff = std::make_unique<door::Line>(base);

  timeout.addLine(std::make_unique<door::Line>(base));
  return timeout;
}

door::Panel make_notime(int mx, int my) {
  door::ANSIColor yellowred =
      door::ANSIColor(door::COLOR::YELLOW, door::COLOR::RED, door::ATTR::BOLD);

  std::string line_text("Sorry, you've used up all your time for today.");
  int msgWidth = line_text.length() + (2 * 3); // + padding * 2
  door::Panel timeout((mx - (msgWidth)) / 2, my / 2 + 4, msgWidth);
  timeout.setStyle(door::BorderStyle::DOUBLE);
  timeout.setColor(yellowred);

  door::Line base(line_text);
  base.setColor(yellowred);
  std::string pad1(3, ' ');

  /*
      std::string pad1(3, '\xb0');
      if (door::unicode) {
        std::string unicode;
        door::cp437toUnicode(pad1.c_str(), unicode);
        pad1 = unicode;
      }
  */

  base.setPadding(pad1, yellowred);
  std::unique_ptr<door::Line> stuff = std::make_unique<door::Line>(base);
  timeout.addLine(std::make_unique<door::Line>(base));
  return timeout;
}

door::Menu make_main_menu(void) {
  door::Menu m(5, 5, 25);
  door::Line mtitle(SPACEACE " Main Menu");
  door::ANSIColor border_color(door::COLOR::CYAN, door::COLOR::BLUE);
  door::ANSIColor title_color(door::COLOR::CYAN, door::COLOR::BLUE,
                              door::ATTR::BOLD);
  m.setColor(border_color);
  mtitle.setColor(title_color);
  mtitle.setPadding(" ", title_color);

  m.setTitle(std::make_unique<door::Line>(mtitle), 1);

  // m.setColorizer(true,
  m.setRender(true, door::Menu::makeRender(
                        door::ANSIColor(door::COLOR::CYAN, door::ATTR::BOLD),
                        door::ANSIColor(door::COLOR::BLUE, door::ATTR::BOLD),
                        door::ANSIColor(door::COLOR::CYAN, door::ATTR::BOLD),
                        door::ANSIColor(door::COLOR::BLUE, door::ATTR::BOLD)));
  // m.setColorizer(false,
  m.setRender(false, door::Menu::makeRender(
                         door::ANSIColor(door::COLOR::YELLOW, door::COLOR::BLUE,
                                         door::ATTR::BOLD),
                         door::ANSIColor(door::COLOR::WHITE, door::COLOR::BLUE,
                                         door::ATTR::BOLD),
                         door::ANSIColor(door::COLOR::YELLOW, door::COLOR::BLUE,
                                         door::ATTR::BOLD),
                         door::ANSIColor(door::COLOR::CYAN, door::COLOR::BLUE,
                                         door::ATTR::BOLD)));

  m.addSelection('P', "Play Cards");
  m.addSelection('S', "View Scores");
  m.addSelection('C', "Configure");
  m.addSelection('H', "Help");
  m.addSelection('A', "About this game");
  m.addSelection('Q', "Quit");

  return m;
}

door::Menu make_config_menu(void) {
  door::Menu m(5, 5, 31);
  door::Line mtitle(SPACEACE " Configuration Menu");
  door::ANSIColor border_color(door::COLOR::CYAN, door::COLOR::BLUE);
  door::ANSIColor title_color(door::COLOR::CYAN, door::COLOR::BLUE,
                              door::ATTR::BOLD);
  m.setColor(border_color);
  mtitle.setColor(title_color);
  mtitle.setPadding(" ", title_color);

  m.setTitle(std::make_unique<door::Line>(mtitle), 1);

  // m.setColorizer(true,
  m.setRender(true, door::Menu::makeRender(
                        door::ANSIColor(door::COLOR::CYAN, door::ATTR::BOLD),
                        door::ANSIColor(door::COLOR::BLUE, door::ATTR::BOLD),
                        door::ANSIColor(door::COLOR::CYAN, door::ATTR::BOLD),
                        door::ANSIColor(door::COLOR::BLUE, door::ATTR::BOLD)));
  // m.setColorizer(false,
  m.setRender(false, door::Menu::makeRender(
                         door::ANSIColor(door::COLOR::YELLOW, door::COLOR::BLUE,
                                         door::ATTR::BOLD),
                         door::ANSIColor(door::COLOR::WHITE, door::COLOR::BLUE,
                                         door::ATTR::BOLD),
                         door::ANSIColor(door::COLOR::YELLOW, door::COLOR::BLUE,
                                         door::ATTR::BOLD),
                         door::ANSIColor(door::COLOR::CYAN, door::COLOR::BLUE,
                                         door::ATTR::BOLD)));

  m.addSelection('D', "Deck Colors");
  m.addSelection('V', "View Settings");
  m.addSelection('Q', "Quit");

  return m;
}

/*
// all the possible deck colors
vector<std::string> deck_colors = {std::string("All"),     std::string("Blue"),
                                   std::string("Cyan"),    std::string("Green"),
                                   std::string("Magenta"), std::string("Red")};
*/

door::Menu make_deck_menu(void) {
  door::Menu m(5, 5, 31);
  door::Line mtitle(SPACEACE " Deck Menu");
  door::ANSIColor border_color(door::COLOR::CYAN, door::COLOR::BLUE);
  door::ANSIColor title_color(door::COLOR::CYAN, door::COLOR::BLUE,
                              door::ATTR::BOLD);
  m.setColor(border_color);
  mtitle.setColor(title_color);
  mtitle.setPadding(" ", title_color);

  m.setTitle(std::make_unique<door::Line>(mtitle), 1);

  m.setRender(true, makeColorsRender(
                        door::ANSIColor(door::COLOR::CYAN, door::ATTR::BOLD),
                        door::ANSIColor(door::COLOR::BLUE, door::ATTR::BOLD),
                        door::ANSIColor(door::COLOR::CYAN, door::ATTR::BOLD)));
  m.setRender(false, makeColorsRender(
                         door::ANSIColor(door::COLOR::YELLOW, door::COLOR::BLUE,
                                         door::ATTR::BOLD),
                         door::ANSIColor(door::COLOR::WHITE, door::COLOR::BLUE,
                                         door::ATTR::BOLD),
                         door::ANSIColor(door::COLOR::YELLOW, door::COLOR::BLUE,
                                         door::ATTR::BOLD)));
  // build the menu options from the colors.  First character = single letter
  // option trigger.
  for (auto iter = deck_colors.begin(); iter != deck_colors.end(); ++iter) {
    char c = (*iter)[0];
    m.addSelection(c, (*iter).c_str());
  }

  // This verifies the render routine is working great.
  // Now, I just need to support multiple color selections like this.
  // m.addSelection('S', "Yellow Red Blue Green Cyan");
  /*
  m.addSelection('A', "All");
  m.addSelection('B', "Blue");
  m.addSelection('C', "Cyan");
  m.addSelection('G', "Green");
  m.addSelection('M', "Magenta");
  m.addSelection('R', "Red");
  */

  return m;
}

#include "play.h" // statusValue

door::Panel make_sysop_config(void) {
  const int W = 35;
  door::Panel p(5, 5, W);
  p.setStyle(door::BorderStyle::DOUBLE);
  door::ANSIColor panel_color =
      door::ANSIColor(door::COLOR::CYAN, door::COLOR::BLUE, door::ATTR::BOLD);
  p.setColor(panel_color);
  p.addLine(std::make_unique<door::Line>(
      "Game Settings - SysOp Configurable", W,
      door::ANSIColor(door::COLOR::GREEN, door::COLOR::BLUE,
                      door::ATTR::BOLD)));
  std::unique_ptr<door::Line> spacer = p.spacer_line(false);
  spacer->setColor(panel_color);
  p.addLine(std::move(spacer));

  ostringstream oss;

  door::renderFunction rf = statusValue(
      door::ANSIColor(door::COLOR::YELLOW, door::COLOR::BLUE, door::ATTR::BOLD),
      door::ANSIColor(door::COLOR::GREEN, door::COLOR::BLUE, door::ATTR::BOLD));

  for (auto cfg : config) {
    std::string key = cfg.first.as<std::string>();
    if (key[0] == '_')
      continue;
    // Replace _ with space.
    while (replace(key, "_", " ")) {
    };
    std::string value = cfg.second.as<std::string>();
    oss << std::setw(20) << key << " : " << value;
    p.addLine(std::make_unique<door::Line>(oss.str(), W, rf));
    oss.clear();
    oss.str(std::string());
  }
  return p;
}