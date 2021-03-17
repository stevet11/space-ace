#ifndef DB_H
#define DB_H

#include <SQLiteCpp/SQLiteCpp.h>

class DBData {
  SQLite::Database db;
  void init(void);

public:
  DBData();
  virtual ~DBData();

  std::string getSetting(const std::string &user, const std::string &setting,
                         std::string ifMissing);
  void setSetting(const std::string &user, const std::string &setting,
                  const std::string &value);
};

#endif