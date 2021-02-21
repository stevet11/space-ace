#include "deck.h"

#include <sstream>

Deck::Deck() {
  cardback = door::ANSIColor(door::COLOR::RED);
  init();
}

Deck::Deck(door::ANSIColor backcolor) : cardback{backcolor} { init(); }

void Deck::init(void) {
  for (int i = 0; i < 52; ++i) {
    cards.push_back(card_of(i));
  }
  // 0 = BLANK, 1-4 levels
  for (int i = 0; i < 5; ++i) {
    backs.push_back(back_of(i));
  }
}

Deck::~Deck() {
  for (auto c : cards) {
    delete c;
  }
  cards.clear();
  for (auto b : backs) {
    delete b;
  }
  backs.clear();
}

int Deck::is_suit(int c) { return c / 13; }
int Deck::is_rank(int c) { return c % 13; }

char Deck::rank_symbol(int c) {
  switch (c) {
  case 0:
    return 'A';
  case 1:
  case 2:
  case 3:
  case 4:
  case 5:
  case 6:
  case 7:
  case 8:
    return char(c + 0x30 + 1);
  case 9:
    return 'T';
  case 10:
    return 'J';
  case 11:
    return 'Q';
  case 12:
    return 'K';
  }
  return '?';
}

std::string Deck::suit_symbol(int c) {
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
  }
  return std::string("!", 1);
}

door::Panel *Deck::card_of(int c) {
  int suit = is_suit(c);
  int rank = is_rank(c);
  bool is_red = (suit < 2);
  door::ANSIColor color;

  if (is_red) {
    color = door::ANSIColor(door::COLOR::RED, door::COLOR::WHITE);
  } else {
    color = door::ANSIColor(door::COLOR::BLACK, door::COLOR::WHITE);
  }
  door::Panel *p = new door::Panel(0, 0, 5);
  // setColor sets border_color.  NOT WHAT I WANT.
  // p->setColor(color);
  char r = rank_symbol(rank);
  std::string s = suit_symbol(suit);

  // build lines
  std::ostringstream oss;
  oss << r << s << "   ";
  std::string str = oss.str();
  p->addLine(std::make_unique<door::Line>(str, 5, color));
  oss.str(std::string());
  oss.clear();
  oss << "  " << s << "  ";
  str = oss.str();
  p->addLine(std::make_unique<door::Line>(str, 5, color));
  oss.str(std::string());
  oss.clear();
  oss << "   " << s << r;
  str = oss.str();
  p->addLine(std::make_unique<door::Line>(str, 5, color));
  oss.str(std::string());
  oss.clear();

  return p;
}

std::string Deck::back_char(int level) {
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

door::Panel *Deck::back_of(int level) {
  // using: \xb0, 0xb1, 0xb2, 0xdb
  // OR:    \u2591, \u2592, \u2593, \u2588

  // door::ANSIColor color(door::COLOR::RED, door::COLOR::BLACK);
  door::Panel *p = new door::Panel(0, 0, 5);
  std::string c = back_char(level);
  std::string l = c + c + c + c + c;
  p->addLine(std::make_unique<door::Line>(l, 5, cardback));
  p->addLine(std::make_unique<door::Line>(l, 5, cardback));
  p->addLine(std::make_unique<door::Line>(l, 5, cardback));
  return p;
}

void Deck::part(int x, int y, door::Door &d, int level, bool left) {
  // Render part of the back of a card.
  y += 2;
  if (!left) {
    x += 2;
  }
  std::string c = back_char(level);
  std::string l = c + c + c;
  door::Goto g(x, y);
  d << g << cardback << l;
}

door::Panel *Deck::card(int c) { return cards[c]; }

door::Panel *Deck::back(int level) { return backs[level]; }

/*
          1         2         3         4         5         6
0123456789012345678901234567890123456789012345678901234567890
         ░░░░░             ░░░░░             ░░░░░
         ░░░░░             ░░░░░             ░░░░░
      ▒▒▒▒▒░▒▒▒▒▒       #####░#####       #####░#####
      ▒▒▒▒▒ ▒▒▒▒▒       ##### #####       ##### #####
   ▓▓▓▓▓▒▓▓▓▓▓▒▓▓▓▓▓ #####=#####=##### #####=#####=#####
   ▓▓▓▓▓ ▓▓▓▓▓ ▓▓▓▓▓ ##### ##### ##### ##### ##### #####
█████▓█████▓█████▓#####=#####=#####=#####=#####=#####=#####
█████ █████ █████ ##### ##### ##### ##### ##### ##### #####
█████ █████ █████ ##### ##### ##### ##### ##### ##### #####
*/
void cardgo(int pos, int &x, int &y, int &level) {
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

  if (pos < 3) {
    // top
    level = 1;
    y = (level - 1) * 2 + 1;
    x = (pos)*18 + 10;
    return;
  } else {
    pos -= 3;
  }
  if (pos < 6) {
    level = 2;
    y = (level - 1) * 2 + 1;
    int group = (pos) / 2;
    x = (pos)*6 + (group * 6) + 7;
    return;
  } else {
    pos -= 6;
  }
  if (pos < 9) {
    level = 3;
    y = (level - 1) * 2 + 1;
    x = (pos)*6 + 4;
    return;
  } else {
    pos -= 9;
  }
  if (pos < 10) {
    level = 4;
    y = (level - 1) * 2 + 1;
    x = (pos)*6 + 1;
    return;
  } else {
    // something is wrong.
    y = -1;
    x = -1;
    level = -1;
  }
}