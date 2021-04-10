#include "db.h"

#include <SQLiteCpp/VariadicBind.h>

#include <iomanip>
#include <iostream>
#include <sstream>

// configuration settings access
#include "yaml-cpp/yaml.h"
extern YAML::Node config;

#include <fstream>
#include <functional>
extern std::function<std::ofstream &(void)> get_logger;

/*
The database access is slow.

So, make sure you set it up so that you do your writes right
before you collect user input.  That way, the user won't see
the lags.
*/

DBData::DBData(void)
    : db("space-data.db", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE) {

  init();
  stmt_getSet = std::make_unique<SQLite::Statement>(
      db, "SELECT value FROM settings WHERE username=? AND setting=?");
  stmt_setSet = std::make_unique<SQLite::Statement>(
      db, "REPLACE INTO settings(username, setting, value) VALUES(?,?,?);");
}

// DBData::DBData(void) : sql(std::string(DB_CONNECT_STRING)) {}

DBData::~DBData() {}

void DBData::init(void) {
  db.exec("CREATE TABLE IF NOT EXISTS \
settings(username TEXT, setting TEXT, value TEXT, \
PRIMARY KEY(username, setting));");
  db.exec("CREATE TABLE IF NOT EXISTS \
scores ( \"username\" TEXT, \"when\" INTEGER, \
\"date\" INTEGER, \"hand\" INTEGER, \"score\" INTEGER, \
PRIMARY KEY(\"username\", \"date\", \"hand\"));");
}

void DBData::setUser(std::string currentUser) { user = currentUser; }

std::string DBData::getSetting(const std::string &setting,
                               std::string ifMissing) {
  stmt_getSet->reset();
  stmt_getSet->bind(1, user);
  stmt_getSet->bind(2, setting);
  if (stmt_getSet->executeStep()) {
    std::string value = stmt_getSet->getColumn(0);
    return value;
  };
  return ifMissing;
}

void DBData::setSetting(const std::string &setting, const std::string &value) {
  stmt_setSet->reset();
  stmt_setSet->bind(1, user);
  stmt_setSet->bind(2, setting);
  stmt_setSet->bind(3, value);
  stmt_setSet->exec();
}

void DBData::save_score(time_t when, time_t date, int hand, int score) {
  try {
    SQLite::Statement stmt(db,
                           "INSERT INTO scores( \"username\", \"when\", "
                           "\"date\", \"hand\", \"score\") VALUES(?,?,?,?,?);");
    stmt.bind(1, user);
    stmt.bind(2, when);
    stmt.bind(3, date);
    stmt.bind(4, hand);
    stmt.bind(5, score);
    stmt.exec();
  } catch (std::exception &e) {
    if (get_logger) {
      get_logger() << "SQLite exception: " << e.what() << std::endl;
    }
  }
}

int DBData::has_played_day(time_t day) {
  // get date from this

  // std::stringstream ss;
  // ss << std::put_time(std::localtime(&day), "%Y/%0m/%0d");

  SQLite::Statement stmt(
      db, "SELECT COUNT(*) FROM scores WHERE \"username\"=? AND \"DATE\"=?;");
  stmt.bind(1, user);
  stmt.bind(2, day);
  int count = -1;
  if (stmt.executeStep()) {
    count = stmt.getColumn(0);
  };
  return count;
}

std::string make_date(time_t tt) {
  std::stringstream ss;
  if (config["date_score"]) {
    std::string custom_format = config["date_score"].as<std::string>();
    ss << std::put_time(std::localtime(&tt), custom_format.c_str());
  } else {
    ss << std::put_time(std::localtime(&tt), "%Y/%0m/%0d");
  }

  std::string date = ss.str();
  return date;
}

void standard_date(time_t &tt, int hour) {
  std::tm *local_tm = localtime(&tt);
  // adjust date to 2:00:00 AM

  tt -= (local_tm->tm_min * 60) + local_tm->tm_sec;
  while (local_tm->tm_hour < hour) {
    ++local_tm->tm_hour;
    tt += 60 * 60;
  }
  if (local_tm->tm_hour > hour) {
    tt -= (60 * 60) * (local_tm->tm_hour - hour);
  }
}