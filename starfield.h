#ifndef STARFIELD_H
#define STARFIELD_H

#include "door.h"
#include <random>
#include <set>

struct star_pos {
  int x;
  int y;
  int symbol;
  int color;

  /**
   * @brief Provide less than operator.
   *
   * This will allow the star_pos to be stored sorted (top->bottom,
   * left->right) in a set.
   *
   * @param rhs
   * @return true
   * @return false
   */
  bool operator<(const star_pos rhs) const {
    if (rhs.y > y)
      return true;
    if (rhs.y == y) {
      if (rhs.x > x)
        return true;
      return false;
    }
    return false;
  }
};

class Starfield {
  door::Door &door;
  std::mt19937 &rng;
  std::set<star_pos> sky;
  std::uniform_int_distribution<int> uni_x;
  std::uniform_int_distribution<int> uni_y;
  star_pos make_pos(void);

  door::ANSIColor white; //(door::COLOR::WHITE);
  door::ANSIColor dark;  //(door::COLOR::BLACK, door::ATTR::BRIGHT);
  const char *stars[2];

public:
  Starfield(door::Door &Door, std::mt19937 &Rng);
  void regenerate(void);
  void display(void);
};

struct moving_star {
  int x;
  int y;
  int symbol;
  int color;
  bool visible;
  double xpos;
  double ypos;
  double movex;
  double movey;
};

typedef std::function<bool(int x, int y)> checkVisibleFunction;

class AnimatedStarfield {
  door::Door &door;
  std::mt19937 &rng;
  std::vector<moving_star> sky;
  std::uniform_int_distribution<int> uni_x;
  std::uniform_int_distribution<int> uni_y;
  moving_star make_pos(bool centered = false);

  int mx;
  int my;
  double cx;
  double cy;
  double max_d;

  door::ANSIColor white;
  door::ANSIColor dark;
  const char *stars[2];
  double distance(double x, double y);
  checkVisibleFunction visible;

public:
  AnimatedStarfield(door::Door &door, std::mt19937 &Rng);
  void regenerate(void);
  void display(void);
  void animate(void);
  void setVisible(checkVisibleFunction cvf);
};

#endif
