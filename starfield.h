#include "door.h"
#include <random>
#include <set>

struct star_pos {
  int x;
  int y;

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

class starfield {
  door::Door &door;
  std::mt19937 &rng;
  std::set<star_pos> sky;

public:
  starfield(door::Door &Door, std::mt19937 &Rng);
  void regenerate(void);
  void display(void);
};
