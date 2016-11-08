//
// Created by Benjamin Piwowarski on 06/11/2016.
//

#include <string>
#include "Application.h"
#include "Connection.h"

#include "json.hpp"

using nlohmann::json;

int main(int argc, char const **argv) {
    // Get the websocket URI
    std::string uri = "ws://localhost:9002";
    if (argc == 2) {
        uri = argv[1];
    }

    // Open the connection
    std::cerr << "Opening connection...\n";
    Connection connection(uri, false);

    json queries = { { { "number", "1"}, {"text", "#1(white house)" } } };
    json command = { { "command", "query" }, { "query", queries }};
    std::cerr << "Sending commmand...\n";
    connection.send(command);

    std::cerr << "Receiving answer...\n";
    json response = connection.receive();
    std::cout << response << "\n";
}
