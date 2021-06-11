#ifndef UTILS_H
#define UTILS_H

#include <fstream>
#include <functional>
#include <string.h>
#include <string>
#include <utility> // pair
#include <vector>

// utility functions go here

bool replace(std::string &str, const std::string &from, const std::string &to);
bool replace(std::string &str, const char *from, const char *to);
bool file_exists(const std::string &name);
bool file_exists(const char *name);
void string_toupper(std::string &str);
bool iequals(const std::string &a, const std::string &b);

std::vector<std::pair<int, int>> find_words(const std::string &text);

// logger access
extern std::function<std::ofstream &(void)> get_logger;

extern std::function<void(void)> cls_display_starfield;

extern std::function<int(void)> press_a_key;

// configuration settings access
#include "yaml-cpp/yaml.h"
extern YAML::Node config;

#endif