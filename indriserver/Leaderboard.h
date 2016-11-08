//
// Created by Benjamin Piwowarski on 07/11/2016.
//

#ifndef INDRISERVER_LEADERBOARD_H
#define INDRISERVER_LEADERBOARD_H


#include <string>
#include <sqlite3.h>
#include "json.hpp"

class Leaderboard {
    sqlite3 *_sqlite;
    std::string _basepath;
    std::map<std::string, std::string> _logins;
public:
    Leaderboard(std::string const &basepath);
    ~Leaderboard();

    nlohmann::json submit(std::string runPath, std::string name, std::string password);
    nlohmann::json status();
};


#endif //INDRISERVER_LEADERBOARD_H
