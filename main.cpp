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

// The idea is that this would be defined elsewhere (maybe)
int user_score = 0;

// NOTE:  When run as a door, we always detect CP437 (because that's what Enigma
// is defaulting to)

void adjust_score(int by) { user_score += by; }

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

door::Menu make_main_menu(void) {
  door::Menu m(5, 5, 25);
  door::Line mtitle("Space-Ace Main Menu");
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

  m.addSelection('1', "Play Cards");
  m.addSelection('2', "View Scores");
  m.addSelection('3', "Configure");
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
  return rf;
}

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

int play_cards(door::Door &door, std::mt19937 &rng) {
  int mx = door.width;
  int my = door.height;
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

  // The idea is to see the cards with <<Something Unique to the card game>>,
  // Year, Month, Day, and game (like 1 of 3).
  // This will make the games the same/fair for everyone.

  std::seed_seq s1{2021, 2, 27, 1};
  cards deck1 = card_shuffle(s1, 1);
  cards state = card_states();

  // I tried setting the cursor before the delay and before displaying the card.
  // It is very hard to see / just about useless.  Not worth the effort.

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

  int r = door.sleep_key(door.inactivity);
  return r;
}

door::Panel make_about(void) {
  door::Panel about(2, 2, 60);
  about.setStyle(door::BorderStyle::DOUBLE_SINGLE);
  about.setColor(door::ANSIColor(door::COLOR::YELLOW, door::COLOR::BLUE,
                                 door::ATTR::BOLD));

  about.addLine(std::make_unique<door::Line>("About This Door", 60));
  /*
  door::Line magic("---------------------------------", 60);
  magic.setColor(door::ANSIColor(door::COLOR::YELLOW, door::COLOR::BLACK,
                                 door::ATTR::BOLD));
*/
  about.addLine(std::make_unique<door::Line>(
      "---------------------------------", 60,
      door::ANSIColor(door::COLOR::CYAN, door::COLOR::BLUE, door::ATTR::BOLD)));
  /*
  123456789012345678901234567890123456789012345678901234567890
  This door was written by Bugz.

  It is written in c++, only supports Linux, and replaces
  opendoors.

  It's written in c++, and replaces the outdated opendoors
  library.

   */
  about.addLine(
      std::make_unique<door::Line>("This door was written by Bugz.", 60));
  about.addLine(std::make_unique<door::Line>("", 60));
  about.addLine(std::make_unique<door::Line>(
      "It is written in c++, only support Linux, and replaces", 60));
  about.addLine(std::make_unique<door::Line>("opendoors.", 60));

  /*
    door::updateFunction updater = [](void) -> std::string {
      std::string text = "Currently: ";
      text.append(return_current_time_and_date());
      return text;
    };
    std::string current = updater();
    door::Line active(current, 60);
    active.setUpdater(updater);
    active.setRender(renderStatusValue(
        door::ANSIColor(door::COLOR::WHITE, door::COLOR::BLUE,
    door::ATTR::BOLD), door::ANSIColor(door::COLOR::YELLOW, door::COLOR::BLUE,
                        door::ATTR::BOLD)));
    about.addLine(std::make_unique<door::Line>(active));
  */

  /*
    about.addLine(std::make_unique<door::Line>(
        "Status: blue", 60,
        statusValue(door::ANSIColor(door::COLOR::GREEN, door::ATTR::BOLD),
                    door::ANSIColor(door::COLOR::MAGENTA, door::ATTR::BLINK))));
    about.addLine(std::make_unique<door::Line>("Name: BUGZ", 60, rStatus));
    about.addLine(std::make_unique<door::Line>(
        "Size: 10240", 60,
        statusValue(door::ANSIColor(door::COLOR::GREEN, door::COLOR::BLUE,
                                    door::ATTR::BOLD),
                    door::ANSIColor(door::COLOR::YELLOW, door::COLOR::BLUE,
                                    door::ATTR::BOLD, door::ATTR::BLINK))));
    about.addLine(std::make_unique<door::Line>("Bugz is here.", 60, rStatus));
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
  // pause 5 seconds so they can enjoy our awesome logo
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
  // door << door::reset << door::cls << door::nl;

  DBData spacedb;

  // spacedb.init();

  /*
  // Example:  How to read/set values in spacedb settings.
    std::string setting = "last_play";
    std::string user = door.username;
    std::string value;
    std::string blank = "<blank>";
    value = spacedb.getSetting(user, setting, blank);

    door << door::reset << "last_play: " << value << door::nl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    value = return_current_time_and_date();
    spacedb.setSetting(user, setting, value);
  */

  // https://stackoverflow.com/questions/5008804/generating-random-integer-from-a-range

  std::random_device rd; // only used once to initialise (seed) engine
  std::mt19937 rng(rd());
  // random-number engine used (Mersenne-Twister in this case)
  // std::uniform_int_distribution<int> uni(min, max); // guaranteed unbiased

  int mx, my; // Max screen width/height
  if (door.width == 0) {
    // screen detect failed, use sensible defaults
    door.width = mx = 80;
    door.height = my = 23;
  } else {
    mx = door.width;
    my = door.height;
  }
  // We assume here that the width and height are something crazy like 10x15. :P

  display_starfield_space_ace(door, rng);

  // for testing inactivity timeout
  // door.inactivity = 10;

  door::Panel timeout = make_timeout(mx, my);
  door::Menu m = make_main_menu();

  door::Panel about = make_about();
  // center the about box
  about.set((mx - 60) / 2, (my - 5) / 2);

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
      r = play_cards(door, rng);
      break;

    case 2: // view scores
      door << "Show scores goes here!" << door::nl;
      r = press_a_key(door);
      break;

    case 3: // configure
      door << "Configure options go here" << door::nl;
      r = press_a_key(door);
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

  door << door::nl;

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

  // The idea is to see the cards with <<Something Unique to the card game>>,
  // Year, Month, Day, and game (like 1 of 3).
  // This will make the games the same/fair for everyone.

  std::seed_seq s1{2021, 2, 27, 1};
  cards deck1 = card_shuffle(s1, 1);
  cards state = card_states();

  // I tried setting the cursor before the delay and before displaying the card.
  // It is very hard to see / just about useless.  Not worth the effort.

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

  return 0;
}
