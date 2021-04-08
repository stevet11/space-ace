#include "door.h"
#include "space.h"
#include <chrono>  // chrono::system_clock
#include <ctime>   // localtime
#include <iomanip> // put_time
#include <iostream>
#include <random>
#include <string>

#include "db.h"
#include "deck.h"
#include "version.h"
#include <algorithm> // transform

void string_toupper(std::string &str) {
  std::transform(str.begin(), str.end(), str.begin(), ::toupper);
}

bool replace(std::string &str, const std::string &from, const std::string &to) {
  size_t start_pos = str.find(from);
  if (start_pos == std::string::npos)
    return false;
  str.replace(start_pos, from.length(), to);
  return true;
}

bool replace(std::string &str, const char *from, const char *to) {
  size_t start_pos = str.find(from);
  if (start_pos == std::string::npos)
    return false;
  str.replace(start_pos, strlen(from), to);
  return true;
}

door::ANSIColor from_string(std::string colorCode);

std::function<std::ofstream &(void)> get_logger;

unsigned long score = 0;
int hand = 1;
int total_hands = 3;
int card_number = 28;
int current_streak = 0;
int best_streak = 0;
int active_card = 23;

std::chrono::_V2::system_clock::time_point play_day;

/*

Cards:

4 layers deep.

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

Card layout.  Actual cards are 3 lines thick.

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

►, ◄, ▲, ▼
\x10, \x11, \x1e, \x1f
\u25ba, \u25c4, \u25b2, \u25bc


The Name is <- 6 to the left.

# is back of card.  = is upper card showing between.

There's no fancy anything here.  Cards overlap the last
line of the previous line/card.


 */

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

door::renderFunction statusValue(door::ANSIColor status,
                                 door::ANSIColor value) {
  door::renderFunction rf = [status,
                             value](const std::string &txt) -> door::Render {
    door::Render r(txt);
    door::ColorOutput co;

    co.pos = 0;
    co.len = 0;
    co.c = status;

    size_t pos = txt.find(':');
    if (pos == std::string::npos) {
      // failed to find :, render digits/numbers in value color
      int tpos = 0;
      for (char const &c : txt) {
        if (std::isdigit(c)) {
          if (co.c != value) {
            r.outputs.push_back(co);
            co.reset();
            co.pos = tpos;
            co.c = value;
          }
        } else {
          if (co.c != status) {
            r.outputs.push_back(co);
            co.reset();
            co.pos = tpos;
            co.c = status;
          }
        }
        co.len++;
        tpos++;
      }
      if (co.len != 0)
        r.outputs.push_back(co);
    } else {
      pos++; // Have : in status color
      co.len = pos;
      r.outputs.push_back(co);
      co.reset();
      co.pos = pos;
      co.c = value;
      co.len = txt.length() - pos;
      r.outputs.push_back(co);
    }

    return r;
  };
  return rf;
}

/*
door::renderFunction rStatus = [](const std::string &txt) -> door::Render {
  door::Render r(txt);
  door::ColorOutput co;

  // default colors STATUS: value
  door::ANSIColor status(door::COLOR::BLUE, door::ATTR::BOLD);
  door::ANSIColor value(door::COLOR::YELLOW, door::ATTR::BOLD);

  co.pos = 0;
  co.len = 0;
  co.c = status;

  size_t pos = txt.find(':');
  if (pos == std::string::npos) {
    // failed to find - use entire string as status color.
    co.len = txt.length();
    r.outputs.push_back(co);
  } else {
    pos++; // Have : in status color
    co.len = pos;
    r.outputs.push_back(co);
    co.reset();
    co.pos = pos;
    co.c = value;
    co.len = txt.length() - pos;
    r.outputs.push_back(co);
  }

  return r;
};
*/

std::string return_current_time_and_date() {
  auto now = std::chrono::system_clock::now();
  auto in_time_t = std::chrono::system_clock::to_time_t(now);

  std::stringstream ss;
  // ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
  ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %r");
  return ss.str();
}

int press_a_key(door::Door &door) {
  door << door::reset << "Press a key to continue...";
  int r = door.sleep_key(door.inactivity);
  door << door::nl;
  return r;
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
  m.addSelection('Q', "Quit");

  return m;
}

// all the possible deck colors
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
          door::ANSIColor c = from_string(found);
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

  m.setRender(true, makeColorRender(
                        door::ANSIColor(door::COLOR::CYAN, door::ATTR::BOLD),
                        door::ANSIColor(door::COLOR::BLUE, door::ATTR::BOLD),
                        door::ANSIColor(door::COLOR::CYAN, door::ATTR::BOLD)));
  m.setRender(false, makeColorRender(
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

// DOES THIS WORK?
bool iequals(const string &a, const string &b) {
  unsigned int sz = a.size();
  if (b.size() != sz)
    return false;
  for (unsigned int i = 0; i < sz; ++i)
    if (tolower(a[i]) != tolower(b[i]))
      return false;
  return true;
}

// convert a string to an option
// an option to the string to store
// This needs to be updated to use deck_colors.
door::ANSIColor from_string(std::string colorCode) {
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

// This does not seem to be working.  I keep getting zero.
int opt_from_string(std::string colorCode) {
  for (std::size_t pos = 0; pos != deck_colors.size(); ++pos) {
    // if (caseInsensitiveStringCompare(colorCode, deck_colors[pos]) == 0) {
    if (iequals(colorCode, deck_colors[pos])) {
      return pos;
    }
  }
  return 0;
}

std::string from_color_option(int opt) { return deck_colors[opt]; }

int configure(door::Door &door, DBData &db) {
  auto menu = make_config_menu();
  int r = 0;

  while (r >= 0) {
    r = menu.choose(door);
    if (r > 0) {
      door << door::reset << door::cls;
      char c = menu.which(r - 1);
      if (c == 'D') {
        // Ok, deck colors
        // get default
        std::string key("DeckColor");
        std::string currentDefault = db.getSetting(key, std::string("ALL"));
        int currentOpt = opt_from_string(currentDefault);

        door << door::reset << door::cls;
        auto deck = make_deck_menu();
        deck.defaultSelection(currentOpt);
        int newOpt = deck.choose(door);
        door << door::reset << door::cls;

        if (newOpt >= 0) {
          newOpt--;
          std::string newColor = from_color_option(newOpt);
          if (newOpt != currentOpt) {
            door.log() << key << " was " << currentDefault << ", " << currentOpt
                       << ". Now " << newColor << ", " << newOpt << std::endl;
            db.setSetting(key, newColor);
          }
        }
      }
      if (c == 'Q') {
        return r;
      }
    }
  }
  return r;
}

door::Panel make_score_panel(door::Door &door) {
  const int W = 25;
  door::Panel p(W);
  p.setStyle(door::BorderStyle::NONE);
  door::ANSIColor statusColor(door::COLOR::WHITE, door::COLOR::BLUE,
                              door::ATTR::BOLD);
  door::ANSIColor valueColor(door::COLOR::YELLOW, door::COLOR::BLUE,
                             door::ATTR::BOLD);
  door::renderFunction svRender = statusValue(statusColor, valueColor);
  // or use renderStatus as defined above.
  // We'll stick with these for now.

  {
    std::string userString = "Name: ";
    userString += door.username;
    door::Line username(userString, W);
    username.setRender(svRender);
    p.addLine(std::make_unique<door::Line>(username));
  }
  {
    door::updateFunction scoreUpdate = [](void) -> std::string {
      std::string text = "Score: ";
      text.append(std::to_string(score));
      return text;
    };
    std::string scoreString = scoreUpdate();
    door::Line score(scoreString, W);
    score.setRender(svRender);
    score.setUpdater(scoreUpdate);
    p.addLine(std::make_unique<door::Line>(score));
  }
  {
    door::updateFunction timeUpdate = [&door](void) -> std::string {
      std::stringstream ss;
      std::string text;
      ss << "Time used: " << setw(3) << door.time_used << " / " << setw(3)
         << door.time_left;
      text = ss.str();
      return text;
    };
    std::string timeString = timeUpdate();
    door::Line time(timeString, W);
    time.setRender(svRender);
    time.setUpdater(timeUpdate);
    p.addLine(std::make_unique<door::Line>(time));
  }
  {
    door::updateFunction handUpdate = [](void) -> std::string {
      std::string text = "Playing Hand ";
      text.append(std::to_string(hand));
      text.append(" of ");
      text.append(std::to_string(total_hands));
      return text;
    };
    std::string handString = handUpdate();
    door::Line hands(handString, W);
    hands.setRender(svRender);
    hands.setUpdater(handUpdate);
    p.addLine(std::make_unique<door::Line>(hands));
  }

  return p;
}

door::Panel make_streak_panel(void) {
  const int W = 20;
  door::Panel p(W);
  p.setStyle(door::BorderStyle::NONE);
  door::ANSIColor statusColor(door::COLOR::WHITE, door::COLOR::BLUE,
                              door::ATTR::BOLD);
  door::ANSIColor valueColor(door::COLOR::YELLOW, door::COLOR::BLUE,
                             door::ATTR::BOLD);
  door::renderFunction svRender = statusValue(statusColor, valueColor);

  {
    std::string text = "Playing: ";
    auto in_time_t = std::chrono::system_clock::to_time_t(play_day);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%B %d");
    text.append(ss.str());
    door::Line current(text, W);
    current.setRender(svRender);
    p.addLine(std::make_unique<door::Line>(current));
  }
  {
    door::updateFunction currentUpdate = [](void) -> std::string {
      std::string text = "Current Streak: ";
      text.append(std::to_string(current_streak));
      return text;
    };
    std::string currentString = currentUpdate();
    door::Line current(currentString, W);
    current.setRender(svRender);
    current.setUpdater(currentUpdate);
    p.addLine(std::make_unique<door::Line>(current));
  }
  {
    door::updateFunction currentUpdate = [](void) -> std::string {
      std::string text = "Longest Streak: ";
      text.append(std::to_string(best_streak));
      return text;
    };
    std::string currentString = currentUpdate();
    door::Line current(currentString, W);
    current.setRender(svRender);
    current.setUpdater(currentUpdate);
    p.addLine(std::make_unique<door::Line>(current));
  }

  return p;
}

door::Panel make_left_panel(void) {
  const int W = 13;
  door::Panel p(W);
  p.setStyle(door::BorderStyle::NONE);
  door::ANSIColor statusColor(door::COLOR::WHITE, door::COLOR::BLUE,
                              door::ATTR::BOLD);
  door::ANSIColor valueColor(door::COLOR::YELLOW, door::COLOR::BLUE,
                             door::ATTR::BOLD);
  door::renderFunction svRender = statusValue(statusColor, valueColor);

  {
    door::updateFunction cardsleftUpdate = [](void) -> std::string {
      std::string text = "Cards left:";
      text.append(std::to_string(51 - card_number));
      return text;
    };
    std::string cardsleftString = "Cards left:--";
    door::Line cardsleft(cardsleftString, W);
    cardsleft.setRender(svRender);
    cardsleft.setUpdater(cardsleftUpdate);
    p.addLine(std::make_unique<door::Line>(cardsleft));
  }
  return p;
}

door::renderFunction commandLineRender(door::ANSIColor bracket,
                                       door::ANSIColor inner,
                                       door::ANSIColor outer) {
  door::renderFunction rf = [bracket, inner,
                             outer](const std::string &txt) -> door::Render {
    door::Render r(txt);
    door::ColorOutput co;

    co.pos = 0;
    co.len = 0;
    co.c = outer;
    bool inOuter = true;

    int tpos = 0;
    for (char const &c : txt) {
      if (inOuter) {

        // we're in the outer text
        if (co.c != outer) {
          if (co.len != 0) {
            r.outputs.push_back(co);
            co.reset();
            co.pos = tpos;
          }
          co.c = outer;
        }

        // check for [
        if (c == '[') {
          if (co.len != 0) {
            r.outputs.push_back(co);
            co.reset();
            co.pos = tpos;
          }
          inOuter = false;
          co.c = bracket;
        }
      } else {
        // We're not in the outer.

        if (co.c != inner) {
          if (co.len != 0) {
            r.outputs.push_back(co);
            co.reset();
            co.pos = tpos;
          }
          co.c = inner;
        }

        if (c == ']') {
          if (co.len != 0) {
            r.outputs.push_back(co);
            co.reset();
            co.pos = tpos;
          }
          inOuter = true;
          co.c = bracket;
        }
      }
      co.len++;
      tpos++;
    }
    if (co.len != 0)
      r.outputs.push_back(co);
    return r;
  };
  return rf;
}

door::Panel make_command_panel(void) {
  const int W = 76;
  door::Panel p(W);
  p.setStyle(door::BorderStyle::NONE);
  std::string commands;

  if (door::unicode) {
    commands = "[4/\u25c4] Left [6/\u25ba] Right [Space] Play Card [Enter] "
               "Draw [Q]uit "
               "[R]edraw [H]elp";
  } else {
    commands =
        "[4/\x11] Left [6/\x10] Right [Space] Play Card [Enter] Draw [Q]uit "
        "[R]edraw [H]elp";
  }

  door::ANSIColor bracketColor(door::COLOR::YELLOW, door::COLOR::BLUE,
                               door::ATTR::BOLD);
  door::ANSIColor innerColor(door::COLOR::CYAN, door::COLOR::BLUE,
                             door::ATTR::BOLD);
  door::ANSIColor outerColor(door::COLOR::GREEN, door::COLOR::BLUE,
                             door::ATTR::BOLD);

  door::renderFunction cmdRender =
      commandLineRender(bracketColor, innerColor, outerColor);
  door::Line cmd(commands, W);
  cmd.setRender(cmdRender);
  p.addLine(std::make_unique<door::Line>(cmd));
  return p;
}

door::Panel make_tripeaks(void) {
  std::string tripeaksText(" " SPACEACE
                           " - Tri-Peaks Solitaire v" SPACEACE_VERSION " ");
  door::Panel spaceAceTriPeaks(tripeaksText.size());
  spaceAceTriPeaks.setStyle(door::BorderStyle::SINGLE);
  spaceAceTriPeaks.setColor(
      door::ANSIColor(door::COLOR::CYAN, door::COLOR::BLACK));
  spaceAceTriPeaks.addLine(
      std::make_unique<door::Line>(tripeaksText, tripeaksText.size()));
  return spaceAceTriPeaks;
}

int play_cards(door::Door &door, DBData &db, std::mt19937 &rng) {
  int mx = door.width;
  int my = door.height;

  // init these values:
  card_number = 28;
  active_card = 23;
  hand = 1;
  score = 0;
  play_day = std::chrono::system_clock::now();

  // cards color --
  // configured by the player.

  door::ANSIColor deck_color;
  // std::string key("DeckColor");
  const char *key = "DeckColor";
  std::string currentDefault = db.getSetting(key, "ALL");
  door.log() << key << " shows as " << currentDefault << std::endl;
  deck_color = from_string(currentDefault);

  const int height = 3;
  Deck d(deck_color); // , height);
  door::Panel *c;

  // This displays the cards in the upper left corner.
  // We want them center, and down some.

  // const int space = 3;

  // int cards_dealt_width = 59; int cards_dealt_height = 9;
  int game_width;
  int game_height = 20; // 13; // 9;
  {
    int cx, cy, level;
    cardgo(27, cx, cy, level);
    game_width = cx + 5; // card width
  }
  int off_x = (mx - game_width) / 2;
  int off_y = (my - game_height) / 2;
  int true_off_y = off_y;
  // The idea is to see the cards with <<Something Unique to the card game>>,
  // Year, Month, Day, and game (like 1 of 3).
  // This will make the games the same/fair for everyone.

next_hand:

  card_number = 28;
  active_card = 23;
  score = 0;
  off_y = true_off_y;

  door::Panel spaceAceTriPeaks = make_tripeaks();
  int tp_off_x = (mx - spaceAceTriPeaks.getWidth()) / 2;
  spaceAceTriPeaks.set(tp_off_x, off_y);

  off_y += 3;

  // figure out what date we're playing / what game we're playing
  time_t tt = std::chrono::system_clock::to_time_t(play_day);
  tm local_tm = *localtime(&tt);
  /*
  std::cout << utc_tm.tm_year + 1900 << '-';
  std::cout << utc_tm.tm_mon + 1 << '-';
  std::cout << utc_tm.tm_mday << ' ';
  std::cout << utc_tm.tm_hour << ':';
  std::cout << utc_tm.tm_min << ':';
  std::cout << utc_tm.tm_sec << '\n';
  */

  std::seed_seq s1{local_tm.tm_year + 1900, local_tm.tm_mon + 1,
                   local_tm.tm_mday, hand};
  cards deck1 = card_shuffle(s1, 1);
  cards state = card_states();

  door::Panel score_panel = make_score_panel(door);
  door::Panel streak_panel = make_streak_panel();
  door::Panel left_panel = make_left_panel();
  door::Panel cmd_panel = make_command_panel();

  {
    int off_yp = off_y + 11;
    int cxp, cyp, levelp;
    int left_panel_x, right_panel_x;
    // find position of card, to position the panels
    cardgo(18, cxp, cyp, levelp);
    left_panel_x = cxp;
    cardgo(15, cxp, cyp, levelp);
    right_panel_x = cxp;
    score_panel.set(left_panel_x + off_x, off_yp);
    streak_panel.set(right_panel_x + off_x, off_yp);
    cmd_panel.set(left_panel_x + off_x, off_yp + 5);
  }

  bool dealing = true;
  int r = 0;

  while ((r >= 0) and (r != 'Q')) {
    // REDRAW everything

    door << door::reset << door::cls;
    door << spaceAceTriPeaks;

    {
      // step 1:
      // draw the deck "source"
      int cx, cy, level;
      cardgo(29, cx, cy, level);

      if (card_number == 51)
        level = 0; // out of cards!
      c = d.back(level);
      c->set(cx + off_x, cy + off_y);
      // p3 is heigh below
      left_panel.set(cx + off_x, cy + off_y + height);
      door << score_panel << left_panel << streak_panel << cmd_panel;
      door << *c;
      if (dealing)
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // I tried setting the cursor before the delay and before displaying the
    // card. It is very hard to see / just about useless.  Not worth the effort.

    for (int x = 0; x < (dealing ? 28 : 29); x++) {
      int cx, cy, level;

      cardgo(x, cx, cy, level);
      // This is hardly visible.
      // door << door::Goto(cx + off_x - 1, cy + off_y + 1);
      if (dealing)
        std::this_thread::sleep_for(std::chrono::milliseconds(75));

      if (dealing) {
        c = d.back(level);
        c->set(cx + off_x, cy + off_y);
        door << *c;
      } else {
        // redrawing -- draw the cards with their correct "state"
        int s = state.at(x);

        switch (s) {
        case 0:
          c = d.back(level);
          c->set(cx + off_x, cy + off_y);
          door << *c;
          break;
        case 1:
          // cardgo(x, space, height, cx, cy, level);
          if (x == 28)
            c = d.card(deck1.at(card_number));
          else
            c = d.card(deck1.at(x));
          c->set(cx + off_x, cy + off_y);
          door << *c;
          break;
        case 2:
          // no card to draw.  :)
          break;
        }
      }
    }

    if (dealing)
      for (int x = 18; x < 29; x++) {
        int cx, cy, level;

        state.at(x) = 1;
        cardgo(x, cx, cy, level);
        // door << door::Goto(cx + off_x - 1, cy + off_y + 1);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        c = d.card(deck1.at(x));
        c->set(cx + off_x, cy + off_y);
        door << *c;
      }

    {
      int cx, cy, level;
      cardgo(active_card, cx, cy, level);
      c = d.marker(1);
      c->set(cx + off_x + 2, cy + off_y + 2);
      door << *c;
    }

    dealing = false;

    left_panel.update(door);
    door << door::reset;

    bool now_what = true;
    while (now_what) {
      // time might have updated, so update score panel too.
      score_panel.update(door);

      r = door.sleep_key(door.inactivity);
      if (r > 0) {
        // They didn't timeout/expire.  They didn't press a function key.
        if (r < 0x1000)
          r = std::toupper(r);
        switch (r) {
        case '\x0d':
          if (card_number < 51) {
            card_number++;
            current_streak = 0;
            streak_panel.update(door);

            // Ok, deal next card from the pile.
            int cx, cy, level;

            if (card_number == 51) {
              cardgo(29, cx, cy, level);
              level = 0; // out of cards
              c = d.back(level);
              c->set(cx + off_x, cy + off_y);
              door << *c;
            }
            cardgo(28, cx, cy, level);
            c = d.card(deck1.at(card_number));
            c->set(cx + off_x, cy + off_y);
            door << *c;
            // update the cards left_panel
            left_panel.update(door);
          }
          break;
        case 'R':
          now_what = false;
          break;
        case 'Q':
          now_what = false;
          break;
        case ' ':
          // can we play this card?
          /*
          get_logger() << "can_play( " << active_card << ":"
                       << deck1.at(active_card) << "/"
                       << d.is_rank(deck1.at(active_card)) << " , "
                       << card_number << "/" << d.is_rank(deck1.at(card_number))
                       << ") = "
                       << d.can_play(deck1.at(active_card),
                                     deck1.at(card_number))
                       << std::endl;
                       */

          if (d.can_play(deck1.at(active_card), deck1.at(card_number))) {
            // if (true) {
            // yes we can.
            ++current_streak;
            if (current_streak > best_streak)
              best_streak = current_streak;
            streak_panel.update(door);
            score += 10;
            if (current_streak > 2)
              score += 5;

            // play card!
            state.at(active_card) = 2;
            {
              // swap the active card with card_number (play card)
              int temp = deck1.at(active_card);
              deck1.at(active_card) = deck1.at(card_number);
              deck1.at(card_number) = temp;
              // active_card is -- invalidated here!  find "new" card.
              int cx, cy, level;

              // erase/clear active_card
              std::vector<int> check = d.unblocks(active_card);
              bool left = false, right = false;
              for (const int c : check) {
                std::pair<int, int> blockers = d.blocks[c];
                if (blockers.first == active_card)
                  right = true;
                if (blockers.second == active_card)
                  left = true;
              }

              d.remove_card(door, active_card, off_x, off_y, left, right);

              /*   // old way of doing this that leaves holes.
              cardgo(active_card, cx, cy, level);
              c = d.back(0);
              c->set(cx + off_x, cy + off_y);
              door << *c;
              */

              // redraw play card #28. (Which is the "old" active_card)
              cardgo(28, cx, cy, level);
              c = d.card(deck1.at(card_number));
              c->set(cx + off_x, cy + off_y);
              door << *c;

              // Did we unhide a card here?

              // std::vector<int> check = d.unblocks(active_card);

              /*
              get_logger() << "active_card = " << active_card
                           << " unblocks: " << check.size() << std::endl;
              */
              int new_card_shown = -1;
              if (!check.empty()) {
                for (const int to_check : check) {
                  std::pair<int, int> blockers = d.blocks[to_check];
                  /*
                  get_logger()
                      << "Check: " << to_check << " " << blockers.first << ","
                      << blockers.second << " " << state.at(blockers.first)
                      << "," << state.at(blockers.second) << std::endl;
                      */
                  if ((state.at(blockers.first) == 2) and
                      (state.at(blockers.second) == 2)) {
                    // WOOT!  Card uncovered.
                    /*
                    get_logger() << "showing: " << to_check << std::endl;
                    */
                    state.at(to_check) = 1;
                    cardgo(to_check, cx, cy, level);
                    c = d.card(deck1.at(to_check));
                    c->set(cx + off_x, cy + off_y);
                    door << *c;
                    new_card_shown = to_check;
                  }
                }
              } else {
                // this would be a "top" card.  Should set status = 4 and
                // display something here?
                get_logger() << "top card cleared?" << std::endl;
                // display something at active_card position
                int cx, cy, level;
                cardgo(active_card, cx, cy, level);
                door << door::Goto(cx + off_x, cy + off_y) << door::reset
                     << "BONUS";
                score += 100;
                state.at(active_card) = 3; // handle this in the "redraw"
              }

              // Find new "number" for active_card to be.
              if (new_card_shown != -1) {
                active_card = new_card_shown;
              } else {
                // active_card++;
                int new_active = find_next_closest(state, active_card);

                if (new_active != -1) {
                  active_card = new_active;
                } else {
                  get_logger() << "This looks like END OF GAME." << std::endl;
                  // bonus for cards left
                  press_a_key(door);
                  if (hand < total_hands) {
                    hand++;
                    door << door::reset << door::cls;
                    goto next_hand;
                  }
                  r = 'Q';
                  now_what = false;
                }
              }
              // update the active_card marker!
              cardgo(active_card, cx, cy, level);
              c = d.marker(1);
              c->set(cx + off_x + 2, cy + off_y + 2);
              door << *c;
            }
          }
          break;
        case XKEY_LEFT_ARROW:
        case '4': {
          int new_active = find_next(true, state, active_card);
          /*
          int new_active = active_card - 1;
          while (new_active >= 0) {
            if (state.at(new_active) == 1)
              break;
            --new_active;
          }*/
          if (new_active >= 0) {

            int cx, cy, level;
            cardgo(active_card, cx, cy, level);
            c = d.marker(0);
            c->set(cx + off_x + 2, cy + off_y + 2);
            door << *c;
            active_card = new_active;
            cardgo(active_card, cx, cy, level);
            c = d.marker(1);
            c->set(cx + off_x + 2, cy + off_y + 2);
            door << *c;
          }
        } break;
        case XKEY_RIGHT_ARROW:
        case '6': {
          int new_active = find_next(false, state, active_card);
          /*
          int new_active = active_card + 1;
          while (new_active < 28) {
            if (state.at(new_active) == 1)
              break;
            ++new_active;
          }
          */
          if (new_active >= 0) { //(new_active < 28) {
            int cx, cy, level;
            cardgo(active_card, cx, cy, level);
            c = d.marker(0);
            c->set(cx + off_x + 2, cy + off_y + 2);
            door << *c;
            active_card = new_active;
            cardgo(active_card, cx, cy, level);
            c = d.marker(1);
            c->set(cx + off_x + 2, cy + off_y + 2);
            door << *c;
          }
        }

        break;
        }
      } else
        now_what = false;
    }
  }
  if (r == 'Q') {
    if (hand < total_hands) {
      press_a_key(door);
      hand++;
      door << door::reset << door::cls;
      goto next_hand;
    }
  }
  return r;
}

door::Panel make_about(void) {
  const int W = 60;
  door::Panel about(W);
  about.setStyle(door::BorderStyle::DOUBLE_SINGLE);
  about.setColor(door::ANSIColor(door::COLOR::YELLOW, door::COLOR::BLUE,
                                 door::ATTR::BOLD));

  about.addLine(std::make_unique<door::Line>("About This Door", W));
  about.addLine(std::make_unique<door::Line>(
      "---------------------------------", W,
      door::ANSIColor(door::COLOR::CYAN, door::COLOR::BLUE, door::ATTR::BOLD)));
  /*
  123456789012345678901234567890123456789012345678901234567890-60
  This door was written by Bugz.

  It is written in c++, only supports Linux, and replaces
  opendoors.

  It's written in c++, and replaces the outdated opendoors
  library.

   */
  about.addLine(
      std::make_unique<door::Line>(SPACEACE " v" SPACEACE_VERSION, W));
  std::string copyright = SPACEACE_COPYRIGHT;
  if (door::unicode) {
    replace(copyright, "(C)", "\u00a9");
  }

  about.addLine(std::make_unique<door::Line>(copyright, W));
  about.addLine(std::make_unique<door::Line>("", W));
  about.addLine(
      std::make_unique<door::Line>("This door was written by Bugz.", W));
  about.addLine(std::make_unique<door::Line>("", W));
  about.addLine(std::make_unique<door::Line>(
      "It is written in c++, only support Linux, and replaces", W));
  about.addLine(std::make_unique<door::Line>("opendoors.", W));

  /*
    door::updateFunction updater = [](void) -> std::string {
      std::string text = "Currently: ";
      text.append(return_current_time_and_date());
      return text;
    };
    std::string current = updater();
    door::Line active(current, 60);
    active.setUpdater(updater);
    active.setRender(statusValue(
        door::ANSIColor(door::COLOR::WHITE, door::COLOR::BLUE,
    door::ATTR::BOLD), door::ANSIColor(door::COLOR::YELLOW,
    door::COLOR::BLUE, door::ATTR::BOLD)));
    about.addLine(std::make_unique<door::Line>(active));
  */

  return about;
}

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

    for (int x = 0; x < MAX_STARS; x++) {
      door::Goto star_at(uni_x(rng), uni_y(rng));
      door << star_at;
      if (x % 5 < 2)
        door << dark;
      else
        door << white;

      if (x % 2 == 0)
        door << stars[0];
      else
        door << stars[1];
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

int main(int argc, char *argv[]) {

  door::Door door("space-ace", argc, argv);

  // store the door log so we can easily access it.
  get_logger = [&door]() -> ofstream & { return door.log(); };

  DBData spacedb;
  spacedb.setUser(door.username);

  // retrieve lastcall
  time_t last_call = std::stol(spacedb.getSetting("LastCall", "0"));

  // store now as lastcall
  time_t now =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

  spacedb.setSetting("LastCall", std::to_string(now));

  // Have they used this door before?
  if (last_call != 0) {
    door << "Welcome Back!" << door::nl;
    auto nowClock = std::chrono::system_clock::from_time_t(now);
    auto lastClock = std::chrono::system_clock::from_time_t(last_call);
    auto delta = nowClock - lastClock;

    // int days = chrono::duration_cast<chrono::days>(delta).count();  //
    // c++ 20
    int hours = chrono::duration_cast<chrono::hours>(delta).count();
    int days = hours / 24;
    int minutes = chrono::duration_cast<chrono::minutes>(delta).count();
    int secs = chrono::duration_cast<chrono::seconds>(delta).count();

    if (days > 1) {
      door << "It's been " << days << " days since you last played."
           << door::nl;
    } else {
      if (hours > 1) {
        door << "It's been " << hours << " hours since you last played."
             << door::nl;
      } else {
        if (minutes > 1) {
          door << "It's been " << minutes << " minutes since you last played."
               << door::nl;
        } else {
          door << "It's been " << secs << " seconds since you last played."
               << door::nl;
          door << "It's like you never left." << door::nl;
        }
      }
    }
    press_a_key(door);
  }

  /*
  // example:  saving the door.log() for global use.

  std::function<std::ofstream &(void)> get_logger;

  get_logger = [&door]() -> ofstream & { return door.log(); };

  if (get_logger) {
    get_logger() << "MEOW" << std::endl;
    get_logger() << "hey! It works!" << std::endl;
  }

  get_logger = nullptr;  // before door destruction
  */

  // https://stackoverflow.com/questions/5008804/generating-random-integer-from-a-range

  std::random_device rd; // only used once to initialise (seed) engine
  std::mt19937 rng(rd());
  // random-number engine used (Mersenne-Twister in this case)
  // std::uniform_int_distribution<int> uni(min, max); // guaranteed
  // unbiased

  int mx, my; // Max screen width/height
  if (door.width == 0) {
    // screen detect failed, use sensible defaults
    door.width = mx = 80;
    door.height = my = 23;
  } else {
    mx = door.width;
    my = door.height;
  }
  // We assume here that the width and height aren't something crazy like
  // 10x15. :P  (or 24x923!)

  display_starfield_space_ace(door, rng);

  // for testing inactivity timeout
  // door.inactivity = 10;

  door::Panel timeout = make_timeout(mx, my);
  door::Menu m = make_main_menu();
  door::Panel about = make_about(); // 8 lines

  // center the about box
  about.set((mx - 60) / 2, (my - 9) / 2);

  int r = 0;
  while ((r >= 0) and (r != 6)) {
    // starfield + menu ?
    display_starfield(door, rng);
    r = m.choose(door);
    // need to reset the colors.  (whoops!)
    door << door::reset << door::cls; // door::nl;
    // OK! The screen is blank at this point!

    switch (r) {
    case 1: // play game
      r = play_cards(door, spacedb, rng);
      break;

    case 2: // view scores
      door << "Show scores goes here!" << door::nl;
      r = press_a_key(door);
      break;

    case 3: // configure
      r = configure(door, spacedb);
      // r = press_a_key(door);
      break;

    case 4: // help
      door << "Help!  Need some help here..." << door::nl;
      r = press_a_key(door);
      break;

    case 5: // about
      display_starfield(door, rng);
      door << about << door::nl;
      r = press_a_key(door);
      break;

    case 6: // quit
      break;
    }
  }

  if (r < 0) {
  TIMEOUT:
    if (r == -1) {
      door.log() << "TIMEOUT" << std::endl;

      door << timeout << door::reset << door::nl << door::nl;
    } else {
      if (r == -3) {
        door.log() << "OUTTA TIME" << std::endl;
        door::Panel notime = make_notime(mx, my);
        door << notime << door::reset << door::nl;
      }
    }
    return 0;
  }

  door << door::nl;

  /*
  // magic time!
  door << door::reset << door::nl << "Press another key...";
  int x;

  for (x = 0; x < 60; ++x) {
    r = door.sleep_key(1);
    if (r == -1) {
      // ok!  Expected timeout!

      // PROBLEM:  regular "local" terminal loses current attributes
      // when cursor is save / restored.

      door << door::SaveCursor;

      if (about.update(door)) {
        // ok I need to "fix" the cursor position.
        // it has moved.
      }

      door << door::RestoreCursor << door::reset;

    } else {
      if (r < 0)
        goto TIMEOUT;
      if (r >= 0)
        break;
    }
  }

  if (x == 60)
    goto TIMEOUT;

*/

#ifdef NNY

  // configured by the player.

  door::ANSIColor deck_color;
  // RED, BLUE, GREEN, MAGENTA, CYAN
  std::uniform_int_distribution<int> rand_color(0, 4);

  switch (rand_color(rng)) {
  case 0:
    deck_color = door::ANSIColor(door::COLOR::RED);
    break;
  case 1:
    deck_color = door::ANSIColor(door::COLOR::BLUE);
    break;
  case 2:
    deck_color = door::ANSIColor(door::COLOR::GREEN);
    break;
  case 3:
    deck_color = door::ANSIColor(door::COLOR::MAGENTA);
    break;
  case 4:
    deck_color = door::ANSIColor(door::COLOR::CYAN);
    break;
  default:
    deck_color = door::ANSIColor(door::COLOR::BLUE, door::ATTR::BLINK);
    break;
  }

  int height = 3;
  Deck d(deck_color, height);
  door::Panel *c;
  door << door::reset << door::cls;

  // This displays the cards in the upper left corner.
  // We want them center, and down some.

  int space = 3;

  // int cards_dealt_width = 59; int cards_dealt_height = 9;
  int game_width;
  {
    int cx, cy, level;
    cardgo(27, space, height, cx, cy, level);
    game_width = cx + 5; // card width
  }
  int off_x = (mx - game_width) / 2;
  int off_y = (my - 9) / 2;

  // The idea is to see the cards with <<Something Unique to the card
  // game>>, Year, Month, Day, and game (like 1 of 3). This will make the
  // games the same/fair for everyone.

  std::seed_seq s1{2021, 2, 27, 1};
  cards deck1 = card_shuffle(s1, 1);
  cards state = card_states();

  // I tried setting the cursor before the delay and before displaying the
  // card. It is very hard to see / just about useless.  Not worth the
  // effort.

  for (int x = 0; x < 28; x++) {
    int cx, cy, level;

    cardgo(x, space, height, cx, cy, level);
    // This is hardly visible.
    // door << door::Goto(cx + off_x - 1, cy + off_y + 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(75));

    c = d.back(level);
    c->set(cx + off_x, cy + off_y);
    door << *c;
  }

  /*
    std::this_thread::sleep_for(
        std::chrono::seconds(1)); // 3 secs seemed too long!
  */

  for (int x = 18; x < 28; x++) {
    int cx, cy, level;
    // usleep(1000 * 20);

    state.at(x) = 1;
    cardgo(x, space, height, cx, cy, level);
    // door << door::Goto(cx + off_x - 1, cy + off_y + 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    c = d.card(deck1.at(x));
    c->set(cx + off_x, cy + off_y);
    door << *c;
  }

  door << door::reset;
  door << door::nl << door::nl;

  r = door.sleep_key(door.inactivity);
  if (r < 0)
    goto TIMEOUT;

#endif

  /*
    door::Panel *p = d.back(2);
    p->set(10, 10);
    door << *p;
    door::Panel *d8 = d.card(8);
    d8->set(20, 8);
    door << *d8;

    r = door.sleep_key(door.inactivity);
    if (r < 0)
      goto TIMEOUT;
  */

  // door << door::reset << door::cls;
  display_starfield(door, rng);
  door << m << door::reset << door::nl;

  // Normal DOOR exit goes here...
  door << door::nl << "Returning you to the BBS, please wait..." << door::nl;

  get_logger = nullptr;
  return 0;
}
