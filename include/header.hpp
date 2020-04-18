// Copyright 2018 Your Name <your_email>

#ifndef NEW_LIFE_HEADER_H
#define NEW_LIFE_HEADER_H

#include <iostream>
#include <boost/asio.hpp>
#include <string>
#include <mutex>
#include <ctime>
#include <boost/thread/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sinks.hpp>
using Endpoint = boost::asio::ip::tcp::endpoint;
using Acceptor = boost::asio::ip::tcp::acceptor;
using Context = boost::asio::io_context;
using Service = boost::asio::io_service;
using Socket = boost::asio::ip::tcp::socket;
namespace logging = boost::log;
namespace keywords = boost::log::keywords;

const static unsigned PORT_NUM = 8001;
const static unsigned MAX_SYM = 1024;
const static Endpoint ep(boost::asio::ip::address::from_string("127.0.0.1"), PORT_NUM);
const unsigned LOG_SIZE = 10 * 1024 * 1024;
const char LOG_NAME_TRACE[] = "../log/trace_%N.log";
const char LOG_NAME_INFO[] = "../log/info_%N.log";
const char NO_NAME[] = "NO_NAME";

class Client{
public:
    Client(Context *io);
    void start_work();
    void cycle();
    void ping_ok(const std::string& msg);
    void reader();
    void set_uname(std::string new_name);
    void ans_analysis();
    void ask_list();
    void cli_list(const std::string& msg);
    std::string get_uname();
    Socket &get_sock();
    time_t last_ping;
private:
    Socket sock_;
    std::string _username;
    char _buff[MAX_SYM];
    unsigned sym_read = 0;
    bool working = false;
};


class Server {
public:
    Server();
    void starter();
    void listen_thread();
    void worker_thread();
    void reader(std::shared_ptr<Client> &b);
    bool timed_out(std::shared_ptr<Client> &b);
    void stoper(std::shared_ptr<Client> &b);
    void ping_ok(std::shared_ptr<Client> &b);
    void on_clients(std::shared_ptr<Client> &b);
    void req_analysis(std::shared_ptr<Client> &b);
    void login_ok(const std::string& msg, std::shared_ptr<Client> &b);
    void logger();

private:
    std::vector<std::shared_ptr<Client>> _client_list;
    char _buff[MAX_SYM];
    unsigned sym_read;
    Service *_context;
    bool clients_changed_ = false;
    std::mutex cs;
};


#endif //NEW_LIFE_HEADER_H
