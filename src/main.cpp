#include <iostream>
#include <ostream>
#include <istream>
#include <sstream>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/asio/ssl.hpp>

using boost::asio::ip::tcp;

std::string create_catalog_search_uri(std::string minTickets,
        std::string sortMethod,
        std::string sortDirection,
        std::string firstEvent,
        std::string maxEvents,
        std::string fieldList)
{
    static std::string url("/search/catalog/events/v2?maxPrice=99999");
    static std::string minTicketLimit("&minAvailableTickets=");
    static std::string sortMethodLimit("&sort=");
    static std::string sortDirectionLimit("%20"); // replace space with 0x20
    static std::string firstEventLimit("&start=");
    static std::string maxEventLimit("&limit=");
    static std::string fieldListLimit("&fieldList=");

    return url
        + minTicketLimit + minTickets
        + sortMethodLimit + sortMethod
        + sortDirectionLimit + sortDirection
        + firstEventLimit + firstEvent
        + maxEventLimit + maxEvents
        + fieldListLimit + fieldList
        ;
}

class client
{
    public:
        typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> socket;

        client(boost::asio::ssl::context& ctx,
                boost::asio::io_service& io_service,
                const std::string& server,
                const std::string& path)
            : resolver_(io_service)
            , socket_(io_service, ctx)
            , server_(server)
    {
        std::ostream request_stream(&request_);
        request_stream << "GET " << path << " HTTP/1.0\r\n";
        request_stream << "Host: " << server << "\r\n";
        request_stream << "Accept: */*\r\n";
        request_stream << "Authorization: Bearer X79M_nCok44Uifr1dnVUG2eqFXYa\r\n";
        request_stream << "Connection: close\r\n\r\n";
        tcp::resolver::query query(server, "https");
        resolver_.async_resolve(query,
                boost::bind(&client::handle_resolve, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::iterator));
    }

 private:
     void handle_resolve(const boost::system::error_code& err,
                         tcp::resolver::iterator endpoint_iterator)
     {
         if (!err) {
             tcp::endpoint endpoint = *endpoint_iterator;
             socket_.lowest_layer().async_connect(endpoint,
                     boost::bind(&client::handle_connect, this,
                         boost::asio::placeholders::error, ++endpoint_iterator));
         }
         else {
             std::cout << "(handle_resolve) Error: " << err.message() << "\n";
         }
     }

     void handle_connect(const boost::system::error_code& err,
                         tcp::resolver::iterator endpoint_iterator)
     {
         namespace ssl = boost::asio::ssl;
         if (!err) {
             socket_.lowest_layer().set_option(tcp::no_delay(true));
             socket_.set_verify_mode(ssl::verify_peer);
             socket_.set_verify_callback(ssl::rfc2818_verification(server_));
             socket_.async_handshake(socket::client,
                     boost::bind(&client::handle_handshake, this,
                         boost::asio::placeholders::error));

             /* boost::asio::async_write(socket_, request_, */
             /*                          boost::bind(&client::handle_write_request, this, */
             /*                                      boost::asio::placeholders::error)); */
         }
         else if (endpoint_iterator != tcp::resolver::iterator()) {
             socket_.lowest_layer().close();
             tcp::endpoint endpoint = *endpoint_iterator;
             socket_.lowest_layer().async_connect(endpoint,
                     boost::bind(&client::handle_connect, this,
                         boost::asio::placeholders::error, ++endpoint_iterator));
         }
         else {
             std::cout << "(handle_connect) Error: " << err.message() << "\n";
         }
     }

     void handle_handshake(const boost::system::error_code& err)
     {
         if (!err)
         {
             boost::asio::async_write(socket_, request_,
                     boost::bind(&client::handle_write_request, this,
                         boost::asio::placeholders::error));
         }
         else
         {
             std::cout << "(handle_handshake) Error: " << err.message() << "\n";
         }
     }

     void handle_write_request(const boost::system::error_code& err)
     {
         if (!err) {
             boost::asio::async_read_until(socket_, response_, "\r\n",
                     boost::bind(&client::handle_read_status_line, this,
                         boost::asio::placeholders::error));
         }
         else {
             std::cout << "(handle_write_request) Error: " << err.message() << "\n";
         }
     }

     void handle_read_status_line(const boost::system::error_code& err)
     {
         if (!err) {
             std::istream response_stream(&response_);
             std::string http_version;
             response_stream >> http_version;
             unsigned int status_code;
             response_stream >> status_code;
             std::string status_message;
             std::getline(response_stream, status_message);
             if (!response_stream || http_version.substr(0, 5) != "HTTP/") {
                 std::cout << "Invalid response\n";
                 return;
             }
             if (status_code != 200) {
                 std::cout << "Response returned with status code ";
                 std::cout << status_code << "\n";
                 return;
             }

             boost::asio::async_read_until(socket_, response_, "\r\n\r\n",
                     boost::bind(&client::handle_read_headers, this,
                         boost::asio::placeholders::error));
         }
         else {
             std::cout << "(handle_read_status_line) Error: " << err << "\n";
         }
     }

     void handle_read_headers(const boost::system::error_code& err)
     {
         if (!err) {
             std::istream response_stream(&response_);
             std::string header;
             while (std::getline(response_stream, header) && header != "\r") {
                 std::cout << header << "\n";
             }
             std::cout << "\n";

             if (response_.size() > 0) {
                 std::cout << &response_;
             }

             boost::asio::async_read(socket_, response_,
                     boost::asio::transfer_at_least(1),
                     boost::bind(&client::handle_read_content, this,
                         boost::asio::placeholders::error));
         }
         else {
             std::cout << "(handle_read_headers) Error: " << err << "\n";
         }
     }

     void handle_read_content(const boost::system::error_code& err)
     {
         if (!err) {
             std::cout << &response_;
             boost::asio::async_read(socket_, response_,
                     boost::asio::transfer_at_least(1),
                     boost::bind(&client::handle_read_content, this,
                         boost::asio::placeholders::error));
         }
         else if (err != boost::asio::error::eof) {
             std::cout << "(handle_read_content) Error: " << err << "\n";
         }
     }

     tcp::resolver resolver_;
     socket socket_;
     std::string server_;
     boost::asio::streambuf request_;
     boost::asio::streambuf response_;
 };

int main(int argc, char **argv)
{
    try {
        boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
        ctx.set_default_verify_paths();
        boost::asio::io_service io_service;
        std::string server("api.stubhub.com");
        std::string path(create_catalog_search_uri("10",
                "popularity",
                "desc",
                "0",                    // first event
                "500",                  // number of events (500 max allowed)
                "id,title,dateLocal,eventInfoUrl,ticketInfo,dateUTC,status&"));
//        client clt(ctx, io_service, "www.boost.org", "/LICENSE_1_0.txt");
        client clt(ctx, io_service, server, path);
        io_service.run();
    }
    catch (std::exception& ex)
    {
        std::cout << "Exception: " << ex.what() << "\n";
    }
    return 0;
}
