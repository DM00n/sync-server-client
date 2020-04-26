// Copyright 2018 Your Name <your_email>

#ifndef INCLUDE_HEADER_HPP_
#define INCLUDE_HEADER_HPP_

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
#include <vector>

using Endpoint = boost::asio::ip::tcp::endpoint;
using Acceptor = boost::asio::ip::tcp::acceptor;
using Context = boost::asio::io_context;
using Service = boost::asio::io_service;
using Socket = boost::asio::ip::tcp::socket;
namespace logging = boost::log;
namespace keywords = boost::log::keywords;

const unsigned PORT_NUM = 8001;
const unsigned MAX_SYM = 1024;
const unsigned TIME_OUT = 10;
const Endpoint ep(boost::asio::ip::address::from_string("127.0.0.1"),
        PORT_NUM);
const unsigned LOG_SIZE = 10 * 1024 * 1024;
const char LOG_NAME_TRACE[] = "../log/trace_%N.log";
const char LOG_NAME_INFO[] = "../log/info_%N.log";
const char NO_NAME[] = "NO_NAME";
const char MSG_DISCONNECT[] = " DISCONNECTED";
const char MSG_LOGGED[] = "LOGGED ";
const char MSG_CONNECT[] = "NEW CLIENT CONNECTED";
const char MSG_TIMED_OUT[] = "timed_out\n";
const char MSG_TOO_LONG[] = "message is too long\n";
const char REQ_LOGIN[] = "login ";
const char REQ_PING[] = "ping";
const char MSG_ASK_CLIENTS[] = "ask_clients";
const char MSG_BAD_MESSAGE[] = "bad message\n";
const char MSG_ALREADY_LOGGED[] = "you are already logged\n";
const char MSG_SAME_CLIENT_NAME[] =
        "client with the same name already exists\n";
const char MSG_LOGIN_OK[] = "login ok\n";
const char MSG_PING_CLIENTS_CHANGED[] = "ping client_list_changed\n";
const char MSG_PING_OK[] = "ping ok\n";
const char MSG_CLIENTS[] = "clients ";
const char MSG_CLIENT_TERMINATED[] = "client terminated";
const char CLI_PING[] = "ping ";
const char MSG_CLIENTS_CHANGED[] = "client_list_changed\n";
const char REQ_ASK_CLIENTS[] = "ask_clients\n";
const char GOOD_ERROR[] = "read_some: Resource temporarily unavailable";

class Client{
public:
    explicit Client(Context *io);
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


#endif //INCLUDE_HEADER_HPP_
