#include "deck.h"

#include <algorithm>
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

Deck::~Deck() {
  /*
  for (auto c : cards) {
    delete c;
  }
  cards.clear();
  for (auto b : backs) {
    delete b;
  }
  backs.clear();
  for (auto m : mark) {
    delete m;
  }
  mark.clear();
  */
}

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
 * @brief Which card (if any) is unblocked by this card
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

vector<std::string> deck_colors = {std::string("All"),     std::string("Blue"),
                                   std::string("Cyan"),    std::string("Green"),
                                   std::string("Magenta"), std::string("Red")};
/**
 * @brief menu render that sets the text color based on the color found in the
 * text itself.
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

// convert a string to an option
// an option to the string to store
// This needs to be updated to use deck_colors.
door::ANSIColor stringToANSIColor(std::string colorCode) {
  std::map<std::string, door::ANSIColor> codeMap = {
      {std::string("BLUE"), door::ANSIColor(door::COLOR::BLUE)},
      {std::string("RED"), door::ANSIColor(door::COLOR::RED)},
      {std::string("CYAN"), door::ANSIColor(door::COLOR::CYAN)},
      {std::string("GREEN"), door::ANSIColor(door::COLOR::GREEN)},
      {std::string("MAGENTA"), door::ANSIColor(door::COLOR::MAGENTA)}};

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
void string_toupper(std::string &str) {
  std::transform(str.begin(), str.end(), str.begin(), ::toupper);
}
