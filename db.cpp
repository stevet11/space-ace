#include "db.h"
#include "utils.h"

#include <SQLiteCpp/VariadicBind.h>

#include <iomanip>
#include <iostream>
#include <sstream>

/*
The database access is slow.

So, make sure you set it up so that you do your writes right
before you collect user input.  That way, the user won't see
the lags.

This might be an issue on rPI systems!
Change the strategy so we only update when the game ends.
*/

DBData::DBData(void)
    : db("space-data.db", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE) {

  create_tables();
  stmt_getSet = std::make_unique<SQLite::Statement>(
      db, "SELECT value FROM settings WHERE username=? AND setting=?");
  stmt_setSet = std::make_unique<SQLite::Statement>(
      db, "REPLACE INTO settings(username, setting, value) VALUES(?,?,?);");
}

DBData::~DBData() {}

/**
 * @brief create tables if they don't exist.
 */
void DBData::create_tables(void) {
  try {
    db.exec("CREATE TABLE IF NOT EXISTS \
settings(username TEXT, setting TEXT, value TEXT, \
PRIMARY KEY(username, setting));");
    db.exec("CREATE TABLE IF NOT EXISTS \
scores ( \"username\" TEXT, \"when\" INTEGER, \
\"date\" INTEGER, \"hand\" INTEGER, \"won\" INTEGER, \"score\" INTEGER, \
PRIMARY KEY(\"username\", \"date\", \"hand\"));");
  } catch (std::exception &e) {
    if (get_logger) {
      get_logger() << "create_tables():" << std::endl;
      get_logger() << "SQLite exception: " << e.what() << std::endl;
    }
  }
}

/**
 * @brief get setting from the settings table
 *
 * We use user and setting.
 * Return isMissing if not found.
 *
 * @param setting
 * @param ifMissing
 * @return std::string
 */
std::string DBData::getSetting(const std::string &setting,
                               std::string ifMissing) {
  try {
    stmt_getSet->reset();
    stmt_getSet->bind(1, user);
    stmt_getSet->bind(2, setting);
    if (stmt_getSet->executeStep()) {
      std::string value = stmt_getSet->getColumn(0);
      return value;
    };
    return ifMissing;
  } catch (std::exception &e) {
    if (get_logger) {
      get_logger() << "getSettings( " << setting << "," << ifMissing
                   << " ): " << user << std::endl;
      get_logger() << "SQLite exception: " << e.what() << std::endl;
    }
  }
  return ifMissing;
}

/**
 * @brief save setting in the settings table
 *
 * We save user setting in the settings table.
 * We use SQLite's REPLACE INTO so it does an update if it exists, or an insert
 * if it is missing.
 * @param setting
 * @param value
 */
void DBData::setSetting(const std::string &setting, const std::string &value) {
  try {
    stmt_setSet->reset();
    stmt_setSet->bind(1, user);
    stmt_setSet->bind(2, setting);
    stmt_setSet->bind(3, value);
    stmt_setSet->exec();
  } catch (std::exception &e) {
    if (get_logger) {
      get_logger() << "setSettings( " << setting << "," << value
                   << " ): " << user << std::endl;
      get_logger() << "SQLite exception: " << e.what() << std::endl;
    }
  }
}

/**
 * @brief save the user's score
 *
 * @param when now()
 * @param date what day they played
 * @param hand which hand they played
 * @param won did they win? 1/0
 * @param score
 */
void DBData::saveScore(time_t when, time_t date, int hand, int won, int score) {
  try {
    SQLite::Statement stmt(
        db, "INSERT INTO scores( \"username\", \"when\", "
            "\"date\", \"hand\", \"won\", \"score\") VALUES(?,?,?,?,?,?);");
    stmt.bind(1, user);
    stmt.bind(2, when);
    stmt.bind(3, date);
    stmt.bind(4, hand);
    stmt.bind(5, won);
    stmt.bind(6, score);
    stmt.exec();
  } catch (std::exception &e) {
    if (get_logger) {
      get_logger() << "saveScore( " << when << "," << date << "," << hand << ","
                   << won << "," << score << " ): " << user << std::endl;
      get_logger() << "SQLite exception: " << e.what() << std::endl;
    }
  }
}

/**
 * @brief Returns number of hands played on given day
 *
 * returns number of hands they played, or 0
 * @param day
 * @return int
 */
int DBData::handsPlayedOnDay(time_t day) {
  try {
    SQLite::Statement stmt(
        db, "SELECT COUNT(*) FROM scores WHERE \"username\"=? AND \"DATE\"=?;");
    stmt.bind(1, user);
    stmt.bind(2, day);
    int count = 0;
    if (stmt.executeStep()) {
      count = stmt.getColumn(0);
    };
    return count;
  } catch (std::exception &e) {
    if (get_logger) {
      get_logger() << "handsPlayedOnDay( " << day << " ): " << user
                   << std::endl;
      get_logger() << "SQLite exception: " << e.what() << std::endl;
    }
  }
  return 0;
}

std::vector<scores_data> DBData::getScoresOnDay(time_t date) {
  std::vector<scores_data> scores;
  try {
    // \"when\",
    SQLite::Statement stmt(db, "SELECT \"username\", \"date\", \"hand\", "
                               "\"won\", \"score\" FROM SCORES WHERE "
                               "\"date\"=? ORDER BY \"username\", \"hand\";");
    stmt.bind(1, date);
    while (stmt.executeStep()) {
      scores_data sd;
      sd.user = (const char *)stmt.getColumn(0);
      sd.date = stmt.getColumn(1);
      sd.hand = stmt.getColumn(2);
      sd.won = stmt.getColumn(3);
      sd.score = stmt.getColumn(4);
      scores.push_back(sd);
    }
  } catch (std::exception &e) {
    if (get_logger) {
      get_logger() << "getScoresOnDay( " << date << " ): " << std::endl;
      get_logger() << "SQLite exception: " << e.what() << std::endl;
    }
    scores.clear();
  }
  return scores;
}

std::map<time_t, std::vector<scores_data>> DBData::getScores(void) {
  std::map<time_t, std::vector<scores_data>> scores;
  try {
    SQLite::Statement stmt(db, "SELECT \"username\", \"date\", \"hand\", "
                               "\"won\", \"score\" FROM SCORES "
                               "ORDER BY \"date\", \"username\", \"hand\";");
    time_t current = 0;
    std::vector<scores_data> vsd;

    while (stmt.executeStep()) {
      time_t the_date = stmt.getColumn(1);

      if (current == 0) {
        // ok, we've got the first one!
        current = the_date;
      } else {
        // Ok, are we on another date now?
        if (the_date != current) {
          scores[current] = std::move(vsd);
          vsd.clear();
          current = the_date;
        }
      }
      scores_data sd;
      sd.user = (const char *)stmt.getColumn(0);
      sd.date = the_date; // stmt.getColumn(1);
      sd.hand = stmt.getColumn(2);
      sd.won = stmt.getColumn(3);
      sd.score = stmt.getColumn(4);
      vsd.push_back(sd);
    }
    if (!vsd.empty()) {
      scores[current] = std::move(vsd);
    }
    vsd.clear();
  } catch (std::exception &e) {
    if (get_logger) {
      get_logger() << "getScores(): " << std::endl;
      get_logger() << "SQLite exception: " << e.what() << std::endl;
    }
    scores.clear();
  }
  return scores;
}

void DBData::expireScores(void) {}

/**
 * @brief Format date to string.
 *
 * We use default "%0m/%0d/%Y", but can be configured by SysOp via
 * config["date_score"] setting.  "%Y/%0m/%0d" for non-US?
 *
 * @param tt
 * @return std::string
 */
std::string convertDateToDateScoreFormat(time_t tt) {
  std::stringstream ss;
  if (config["date_score"]) {
    std::string custom_format = config["date_score"].as<std::string>();
    ss << std::put_time(std::localtime(&tt), custom_format.c_str());
  } else {
    ss << std::put_time(std::localtime(&tt), "%0m/%0d/%Y");
  }

  std::string date = ss.str();
  return date;
}

/**
 * @brief change datetime to have consistent time
 *
 * This converts the time part to hour:00
 *
 * @param tt
 * @param hour
 */
void normalizeDate(time_t &tt, int hour) {
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
  /* // possible DST adjustment. LMTATSM
  if (local_tm->tm_isdst) {
    // DST in effect
    tt -= (60*60);
  }
  */
}