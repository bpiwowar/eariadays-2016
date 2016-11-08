/*==========================================================================
 * Copyright (c) 2004 University of Massachusetts.  All Rights Reserved.
 *
 * Use of the Lemur Toolkit for Language Modeling and Information Retrieval
 * is subject to the terms of the software license set forth in the LICENSE
 * file included with this software, and also available at
 * http://www.lemurproject.org/license.html
 *
 *==========================================================================


 Adapted from Indri (modified version) to store an access to the different files

*/


#include <libkern/OSAtomic.h>
#include <csignal>
#include <boost/program_options.hpp>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include "handler.h"
#include "Leaderboard.h"

typedef websocketpp::server<websocketpp::config::asio> server;
typedef websocketpp::frame::opcode::value OpValue;


class WebSocketRequest : public Request {
    server &_server;
    websocketpp::connection_hdl _hdl;
    const std::string _message;
public:
    WebSocketRequest(server &server, websocketpp::connection_hdl hdl, const string &message) :
            _server(server), _hdl(hdl), _message(message) {}

    virtual const string &message() const override {
        return _message;
    }

    virtual void send(std::string const &message) override {
        _server.send(_hdl, message, OpValue::text);
    }
    virtual void sendBinary(std::string const &message) override {
        _server.send(_hdl, message, OpValue::binary);
    }

};

std::unique_ptr<IndriServer> indriServer;
server _server;

void on_message(websocketpp::connection_hdl hdl, server::message_ptr msg) {
    WebSocketRequest request(_server, hdl, msg->get_payload());
    indriServer->handle(request);
}

void serve(std::string const &path, int port) {
    indriServer = std::unique_ptr<IndriServer>(new IndriServer(path));

    _server.clear_access_channels(websocketpp::log::alevel::all);

    _server.set_message_handler(
            [&](websocketpp::connection_hdl hdl, server::message_ptr msg) {
                on_message(hdl, msg);
            }
    );

    _server.init_asio();
    _server.set_reuse_addr(true);
    _server.listen(port);
    _server.start_accept();

    auto stopServer = [](int signum) {
        std::cerr << "Interrupt signal (" << signum << ") received: closing server...\n";
        _server.stop();
        std::cerr << "Done.\n";
        exit(signum);
    };
    std::signal(SIGINT, stopServer );
    std::signal(SIGTERM, stopServer );

    std::cerr << "Listening on " << port << std::endl;
    _server.run();

}


namespace po = boost::program_options;


int main(int ac, char **av) {
    po::options_description desc("Indri websocket server");
    std::string repName;
    int port;

    desc.add_options()
            ("help", "produce help message")
            ("port", po::value<int>(&port)->required(), "port number")
            ("repository", po::value<std::string>(&repName)->required(), "Path to Indri repository");

    po::positional_options_description p;
    p.add("repository", 1);

    po::variables_map vm;
    po::store(po::command_line_parser(ac, av).
            options(desc).positional(p).run(), vm);
    po::notify(vm);
    if (vm.count("help")) {
        cout << desc << "\n";
        return 1;
    }

    // Params
    try {
        serve(repName, port);
    } catch (std::exception &e) {
        std::cerr << "Error while launching server: " << e.what() << std::endl;
    } catch(lemur::api::Exception &e) {
        std::cerr << "Error while launching server: " << e.what() << std::endl;
    }
    return 0;
}