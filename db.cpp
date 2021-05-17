#include "db.h"
#include "utils.h"

#include <SQLiteCpp/VariadicBind.h>

#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

/*
database is locked

This happens when more then one node plays the game.

The database access is slow.

So, make sure you set it up so that you do your writes right
before you collect user input.  That way, the user won't see
the lags.

This might be an issue on rPI systems!
Change the strategy so we only update when the game ends.
*/

DBData::DBData(void)
    : db("space-data.db", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE) {
  locked_retries = 50;
  create_tables();
}

DBData::~DBData() {}

void DBData::retry_wait(void) {
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

#define DBLOCK "database is locked"

/**
 * @brief create tables if they don't exist.
 */
void DBData::create_tables(void) {
  int tries = 0;

retry:
  try {
    db.exec("CREATE TABLE IF NOT EXISTS settings(username TEXT, setting TEXT, "
            "value TEXT, PRIMARY KEY(username, setting));");
    db.exec(
        "CREATE TABLE IF NOT EXISTS scores ( \"username\" TEXT, \"when\" "
        "INTEGER, \"date\" INTEGER, \"hand\" INTEGER, \"won\" INTEGER, "
        "\"score\" INTEGER, PRIMARY KEY(\"username\", \"date\", \"hand\"));");
    db.exec("CREATE TABLE IF NOT EXISTS \"monthly\" ( \"month\"	INTEGER, "
            "\"username\" TEXT, \"days\" INTEGER, \"hands_won\" INTEGER, "
            "\"score\" INTEGER, PRIMARY KEY(\"month\",\"username\") );");
  } catch (std::exception &e) {
    if (get_logger) {
      get_logger() << "create_tables():" << std::endl;
      get_logger() << "SQLite exception: " << e.what() << std::endl;
    }
    if (strcmp(e.what(), DBLOCK) == 0) {
      ++tries;
      if (tries < locked_retries) {
        retry_wait();
        goto retry;
      }
      if (get_logger)
        get_logger() << "giving up! " << tries << " retries." << std::endl;
    }
  }
  if (tries > 0) {
    if (get_logger)
      get_logger() << "success after " << tries << std::endl;
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
  SQLite::Statement stmt_getSet = SQLite::Statement(
      db, "SELECT value FROM settings WHERE username=? AND setting=?");
  int tries = 0;

retry:
  try {

    stmt_getSet.bind(1, user);
    stmt_getSet.bind(2, setting);
    if (stmt_getSet.executeStep()) {
      std::string value = stmt_getSet.getColumn(0);
      return value;
    };
    return ifMissing;
  } catch (std::exception &e) {
    if (get_logger) {
      get_logger() << "getSettings( " << setting << "," << ifMissing
                   << " ): " << user << std::endl;
      get_logger() << "SQLite exception: " << e.what() << std::endl;
    }
    if (strcmp(e.what(), DBLOCK) == 0) {
      ++tries;
      if (tries < locked_retries) {
        retry_wait();
        goto retry;
      }
      if (get_logger)
        get_logger() << "giving up! " << tries << " retries." << std::endl;
    }
  }
  if (tries > 0) {
    if (get_logger)
      get_logger() << "success after " << tries << std::endl;
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
  SQLite::Statement stmt_setSet = SQLite::Statement(
      db, "REPLACE INTO settings(username, setting, value) VALUES(?,?,?);");
  int tries = 0;

retry:
  try {
    stmt_setSet.bind(1, user);
    stmt_setSet.bind(2, setting);
    stmt_setSet.bind(3, value);
    stmt_setSet.exec();
  } catch (std::exception &e) {
    if (get_logger) {
      get_logger() << "setSettings( " << setting << "," << value
                   << " ): " << user << std::endl;
      get_logger() << "SQLite exception: " << e.what() << std::endl;
    }
    if (strcmp(e.what(), DBLOCK) == 0) {
      ++tries;
      if (tries < locked_retries) {
        retry_wait();
        goto retry;
      }
      if (get_logger)
        get_logger() << "giving up! " << tries << " retries." << std::endl;
    }
  }
  if (tries > 0)
    if (get_logger)
      get_logger() << "success after " << tries << std::endl;
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
  int tries = 0;

retry:
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
    if (strcmp(e.what(), DBLOCK) == 0) {
      ++tries;
      if (tries < locked_retries) {
        retry_wait();
        goto retry;
      }
      if (get_logger)
        get_logger() << "giving up! " << tries << " retries." << std::endl;
    }
  }
  if (tries > 0)
    if (get_logger)
      get_logger() << "success after " << tries << std::endl;
}

/**
 * @brief Returns number of hands played on given day
 *
 * returns number of hands they played, or 0
 * @param day
 * @return int
 */
int DBData::handsPlayedOnDay(time_t day) {
  int tries = 0;

retry:
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
    if (strcmp(e.what(), DBLOCK) == 0) {
      ++tries;
      if (tries < locked_retries) {
        retry_wait();
        goto retry;
      }
      if (get_logger)
        get_logger() << "giving up! " << tries << " retries." << std::endl;
    }
  }
  if (tries > 0)
    if (get_logger)
      get_logger() << "success after " << tries << std::endl;
  return 0;
}

/*
 * If you're looking at scores, you're not really looking for all the details.
 * I think using the group/SUM would be better, and it sorts scores from highest
 * to lowest.  Let SQL do the work for me.
 */
// select date,username,SUM(score),SUM(won) FROM scores group by date,username
// ORDER BY SUM(score) DESC;

std::vector<scores_details> DBData::getScoresOnDay(time_t date) {
  std::vector<scores_details> scores;
  int tries = 0;

retry:
  try {
    // \"when\",
    SQLite::Statement stmt(db, "SELECT \"username\", \"date\", \"hand\", "
                               "\"won\", \"score\" FROM SCORES WHERE "
                               "\"date\"=? ORDER BY \"username\", \"hand\";");
    stmt.bind(1, date);
    while (stmt.executeStep()) {
      scores_details sd;
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
    if (strcmp(e.what(), DBLOCK) == 0) {
      ++tries;
      if (tries < locked_retries) {
        retry_wait();
        goto retry;
      }
      if (get_logger)
        get_logger() << "giving up! " << tries << " retries." << std::endl;
    }
  }
  if (tries > 0)
    if (get_logger)
      get_logger() << "success after " << tries << std::endl;
  return scores;
}

/**
 * @brief Gets scores, time_t is day, vector has user and scores sorted highest
 * to lowest.
 *
 * @return std::map<time_t, std::vector<scores_data>>
 */
std::vector<scores_data> DBData::getScores(int limit) {
  std::vector<scores_data> scores;
  int tries = 0;

retry:
  try {
    SQLite::Statement stmt(
        db, "SELECT `date`,username,SUM(score),SUM(won) FROM scores "
            "GROUP BY `date`,username ORDER BY SUM(score) DESC LIMIT ?;");
    stmt.bind(1, limit);
    //        db, "SELECT `date`,username,SUM(score),SUM(won) FROM scores "
    //            "GROUP BY `date`,username ORDER BY `date`,SUM(score) DESC;");

    while (stmt.executeStep()) {
      scores_data sd;
      sd.date = (long)stmt.getColumn(0);
      sd.user = (const char *)stmt.getColumn(1);
      sd.score = stmt.getColumn(2);
      sd.won = stmt.getColumn(3);
      scores.push_back(sd);
    }

  } catch (std::exception &e) {
    if (get_logger) {
      get_logger() << "getScores(): " << std::endl;
      get_logger() << "SQLite exception: " << e.what() << std::endl;
    }
    scores.clear();
    if (strcmp(e.what(), DBLOCK) == 0) {
      ++tries;
      if (tries < locked_retries) {
        retry_wait();
        goto retry;
      }
      if (get_logger)
        get_logger() << "giving up! " << locked_retries << " retries."
                     << std::endl;
    }
  }
  if (tries > 0)
    if (get_logger)
      get_logger() << "success after " << tries << std::endl;
  return scores;
}

/**
 * @brief Gets scores, time_t is day, vector has user and scores sorted highest
 * to lowest.
 *
 * (	\"month\", \"username\"	, \"days\",	hands_won, "\"score\"
 *
 * @return std::map<time_t, std::vector<scores_data>>
 */
std::vector<monthly_data> DBData::getMonthlyScores(int limit) {
  std::vector<monthly_data> scores;
  scores.reserve(limit);
  int tries = 0;

retry:
  try {
    SQLite::Statement stmt(
        db, "SELECT month, username, days, hands_won, score FROM monthly "
            "ORDER BY score DESC LIMIT ?;");
    stmt.bind(1, limit);

    while (stmt.executeStep()) {
      monthly_data data;
      data.date = (long)stmt.getColumn(0);
      data.user = (const char *)stmt.getColumn(1);
      data.days = stmt.getColumn(2);
      data.hands_won = stmt.getColumn(3);
      data.score = stmt.getColumn(4);
      scores.push_back(data);
    }
  } catch (std::exception &e) {
    if (get_logger) {
      get_logger() << "getMonthlyScores( " << limit << "): " << std::endl;
      get_logger() << "SQLite exception: " << e.what() << std::endl;
    }
    scores.clear();
    if (strcmp(e.what(), DBLOCK) == 0) {
      ++tries;
      if (tries < locked_retries) {
        retry_wait();
        goto retry;
      }
      if (get_logger)
        get_logger() << "giving up! " << locked_retries << " retries."
                     << std::endl;
    }
  }
  if (tries > 0)
    if (get_logger)
      get_logger() << "success after " << tries << std::endl;
  return scores;
}

/**
 * @brief Get hands played per day
 *
 * Uses the user value.
 *
 * @return std::map<time_t, int>
 */
std::map<time_t, int> DBData::getPlayed(void) {
  std::map<time_t, int> hands;
  int tries = 0;

retry:
  try {
    SQLite::Statement stmt(
        // select date, count(hand) from scores where username='grinder' group
        // by date;
        db, "SELECT `date`,COUNT(hand) FROM scores "
            "WHERE username=? GROUP BY `date`;");
    stmt.bind(1, user);
    while (stmt.executeStep()) {
      time_t the_date = stmt.getColumn(0);
      hands[the_date] = stmt.getColumn(1);
    }
  } catch (std::exception &e) {
    if (get_logger) {
      get_logger() << "getPlayed(): " << user << std::endl;
      get_logger() << "SQLite exception: " << e.what() << std::endl;
    }
    hands.clear();
    if (strcmp(e.what(), DBLOCK) == 0) {
      ++tries;
      if (tries < locked_retries) {
        retry_wait();
        goto retry;
      }
      if (get_logger)
        get_logger() << "giving up! " << tries << " retries." << std::endl;
    }
  }
  if (tries > 0)
    if (get_logger)
      get_logger() << "success after " << tries << std::endl;
  return hands;
}

/**
 * @brief When has the user played?
 *
 * This returns a map of date (time_t), and number of hands played on that
 * date.
 *
 * @return std::map<time_t, long>
 */
std::map<time_t, int> DBData::whenPlayed(void) {
  // select "date", count(hand) from scores where username='?' group by
  // "date";
  std::map<time_t, int> plays;
  int tries = 0;

retry:
  try {
    SQLite::Statement stmt(db, "SELECT `date`, COUNT(hand) FROM scores WHERE "
                               "username=? GROUP BY `date`;");
    stmt.bind(1, user);

    while (stmt.executeStep()) {
      time_t d = (long)stmt.getColumn(0);
      plays[d] = stmt.getColumn(1);
    }
  } catch (std::exception &e) {
    if (get_logger) {
      get_logger() << "whenPlayed(): " << std::endl;
      get_logger() << "SQLite exception: " << e.what() << std::endl;
    }
    plays.clear();
    if (strcmp(e.what(), DBLOCK) == 0) {
      ++tries;
      if (tries < locked_retries) {
        retry_wait();
        goto retry;
      }
      if (get_logger)
        get_logger() << "giving up! " << tries << " retries." << std::endl;
    }
  }
  if (tries > 0)
    if (get_logger)
      get_logger() << "success after " << tries << std::endl;
  return plays;
}

struct month_user {
  time_t date;
  std::string username;
  // friend bool operator<(const month_user &l, const month_user &r);
};

bool operator<(const month_user &l, const month_user &r) {
  if (l.date < r.date)
    return true;
  if (l.date > r.date)
    return false;

  // Otherwise a are equal
  if (l.username < r.username)
    return true;
  if (l.username > r.username)
    return false;

  // Otherwise both are equal
  return false;
}

struct month_stats {
  int days;
  int hands_won;
  int score;
};

/**
 * @brief This will expire out old scores
 *
 * Merges scores into monthly table.
 *
 * @param month_first_t
 *
 */
void DBData::expireScores(time_t month_first_t) {
  // step 1:  aquire lock
  std::ofstream lockfile;
  lockfile.open("db.lock", std::ofstream::out | std::ofstream::app);
  long pos = lockfile.tellp();
  if (pos == 0) {
    lockfile << "OK.";
    lockfile.flush();
    lockfile.close();
  } else {
    if (get_logger()) {
      get_logger() << "db.lock file exists.  Skipping maint." << std::endl;
    }
    lockfile.close();
    return;
  }

  std::map<month_user, month_stats> monthly;

  // Ok, do maint things here
  SQLite::Statement stmt(
      db, "SELECT `date`,username,SUM(score),SUM(won) FROM scores "
          "WHERE `date` < ? GROUP BY `date`,username ORDER BY "
          "`date`,SUM(score) DESC;");

  try {
    stmt.bind(1, (long)month_first_t);

    while (stmt.executeStep()) {
      // get time_t, conver to time_point, find first of month, convert back
      // to time_t
      std::chrono::_V2::system_clock::time_point date;

      date = std::chrono::system_clock::from_time_t((long)stmt.getColumn(0));
      firstOfMonthDate(date);
      time_t date_t = std::chrono::system_clock::to_time_t(date);

      month_user mu;

      mu.date = date_t;
      mu.username = (const char *)stmt.getColumn(1);

      auto map_iter = monthly.find(mu);
      if (map_iter == monthly.end()) {
        // not found
        month_stats ms;
        ms.days = 1;
        ms.score = stmt.getColumn(2);
        ms.hands_won = stmt.getColumn(3);
        monthly[mu] = ms;
      } else {
        // was found
        month_stats &ms = monthly[mu];
        ms.days++;
        ms.score += (int)stmt.getColumn(2);
        ms.hands_won += (int)stmt.getColumn(3);
      }
    }
  } catch (std::exception &e) {
    if (get_logger) {
      get_logger() << "expireScores() SELECT failed" << std::endl;
      get_logger() << "SQLite exception: " << e.what() << std::endl;
    }
  }

  // Ok! I have the data that I need!
  if (monthly.empty()) {
    std::remove("db.lock");
    return;
  }

  try {

    SQLite::Transaction transaction(db);

    // If we fail for any reason within here -- we should be safe.
    SQLite::Statement stmt_delete(db, "DELETE FROM scores WHERE `date` < ?;");

    stmt_delete.bind(1, (long)month_first_t);
    stmt_delete.exec();

    /*
        if (get_logger) {
          get_logger() << "Ok, deleted records < " << month_first_t <<
       std::endl;
        }
    */

    SQLite::Statement stmt_insert(db,
                                  "INSERT INTO monthly(month, username, days, "
                                  "hands_won, score) VALUES(?,?,?,?,?);");
    for (auto key : monthly) {
      /*
            if (get_logger) {
              get_logger() << key.first.date << ":" << key.first.username << "
         "
                           << key.second.score << std::endl;
            }
      */
      stmt_insert.bind(1, (long)key.first.date);
      stmt_insert.bind(2, key.first.username.c_str());
      stmt_insert.bind(3, key.second.days);
      stmt_insert.bind(4, key.second.hands_won);
      stmt_insert.bind(5, key.second.score);
      stmt_insert.exec();
      stmt_insert.reset();
    }

    transaction.commit();

  } catch (std::exception &e) {
    if (get_logger) {
      get_logger() << "expireScores() DELETE/INSERTs failed" << std::endl;
      get_logger() << "SQLite exception: " << e.what() << std::endl;
    }
  }

  if (get_logger) {
    for (auto key : monthly) {
      get_logger() << key.first.date << ":" << key.first.username << "  "
                   << key.second.days << ", " << key.second.hands_won << ", "
                   << key.second.score << ::std::endl;
    }
  }
  // clean up

  std::remove("db.lock");
}

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
 * @brief Format date to string.
 *
 * We use default "%0m/%0d/%Y", but can be configured by SysOp via
 * config["date_score"] setting.  "%Y/%0m/%0d" for non-US?
 * https://en.cppreference.com/w/cpp/io/manip/put_time
 *
 * @param tt
 * @return std::string
 */
std::string convertDateToMonthlyFormat(time_t tt) {
  std::stringstream ss;
  if (config["date_monthly"]) {
    std::string custom_format = config["date_monthly"].as<std::string>();
    ss << std::put_time(std::localtime(&tt), custom_format.c_str());
  } else {
    ss << std::put_time(std::localtime(&tt), "%B %Y");
  }

  std::string date = ss.str();
  return date;
}

void normalizeDate(std::chrono::_V2::system_clock::time_point &date) {
  time_t date_t = std::chrono::system_clock::to_time_t(date);
  normalizeDate(date_t);
  date = std::chrono::system_clock::from_time_t(date_t);
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

void firstOfMonthDate(std::chrono::_V2::system_clock::time_point &date) {
  using namespace std::literals;
  time_t date_t = std::chrono::system_clock::to_time_t(date);
  // adjust to first day of the month
  std::tm date_tm;
  localtime_r(&date_t, &date_tm);

  if (date_tm.tm_mday > 1) {
    date -= 24h * (date_tm.tm_mday - 1);
  }
  normalizeDate(date);
}
