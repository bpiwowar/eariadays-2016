//
// Created by Benjamin Piwowarski on 04/11/2016.
//

#ifndef INDRISERVER_INDRIQUERY_H
#define INDRISERVER_INDRIQUERY_H


#include <string>
#include <vector>
#include <indri/QueryAnnotation.hpp>
#include <indri/UtilityThread.hpp>
#include <indri/ConditionVariable.hpp>
#include <queue>
#include <indri/QueryEnvironment.hpp>
#include <indri/QueryExpander.hpp>
#include "handler.h"
#include "json.hpp"


void handleQuery(std::string const & repository, Request &request, nlohmann::json &parameters);


#endif //INDRISERVER_INDRIQUERY_H
