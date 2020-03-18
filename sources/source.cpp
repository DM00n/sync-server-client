// Copyright 2018 Your Name <your_email>
#include "header.hpp"

My_server::My_server():sym_read(0){
    _context = new Service;
}

void My_server::starter() {
    logger();
    boost::thread t1(&My_server::listen_thread, this);
    boost::thread t2(&My_server::worker_thread, this);
    t1.join();
    t2.join();
}

void My_server::listen_thread(){
    while (true){
        Acceptor acc(*_context, ep);
        My_client cli(_context);
        acc.accept(cli.get_sock());
        cli.last_ping = time(nullptr);
        cs.lock();
        clients_changed_ = true;
        _client_list.emplace_back(std::make_shared<My_client>
                                          (std::move(cli)));
        cs.unlock();
        BOOST_LOG_TRIVIAL(info) << "NEW CLIENT CONNECTED" << std::endl;
    }
}

void My_server::worker_thread() {
    while (true){
        if (_client_list.empty()) continue;
        cs.lock();
        for (auto it = _client_list.begin(); it != _client_list.end();) {
            try
            {
                (*it)->get_sock().non_blocking(true);
                if (timed_out(*it)) throw "c";
                if (!((*it)->get_sock().is_open())) throw 101;
                reader(*it);
                req_analysis(*it);
            }
            catch (std::exception &e)
            {
                if (strcmp(e.what(),
                          "read_some: Resource temporarily unavailable")){
                    //std::cerr << e.what();
                    stoper(*it);
                    _client_list.erase(it);
                    continue;
                }
            }
            catch (char const*){
                (*it)->get_sock().write_some(boost::asio::
                                             buffer("timed_out\n"));
                //std::cerr<<"end";
                stoper(*it);
                _client_list.erase(it);
                continue;
            }
            it++;
        }
        cs.unlock();
    }
}

bool My_server::timed_out(std::shared_ptr<My_client> &b) {
    time_t now = time(nullptr);
    time_t s = now - b->last_ping;
    return s > 5;
}

void My_server::reader(std::shared_ptr<My_client> &b) {
    sym_read = b->get_sock().read_some(boost::asio::buffer(_buff));
}

void My_server::req_analysis(std::shared_ptr<My_client> &b) {
    std::string buffer(_buff, sym_read);
    if (sym_read >= MAX_SYM){
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
}

void My_server::login_ok(const std::string &msg,
        std::shared_ptr<My_client> &b) {
    if (!(b->get_uname().empty())) {
        b->get_sock().write_some(boost::asio::
                                 buffer("you are already logged\n"));
        return;
    }
    std::string n_n(msg, 6);
    for (auto it = _client_list.begin(); it != _client_list.end();){
        if ((*it)->get_uname() == n_n){
            b->get_sock().write_some(boost::asio::
            buffer("client with the same name already exists\n"));
        }
        it++;
    }
    b->set_uname(n_n);
    b->get_sock().write_some(boost::asio::buffer("login ok\n"));
    clients_changed_ = true;
}

void My_server::ping_ok(std::shared_ptr<My_client> &b) {
    if (clients_changed_) {
        b->get_sock().write_some(boost::asio::
                                 buffer("ping client_list_changed\n"));
    }else{
        b->get_sock().write_some(boost::asio::buffer("ping ok\n"));
    }
    clients_changed_ = false;
}

void My_server::on_clients(std::shared_ptr<My_client> &b) {
    std::string msg;
    for (auto it = _client_list.begin(); it != _client_list.end();){
        msg += (*it)->get_uname() + " ";
        it++;
    }
    std::string clients = "clients " + msg + "\n";
    b->get_sock().write_some(boost::asio::buffer(clients));
}

void My_server::stoper(std::shared_ptr<My_client> &b) {
    b->get_sock().close();
}

void My_server::logger() {
    logging::add_common_attributes();
    logging::add_file_log(
            logging::keywords::file_name = LOG_NAME_TRACE,
            logging::keywords::rotation_size = LOG_SIZE,
            logging::keywords::time_based_rotation =
                    logging::sinks::file::rotation_at_time_point(0, 0, 0),
            logging::keywords::filter = logging::trivial::severity
                                        >= logging::trivial::trace,
            logging::keywords::format =
                    "[%TimeStamp%]: %Message%");

    logging::add_file_log(
            logging::keywords::file_name = LOG_NAME_INFO,
            logging::keywords::rotation_size = LOG_SIZE,
            logging::keywords::time_based_rotation =
                    logging::sinks::file::rotation_at_time_point(0, 0, 0),
            logging::keywords::filter = logging::trivial::severity
                                        >= logging::trivial::info,
            logging::keywords::format =
                    "[%TimeStamp%]: %Message%");
}

//------------------------------------------------------------------------

My_client::My_client(Context *io):sock_(*io), last_ping(0){}

void My_client::start_work() {
    try {
        sock_.connect(ep);
        working = true;
        cycle();
    }
    catch(boost::system::system_error & err) {
        working = false;
        std::cout << "client terminated" << std::endl;
    }
}

void My_client::cycle() {
    sock_.write_some(boost::asio::buffer("login " + _username + "\n"));
    reader();
    while (working) {
        std::string msg;
        std::getline(std::cin, msg);
        sock_.write_some(boost::asio::buffer(msg + "\n"));
        reader();
        //boost::this_thread::sleep(boost::posix_time::millisec(rand() % 7000));
    }
}

Socket &My_client::get_sock() {
    return sock_;
}

void My_client::set_uname(std::string new_name) {
    _username = std::move(new_name);
}

std::string My_client::get_uname() {
    return _username;
}
void My_client::reader() {
    sym_read = sock_.read_some(boost::asio::buffer(_buff));
    ans_analysis();
}
void My_client::ans_analysis() {
    std::string msg(_buff, 0, sym_read);
    std::cout << msg;
    if (msg.find("ping ") == 0) ping_ok(msg);
    else if (msg.find("clients ") == 0) cli_list(msg);
    else if (msg.find("timed_out") == 0) working = false;
}
void My_client::ping_ok(const std::string& msg) {
    std::string str(msg, 5);
    if (str == "client_list_changed\n") ask_list();
}

void My_client::ask_list() {
    write(sock_, boost::asio::buffer("ask_clients\n"));
    reader();
}

void My_client::cli_list(const std::string& msg) {
    std::string str(msg, 8);
}
