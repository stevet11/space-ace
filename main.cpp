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
#include "scores.h"
#include "utils.h"
#include "version.h"
#include "starfield.h"
#include <algorithm> // transform

// configuration here -- access via extern
YAML::Node config;

door::ANSIColor stringToANSIColor(std::string colorCode);

std::function<std::ofstream &(void)> get_logger;
std::function<void(void)> cls_display_starfield;
std::function<int(void)> press_a_key;

std::string return_current_time_and_date() {
  auto now = std::chrono::system_clock::now();
  auto in_time_t = std::chrono::system_clock::to_time_t(now);

  std::stringstream ss;
  // ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
  ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %r");
  return ss.str();
}

door::renderFunction any_color = [](const std::string &txt) -> door::Render {
  door::Render r(txt);

  door::ANSIColor blue(door::COLOR::GREEN, door::ATTR::BOLD);
  door::ANSIColor cyan(door::COLOR::YELLOW, door::ATTR::BOLD);

  for (char const &c : txt) {
    if (isupper(c))
      r.append(blue);
    else
      r.append(cyan);
  }
  return r;
};

int press_any_key(door::Door &door) {
  static std::default_random_engine generator;
  static std::uniform_int_distribution<int> distribution(0, 1);
  int use_set = distribution(generator);
  std::string text = "Press a key to continue...";

  door << door::reset;
  any_color(text).output(door);
  // << text;

  int r;
  int t = 0;
  // alternate animation

#define ALT_ANIMATION
#ifdef ALT_ANIMATION
  std::vector<std::pair<int, int>> words = find_words(text);
  std::string work_text = text;
  int text_length = text.size();

  // current word
  unsigned int word = 0;

  std::string current_word = text.substr(words[word].first, words[word].second);
  unsigned int wpos = 0;
  int ms_sleep = 0;
  int sleep_ms = 250;
  t = 0;

  while ((r = door.sleep_ms_key(sleep_ms)) == TIMEOUT) {
    ms_sleep += sleep_ms;
    if (ms_sleep > 1000) {
      ms_sleep -= 1000;
      t++;

      if (t >= door.inactivity) {
        // restore text
        door << std::string(text_length, '\b');
        any_color(text).output(door);
        // << text;
        door << door::nl;
        return TIMEOUT;
      }
    }
    ++wpos;
    if (wpos == current_word.size()) {
      ++word;
      work_text = text;
      wpos = 0;
      if (word == words.size())
        word = 0;
      current_word = text.substr(words[word].first, words[word].second);
    }

    int c = current_word[wpos];

    while (!std::islower(c)) {
      // It is not lower case
      wpos++;
      if (wpos == current_word.size()) {
        // Ok, this word is done.
        wpos = 0;
        word++;
        work_text = text;
        if (word == words.size())
          word = 0;
        current_word = text.substr(words[word].first, words[word].second);
        wpos = 0;
      }
      c = current_word[wpos];
    }

    // Ok, I found something that's lower case.

    work_text[words[word].first + wpos] = std::toupper(c);
    //    std::toupper(words[word].first + wpos);
    door << std::string(text_length, '\b');
    any_color(work_text).output(door);
    // << work_text;
  }

  // ok, restore the original string
  door << std::string(text_length, '\b');
  any_color(text).output(door);
  //<< text;
  return r;

#endif

#ifdef ALT_ANIMATION_2
  // This can be cleaned up to be simpler -- and do the same effect.  We don't
  // need the words here, we're not really using them.

  std::vector<std::pair<int, int>> words = find_words(text);
  std::string work_text = text;
  int text_length = text.size();

  // current word
  unsigned int word = 0;

  std::string current_word = text.substr(words[word].first, words[word].second);
  unsigned int wpos = 0;

  // DOES NOT HONOR door.inactivity (AT THIS TIME)
  while ((r = door.sleep_ms_key(200)) == TIMEOUT) {
    ++wpos;
    if (wpos == current_word.size()) {
      ++word;
      wpos = 0;
      if (word == words.size())
        word = 0;
      current_word = text.substr(words[word].first, words[word].second);
    }

    int c = current_word[wpos];

    while (!std::islower(c)) {
      // It is not lower case
      wpos++;
      if (wpos == current_word.size()) {
        // Ok, this word is done.
        wpos = 0;
        word++;
        if (word == words.size())
          word = 0;
        current_word = text.substr(words[word].first, words[word].second);
        wpos = 0;
      }
      c = current_word[wpos];
    }

    // Ok, I found something that's lower case.
    work_text = text;
    work_text[words[word].first + wpos] = std::toupper(c);
    //    std::toupper(words[word].first + wpos);
    door << std::string(text_length, '\b') << work_text;
  }
  // ok, restore the original string
  door << std::string(text_length, '\b') << text;
  return r;

#endif

  // animation spinner that reverses itself.  /-\| and blocks.
  t = 0;
  int loop = 0;

  const char *loop_chars[3][4] = {{"/", "-", "\\", "|"},
                                  {"\xde", "\xdc", "\xdd", "\xdf"},
                                  {"\u2590", "\u2584", "\u258c", "\u2580"}};
  std::array<int, 11> sleeps = {1000, 700, 500, 300, 200, 200,
                                200,  300, 500, 700, 1000};
  // start out at
  int sleep_number = 5;
  bool forward = true;

  if (door::unicode) {
    if (use_set == 1)
      use_set = 2;
  }

  door << loop_chars[use_set][loop % 4];
  ms_sleep = 0;
  bool check_speed = false;

  while ((r = door.sleep_ms_key(sleeps[sleep_number])) == TIMEOUT) {
    ms_sleep += sleeps[sleep_number];
    if (ms_sleep > 1000) {
      ms_sleep -= 1000;
      t++;

      if (t >= door.inactivity) {
        door << "\b \b";
        door << door::nl;
        return TIMEOUT;
      }
    }
    if (forward) {
      loop++;
      if (loop == 4) {
        loop = 0;
        check_speed = true;
      }
    } else {
      --loop;
      if (loop < 0) {
        loop = 3;
        check_speed = true;
      }
    }
    if (check_speed) {
      sleep_number++;
      if (sleep_number >= (int)sleeps.size()) {
        sleep_number = 0;
        forward = !forward;
      }
      check_speed = false;
    }
    door << "\b" << loop_chars[use_set][loop];
  }
  door << "\b \b";
  door << door::nl;
  return r;
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
  // pre-warm the parser
  find_words(std::string(""));

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

        door::Panel config_panel = make_sysop_config();
        // config_panel.set(1, 1);
        door << config_panel << door::reset << door::nl;

        r = press_a_key();
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

int main(int argc, char *argv[]) {

  door::Door door("space-ace", argc, argv);

  // store the door log so we can easily access it.
  get_logger = [&door]() -> ofstream & { return door.log(); };
  press_a_key = [&door]() -> int { return press_any_key(door); };

  std::random_device rd;
  std::mt19937 rng(rd());

  starfield mainfield(door, rng);

  cls_display_starfield = [&mainfield]() -> void {
    mainfield.display();
    //display_starfield(door, rng);
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

  if (!config["date_monthly"]) {
    config["date_monthly"] = "%B %Y";
    update_config = true;
  }

  if (!config["_seed"]) {
    // pick numbers to use for seed
    std::string seeds;
    seeds += std::to_string(rng());
    seeds += ",";
    seeds += std::to_string(rng());
    seeds += ",";
    seeds += std::to_string(rng());
    config["_seed"] = seeds;
    update_config = true;
  }

  // save configuration -- something was updated
  if (update_config) {
    std::ofstream fout("space-ace.yaml");
    fout << config << std::endl;
  }

  // retrieve lastcall
  time_t last_call = std::stol(spacedb.getSetting("LastCall", "0"));

  std::chrono::_V2::system_clock::time_point now =
      std::chrono::system_clock::now();

  // store now as lastcall
  time_t now_t = std::chrono::system_clock::to_time_t(now);

  spacedb.setSetting("LastCall", std::to_string(now_t));

  // run maint
  {
    std::chrono::_V2::system_clock::time_point maint_date = now;
    firstOfMonthDate(maint_date);

    if (spacedb.expireScores(
            std::chrono::system_clock::to_time_t(maint_date))) {
      if (get_logger)
        get_logger() << "maint completed" << std::endl;
      door << "Thanks for waiting..." << door::nl;
    }
  }

  // Have they used this door before?
  if (last_call != 0) {
    door << door::ANSIColor(door::COLOR::YELLOW, door::ATTR::BOLD)
         << "Welcome Back!" << door::nl;
    auto nowClock = std::chrono::system_clock::from_time_t(now_t);
    auto lastClock = std::chrono::system_clock::from_time_t(last_call);
    auto delta = nowClock - lastClock;

    // int days = chrono::duration_cast<chrono::days>(delta).count();  //
    // c++ 20
    int hours = chrono::duration_cast<chrono::hours>(delta).count();
    int days = hours / 24;
    int minutes = chrono::duration_cast<chrono::minutes>(delta).count();
    int secs = chrono::duration_cast<chrono::seconds>(delta).count();

    if (days > 1) {
      door << door::ANSIColor(door::COLOR::GREEN, door::ATTR::BOLD)
           << "It's been " << days << " days since you last played."
           << door::nl;
    } else {
      if (hours > 1) {
        door << door::ANSIColor(door::COLOR::CYAN) << "It's been " << hours
             << " hours since you last played." << door::nl;
      } else {
        if (minutes > 1) {
          door << door::ANSIColor(door::COLOR::CYAN) << "It's been " << minutes
               << " minutes since you last played." << door::nl;
        } else {
          door << door::ANSIColor(door::COLOR::YELLOW, door::ATTR::BOLD)
               << "It's been " << secs << " seconds since you last played."
               << door::nl;
          door << "It's like you never left." << door::nl;
        }
      }
    }

    if (press_a_key() < 0)
      return 0;
  }

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
  door::Panel help = make_help();

  // center the about box
  about.set((mx - 60) / 2, (my - 9) / 2);
  // center the help box
  help.set((mx - 60) / 2, (my - 15) / 2);

  PlayCards pc(door, spacedb, rng);

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
      // PlayCards pc(door, spacedb, rng);
      r = pc.play();
      // r = play_cards(door, spacedb, rng);
    }; break;

    case 2: // view scores

      // door << door::cls;
      {
        Scores score(door, spacedb);

        score.display_scores(door);
      }
      r = press_a_key();
      break;

    case 3: // configure
      r = configure(door, spacedb);
      // r = press_a_key();
      break;

    case 4: // help
      display_starfield(door, rng);
      door << help << door::nl;
      r = press_a_key();
      break;

    case 5: // about
      display_starfield(door, rng);
      door << about << door::nl;
      r = press_a_key();
      break;

    case 6: // quit
      break;
    }
  }
  if (r < 0) {
    // DOOR_ERROR:
    if (r == TIMEOUT) {
      door.log() << "TIMEOUT" << std::endl;

      door << timeout << door::reset << door::nl << door::nl;
    } else {
      if (r == OUTOFTIME) {
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

  press_a_key = nullptr;
  get_logger = nullptr;
  cls_display_starfield = nullptr;
  return 0;
}
