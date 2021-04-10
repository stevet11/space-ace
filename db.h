#ifndef DB_H
#define DB_H

#include <SQLiteCpp/SQLiteCpp.h>

class DBData {
  SQLite::Database db;
  void init(void);
  std::string user;
  std::unique_ptr<SQLite::Statement> stmt_getSet;
  std::unique_ptr<SQLite::Statement> stmt_setSet;

public:
  DBData();
  virtual ~DBData();
  void setUser(std::string user);
  void clearUser(void) { user.clear(); };
  /*
    std::string getSetting(const std::string &user, const std::string &setting,
                           std::string ifMissing);
    void setSetting(const std::string &user, const std::string &setting,
                    const std::string &value);*/
  std::string getSetting(const std::string &setting, std::string ifMissing);
  void setSetting(const std::string &setting, const std::string &value);
  void save_score(time_t when, time_t date, int hand, int score);
  int has_played_day(time_t day);
};

void standard_date(time_t &tt, int hour = 2);
std::string make_date(time_t tt);

#endif