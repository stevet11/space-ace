#include "play.h"
#include "db.h"
#include "deck.h"
#include "version.h"

#include <iomanip> // put_time
#include <sstream>

static std::function<std::ofstream &(void)> get_logger;

static int press_a_key(door::Door &door) {
  door << door::reset << "Press a key to continue...";
  int r = door.sleep_key(door.inactivity);
  door << door::nl;
  return r;
}

/*
In the future, this will probably check to see if they can play today or not, as
well as displaying a calendar to show what days are available to be played.

For now, it will play today.
*/

PlayCards::PlayCards(door::Door &d, DBData &dbd, std::mt19937 &r)
    : door{d}, db{dbd}, rng{r} {

  init_values();

  spaceAceTriPeaks = make_tripeaks();
  score_panel = make_score_panel();
  streak_panel = make_streak_panel();
  left_panel = make_left_panel();
  cmd_panel = make_command_panel();

  int mx = door.width;
  int my = door.height;

  play_day = std::chrono::system_clock::now();
  get_logger = [this]() -> ofstream & { return door.log(); };
}

PlayCards::~PlayCards() { get_logger = nullptr; }

void PlayCards::init_values(void) {
  hand = 1;
  total_hands = 3;
  card_number = 28;
  current_streak = 0;
  best_streak = 0;
  active_card = 23;
  score = 0;
}

void PlayCards::bonus(void) {
  door << door::ANSIColor(door::COLOR::YELLOW, door::ATTR::BOLD) << "BONUS";
}

int PlayCards::play_cards(void) {
  play_day = std::chrono::system_clock::now();
  init_values();
  std::string currentDefault = db.getSetting("DeckColor", "ALL");
  get_logger() << "DeckColor shows as " << currentDefault << std::endl;
  deck_color = from_string(currentDefault);

  dp = Deck(deck_color);

  // Calculate the game size
  int game_width;
  int game_height = 20;
  {
    int cx, cy, level;
    cardgo(27, cx, cy, level);
    game_width = cx + 5;
  }

  int mx = door.width;
  int my = door.height;

  off_x = (mx - game_width) / 2;
  off_y = (my - game_height) / 2;
  int true_off_y = off_y;

  // we can now position things properly centered

  int tp_off_x = (mx - spaceAceTriPeaks->getWidth()) / 2;
  spaceAceTriPeaks->set(tp_off_x, off_y);
  off_y += 3; // adjust for tripeaks panel

next_hand:
  card_number = 28;
  active_card = 23;
  score = 0;

  // Use play_day to seed the rng
  {
    time_t tt = std::chrono::system_clock::to_time_t(play_day);
    tm local_tm = *localtime(&tt);

    std::seed_seq seq{local_tm.tm_year + 1900, local_tm.tm_mon + 1,
                      local_tm.tm_mday, hand};
    deck = card_shuffle(seq, 1);
    state = card_states();
  }

  /*
    door::Panel score_panel = make_score_panel();
    door::Panel streak_panel = make_streak_panel();
    door::Panel left_panel = make_left_panel();
    door::Panel cmd_panel = make_command_panel();
  */

  {
    int off_yp = off_y + 11;
    int cxp, cyp, levelp;
    int left_panel_x, right_panel_x;
    // find position of card, to position the panels
    cardgo(18, cxp, cyp, levelp);
    left_panel_x = cxp;
    cardgo(15, cxp, cyp, levelp);
    right_panel_x = cxp;
    score_panel->set(left_panel_x + off_x, off_yp);
    streak_panel->set(right_panel_x + off_x, off_yp);
    cmd_panel->set(left_panel_x + off_x, off_yp + 5);
  }

  bool dealing = true;
  int r = 0;

  redraw(dealing);

  dealing = false;

  left_panel->update(door);
  door << door::reset;

  door::Panel *c;

  bool in_game = true;
  while (in_game) {
    // time might have updated, so update score panel too.
    score_panel->update(door);

    r = door.sleep_key(door.inactivity);
    if (r > 0) {
      // not a timeout or expire.
      if (r < 0x1000) // not a function key
        r = std::toupper(r);
      switch (r) {
      case '\x0d':
        if (card_number < 51) {
          card_number++;
          current_streak = 0;
          streak_panel->update(door);

          // Ok, deal next card from the pile.
          int cx, cy, level;

          if (card_number == 51) {
            cardgo(29, cx, cy, level);
            level = 0; // out of cards
            c = dp.back(level);
            c->set(cx + off_x, cy + off_y);
            door << *c;
          }
          cardgo(28, cx, cy, level);
          c = dp.card(deck.at(card_number));
          c->set(cx + off_x, cy + off_y);
          door << *c;
          // update the cards left_panel
          left_panel->update(door);
        }
        break;
      case 'R':
        // now_what = false;
        redraw(false);
        break;
      case 'Q':
        in_game = false;
        break;
      case ' ':
      case '5':
        // can we play this card?
        /*
        get_logger() << "can_play( " << active_card << ":"
                     << deck1.at(active_card) << "/"
                     << d.is_rank(deck1.at(active_card)) << " , "
                     << card_number << "/" <<
        d.is_rank(deck1.at(card_number))
                     << ") = "
                     << d.can_play(deck1.at(active_card),
                                   deck1.at(card_number))
                     << std::endl;
                     */

        if (dp.can_play(deck.at(active_card), deck.at(card_number))) {
          // if (true) {
          // yes we can.
          ++current_streak;
          if (current_streak > best_streak)
            best_streak = current_streak;
          streak_panel->update(door);
          score += 10;
          if (current_streak > 2)
            score += 5;

          // play card!
          state.at(active_card) = 2;
          {
            // swap the active card with card_number (play card)
            int temp = deck.at(active_card);
            deck.at(active_card) = deck.at(card_number);
            deck.at(card_number) = temp;
            // active_card is -- invalidated here!  find "new" card.
            int cx, cy, level;

            // erase/clear active_card
            std::vector<int> check = dp.unblocks(active_card);
            bool left = false, right = false;
            for (const int c : check) {
              std::pair<int, int> blockers = dp.blocks[c];
              if (blockers.first == active_card)
                right = true;
              if (blockers.second == active_card)
                left = true;
            }

            dp.remove_card(door, active_card, off_x, off_y, left, right);

            /*   // old way of doing this that leaves holes.
            cardgo(active_card, cx, cy, level);
            c = d.back(0);
            c->set(cx + off_x, cy + off_y);
            door << *c;
            */

            // redraw play card #28. (Which is the "old" active_card)
            cardgo(28, cx, cy, level);
            c = dp.card(deck.at(card_number));
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
                std::pair<int, int> blockers = dp.blocks[to_check];
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
                  c = dp.card(deck.at(to_check));
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
              door << door::Goto(cx + off_x, cy + off_y);
              bonus();

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
                in_game = false;
              }
            }
            // update the active_card marker!
            cardgo(active_card, cx, cy, level);
            c = dp.marker(1);
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
          c = dp.marker(0);
          c->set(cx + off_x + 2, cy + off_y + 2);
          door << *c;
          active_card = new_active;
          cardgo(active_card, cx, cy, level);
          c = dp.marker(1);
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
          c = dp.marker(0);
          c->set(cx + off_x + 2, cy + off_y + 2);
          door << *c;
          active_card = new_active;
          cardgo(active_card, cx, cy, level);
          c = dp.marker(1);
          c->set(cx + off_x + 2, cy + off_y + 2);
          door << *c;
        }
      }

      break;
      }
    } else
      in_game = false;
  }

  if (r == 'Q') {
    // continue with hand, or quit?
  }

  return r;
}

void PlayCards::redraw(bool dealing) {
  door::Panel *c;

  door << door::reset << door::cls;
  door << *spaceAceTriPeaks;

  {
    // step 1:
    // draw the deck "source"
    int cx, cy, level;
    cardgo(29, cx, cy, level);

    if (card_number == 51)
      level = 0; // out of cards!
    c = dp.back(level);
    c->set(cx + off_x, cy + off_y);
    // p3 is heigh below
    left_panel->set(cx + off_x, cy + off_y + height);

    // how do I update these? (hand >1)
    score_panel->update();
    left_panel->update();
    streak_panel->update();
    cmd_panel->update();

    door << *score_panel << *left_panel << *streak_panel << *cmd_panel;
    door << *c;
    if (dealing)
      std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  for (int x = 0; x < (dealing ? 28 : 29); x++) {
    int cx, cy, level;

    cardgo(x, cx, cy, level);
    // This is hardly visible.
    // door << door::Goto(cx + off_x - 1, cy + off_y + 1);
    if (dealing)
      std::this_thread::sleep_for(std::chrono::milliseconds(75));

    if (dealing) {
      c = dp.back(level);
      c->set(cx + off_x, cy + off_y);
      door << *c;
    } else {
      // redrawing -- draw the cards with their correct "state"
      int s = state.at(x);

      switch (s) {
      case 0:
        c = dp.back(level);
        c->set(cx + off_x, cy + off_y);
        door << *c;
        break;
      case 1:
        // cardgo(x, space, height, cx, cy, level);
        if (x == 28)
          c = dp.card(deck.at(card_number));
        else
          c = dp.card(deck.at(x));
        c->set(cx + off_x, cy + off_y);
        door << *c;
        break;
      case 2:
        // no card to draw.  :)
        break;
      case 3:
        // peak cleared, draw bonus
        door << door::Goto(cx + off_x, cy + off_y);
        bonus();
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

      c = dp.card(deck.at(x));
      c->set(cx + off_x, cy + off_y);
      door << *c;
    }

  {
    int cx, cy, level;
    cardgo(active_card, cx, cy, level);
    c = dp.marker(1);
    c->set(cx + off_x + 2, cy + off_y + 2);
    door << *c;
  }
}

door::renderFunction PlayCards::statusValue(door::ANSIColor status,
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

std::unique_ptr<door::Panel> PlayCards::make_score_panel() {
  const int W = 25;
  std::unique_ptr<door::Panel> p = std::make_unique<door::Panel>(W);
  p->setStyle(door::BorderStyle::NONE);
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
    p->addLine(std::make_unique<door::Line>(username));
  }
  {
    door::updateFunction scoreUpdate = [this](void) -> std::string {
      std::string text = "Score: ";
      text.append(std::to_string(score));
      return text;
    };
    std::string scoreString = scoreUpdate();
    door::Line score(scoreString, W);
    score.setRender(svRender);
    score.setUpdater(scoreUpdate);
    p->addLine(std::make_unique<door::Line>(score));
  }
  {
    door::updateFunction timeUpdate = [this](void) -> std::string {
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
    p->addLine(std::make_unique<door::Line>(time));
  }
  {
    door::updateFunction handUpdate = [this](void) -> std::string {
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
    p->addLine(std::make_unique<door::Line>(hands));
  }

  return p;
}

std::unique_ptr<door::Panel> PlayCards::make_streak_panel(void) {
  const int W = 20;
  std::unique_ptr<door::Panel> p = std::make_unique<door::Panel>(W);
  p->setStyle(door::BorderStyle::NONE);
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
    p->addLine(std::make_unique<door::Line>(current));
  }
  {
    door::updateFunction currentUpdate = [this](void) -> std::string {
      std::string text = "Current Streak: ";
      text.append(std::to_string(current_streak));
      return text;
    };
    std::string currentString = currentUpdate();
    door::Line current(currentString, W);
    current.setRender(svRender);
    current.setUpdater(currentUpdate);
    p->addLine(std::make_unique<door::Line>(current));
  }
  {
    door::updateFunction currentUpdate = [this](void) -> std::string {
      std::string text = "Longest Streak: ";
      text.append(std::to_string(best_streak));
      return text;
    };
    std::string currentString = currentUpdate();
    door::Line current(currentString, W);
    current.setRender(svRender);
    current.setUpdater(currentUpdate);
    p->addLine(std::make_unique<door::Line>(current));
  }

  return p;
}

std::unique_ptr<door::Panel> PlayCards::make_left_panel(void) {
  const int W = 13;
  std::unique_ptr<door::Panel> p = std::make_unique<door::Panel>(W);
  p->setStyle(door::BorderStyle::NONE);
  door::ANSIColor statusColor(door::COLOR::WHITE, door::COLOR::BLUE,
                              door::ATTR::BOLD);
  door::ANSIColor valueColor(door::COLOR::YELLOW, door::COLOR::BLUE,
                             door::ATTR::BOLD);
  door::renderFunction svRender = statusValue(statusColor, valueColor);

  {
    door::updateFunction cardsleftUpdate = [this](void) -> std::string {
      std::string text = "Cards left:";
      text.append(std::to_string(51 - card_number));
      return text;
    };
    std::string cardsleftString = "Cards left:--";
    door::Line cardsleft(cardsleftString, W);
    cardsleft.setRender(svRender);
    cardsleft.setUpdater(cardsleftUpdate);
    p->addLine(std::make_unique<door::Line>(cardsleft));
  }
  return p;
}

door::renderFunction PlayCards::commandLineRender(door::ANSIColor bracket,
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

std::unique_ptr<door::Panel> PlayCards::make_command_panel(void) {
  const int W = 76;
  std::unique_ptr<door::Panel> p = make_unique<door::Panel>(W);
  p->setStyle(door::BorderStyle::NONE);
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
  p->addLine(std::make_unique<door::Line>(cmd));
  return p;
}

std::unique_ptr<door::Panel> PlayCards::make_tripeaks(void) {
  std::string tripeaksText(" " SPACEACE
                           " - Tri-Peaks Solitaire v" SPACEACE_VERSION " ");
  std::unique_ptr<door::Panel> spaceAceTriPeaks =
      std::make_unique<door::Panel>(tripeaksText.size());
  spaceAceTriPeaks->setStyle(door::BorderStyle::SINGLE);
  spaceAceTriPeaks->setColor(
      door::ANSIColor(door::COLOR::CYAN, door::COLOR::BLACK));
  spaceAceTriPeaks->addLine(
      std::make_unique<door::Line>(tripeaksText, tripeaksText.size()));
  return spaceAceTriPeaks;
}

#ifdef OLD_WAY

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
        case '5':
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
    // continue with hand or quit?
    if (hand < total_hands) {
      press_a_key(door);
      hand++;
      door << door::reset << door::cls;
      goto next_hand;
    }
  }
  return r;
}
#endif