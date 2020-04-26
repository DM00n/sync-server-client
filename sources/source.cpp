// Copyright 2018 Your Name <your_email>
#include "header.hpp"

Server::Server() : sym_read(0) {
    _context = new Service;
}

void Server::starter() {
    logger();
    boost::thread t1(&Server::listen_thread, this);
    boost::thread t2(&Server::worker_thread, this);
    t1.join();
    t2.join();
}

void Server::listen_thread() {
    while (true) {
        Acceptor acc(*_context, ep);
        Client cli(_context);
        acc.accept(cli.get_sock());
        cli.last_ping = time(nullptr);
        cs.lock();
        clients_changed_ = true;
        _client_list.emplace_back(std::make_shared<Client>
                                          (std::move(cli)));
        cs.unlock();
        BOOST_LOG_TRIVIAL(info) << "NEW CLIENT CONNECTED";
    }
}

void Server::worker_thread() {
    while (true) {
        if (_client_list.empty()) continue;
        cs.lock();
        for (auto it = _client_list.begin(); it != _client_list.end();) {
            try {
                (*it)->get_sock().non_blocking(true);
                if (timed_out(*it)) throw "c";
                if (!((*it)->get_sock().is_open())) throw 101;
                reader(*it);
                req_analysis(*it);
            }
            catch (std::exception &e) {
                if (strcmp(e.what(),
                           "read_some: Resource temporarily unavailable")) {
                    stoper(*it);
                    BOOST_LOG_TRIVIAL(info) << (*it)->get_uname()
                        << " DISCONNECTED";
                    _client_list.erase(it);
                    continue;
                }
            }
            catch (char const *) {
                (*it)->get_sock().write_some(boost::asio::
                                             buffer("timed_out\n"));
                stoper(*it);
                BOOST_LOG_TRIVIAL(info) << (*it)->get_uname()
                    << " DISCONNECTED";
                _client_list.erase(it);
                continue;
            }
            it++;
        }
        cs.unlock();
    }
}

bool Server::timed_out(std::shared_ptr<Client> &b) {
    time_t now = time(nullptr);
    time_t s = now - b->last_ping;
    return s > 5;
}

void Server::reader(std::shared_ptr<Client> &b) {
    sym_read = b->get_sock().read_some(boost::asio::buffer(_buff));
}

void Server::req_analysis(std::shared_ptr<Client> &b) {
    std::string buffer(_buff, sym_read);
    if (sym_read >= MAX_SYM) {
        b->get_sock().write_some(boost::asio::
                                 buffer("message is too long\n"));
        return;
    }
    if (!sym_read) return;
    unsigned n_pos = buffer.find('\n', 0);
    if (n_pos >= MAX_SYM) return;
    b->last_ping = time(nullptr);
    std::string msg(_buff, 0, n_pos);
    if (msg.find("login ") == 0) login_ok(msg, b);
    else if (msg == "ping") ping_ok(b);
    else if (msg == "ask_clients") on_clients(b);
    else
        b->get_sock().write_some(boost::asio::buffer("bad message\n"));
    memset(_buff, 0, MAX_SYM);
    sym_read = 0;
}

void Server::login_ok(const std::string &msg, std::shared_ptr<Client> &b) {
    if ((strcmp(b->get_uname().c_str(), NO_NAME))) {
        b->get_sock().write_some(boost::asio::
                                 buffer("you are already logged\n"));
        return;
    }
    std::string n_n(msg, 6);
    for (auto it = _client_list.begin(); it != _client_list.end();) {
        if ((*it)->get_uname() == n_n) {
            b->get_sock().write_some(boost::asio::
                buffer("client with the same name already exists\n"));
            return;
        }
        it++;
    }
    b->set_uname(n_n);
    BOOST_LOG_TRIVIAL(info) << "LOGGED " << n_n;
    b->get_sock().write_some(boost::asio::buffer("login ok\n"));
    clients_changed_ = true;
}

void Server::ping_ok(std::shared_ptr<Client> &b) {
    if (clients_changed_) {
        b->get_sock().write_some(boost::asio::
                                 buffer("ping client_list_changed\n"));
    } else {
        b->get_sock().write_some(boost::asio::buffer("ping ok\n"));
    }
    clients_changed_ = false;
}

void Server::on_clients(std::shared_ptr<Client> &b) {
    std::string msg;
    for (auto it = _client_list.begin(); it != _client_list.end();) {
        msg += (*it)->get_uname() + " ";
        it++;
    }
    std::string clients = "clients " + msg + "\n";
    b->get_sock().write_some(boost::asio::buffer(clients));
}

void Server::stoper(std::shared_ptr<Client> &b) {
    b->get_sock().close();
}

void Server::logger() {
    boost::shared_ptr< logging::core > core = logging::core::get();

    boost::shared_ptr< boost::log::sinks::text_file_backend > backend =
            boost::make_shared< boost::log::sinks::text_file_backend >(
                    keywords::file_name = "../file_%5N.log",
                    keywords::rotation_size = 5 * 1024 * 1024,
                    keywords::format = "[%TimeStamp%]: %Message%",
                    keywords::time_based_rotation =
                        boost::log::sinks::file::
                            rotation_at_time_point(12, 0, 0));

    typedef boost::log::sinks::synchronous_sink
    < boost::log::sinks::text_file_backend > sink_t;
    boost::shared_ptr< sink_t > sink(new sink_t(backend));
// sink ->set_filter(logging::trivial::severity >= logging::trivial::info);
    core->add_sink(sink);
    logging::add_console_log
            (
                    std::cout,
                    logging::keywords::format =
                            "[%TimeStamp%]: %Message%");
}

//------------------------------------------------------------------------

Client::Client(Context *io) : last_ping(0), sock_(*io), _username(NO_NAME) {}

void Client::start_work() {
    try {
        sock_.connect(ep);
        working = true;
        cycle();
    }
    catch (boost::system::system_error &err) {
        working = false;
        std::cout << "client terminated" << std::endl;
    }
}

void Client::cycle() {
    sock_.write_some(boost::asio::buffer("login " + _username + "\n"));
    reader();
    while (working) {
        std::string msg;
        std::getline(std::cin, msg);
        sock_.write_some(boost::asio::buffer(msg + "\n"));
        reader();
    }
}

Socket &Client::get_sock() {
    return sock_;
}

void Client::set_uname(std::string new_name) {
    _username = std::move(new_name);
}

std::string Client::get_uname() {
    return _username;
}

void Client::reader() {
    sym_read = sock_.read_some(boost::asio::buffer(_buff));
    ans_analysis();
}

void Client::ans_analysis() {
    std::string msg(_buff, 0, sym_read);
    std::cout << msg;
    if (msg.find("ping ") == 0) ping_ok(msg);
    else if (msg.find("clients ") == 0) cli_list(msg);
    else if (msg.find("timed_out") == 0)
    {
         working = false;
    }
}

void Client::ping_ok(const std::string &msg) {
    std::string str(msg, 5);
    if (str == "client_list_changed\n") ask_list();
}

void Client::ask_list() {
    write(sock_, boost::asio::buffer("ask_clients\n"));
    reader();
}

void Client::cli_list(const std::string &msg) {
    std::string str(msg, 8);
}
