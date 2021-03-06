#include "db.h"
#include "door.h"
#include "starfield.h"

class Scores {
private:
  door::Door &door;
  DBData &db;
  Starfield &stars;

  std::unique_ptr<door::Panel> top_scores;
  std::unique_ptr<door::Panel> top_this_month;

  std::unique_ptr<door::Panel> make_top_scores_panel();
  std::unique_ptr<door::Panel> make_top_this_month_panel();

public:
  Scores(door::Door &d, DBData &dbd, Starfield &sf);

  void display_scores(door::Door &door);
};
