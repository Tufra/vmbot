#pragma once

#ifndef TELEGRAM_H
#define TELEGRAM

#include <fmt/core.h>
#include <iostream>
#include <vector>
#include <regex>
#include <algorithm>
#include <future>

#include <boost/json.hpp>
#include "http/requests.hpp"

namespace json = boost::json;
namespace beast = boost::beast;

namespace tgbot 
{

    /**
     * Predefined regex for handlers
    */
    namespace handler
    {
        auto text = std::regex(".*");
        auto start = std::regex("\\/start");
        auto help = std::regex("\\/help");
    }

    typedef std::pair<std::regex, std::function<void(json::value)>> handler_entry;

/**
 * Telegram Bot class
*/
class Bot 
{
private:
    std::string token;
    const std::string host = "api.telegram.org";

    int64_t offset = 0;

    std::vector<handler_entry> handlers;

public:
    Bot(std::string token): token(token), offset(0) {}
    Bot(std::string token, int64_t offset): token(token), offset(offset) {}
    Bot(Bot& bot): token(bot.token) {}
    ~Bot() {}

    /**
     * Requests updates using long polling
     * @param timeout timeout on request
     * 
     * @return json-array of updates
    */
    json::value get_updates(std::chrono::seconds timeout) 
    {
        auto target = fmt::format("/bot{}/getUpdates", token);

        query_params_t params;
        if (offset != 0) {
            params["offset"] = std::to_string(offset);
        }
        params["timeout"] = "20";


        try 
        {
            auto resp = do_https(host, target, beast::http::verb::get, params, {}, timeout);
            return json::parse(resp);
        } 
        catch(std::exception e) 
        {
            std::cerr << e.what() << std::endl;
            return json::parse("{}");
        }
    }

    /**
     * Send text message to specified chat
     * 
     * @param to chat id
     * @param text message content
     * 
     * @return json representing the message
    */
    json::value send_message(int64_t to, std::string text) 
    {
        auto target = fmt::format("/bot{}/sendMessage", token);

        json::value body {
            {"chat_id", to},
            {"text", text}
        };

        query_params_t params;
        params["chat_id"] = std::to_string(to);
        params["text"] = text;

        std::cout << "Sending " << body << " to " << to << '\n';

        try 
        {
            auto resp = do_https(host, target, beast::http::verb::post, params, {}, std::chrono::seconds(60));
            return json::parse(resp);
        } 
        catch(const std::exception& e) 
        {
            std::cerr << e.what() << '\n';
            return json::parse("{}");
        }
    }

    /**
     * Adds handler for updates
     * 
     * @param trigger regex representing target message
     * @param handler handler function (accepts `json::value` with update)
     * 
    */
    void register_handler(std::regex trigger, std::function<void(json::value)> handler) 
    {
        handlers.push_back(std::make_pair(trigger, handler));
    }

    void handle_updates(json::value updates) 
    {
        if (updates.at("ok").as_bool())
        {
            for(const auto& update : updates.at("result").as_array())
            {
                auto match = std::find_if(
                    handlers.begin(), 
                    handlers.end(), 
                    [=](handler_entry handler) {
                        return std::regex_match(
                            update.at("message").at("text").as_string().c_str(),
                            handler.first);
                    });

                if (match != handlers.end())
                {
                    offset = update.at("update_id").as_int64() + 1;
                    std::async(std::launch::async, match->second, update);
                }
            }
        }
    }

    /**
     * Performs infinite polling
     * 
     * @param timeout timeout on requests
    */
    void polling(std::chrono::seconds timeout) 
    {
        try
        {
            while (true)
            {
                json::value updates = get_updates(timeout);
                std::cout << updates << '\n';
                handle_updates(updates);
            }
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
    }

};

};

#endif