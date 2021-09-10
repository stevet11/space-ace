#include "scores.h"

#include "utils.h"
#include <algorithm> // max
#include <iomanip>   // setw
#include <sstream>   // ostringstream

door::renderFunction scoresRender(door::ANSIColor date, int dlen,
                                  door::ANSIColor nick, int nlen,
                                  door::ANSIColor score)
{
  door::renderFunction rf = [date, dlen, nick, nlen,
                             score](const std::string &txt) -> door::Render
  {
    door::Render r(txt);
    r.append(date, dlen);
    r.append(nick, nlen);
    int left = txt.size() - (dlen + nlen);
    r.append(score, left);

    return r;
  };
  return rf;
}

std::unique_ptr<door::Panel> Scores::make_top_scores_panel()
{
  const int W = 38;
  door::COLOR panel_bg = door::COLOR::BLUE;
  door::ANSIColor panel_color =
      door::ANSIColor(door::COLOR::CYAN, panel_bg); //, door::ATTR::BOLD);
  door::ANSIColor heading_color = panel_color;
  heading_color.setFg(door::COLOR::GREEN, door::ATTR::BOLD);

  std::unique_ptr<door::Panel> p = std::make_unique<door::Panel>(W);
  p->setStyle(door::BorderStyle::DOUBLE);
  p->setColor(panel_color);

  std::unique_ptr<door::Line> heading =
      std::make_unique<door::Line>("The TOP Monthly Scores:", W, heading_color);
  // heading->setColor(heading_color);
  p->addLine(std::move(heading));

  std::unique_ptr<door::Line> spacer = p->spacer_line(false);
  p->addLine(std::move(spacer));

  auto monthly_scores = db.getMonthlyScores(15);
  if (monthly_scores.empty())
  {
    // No Monthly Scores
    door::ANSIColor nny_color =
        door::ANSIColor(door::COLOR::YELLOW, panel_bg, door::ATTR::BOLD);

    std::unique_ptr<door::Line> heading =
        std::make_unique<door::Line>("No, Not Yet!", W, nny_color);
    // heading->setColor(heading_color);
    p->addLine(std::move(heading));
  }

  ostringstream oss;

  // get length of longest month
  time_t longest = 1631280600; // 9/10/2021 9:30:10
  std::string longest_date = convertDateToMonthlyFormat(longest);
  int longest_month = longest_date.size();

#ifdef DEBUG_OUTPUT
  if (get_logger)
    get_logger() << "longest_date: " << longest_date << " " << longest_month
                 << std::endl;
#endif

  door::ANSIColor nick = panel_color;
  nick.setFg(door::COLOR::CYAN);
  door::ANSIColor yourNick = panel_color;
  yourNick.setFg(door::COLOR::GREEN, door::ATTR::BOLD);

  door::ANSIColor date = panel_color;
  date.setFg(door::COLOR::WHITE, door::ATTR::BOLD);
  door::ANSIColor score = panel_color;
  score.setFg(door::COLOR::CYAN, door::ATTR::BOLD);
  door::ANSIColor yourScore = panel_color;
  yourScore.setFg(door::COLOR::YELLOW, door::ATTR::BOLD);

  door::renderFunction scoreColors =
      scoresRender(date, longest_month, nick, 17, score);

  door::renderFunction yourScoreColors =
      scoresRender(date, longest_month, yourNick, 17, yourScore);

  for (auto it : monthly_scores)
  {
    time_t date = it.date;
    std::string nice_date = convertDateToMonthlyFormat(date);
    oss.clear();
    oss.str(std::string());

    oss << std::setw(longest_month) << nice_date << " " << std::setw(16)
        << it.user << " " << it.score;
    std::unique_ptr<door::Line> line =
        std::make_unique<door::Line>(oss.str(), W);
    if ((it.user == door.username) or (it.user == door.handle))
      line->setRender(yourScoreColors);
    else
      line->setRender(scoreColors);

    p->addLine(std::move(line));
  }

  return p;
}

std::unique_ptr<door::Panel> Scores::make_top_this_month_panel()
{
  const int W = 34;
  door::COLOR panel_bg = door::COLOR::BLUE;
  door::ANSIColor panel_color = door::ANSIColor(door::COLOR::CYAN, panel_bg);
  door::ANSIColor heading_color =
      door::ANSIColor(door::COLOR::GREEN, panel_bg, door::ATTR::BOLD);

  std::unique_ptr<door::Panel> p = std::make_unique<door::Panel>(W);
  p->setStyle(door::BorderStyle::DOUBLE);
  p->setColor(panel_color);

  std::string text = "The TOP Scores for ";
  {
    auto now = std::chrono::system_clock::now();
    time_t date = std::chrono::system_clock::to_time_t(now);

    text += convertDateToMonthlyFormat(date);
    text += ":";
  }
  std::unique_ptr<door::Line> heading =
      std::make_unique<door::Line>(text, W, heading_color);
  // heading->setColor(heading_color);
  p->addLine(std::move(heading));

  std::unique_ptr<door::Line> spacer = p->spacer_line(false);
  p->addLine(std::move(spacer));

  auto monthly_scores = db.getScores(15);
  if (monthly_scores.empty())
  {
    // No Monthly Scores
    door::ANSIColor nny_color =
        door::ANSIColor(door::COLOR::YELLOW, panel_bg, door::ATTR::BOLD);
    std::unique_ptr<door::Line> heading =
        std::make_unique<door::Line>("No, Not Yet!", W, nny_color);
    // heading->setColor(heading_color);
    p->addLine(std::move(heading));
  }

  ostringstream oss;

  time_t longest = 1631280600; // 9/10/2021 9:30:10
  std::string longest_date = convertDateToMonthDayFormat(longest);
  int longest_month = longest_date.size();

#ifdef DEBUG_OUTPUT
  if (get_logger)
    get_logger() << __FILE__ << "@" << __LINE__
                 << " longest_date: " << longest_date << " " << longest_month
                 << std::endl;
#endif

  door::ANSIColor nick = panel_color;
  nick.setFg(door::COLOR::CYAN);
  door::ANSIColor yourNick = panel_color;
  yourNick.setFg(door::COLOR::GREEN, door::ATTR::BOLD);

  door::ANSIColor date = panel_color;
  date.setFg(door::COLOR::WHITE, door::ATTR::BOLD);
  door::ANSIColor score = panel_color;
  score.setFg(door::COLOR::CYAN, door::ATTR::BOLD);
  door::ANSIColor yourScore = panel_color;
  yourScore.setFg(door::COLOR::YELLOW, door::ATTR::BOLD);

  door::renderFunction scoreColors =
      scoresRender(date, longest_month, nick, 17, score);

  door::renderFunction yourScoreColors =
      scoresRender(date, longest_month, yourNick, 17, yourScore);

  for (auto it : monthly_scores)
  {
    time_t date = it.date;
    std::string nice_date = convertDateToMonthDayFormat(date);
    oss.clear();
    oss.str(std::string());

    oss << std::setw(longest_month) << nice_date << " " << std::setw(16)
        << it.user << " " << it.score;
    std::unique_ptr<door::Line> line =
        std::make_unique<door::Line>(oss.str(), W);
    if ((it.user == door.username) or (it.user == door.handle))
      line->setRender(yourScoreColors);
    else
      line->setRender(scoreColors);
    p->addLine(std::move(line));
  }

  return p;
}

Scores::Scores(door::Door &d, DBData &dbd, Starfield &sf)
    : door{d}, db{dbd}, stars{sf}
{
  top_scores = make_top_scores_panel();
  top_this_month = make_top_this_month_panel();
}

void Scores::display_scores(door::Door &door)
{
  if (cls_display_starfield)
    cls_display_starfield();

  int mx = door.width;
  int my = door.height;
  int h = std::max(top_scores->getHeight(), top_this_month->getHeight());

  int padx = (mx - (top_scores->getWidth() + top_this_month->getWidth())) / 3;
  int pady = (my - h) / 2;
  top_scores->set(padx, pady);
  // display top_this_month, then top_scores.
  // This gives the press a key prompt a good place to be.
  top_this_month->set(padx * 2 + top_scores->getWidth(), pady);
  door << *top_this_month << *top_scores << door::reset << door::nl;
}