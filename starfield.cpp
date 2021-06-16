#include "starfield.h"

starfield::starfield(door::Door &Door, std::mt19937 &Rng)
    : door{Door}, rng{Rng} {
  regenerate();
}

void starfield::regenerate(void) {
  int mx = door.width;
  int my = door.height;

  // Make uniform random distribution between 1 and MAX screen size X/Y
  std::uniform_int_distribution<int> uni_x(1, mx);
  std::uniform_int_distribution<int> uni_y(1, my);

  // 10 is too many, 100 is too few. 40 looks ok.
  int MAX_STARS = ((mx * my) / 40);
  // door.log() << "Generating starmap using " << mx << "," << my << " : "
  //           << MAX_STARS << " stars." << std::endl;

  for (int i = 0; i < MAX_STARS; i++) {
    star_pos pos;
    bool valid;
    do {
      pos.x = uni_x(rng);
      pos.y = uni_y(rng);
      auto ret = sky.insert(pos);
      // was insert a success?  (not a duplicate)
      valid = ret.second;
    } while (!valid);
  }
}

void starfield::display(void) {
  door << door::reset << door::cls;

  door::ANSIColor white(door::COLOR::WHITE);
  door::ANSIColor dark(door::COLOR::BLACK, door::ATTR::BRIGHT);

  // display starfield
  const char *stars[2];

  stars[0] = ".";
  if (door::unicode) {
    stars[1] = "\u2219"; // "\u00b7";

  } else {
    stars[1] = "\xf9"; // "\xfa";
  };

  int i = 0;
  star_pos last_pos;

  for (auto &pos : sky) {
    bool use_goto = true;

    if (i != 0) {
      // check last_pos to current position
      if (pos.y == last_pos.y) {
        // Ok, same row -- try some optimizations
        int dx = pos.x - last_pos.x;
        if (dx == 0) {
          use_goto = false;
        } else {
          if (dx < 5) {
            door << std::string(dx, ' ');
            use_goto = false;
          } else {
            // Use ANSI Cursor Forward
            door << "\x1b[" << dx << "C";
            use_goto = false;
          }
        }
      }
    }

    if (use_goto) {
      door::Goto star_at(pos.x, pos.y);
      door << star_at;
    }

    if (i % 5 < 2)
      door << dark;
    else
      door << white;

    if (i % 2 == 0)
      door << stars[0];
    else
      door << stars[1];

    ++i;
    last_pos = pos;
    last_pos.x++; // star output moves us by one.
  }
}

void display___starfield(door::Door &door, std::mt19937 &rng) {
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

    std::set<star_pos> sky;

    for (int i = 0; i < MAX_STARS; i++) {
      star_pos pos;
      bool valid;
      do {
        pos.x = uni_x(rng);
        pos.y = uni_y(rng);
        auto ret = sky.insert(pos);
        // was insert a success?  (not a duplicate)
        valid = ret.second;
      } while (!valid);
    }

    int i = 0;
    star_pos last_pos;

    for (auto &pos : sky) {
      bool use_goto = true;

      if (i != 0) {
        // check last_pos to current position
        if (pos.y == last_pos.y) {
          // Ok, same row -- try some optimizations
          int dx = pos.x - last_pos.x;
          if (dx == 0) {
            use_goto = false;
          } else {
            if (dx < 5) {
              door << std::string(dx, ' ');
              use_goto = false;
            } else {
              // Use ANSI Cursor Forward
              door << "\x1b[" << dx << "C";
              use_goto = false;
            }
          }
        }
      }

      if (use_goto) {
        door::Goto star_at(pos.x, pos.y);
        door << star_at;
      }

      if (i % 5 < 2)
        door << dark;
      else
        door << white;

      if (i % 2 == 0)
        door << stars[0];
      else
        door << stars[1];

      ++i;
      last_pos = pos;
      last_pos.x++; // star output moves us by one.
    }
  }
}
