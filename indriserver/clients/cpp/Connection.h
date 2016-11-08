//
// Created by Benjamin Piwowarski on 06/11/2016.
//

#ifndef INDRICLIENT_CONNECTION_H
#define INDRICLIENT_CONNECTION_H

#include <string>
#include <thread>
#include "json.hpp"

// Hidden class
class _Connection;

class Connection {
    Connection(Connection const &) = delete;
    _Connection * _connection;
public:
    typedef nlohmann::json json;

    /// Construct a connection with a given URI
    Connection(std::string const &uri, bool debug);

    ~Connection();

    /// Connect to the WebSocket server
    void connect();

    /// Send a JSON message
    void send(json json);

    /// Receive a JSON message
    json receive();
};


#endif //INDRICLIENT_CONNECTION_H
