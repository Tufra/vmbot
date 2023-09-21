#include <iostream>
#include <chrono>
#include <cstdlib>

#include "telegram.hpp"

namespace json = boost::json;

int main(int argc, char* argv[])
{
    
    std::string token = std::getenv("TGBOT_TOKEN");

    tgbot::Bot bot(token);
    bot.register_handler(
        tgbot::handler::text, 
        [&bot](json::value update) {
            std::cout << "Hello" << update.at("message") << " " << update.at("message").at("chat") << '\n';
            bot.send_message(update.at("message").at("chat").at("id").as_int64(), update.at("message").at("text").as_string().c_str());
        }
    );

    std::cout << "Begin polling" << '\n';
    bot.polling(std::chrono::seconds(10));
    
    return EXIT_SUCCESS;
}