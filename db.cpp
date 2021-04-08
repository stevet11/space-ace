#include "db.h"

#include <SQLiteCpp/VariadicBind.h>
#include <iomanip>
#include <iostream>
#include <sstream>

DBData::DBData(void)
    : db("space-data.db", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE) {
  init();
}

// DBData::DBData(void) : sql(std::string(DB_CONNECT_STRING)) {}

DBData::~DBData() {}

void DBData::init(void) {
  db.exec("CREATE TABLE IF NOT EXISTS \
settings(username TEXT, setting TEXT, value TEXT, \
PRIMARY KEY(username, setting));");
  db.exec("CREATE TABLE IF NOT EXISTS \
scores ( \"username\" TEXT, \"when\" INTEGER, \
\"date\" TEXT, \"hand\" INTEGER, \"score\" INTEGER, \
PRIMARY KEY(\"username\", \"date\", \"hand\"));");
}

void DBData::setUser(std::string currentUser) { user = currentUser; }

std::string DBData::getSetting(const std::string &setting,
                               std::string ifMissing) {
  SQLite::Statement query(
      db, "SELECT value FROM settings WHERE username=? AND setting=?");
  query.reset();
  query.bind(1, user);
  query.bind(2, setting);
  if (query.executeStep()) {
    std::string value = query.getColumn(0);
    return value;
  };
  return ifMissing;
}

void DBData::setSetting(const std::string &setting, const std::string &value) {
  SQLite::Statement stmt(
      db, "REPLACE INTO settings(username, setting, value) VALUES(?,?,?);");
  stmt.reset();
  stmt.bind(1, user);
  stmt.bind(2, setting);
  stmt.bind(3, value);
  stmt.exec();
}

void DBData::save_score(time_t when, std::string date, int hand, int score) {
  SQLite::Statement stmt(db, "INSERT INTO scores( \"username\", \"when\", "
                             "\"date\", \"hand\", \"score\") VALUES(?,?,?,?);");
  stmt.bind(1, user);
  stmt.bind(2, when);
  stmt.bind(3, date);
  stmt.bind(4, hand);
  stmt.bind(5, score);
  stmt.exec();
}

bool DBData::has_played_day(time_t day) {
  // get date from this

  // std::stringstream ss;
  // ss << std::put_time(std::localtime(&day), "%Y/%0m/%0d");
  std::string today = make_date(day);
  SQLite::Statement stmt(
      db, "SELECT COUNT(*) FROM scores WHERE \"username\"=? AND \"DATE\"=?;");
  stmt.bind(1, user);
  stmt.bind(2, today);
  int count = -1;
  if (stmt.executeStep()) {
    count = stmt.getColumn(0);
  };
  return (count > 0);
}

std::string make_date(time_t tt) {
  std::stringstream ss;
  ss << std::put_time(std::localtime(&tt), "%Y/%0m/%0d");
  std::string date = ss.str();
  return date;
}