
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/asio/ip/basic_resolver.hpp>

#include "TCPServer.hpp"

#include <iostream>
#include <memory>
//thanks kalven for tips/debugging

using boost::asio::ip::tcp;


Session::Session(boost::asio::io_service& ios) : socket(ios) {}
    
    tcp::socket& Session::get_socket()
    {
        return socket;
    }
    
    void Session::start()
    {
        socket.async_read_some(
                               boost::asio::buffer(data, max_length),
                               boost::bind(&Session::handle_read, this,
                                           shared_from_this(),
                                           boost::asio::placeholders::error,
                                           boost::asio::placeholders::bytes_transferred));
    }
    
    void Session::handle_read(std::shared_ptr<Session>& s,
                     const boost::system::error_code& err,
                     size_t bytes_transferred)
    {
        if (!err) {
            //std::cout << "recv: " << std::endl;
			try {
				std::string latestMsg = getLastOrientationMessage(bytes_transferred);
				std::istringstream iss(latestMsg);
				std::vector<std::string> split_tokens(std::istream_iterator<std::string>{iss},
					std::istream_iterator<std::string>());

				if (split_tokens.size() == 6) {
					std::string yaw = split_tokens[1];
					//std::cout << yaw << std::endl;
					ReframeTCPServer::getInstance().setYaw(stof(yaw));
					ReframeTCPServer::getInstance().setPitch(stof(split_tokens[3]));
					ReframeTCPServer::getInstance().setRoll(stof(split_tokens[5]));
				}
				else {
					int error = 0; // just for breakpoint
				}
			}
			catch (const std::exception&)
			{

				// just leave angles like they are
				std::cerr << "err (recv), message corrupt: " << err.message() << std::endl;
			}
            
            boost::this_thread::interruption_point();
            
            socket.async_read_some(
                                   boost::asio::buffer(data, max_length),
                                   boost::bind(&Session::handle_read, this,
                                               shared_from_this(),
                                               boost::asio::placeholders::error,
                                               boost::asio::placeholders::bytes_transferred));
            
        } else {
            std::cerr << "err (recv): " << err.message() << std::endl;
        }
    }
    
std::string Session::getLastOrientationMessage(int bytesTransferred) {
	mtx.lock();
	try {
		std::string dataString(data);
		std::string endString("msgEnd");
		std::string yawString("yaw");
		size_t msgEndPos = dataString.size();
		std::string msgStr = "";

		int continue_search = 1;
		do {
			msgEndPos = dataString.rfind(endString, msgEndPos-1);
			size_t yawPos = dataString.rfind(yawString, msgEndPos);

			msgStr = dataString.substr(yawPos, msgEndPos - yawPos);
			std::istringstream iss(msgStr);
			std::vector<std::string> split_tokens(std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>());
			if (split_tokens.size() == 6 || msgEndPos == std::string::npos || yawPos == std::string::npos)
				continue_search = 0;
			else
				continue_search = 1;
		} while (continue_search);
		mtx.unlock();
		return msgStr;
	}
	catch (const std::exception&){
		mtx.unlock();
	}
	return "";
}

ReframeTCPServer::ReframeTCPServer() :  ios(),
acceptor(ios, tcp::endpoint(tcp::v4(), 8080))
    {

    }

    std::string ReframeTCPServer::startServer(){
        boost::asio::ip::tcp::resolver resolver(ios);
        
        std::string h = boost::asio::ip::host_name();
        std::stringstream sstream;
        sstream << "Host name is " << h << '\n';
        sstream << "Please connect to: \n";
        boost::asio::ip::basic_resolver_results<boost::asio::ip::tcp> re = resolver.resolve({h, ""});
        
        for(auto it = re.begin(); it != re.end(); ++it){
            std::string addr = it->endpoint().address().to_string();
            if(addr.find(".0.") != std::string::npos){
                sstream << it->endpoint().address() << '\n';
            }
        }
        
        std::shared_ptr<Session> session = std::make_shared<Session>(ios);
        acceptor.async_accept(session->get_socket(),
                              boost::bind(&ReframeTCPServer::handle_accept, this,
                                          session,
                                          boost::asio::placeholders::error));
        
        run_thread = new boost::thread(boost::bind(&boost::asio::io_service::run, boost::ref(ios)));
        
        return sstream.str();
    }

    void ReframeTCPServer::stopServer(){
        run_thread->interrupt();
    }

    void ReframeTCPServer::handle_accept(std::shared_ptr<Session> session,
                       const boost::system::error_code& err)
    {
        if (!err) {
            session->start();
            session = std::make_shared<Session>(ios);
            acceptor.async_accept(session->get_socket(),
                                  boost::bind(&ReframeTCPServer::handle_accept, this, session,
                                              boost::asio::placeholders::error));
        }
        else {
            std::cerr << "err: " + err.message() << std::endl;
            session.reset();
        }
    }

float ReframeTCPServer::getYaw(){
    return yaw;
}

void ReframeTCPServer::setYaw(float yaw){
    mtx.lock();
    this->yaw = yaw;
    mtx.unlock();
}

float ReframeTCPServer::getPitch(){
    return pitch;
}

void ReframeTCPServer::setPitch(float pitch){
    mtx.lock();
    this->pitch = pitch;
    mtx.unlock();
}

float ReframeTCPServer::getRoll(){
    return roll;
}

void ReframeTCPServer::setRoll(float roll){
    mtx.lock();
    this->roll = roll;
    mtx.unlock();
}
