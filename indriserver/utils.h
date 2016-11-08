//
// Created by Benjamin Piwowarski on 05/11/2016.
//

#ifndef INDRISERVER_UTILS_H
#define INDRISERVER_UTILS_H

#include "json.hpp"

template<typename T>
T getOrDefault(nlohmann::json const & json, std::string const & key, T const & value) {
    if (json.count(key) == 0) return value;
    return json[key];
}

#endif //INDRISERVER_UTILS_H
