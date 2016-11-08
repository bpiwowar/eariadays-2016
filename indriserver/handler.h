//
// Created by Benjamin Piwowarski on 04/11/2016.
//

#ifndef INDRISERVER_HANDLER_H
#define INDRISERVER_HANDLER_H

#include <indri/Repository.hpp>

class Request {
public:
    virtual std::string const & message() const = 0;
    virtual void send(std::string const & message) = 0;
    virtual void sendBinary(std::string const & message) = 0;
    /**
     * Send an error message if assertion is false
     * Returns true if the assertion is false (i.e. !assertion)
     */
    bool assertion(bool assertion, std::string const & message);
};

class _IndriServer;

class IndriServer {
    _IndriServer *_indri;
public:
    IndriServer(IndriServer const &) = delete;
    ~IndriServer();

    IndriServer(std::string const &path);

    void handle(Request & request);

};

#endif //INDRISERVER_HANDLER_H
