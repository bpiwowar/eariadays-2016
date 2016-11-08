//
// Created by Benjamin Piwowarski on 07/11/2016.
//

#include <cstdlib>
#include <iostream>
#include <fstream>
#include "Leaderboard.h"

using nlohmann::json;

void executeSQL(sqlite3 *_sqlite, std::string const &sql) {
    char *zErrMsg = 0;
    int rc = sqlite3_exec(_sqlite, sql.c_str(), nullptr, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        std::string message = zErrMsg;
        sqlite3_free(zErrMsg);
        throw std::runtime_error(message);
    }
}

Leaderboard::Leaderboard(std::string const &basepath) : _basepath(basepath) {
    std::string path = _basepath + "/db.sqlite";
    std::cerr << "Opening SQL database " << path << "\n";
    if (sqlite3_open(path.c_str(), &_sqlite) != SQLITE_OK) {
        throw std::runtime_error("Could not open sqlite database");
    }

    std::ifstream passwords(_basepath + "passwd");
    std::string login, password;
    while (passwords >> login >> password) {
        _logins[login] = password;
    }

    std::string sql = "CREATE TABLE IF NOT EXISTS SCORES("  \
         "NAME           CHAR(50) PRIMARY KEY NOT NULL," \
         "AP             REAL," \
         "PC10           REAL );";

    executeSQL(_sqlite, sql);
}

Leaderboard::~Leaderboard() {
    std::cerr << "Closing SQL database\n";
    sqlite3_close(_sqlite);
}

std::string readFile(std::string const &path) {
    std::ifstream in(path);
    std::string str;
    in.seekg(0, std::ios::end);
    str.reserve(in.tellg());
    in.seekg(0, std::ios::beg);
    str.assign((std::istreambuf_iterator<char>(in)),
               std::istreambuf_iterator<char>());
    return std::move(str);
}

json Leaderboard::submit(std::string run, std::string name, std::string password) {
    auto entry = _logins.find(name);
    if (entry == _logins.end()) {
        return {{"error",   true},
                {"message", "Invalid login"}};
    }

    if (entry->second != password) {
        return {{"error",   true},
                {"message", "Invalid password"}};
    }

    const std::string runPath = _basepath + "/runs/" + name + ".run";
    {
        std::ofstream out(runPath.c_str());
        out << run;
    }


    char buffer[128];
    std::string command =
            "/usr/bin/python"
                    " " + _basepath + "/ref/session_eval_main.py"
                    " --qrel_file " + _basepath + "/ref/qrels.txt"
                    " --mapping_file " + _basepath + "/ref/sessiontopicmap.txt "
                    " --run_file " + runPath
            + " > " + runPath + ".out"
            + " 2> " + runPath + ".err";

    const int code = std::system(command.c_str());

    if (code != 0) {
        return {
                {"error",   true},
                {"code",    code},
                {"message", readFile(runPath + ".err")}
        };

    }

    std::ifstream in(runPath + ".out");
    std::string session, ap, pc10;
    in >> session >> ap >> pc10;
    if (session != "session" && ap != "AP" && ap != "PC@10") {
        return {
                {"error",   true},
                {"message", "Result file has a wrong format"}
        };
    }

    json r_ap = json::object();
    json r_pc10 = json::object();
    while (in >> session >> ap >> pc10) {
        r_ap[session] = std::atof(ap.c_str());
        r_pc10[session] = std::atof(pc10.c_str());
    }

    // TODO: store r_ap["all"] et r_pc["all"]
    std::string sql = "REPLACE INTO SCORES (NAME, AP, PC10)"
                              "VALUES (  '" + name + "',"
                      + std::to_string((float) r_ap["all"]) + ", "
                      + std::to_string((float) r_pc10["all"]) + ")";
    executeSQL(_sqlite, sql);

    return {{"AP",   std::move(r_ap)},
            {"P@10", std::move(r_pc10)}};
}

json Leaderboard::status() {
    return json();
}
