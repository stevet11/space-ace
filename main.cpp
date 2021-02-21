#include "door.h"
#include "space.h"
#include <iostream>
#include <random>
#include <string>

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
  m.addSelection('3', "Drop to DOS");
  m.addSelection('4', "Chat with BUGZ");
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
      door::ANSIColor(door::COLOR::YELLOW, door::COLOR::BLACK,
                      door::ATTR::BOLD)));
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

  return about;
}

void display_starfield(int mx, int my, door::Door &door, std::mt19937 &rng) {
  door << door::reset << door::cls;

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

    for (int x = 0; x < (mx * my / 100); x++) {
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

void display_space_ace(int mx, int my, door::Door &door) {
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

void display_starfield_space_ace(int mx, int my, door::Door &door,
                                 std::mt19937 &rng) {
  display_starfield(mx, my, door, rng);
  display_space_ace(mx, my, door);
  door << door::reset;
}

int main(int argc, char *argv[]) {
  /*
  google::InitGoogleLogging(argv[0]);
  LOG(INFO) << "Welcome!";
  */

  door::Door door("space-ace", argc, argv);
  // door << door::reset << door::cls << door::nl;
  door::ANSIColor ac(door::COLOR::YELLOW, door::ATTR::BOLD);

  door::ANSIColor mb(door::COLOR::MAGENTA, door::ATTR::BLINK);

  door << mb << "Does this work?" << door::reset << door::nl;

  // https://stackoverflow.com/questions/5008804/generating-random-integer-from-a-range

  std::random_device rd; // only used once to initialise (seed) engine
  std::mt19937 rng(
      rd()); // random-number engine used (Mersenne-Twister in this case)
  // std::uniform_int_distribution<int> uni(min, max); // guaranteed unbiased

  int mx, my; // Max screen width/height
  if (door.width == 0) {
    // screen detect failed, use sensible defaults
    mx = 80;
    my = 23;
  } else {
    mx = door.width;
    my = door.height;
  }
  // We assume here that the width and height are something crazy like 10x15. :P

  display_starfield_space_ace(mx, my, door, rng);

  // for testing inactivity timeout
  // door.inactivity = 10;

  door::Panel timeout = make_timeout(mx, my);
  door::Menu m = make_main_menu();

  int r = m.choose(door);
  // need to reset the colors.  (whoops!)
  door << door::reset << door::nl;

  if (r < 0) {
  TIMEOUT:
    if (r == -1) {
      door.log("TIMEOUT");

      door << timeout << door::reset << door::nl << door::nl;
    } else {
      if (r == -3) {
        door.log("OUTTA TIME");
        door::Panel notime = make_notime(mx, my);
        door << notime << door::reset << door::nl;
      }
    }
    return 0;
  }

  /*
    display_starfield(mx, my, door, rng);
    // WARNING: After starfield, cursor position is random!

    door << door::Goto(1, 9); // door::nl << door::nl; // door::cls;

    door << "Your name: "
         << door::ANSIColor(door::COLOR::WHITE, door::COLOR::GREEN,
                            door::ATTR::BOLD);
    std::string istring;
    istring = door.input_string(25);
    door << door::reset << door::nl << "You typed in [" << istring << "]"
         << door::nl;

    door << "Hello, " << door.username << ", you chose option " << r << "!"
         << door::nl;

    door << "Press a key...";
    r = door.sleep_key(door.inactivity);
    if (r < 0)
      goto TIMEOUT;

  */

  door << door::nl;

  door::Panel about = make_about();
  about.set((mx - 60) / 2, (my - 5) / 2);

  door << about;

  door << door::reset << door::nl << "Press another key...";
  r = door.sleep_key(door.inactivity);
  if (r < 0)
    goto TIMEOUT;

  door << door::nl;

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

  Deck d(deck_color);
  door::Panel *c;
  door << door::reset << door::cls;

  // This displays the cards in the upper left corner.
  // We want them center, and down some.

  // int cards_delt_width = 59; int cards_delt_height = 9;
  int off_x = (mx - 59) / 2;
  int off_y = (my - 9) / 2;

  for (int x = 0; x < 28; x++) {
    int cx, cy, level;
    cardgo(x, cx, cy, level);
    c = d.back(level);
    c->set(cx + off_x, cy + off_y);
    door << *c;
  }
  door << door::nl << door::nl;

  r = door.sleep_key(door.inactivity);
  if (r < 0)
    goto TIMEOUT;

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
  display_starfield(mx, my, door, rng);
  door << m << door::reset << door::nl << "This is what the menu looked liked!"
       << door::nl;

  // Normal DOOR exit goes here...
  door << door::nl << "Returning you to the BBS, please wait..." << door::nl;

  return 0;
}
