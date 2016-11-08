//
// Created by Benjamin Piwowarski on 06/11/2016.
//

#include <mutex>
#include <condition_variable>

#include "Connection.h"

#define ASIO_STANDALONE

#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#include <iostream>
#include "json.hpp"

using nlohmann::json;
typedef websocketpp::frame::opcode::value OpValue;

typedef websocketpp::client<websocketpp::config::asio_client> client;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

// pull out the type of messages sent by our config
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;


struct _Connection {
    /// Client
    client c;

    std::string uri;

    /// The connection
    websocketpp::connection_hdl hdl;

    std::mutex m_open;
    std::condition_variable cv_open;

    std::mutex m_incoming_message;
    std::condition_variable cv_incoming_message;

    // Incoming messages
    std::queue<std::string> incoming;

    // Thread
    std::thread _thread;

    bool debug;

    ~_Connection() {
        // Closing connection
        c.close(hdl, 0, "Finished");

        // Wait for close signal
        std::unique_lock<std::mutex> lk(m_open);
        cv_open.wait(lk, [&] { return hdl.expired(); });

        // Wait for websocket to be closed
        _thread.join();

        std::cerr << "Connection closed";
    }

    _Connection(std::string const &uri, bool debug) : uri(uri), debug(debug) {
        _thread = std::thread([&] {
            start();
        });
    }

    void start() {
        try {
            if (debug) {
                // Set logging to be pretty verbose (everything except message payloads)
                c.set_error_channels(websocketpp::log::alevel::all);
                c.clear_error_channels(websocketpp::log::alevel::frame_payload);
                c.clear_error_channels(websocketpp::log::alevel::frame_header);
            } else {
                c.set_access_channels(websocketpp::log::alevel::none);
            }

            // Initialize ASIO
            c.init_asio();

            // Register our message handler
            c.set_message_handler([&](websocketpp::connection_hdl hdl, message_ptr msg) {
                this->on_message(hdl, msg);
            });
            c.set_open_handler([&](websocketpp::connection_hdl hdl) {
                {
                    std::lock_guard<std::mutex> lk(m_open);
                    this->hdl = hdl;
                }
                std::cerr << "Connection opened, notifying" << std::endl;
                cv_open.notify_all();
            });
            c.set_close_handler([&](websocketpp::connection_hdl hdl) {
                {
                    std::lock_guard<std::mutex> lk(m_open);
                    this->hdl.reset();
                }
                std::cerr << "Connection closed, notifying" << std::endl;
                cv_open.notify_all();
            });

            websocketpp::lib::error_code ec;
            client::connection_ptr con = c.get_connection(uri, ec);
            if (ec) {
                throw std::runtime_error("could not create connection because: " + ec.message());
            }

            // Note that connect here only requests a connection. No network messages are
            // exchanged until the event loop starts running in the next line.
            c.connect(con);


            // Start the ASIO io_service run loop
            // this will cause a single connection to be made to the server. c.run()
            // will exit when this connection is closed.
            c.run();
        } catch (websocketpp::exception const &e) {
            throw std::runtime_error(e.what());
        }
    }

    void send(json const &message) {
        while (true) {
            std::unique_lock<std::mutex> lk(m_open);
            cv_open.wait(lk);
            if (auto ptr = hdl.lock()) {
                c.send(hdl, message.dump(), OpValue::text);
                return;
            }
        }
    }

    void on_message(websocketpp::connection_hdl hdl, message_ptr msg) {
        {
            std::lock_guard<std::mutex> lk(m_incoming_message);
            incoming.push(msg->get_payload());
        }
        cv_incoming_message.notify_one();
    }

    json receive() {
        std::unique_lock<std::mutex> lk(m_incoming_message);
        cv_incoming_message.wait(lk, [&] { return !incoming.empty(); });
        json message = incoming.front();
        incoming.pop();
        return message;
    }
};


Connection::Connection(std::string const &uri, bool debug) {
    _connection = new _Connection(uri, debug);
}

void Connection::send(json json) {
    _connection->send(json);
}

json Connection::receive() {
    return _connection->receive();
}

Connection::~Connection() {
    std::cerr << "Closing connection...\n";
    // Delete connection
    delete _connection;
}
