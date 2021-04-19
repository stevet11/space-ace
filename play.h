#ifndef PLAY_H
#define PLAY_H

#include "db.h"
#include "deck.h"
#include "door.h"

#include <chrono>
#include <random>

class PlayCards {
private:
  door::Door &door;
  DBData &db;
  std::mt19937 &rng;

  std::unique_ptr<door::Panel> spaceAceTriPeaks;
  std::unique_ptr<door::Panel> score_panel;
  std::unique_ptr<door::Panel> streak_panel;
  std::unique_ptr<door::Panel> left_panel;
  std::unique_ptr<door::Panel> cmd_panel;
  std::unique_ptr<door::Panel> next_quit_panel;

  std::unique_ptr<door::Panel> make_score_panel();
  std::unique_ptr<door::Panel> make_tripeaks(void);
  std::unique_ptr<door::Panel> make_command_panel(void);
  std::unique_ptr<door::Panel> make_streak_panel(void);
  std::unique_ptr<door::Panel> make_left_panel(void);
  std::unique_ptr<door::Panel> make_next_panel(void);

  int hand;
  int total_hands;
  int play_card;
  int current_streak;
  int best_streak;
  int select_card; // the card the player selects, has state=1
  unsigned long score;
  int days_played;

  Deck dp; // deckPanels
  int off_x, off_y;
  const int height = 3;

  std::chrono::_V2::system_clock::time_point play_day;

  door::ANSIColor deck_color;
  cards deck;
  cards state;

  void redraw(bool dealing);
  void bonus(void);

  int play_cards(void);

public:
  PlayCards(door::Door &d, DBData &dbd, std::mt19937 &r);
  ~PlayCards();

  int play(void);
  void init_values(void);

  door::renderFunction statusValue(door::ANSIColor status,
                                   door::ANSIColor value);
  door::renderFunction commandLineRender(door::ANSIColor bracket,
                                         door::ANSIColor inner,
                                         door::ANSIColor outer);
};

#endif