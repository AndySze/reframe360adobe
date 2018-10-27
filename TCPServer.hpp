#pragma once

#include <iostream>
#include <memory>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>

#include <mutex>

//thanks kalven for tips/debugging

using boost::asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session>
{
    
public:
    Session(boost::asio::io_service& ios);
    
    tcp::socket& get_socket();
    
    void start();
    
    void handle_read(std::shared_ptr<Session>& s,
                     const boost::system::error_code& err,
                     size_t bytes_transferred);
    
private:
    tcp::socket socket;
    enum { max_length = 128 };
    char data[max_length];
    
    std::string getLastOrientationMessage();
    std::mutex mtx;

};

class ReframeTCPServer {
public:
    static ReframeTCPServer& getInstance()
    {
        static ReframeTCPServer    instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }

private:
    ReframeTCPServer();
    
    boost::asio::io_service ios;
    tcp::acceptor acceptor;
    
    float yaw, pitch, roll;
    
    std::mutex mtx;
    
    boost::thread* run_thread;
    
public:
    //ReframeTCPServer(boost::asio::io_service& ios, short port);
    ReframeTCPServer(ReframeTCPServer const&) = delete;
    void operator=(ReframeTCPServer const&) = delete;
    
    void handle_accept(std::shared_ptr<Session> session,
                       const boost::system::error_code& err);
    
    std::string startServer();
    void stopServer();
    
    float getYaw();
    void setYaw(float yaw);
    
    float getPitch();
    void setPitch(float pitch);
    
    float getRoll();
    void setRoll(float roll);
};
