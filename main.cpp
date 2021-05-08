#include "door.h"

#include "yaml-cpp/yaml.h"
#include <chrono>  // chrono::system_clock
#include <ctime>   // localtime
#include <iomanip> // put_time
#include <iostream>
#include <random>
#include <string>

#include "db.h"
#include "deck.h"
#include "play.h"
#include "utils.h"
#include "version.h"
#include <algorithm> // transform

// configuration here -- access via extern
YAML::Node config;

door::ANSIColor stringToANSIColor(std::string colorCode);

std::function<std::ofstream &(void)> get_logger;
std::function<void(void)> cls_display_starfield;

/*
unsigned long score = 0;
int hand = 1;
int total_hands = 3;
int card_number = 28;
int current_streak = 0;
int best_streak = 0;
int active_card = 23;

std::chrono::_V2::system_clock::time_point play_day;
*/

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

int configure(door::Door &door, DBData &db) {
  auto menu = make_config_menu();
  int r = 0;
  bool save_deckcolor = false;
  const char *deckcolor = "DeckColor";
  std::string newColor;

  while (r >= 0) {

    if (save_deckcolor) {
      door << menu;
      db.setSetting(deckcolor, newColor);
      save_deckcolor = false;
    }

    if (cls_display_starfield)
      cls_display_starfield();
    else
      door << door::reset << door::cls;

    r = menu.choose(door);
    if (r > 0) {
      door << door::reset << door::cls;
      char c = menu.which(r - 1);
      if (c == 'D') {
        // Ok, deck colors
        // get default

        std::string currentDefault = db.getSetting(deckcolor, "ALL");
        int currentOpt = opt_from_string(currentDefault);

        door << door::reset << door::cls;
        auto deck = make_deck_menu();
        deck.defaultSelection(currentOpt);

        if (cls_display_starfield)
          cls_display_starfield();
        else
          door << door::reset << door::cls;

        int newOpt = deck.choose(door);
        door << door::reset << door::cls;

        if (newOpt >= 0) {
          newOpt--;
          newColor = stringFromColorOptions(newOpt);
          if (newOpt != currentOpt) {
            door.log() << deckcolor << " was " << currentDefault << ", "
                       << currentOpt << ". Now " << newColor << ", " << newOpt
                       << std::endl;
            save_deckcolor = true;
          }
        }
      }
      if (c == 'V') {
        // view settings -- Sysop Configuration
        if (cls_display_starfield)
          cls_display_starfield();
        else
          door << door::reset << door::cls;
        door << door::Goto(1, 1);

        door << door::ANSIColor(door::COLOR::CYAN, door::COLOR::BLACK)
             << "Game Settings - SysOp Configurable" << door::reset << door::nl
             << door::nl;
        for (auto cfg : config) {
          std::string key = cfg.first.as<std::string>();
          if (key[0] == '_')
            continue;
          // TODO: replace _ with ' ' in string.
          while (replace(key, "_", " ")) {
          };
          std::string value = cfg.second.as<std::string>();
          door << door::ANSIColor(door::COLOR::CYAN, door::COLOR::BLACK,
                                  door::ATTR::BOLD)
               << std::setw(20) << key
               << door::ANSIColor(door::COLOR::YELLOW, door::COLOR::BLACK,
                                  door::ATTR::BOLD)
               << " : "
               << door::ANSIColor(door::COLOR::CYAN, door::COLOR::BLACK,
                                  door::ATTR::BOLD)
               << value << door::reset << door::nl;
        }
        r = press_a_key(door);
        if (r < 0)
          return r;
        door << door::reset << door::cls;
      }
      if (c == 'Q') {
        return r;
      }
    }
  }
  return r;
}

/*
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
*/

int main(int argc, char *argv[]) {

  door::Door door("space-ace", argc, argv);

  // store the door log so we can easily access it.
  get_logger = [&door]() -> ofstream & { return door.log(); };

  std::random_device rd;
  std::mt19937 rng(rd());

  cls_display_starfield = [&door, &rng]() -> void {
    display_starfield(door, rng);
  };

  DBData spacedb;
  spacedb.setUser(door.username);

  if (file_exists("space-ace.yaml")) {
    config = YAML::LoadFile("space-ace.yaml");
  }

  bool update_config = false;

  // populate with "good" defaults
  if (!config["hands_per_day"]) {
    config["hands_per_day"] = 3;
    update_config = true;
  }

  if (!config["date_format"]) {
    config["date_format"] = "%B %d"; // Month day or "%b %d,%Y" Mon,d YYYY
    update_config = true;
  }

  if (!config["date_score"]) {
    config["date_score"] = "%m/%d/%Y"; // or "%Y/%0m/%0d";
    update_config = true;
  }

  if (!config["makeup_per_day"]) {
    config["makeup_per_day"] = 5;
    update_config = true;
  }

  if (!config["play_days_ahead"]) {
    config["play_days_ahead"] = 2;
    update_config = true;
  }

  /*
    if (config["hands_per_day"]) {
      get_logger() << "hands_per_day: " << config["hands_per_day"].as<int>()
                   << std::endl;
    }
  */

  // save configuration -- something was updated
  if (update_config) {
    std::ofstream fout("space-ace.yaml");
    fout << config << std::endl;
  }

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
    {
      PlayCards pc(door, spacedb, rng);
      r = pc.play();
      // r = play_cards(door, spacedb, rng);
    }; break;

    case 2: // view scores
    {
      door << door::cls;
      auto all_scores = spacedb.getScores();
      for (auto it : all_scores) {
        time_t on_this_date = it.first;
        std::string nice_date = convertDateToDateScoreFormat(on_this_date);
        door << "  *** " << nice_date << " ***" << door::nl;

        for (auto sd : it.second) {
          door << setw(15) << sd.user << " " << sd.won << " " << sd.score
               << door::nl;
        }
      }
      door << "====================" << door::nl;
    }

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
    // TIMEOUT:
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

  // door << door::reset << door::cls;
  display_starfield(door, rng);
  door << m << door::reset << door::nl;

  // Normal DOOR exit goes here...
  door << door::nl << "Returning you to the BBS, please wait..." << door::nl;

  get_logger = nullptr;
  cls_display_starfield = nullptr;
  return 0;
}
