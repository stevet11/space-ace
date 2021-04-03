#include "db.h"

#include <SQLiteCpp/VariadicBind.h>
#include <iostream>

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