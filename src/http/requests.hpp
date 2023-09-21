#pragma once

#ifndef REQUESTS_H
#define REQUESTS_H

#include <iostream>
#include <map>

#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <fmt/core.h>

namespace beast = boost::beast;     // from <boost/beast.hpp>
namespace http = beast::http;       // from <boost/beast/http.hpp>
namespace net = boost::asio;        // from <boost/asio.hpp>
using tcp = net::ip::tcp;           // from <boost/asio/ip/tcp.hpp>
namespace ssl = boost::asio::ssl;
typedef ssl::stream<tcp::socket> ssl_socket;

typedef std::map<std::string, std::string> query_params_t;
typedef std::map<http::field, std::string> headers_t;

// Returns a bad request response
auto const bad_request = [](http::request<http::string_body>& req, beast::string_view why)
    {
        http::response<http::string_body> res{http::status::bad_request, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = std::string(why);
        res.prepare_payload();
        return res;
    };

// Returns a not found response
auto const not_found = [](http::request<http::string_body>& req, beast::string_view target)
    {
        http::response<http::string_body> res{http::status::not_found, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "The resource '" + std::string(target) + "' was not found.";
        res.prepare_payload();
        return res;
    };

// Returns a server error response
auto const server_error = [](http::request<http::string_body>& req, beast::string_view what)
    {
        http::response<http::string_body> res{http::status::internal_server_error, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "An error occurred: '" + std::string(what) + "'";
        res.prepare_payload();
        return res;
    };

std::string do_https (
    const std::string host, 
    const std::string target, 
    http::verb method, 
    std::string body, 
    query_params_t query,  
    headers_t headers, 
    std::chrono::seconds timeout)
{
    try
    {
        auto const port = "443";
        int version = 11;

        // The io_context is required for all I/O
        net::io_context ioc;

        ssl::context ctx(ssl::context::sslv23);
        ctx.set_default_verify_paths();

        // These objects perform our I/O
        tcp::resolver resolver(ioc);
        // beast::tcp_stream stream(ioc);
        beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

        // Look up the domain name
        tcp::resolver::query resolve_query(host, "https");
        auto const results = resolver.resolve(resolve_query);

        // Make the connection on the IP address we get from a lookup
        beast::get_lowest_layer(stream).connect(results);
        beast::get_lowest_layer(stream).expires_after(timeout);

        stream.handshake(ssl_socket::client);

        std::string compiled_target = target;

        if (query.size() > 0) {
            compiled_target += '?';

            for (const auto &[key, value] : query) {
                compiled_target.append(key + "=" + value);    
                compiled_target += '&';
            }
            compiled_target = compiled_target.erase(compiled_target.size() - 1);
        }

        // Set up an HTTP GET request message
        http::request<http::string_body> req{method, compiled_target, version};

        // Set up request content
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        for (const auto &[key, value] : headers) {
            req.set(key, value);
        }
        
        if (body.size() > 0) {
            req.body() = body;
            req.prepare_payload();
        }

        // std::cout << std::endl << req << std::endl;

        // Send the HTTP request to the remote host
        http::write(stream, req);

        // This buffer is used for reading and must be persisted
        beast::flat_buffer buffer;

        // Declare a container to hold the response
        http::response<http::string_body> res;

        // Receive the HTTP response
        http::read(stream, buffer, res);

        // Write the message to standard out

        if (res.result_int() >= 400) {
            throw std::runtime_error(res.reason());
        }
        
        // Gracefully close the socket
        beast::error_code ec;
        // stream.shutdown(ec);

        // not_connected happens sometimes
        // so don't bother reporting it.
        //
        if(ec && ec != beast::errc::not_connected)
            throw beast::system_error{ec};

        // If we get here then the connection is closed gracefully

        return res.body();
    }
    catch(std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return "";
    }
}

std::string do_https (
    const std::string host, 
    const std::string target, 
    http::verb method, 
    query_params_t query,
    headers_t headers, 
    std::chrono::seconds timeout) 
{
    return do_https(host, target, method, "", query, headers, timeout);
}

#endif