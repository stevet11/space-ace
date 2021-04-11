#include "play.h"
#include "db.h"
#include "deck.h"
#include "version.h"

#include <iomanip> // put_time
#include <sstream>

// configuration settings access
#include "yaml-cpp/yaml.h"
extern YAML::Node config;

#define CHEATER "CHEAT_YOUR_ASS_OFF"

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

PlayCards::PlayCards(door::Door &d, DBData &dbd) : door{d}, db{dbd} {
  get_logger = [this]() -> ofstream & { return door.log(); };
  init_values();

  play_day = std::chrono::system_clock::now();
  // adjustment
  time_t time_play_day = std::chrono::system_clock::to_time_t(play_day);
  if (get_logger) {
    get_logger() << "before: "
                 << std::put_time(std::localtime(&time_play_day), "%F %R")
                 << std::endl;
  };
  normalizeDate(time_play_day);
  play_day = std::chrono::system_clock::from_time_t(time_play_day);
  if (get_logger) {
    get_logger() << "after: "
                 << std::put_time(std::localtime(&time_play_day), "%F %R")
                 << std::endl;
  };
  spaceAceTriPeaks = make_tripeaks();
  score_panel = make_score_panel();
  streak_panel = make_streak_panel();
  left_panel = make_left_panel();
  cmd_panel = make_command_panel();
  next_quit_panel = make_next_panel();

  /*
    int mx = door.width;
    int my = door.height;
  */
}

PlayCards::~PlayCards() { get_logger = nullptr; }

void PlayCards::init_values(void) {
  hand = 1;
  if (config["hands_per_day"]) {
    total_hands = config["hands_per_day"].as<int>();
  } else
    total_hands = 3;

  play_card = 28;
  current_streak = 0;
  best_streak = 0;
  std::string best;
  best = db.getSetting("best_streak", "0");
  best_streak = std::stoi(best);
  select_card = 23;
  score = 0;
}

void PlayCards::bonus(void) {
  door << door::ANSIColor(door::COLOR::YELLOW, door::ATTR::BOLD) << "BONUS";
}

int PlayCards::play_cards(void) {
  init_values();
  std::string currentDefault = db.getSetting("DeckColor", "ALL");
  get_logger() << "DeckColor shows as " << currentDefault << std::endl;
  deck_color = stringToANSIColor(currentDefault);

  dp = Deck(deck_color);

  // Calculate the game size
  int game_width;
  int game_height = 20;
  {
    int cx, cy;
    cardPos(27, cx, cy);
    game_width = cx + 5;
  }

  int mx = door.width;
  int my = door.height;

  off_x = (mx - game_width) / 2;
  off_y = (my - game_height) / 2;
  // int true_off_y = off_y;

  // we can now position things properly centered

  int tp_off_x = (mx - spaceAceTriPeaks->getWidth()) / 2;
  spaceAceTriPeaks->set(tp_off_x, off_y);
  off_y += 3; // adjust for tripeaks panel

next_hand:
  play_card = 28;
  select_card = 23;
  score = 0;
  current_streak = 0;

  // Use play_day to seed the rng
  {
    time_t tt = std::chrono::system_clock::to_time_t(play_day);
    tm local_tm = *localtime(&tt);

    std::seed_seq seq{local_tm.tm_year + 1900, local_tm.tm_mon + 1,
                      local_tm.tm_mday, hand};
    deck = shuffleCards(seq, 1);
    state = makeCardStates();
  }

  /*
    door::Panel score_panel = make_score_panel();
    door::Panel streak_panel = make_streak_panel();
    door::Panel left_panel = make_left_panel();
    door::Panel cmd_panel = make_command_panel();
  */

  {
    int off_yp = off_y + 11;
    int cx, cy;
    int left_panel_x, right_panel_x;
    // find position of card, to position the panels
    cardPos(18, cx, cy);
    left_panel_x = cx;
    cardPos(15, cx, cy);
    right_panel_x = cx;
    score_panel->set(left_panel_x + off_x, off_yp);
    streak_panel->set(right_panel_x + off_x, off_yp);
    cmd_panel->set(left_panel_x + off_x, off_yp + 5);

    // next panel position
    cardPos(10, cx, cy);
    int next_off_x = (mx - next_quit_panel->getWidth()) / 2;
    next_quit_panel->set(next_off_x, cy + off_y);
  }

  bool dealing = true;
  int r = 0;

  redraw(dealing);

  dealing = false;

  left_panel->update(door);
  door << door::reset;

  shared_panel c;

  bool in_game = true;
  bool save_streak = false;

  while (in_game) {
    // time might have updated, so update score panel too.
    score_panel->update(door);

    // do the save here -- try to hide the database lag
    if (save_streak) {
      save_streak = false;
      std::string best = std::to_string(best_streak);
      db.setSetting("best_streak", best);
    }

    r = door.sleep_key(door.inactivity);
    if (r > 0) {
      // not a timeout or expire.
      if (r < 0x1000) // not a function key
        r = std::toupper(r);
      switch (r) {
      case '\x0d':
        if (play_card < 51) {
          play_card++;
          current_streak = 0;
          streak_panel->update(door);

          // update the cards left_panel
          left_panel->update(door);

          // Ok, deal next card from the pile.
          int cx, cy, level;

          if (play_card == 51) {
            cardPosLevel(29, cx, cy, level);
            level = 0; // out of cards
            c = dp.back(level);
            c->set(cx + off_x, cy + off_y);
            door << *c;
          }
          cardPos(28, cx, cy);
          c = dp.card(deck.at(play_card));
          c->set(cx + off_x, cy + off_y);
          door << *c;
        }
        break;
      case 'R':
        redraw(false);
        break;
      case 'Q':
        // possibly prompt here for [N]ext hand or [Q]uit ?
        next_quit_panel->update();
        door << *next_quit_panel;

        if (hand < total_hands) {
          r = door.get_one_of("CQN");
        } else {
          r = door.get_one_of("CQ");
        }

        if (r == 0) {
          // continue
          redraw(false);
          break;
        }
        if (r == 1) {
          // Ok, we are calling it quits.
          // save score if > 0
          if (score >= 50) {
            time_t right_now = std::chrono::system_clock::to_time_t(
                std::chrono::system_clock::now());
            db.saveScore(right_now,
                         std::chrono::system_clock::to_time_t(play_day), hand,
                         score);
          }
          in_game = false;
          r = 'Q';
        }
        if (r == 2) {
          // no.  If you want to play the next hand, we have to save this.
          get_logger() << "SCORE: " << score << std::endl;
          time_t right_now = std::chrono::system_clock::to_time_t(
              std::chrono::system_clock::now());
          db.saveScore(right_now,
                       std::chrono::system_clock::to_time_t(play_day), hand,
                       score);
          hand++;
          goto next_hand;
        }
        // in_game = false;
        break;
      case ' ':
      case '5':
        // can we play this card?
        /*
        get_logger() << "canPlay( " << select_card << ":"
                     << deck1.at(select_card) << "/"
                     << d.getRank(deck1.at(select_card)) << " , "
                     << play_card << "/" <<
        d.getRank(deck1.at(play_card))
                     << ") = "
                     << d.canPlay(deck1.at(select_card),
                                   deck1.at(play_card))
                     << std::endl;
                     */

        if (dp.canPlay(deck.at(select_card), deck.at(play_card)) or
            config[CHEATER]) {
          // if (true) {
          // yes we can.
          ++current_streak;
          if (current_streak > best_streak) {
            best_streak = current_streak;
            if (!config[CHEATER]) {
              save_streak = true;
            }
          }
          streak_panel->update(door);
          score += 10;
          if (current_streak > 1)
            score += current_streak * 5;
          score_panel->update(door);
          /*
          if (get_logger)
            get_logger() << "score_panel update : " << score << std::endl;
            */

          // play card!
          state.at(select_card) = 2;
          {
            // swap the select card with play_card
            int temp = deck.at(select_card);
            deck.at(select_card) = deck.at(play_card);
            deck.at(play_card) = temp;
            // select_card is -- invalidated here!  find "new" card.
            int cx, cy;

            // erase/clear select_card
            std::vector<int> check = dp.unblocks(select_card);
            bool left = false, right = false;
            for (const int c : check) {
              std::pair<int, int> blockers = dp.blocks[c];
              if (blockers.first == select_card)
                right = true;
              if (blockers.second == select_card)
                left = true;
            }

            dp.removeCard(door, select_card, off_x, off_y, left, right);

            /*   // old way of doing this that leaves holes.
            cardPosLevel(select_card, cx, cy, level);
            c = d.back(0);
            c->set(cx + off_x, cy + off_y);
            door << *c;
            */

            // redraw play card #28. (Which is the "old" select_card)
            cardPos(28, cx, cy);
            c = dp.card(deck.at(play_card));
            c->set(cx + off_x, cy + off_y);
            door << *c;

            // Did we unhide a card here?

            // std::vector<int> check = d.unblocks(select_card);

            /*
            get_logger() << "select_card = " << select_card
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
                  cardPos(to_check, cx, cy);
                  c = dp.card(deck.at(to_check));
                  c->set(cx + off_x, cy + off_y);
                  door << *c;
                  new_card_shown = to_check;
                }
              }
            } else {
              // top card cleared
              // get_logger() << "top card cleared?" << std::endl;
              // display something at select_card position
              int cx, cy;
              cardPos(select_card, cx, cy);
              door << door::Goto(cx + off_x, cy + off_y);
              bonus();

              score += 100;
              state.at(select_card) = 3; // handle this in the "redraw"
              score_panel->update(door);
            }

            // Find new "number" for select_card to be.
            if (new_card_shown != -1) {
              select_card = new_card_shown;
            } else {
              // select_card++;
              int new_select = findClosestActiveCard(state, select_card);

              if (new_select != -1) {
                select_card = new_select;
              } else {
                get_logger() << "Winner!" << std::endl;
                select_card = -1; // winner marker?

                // bonus for cards left
                int bonus = 15 * (51 - play_card);

                score += 15 * (51 - play_card);
                score_panel->update(door);

                // maybe display this somewhere?
                // door << " BONUS: " << bonus << door::nl;
                get_logger()
                    << "SCORE: " << score << ", " << bonus << std::endl;

                // if (!config[CHEATER]) {
                time_t right_now = std::chrono::system_clock::to_time_t(
                    std::chrono::system_clock::now());
                db.saveScore(right_now,
                             std::chrono::system_clock::to_time_t(play_day),
                             hand, score);
                //}
                next_quit_panel->update();
                door << *next_quit_panel;

                if (hand < total_hands) {
                  r = door.get_one_of("QN");
                } else {
                  r = door.get_one_of("Q");
                }

                if (r == 1) {
                  hand++;
                  goto next_hand;
                }

                if (r == 0) {
                  r = 'Q';
                }
                /*
                press_a_key(door);

                if (hand < total_hands) {
                  hand++;
                  // current_streak = 0;
                  door << door::reset << door::cls;
                  goto next_hand;
                }
                r = 'Q';
                */
                in_game = false;
              }
            }
            // update the select_card marker!
            cardPos(select_card, cx, cy);
            c = dp.marker(1);
            c->set(cx + off_x + 2, cy + off_y + 2);
            door << *c;
          }
        }
        break;
      case XKEY_LEFT_ARROW:
      case '4': {
        int new_select = findNextActiveCard(true, state, select_card);
        /*
        int new_active = active_card - 1;
        while (new_active >= 0) {
          if (state.at(new_active) == 1)
            break;
          --new_active;
        }*/
        if (new_select >= 0) {

          int cx, cy;
          cardPos(select_card, cx, cy);
          c = dp.marker(0);
          c->set(cx + off_x + 2, cy + off_y + 2);
          door << *c;
          select_card = new_select;
          cardPos(select_card, cx, cy);
          c = dp.marker(1);
          c->set(cx + off_x + 2, cy + off_y + 2);
          door << *c;
        }
      } break;
      case XKEY_RIGHT_ARROW:
      case '6': {
        int new_select = findNextActiveCard(false, state, select_card);
        /*
        int new_active = active_card + 1;
        while (new_active < 28) {
          if (state.at(new_active) == 1)
            break;
          ++new_active;
        }
        */
        if (new_select >= 0) { //(new_active < 28) {
          int cx, cy;
          cardPos(select_card, cx, cy);
          c = dp.marker(0);
          c->set(cx + off_x + 2, cy + off_y + 2);
          door << *c;
          select_card = new_select;
          cardPos(select_card, cx, cy);
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
    // continue, play next hand (if applicable), or quit?
    // if score < 50, don't bother saving.

    // continue -- eat r & redraw.
    // quit, save score and exit  (unless score is zero).
  }

  return r;
}

void PlayCards::redraw(bool dealing) {
  shared_panel c;

  door << door::reset << door::cls;
  door << *spaceAceTriPeaks;

  {
    // step 1:
    // draw the deck "source"
    int cx, cy, level;
    cardPosLevel(29, cx, cy, level);

    if (play_card == 51)
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

    cardPosLevel(x, cx, cy, level);
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
        // cardPosLevel(x, space, height, cx, cy, level);
        if (x == 28)
          c = dp.card(deck.at(play_card));
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
      int cx, cy;

      state.at(x) = 1;
      cardPos(x, cx, cy);
      // door << door::Goto(cx + off_x - 1, cy + off_y + 1);
      std::this_thread::sleep_for(std::chrono::milliseconds(200));

      c = dp.card(deck.at(x));
      c->set(cx + off_x, cy + off_y);
      door << *c;
    }

  {
    int cx, cy;
    cardPos(select_card, cx, cy);
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
    door::Line scoreline(scoreString, W);
    scoreline.setRender(svRender);
    scoreline.setUpdater(scoreUpdate);
    p->addLine(std::make_unique<door::Line>(scoreline));
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
    if (config["date_format"]) {
      std::string fmt = config["date_format"].as<std::string>();
      ss << std::put_time(std::localtime(&in_time_t), fmt.c_str());
    } else
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
      text.append(std::to_string(51 - play_card));
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

std::unique_ptr<door::Panel> PlayCards::make_next_panel(void) {
  const int W = 50;
  std::unique_ptr<door::Panel> p = make_unique<door::Panel>(W);
  door::ANSIColor panelColor(door::COLOR::YELLOW, door::COLOR::GREEN,
                             door::ATTR::BOLD);
  p->setStyle(door::BorderStyle::DOUBLE);
  p->setColor(panelColor);

  door::updateFunction nextUpdate = [this](void) -> std::string {
    std::string text;
    if (select_card == -1) {
      // winner
      if (hand < total_hands) {
        text = " [N]ext Hand - [Q]uit";
      } else
        text = " All hands played - [Q]uit";
    } else if (hand < total_hands) {
      text = " [C]ontinue this hand - [N]ext Hand - [Q]uit";
    } else {
      text = " [C]ontinue this hand - [Q]uit";
    }
    return text;
  };

  std::string text;
  text = nextUpdate();

  door::ANSIColor bracketColor(door::COLOR::YELLOW, door::COLOR::BLUE,
                               door::ATTR::BOLD);
  door::ANSIColor innerColor(door::COLOR::CYAN, door::COLOR::BLUE,
                             door::ATTR::BOLD);
  door::ANSIColor outerColor(door::COLOR::GREEN, door::COLOR::BLUE,
                             door::ATTR::BOLD);

  door::renderFunction textRender =
      commandLineRender(bracketColor, innerColor, outerColor);
  door::Line nextLine(text, W);
  nextLine.setRender(textRender);
  // nextLine.setPadding(" ", panelColor);
  nextLine.setUpdater(nextUpdate);
  p->addLine(std::make_unique<door::Line>(nextLine));
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
