#include "utils.h"

#include <algorithm>
#include <regex>

bool replace(std::string &str, const std::string &from, const std::string &to) {
  size_t start_pos = str.find(from);
  if (start_pos == std::string::npos)
    return false;
  str.replace(start_pos, from.length(), to);
  return true;
}

bool replace(std::string &str, const char *from, const char *to) {
  size_t start_pos = str.find(from);
  if (start_pos == std::string::npos)
    return false;
  str.replace(start_pos, strlen(from), to);
  return true;
}

bool file_exists(const std::string &name) {
  std::ifstream f(name.c_str());
  return f.good();
}

bool file_exists(const char *name) {
  std::ifstream f(name);
  return f.good();
}

void string_toupper(std::string &str) {
  std::transform(str.begin(), str.end(), str.begin(), ::toupper);
}

/**
 * @brief Case Insensitive std::string compare
 *
 * @param a
 * @param b
 * @return true
 * @return false
 */
bool iequals(const std::string &a, const std::string &b) {
  unsigned int sz = a.size();
  if (b.size() != sz)
    return false;
  for (unsigned int i = 0; i < sz; ++i)
    if (std::tolower(a[i]) != std::tolower(b[i]))
      return false;
  return true;
}

std::vector<std::pair<int, int>> find_words(const std::string &text) {
  std::vector<std::pair<int, int>> words;
  // this is expensive to construct, so only construct it once.
  static std::regex word("([a-zA-Z]+)");

  for (auto it = std::sregex_iterator(text.begin(), text.end(), word);
       it != std::sregex_iterator(); ++it) {

    words.push_back(std::make_pair(it->position(), it->length()));
  }
  return words;
}

// From: https://stackoverflow.com/questions/236129/how-do-i-iterate-over-the-words-of-a-string

std::vector<std::string> split(const std::string &text, char sep) {
    std::vector<std::string> tokens;
    std::size_t start = 0, end = 0;
    while ((end = text.find(sep, start)) != std::string::npos) {
        if (end != start) {
          tokens.push_back(text.substr(start, end - start));
        }
        start = end + 1;
    }
    if (end != start) {
       tokens.push_back(text.substr(start));
    }
    return tokens;
}