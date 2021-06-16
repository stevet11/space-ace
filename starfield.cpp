#include "starfield.h"
#include "utils.h"

Starfield::Starfield(door::Door &Door, std::mt19937 &Rng)
    : door{Door}, rng{Rng}, uni_x{1, Door.width}, uni_y{1, Door.height},
      white{door::COLOR::WHITE}, dark{door::COLOR::BLACK, door::ATTR::BRIGHT} {
  // std::uniform_int_distribution<int> uni_x(1, mx);
  // std::uniform_int_distribution<int> uni_y(1, my);
  regenerate();
}

star_pos Starfield::make_pos(void) {
  star_pos pos;
  do {
    pos.x = uni_x(rng);
    pos.y = uni_y(rng);
  } while (sky.find(pos) != sky.end());
  return pos;
}

void Starfield::regenerate(void) {
  int mx = door.width;
  int my = door.height;

  // Make uniform random distribution between 1 and MAX screen size X/Y
  sky.clear();

  // 10 is too many, 100 is too few. 40 looks ok.
  int MAX_STARS = ((mx * my) / 40);
  // door.log() << "Generating starmap using " << mx << "," << my << " : "
  //           << MAX_STARS << " stars." << std::endl;

  for (int i = 0; i < MAX_STARS; i++) {
    star_pos pos = make_pos();
    // store symbol and color in star_pos.
    // If we do any animation, we won't be able to rely on the star_pos keeping
    // the same position in the set.
    pos.symbol = i % 2;
    pos.color = i % 5 < 2;
    sky.insert(pos);
  }
}

void Starfield::display(void) {
  door << door::reset << door::cls;

  stars[0] = ".";
  if (door::unicode) {
    stars[1] = "\u2219"; // "\u00b7";
  } else {
    stars[1] = "\xf9"; // "\xfa";
  };

  int i = 0;
  // lose int i, and set last_pos to -1, -1.
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

    if (pos.color)
      door << dark;
    else
      door << white;

    if (pos.symbol)
      door << stars[0];
    else
      door << stars[1];

    ++i;
    last_pos = pos;
    last_pos.x++; // star output moves us by one.
  }
}

AnimatedStarfield::AnimatedStarfield(door::Door &Door, std::mt19937 &Rng)
    : door{Door}, rng{Rng}, uni_x{1, Door.width}, uni_y{1, Door.height},
      white{door::COLOR::WHITE}, dark{door::COLOR::BLACK, door::ATTR::BRIGHT} {
  mx = door.width;
  my = door.height;
  cx = mx / 2.0;
  cy = my / 2.0;
  max_d = distance(0, 0);

  // std::uniform_int_distribution<int> uni_x(1, mx);
  // std::uniform_int_distribution<int> uni_y(1, my);
  regenerate();
}

/**
 * @brief Make a moving_star
 *
 * This makes a new x,y point.
 *
 * If "centered", we create a point nearer to the center of the screen.
 *
 * Also, if centered, we don't allow y == cy.  FIXES BUG: syncterm 80x25.
 *
 * moving_star.visible is set by the visibleFunction.
 * moving_star.xpos, ypos is initialized from x,y.
 * moving_star.movex, movey is initialized from the rise/run from center.
 * @param centered
 * @return moving_star
 */
moving_star AnimatedStarfield::make_pos(bool centered) {
  moving_star pos;
  do {
    pos.x = uni_x(rng);
    pos.y = uni_y(rng);
    pos.visible = true;
    if (centered) {
      pos.x /= 4;
      pos.x += cx - (cx / 4);
      pos.y /= 4;
      pos.y += cy - (cy / 4);
    }
    pos.xpos = pos.x;
    pos.ypos = pos.y;
    pos.movex = pos.xpos - cx;
    pos.movey = pos.ypos - cy;
    double bigd = max(abs(pos.movex), abs(pos.movey));
    pos.movex /= bigd;
    pos.movey /= bigd;

    if (visible) {
      pos.visible = visible(pos.x, pos.y);
    }
  } while (centered and (pos.y == cy));

  /*
    if (get_logger)
      get_logger() << pos.x << "," << pos.y << " " << pos.movex << "/"
                   << pos.movey << std::endl;
  */

  return pos;
}

void AnimatedStarfield::regenerate(void) {
  // Make uniform random distribution between 1 and MAX screen size X/Y
  sky.clear();

  // 10 is too many, 100 is too few. 40 looks ok.
  int MAX_STARS = ((mx * my) / 40);
  // door.log() << "Generating starmap using " << mx << "," << my << " : "
  //           << MAX_STARS << " stars." << std::endl;

  for (int i = 0; i < MAX_STARS; i++) {
    moving_star pos = make_pos();
    pos.symbol = i % 2;
    pos.color = i % 5 < 2;
    sky.push_back(pos);
  }
}

void AnimatedStarfield::display(void) {
  door << door::reset << door::cls;

  stars[0] = ".";
  if (door::unicode) {
    stars[1] = "\u2219"; // "\u00b7";
  } else {
    stars[1] = "\xf9"; // "\xfa";
  };

  for (auto &pos : sky) {
    if (pos.visible) {
      door::Goto star_at(pos.x, pos.y);
      door << star_at;

      if (pos.color)
        door << dark;
      else
        door << white;

      if (pos.symbol)
        door << stars[0];
      else
        door << stars[1];
    }
  }
}

void AnimatedStarfield::animate(void) {

  for (auto &star : sky) {
    // just start with basic movement
    // double speed = abs((cx / 2) - distance(star.movex, star.movey));
    double speed = max_d / distance(star.movex, star.movey);
    star.xpos += star.movex / (speed * (mx / my));
    star.ypos += star.movey / speed;

    int nx = int(star.xpos + 0.5);
    int ny = int(star.ypos + 0.5);

    // watch the bottom of the screen!  It'll scroll!
    // watch new position we're given!

    while ((star.xpos < 1) or (star.xpos >= mx - 1) or (star.ypos < 1) or
           (star.ypos >= my - 1)) {
      // off the screen!  Whoops!
      moving_star new_pos = make_pos(true);
      nx = new_pos.x;
      ny = new_pos.y;
      star.xpos = new_pos.x;
      star.ypos = new_pos.y;
      star.movex = new_pos.movex;
      star.movey = new_pos.movey;
      // new position updated -- next section will erase and display in new
      // position
    }

    // if ((star.x != int(star.xpos)) or (star.y == int(star.ypos))) {
    if ((star.x != nx) or (star.y != ny)) {
      // star has moved, update time
      if (star.visible) {
        door << door::Goto(star.x, star.y) << " ";
      };
      star.x = nx; // int(star.xpos);
      star.y = ny; // int(star.ypos);

      if (visible) {
        star.visible = visible(star.x, star.y);
      }

      // still visible?
      if (star.visible) {
        door << door::Goto(star.x, star.y);
        if (star.color)
          door << dark;
        else
          door << white;

        if (star.symbol)
          door << stars[0];
        else
          door << stars[1];
      }
    }
  }
}

double AnimatedStarfield::distance(double x, double y) {
  return sqrt(((cx - x) * (cx - x)) + ((cy - y) * (cy - y)));
}

void AnimatedStarfield::setVisible(checkVisibleFunction cvf) { visible = cvf; }