#ifndef DB_H
#define DB_H

#include <SQLiteCpp/SQLiteCpp.h>

#include <chrono>
#include <map>
#include <string>
#include <vector>

typedef struct {
  time_t date;
  std::string user;
  int hand;
  int score;
  int won;
} scores_details;

typedef struct {
  time_t date;
  std::string user;
  int score;
  int won;
} scores_data;

typedef struct {
  time_t date;
  std::string user;
  int days;
  int hands_won;
  int score;
} monthly_data;

class DBData {
  SQLite::Database db;
  void create_tables(void);
  std::string user;
  int locked_retries;

  /*
  I thought some of my performance problems were from the prepared statement
  calls.  It was not the case, there's a weird delay when I save/hit the
  database.  This didn't fix it, but they are still present.
  Prepared statements, prepared ahead of time, are a good thing.

  Unless, they lock the entire database.

  std::unique_ptr<SQLite::Statement> stmt_getSet;
  std::unique_ptr<SQLite::Statement> stmt_setSet;
  */
  void retry_wait(void);

public:
  DBData();
  virtual ~DBData();

  void setUser(std::string currentUser) { user = currentUser; };
  void clearUser(void) { user.clear(); };
  std::string getSetting(const std::string &setting, std::string ifMissing);
  void setSetting(const std::string &setting, const std::string &value);
  void saveScore(time_t when, time_t date, int hand, int won, int score);

  std::vector<scores_details> getScoresOnDay(time_t date);
  std::vector<monthly_data> getMonthlyScores(int limit = 10);
  std::vector<scores_data> getScores(int limit = 10);
  std::map<time_t, int> getPlayed(void);
  void expireScores(time_t month_first_t);

  int handsPlayedOnDay(time_t day);
  std::map<time_t, int> whenPlayed(void);
};

void normalizeDate(std::chrono::_V2::system_clock::time_point &date);
void normalizeDate(time_t &tt, int hour = 2);
void firstOfMonthDate(std::chrono::_V2::system_clock::time_point &date);
std::string convertDateToDateScoreFormat(time_t tt);
std::string convertDateToMonthlyFormat(time_t tt);

#endif