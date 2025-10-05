#include <tgbot/tgbot.h>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cctype>
#include <algorithm>

using namespace std;

struct Session{
    time_t since;
};

//main programing
int main() {
    /*take the token of the bot to BOT_TOKEN variable
     but isn't define using the last string in hardcoding */ 
    string token = getenv("BOT_TOKEN") ? getenv("BOT_TOKEN") : "8035986148:AAFq1DEEMThtlia_MTfan61SS9WyRIZcRCg";
    //make bot object
    TgBot::Bot bot(token);
    //password
    const string PASSWORD = getenv("BOT_PASSWORD") ? getenv("BOT_PASSWORD") : "1234";;
    
    // sessions and fail tryings
    unordered_map<int64_t,Session> sessions;
    unordered_map<int64_t,int> failed_attempts;

    //TTL expiration
    const int SESSION_TTL_SEC = 3600;
    //system time consult
    auto now_sec = []() {return static_cast<time_t>(time(nullptr));};

    //check time expiration for the user
    auto is_authed = [&](int64_t uid) -> bool {
        auto it = sessions.find(uid);
        if (it == sessions.end()) return false;
        if (SESSION_TTL_SEC > 0) {
            if (now_sec() - it->second.since > SESSION_TTL_SEC) {
                sessions.erase(it);
                return false;
            } 
        }
        return true;
    };

    //check if the user need authentication
    auto require_auth = [&](auto protected_handler) {
        return [&](TgBot::Message::Ptr msg) {
            if (!msg || !msg->from) return;
            if (!is_authed(msg->from->id)) {
                bot.getApi().sendMessage(msg->chat->id,"Necesita iniciar sesion. Usar :/login <contrasenia>");
                return;
            }
            protected_handler(msg);
        };
    };

    // /start is for warm that need login
    bot.getEvents().onCommand("start",[&](TgBot::Message::Ptr msg) {
        if (!msg) return;
        auto lpo = make_shared<TgBot::LinkPreviewOptions>();
        lpo->isDisabled = true;
        
        bot.getApi().sendMessage(msg->chat->id,           //chatId
                                 "Bienvenido!!. Para usar el bot, inicia sesion: \n"
                                 "`/login <contraseña>`", //text
                                 lpo,                     //linkPreviewOptions
                                 nullptr,                 //replyParameters
                                 nullptr,                 //replyMarkup
                                 "Markdown",              //parseMode
                                 false,                   //disableNotifications
                                 {},                      //entities
                                 0,                       //messageThreadId
                                 false,                   //protectContent
                                 "");                     //messageEffictId
    });


    // /login <pass>: only "free"
    bot.getEvents().onCommand("login", [&](TgBot::Message::Ptr msg) {
        if (!msg || !msg->from) return;

        //extract the next to "/login "
        string input;
        const string &t = msg->text;
        if (t.size() > 7) input = t.substr(7);

        //trim simple
        while (!input.empty() && isspace(static_cast<unsigned char>(input.front()))) input.erase(input.begin());
        while (!input.empty() && isspace(static_cast<unsigned char>(input.back()))) input.pop_back();

        bool ok = (input == PASSWORD);

        //optional: delete the message with the pass
        try{ bot.getApi().deleteMessage(msg->chat->id,msg->messageId);}
        catch(const TgBot::TgException &e){
            cerr << "[BOT]: error of Telegram API trying of delete message:  " << e.what() << endl;
        }catch(const exception &e){
            cerr << "[SYSTEM]: error of server trying of delete message:  " << e.what() << endl;
        }catch(...) {
            cerr << "[UNDEFINED]: error of server trying of delete message:  " << endl;
        
        };

        if (ok) {
            sessions[msg->from->id] = Session{ now_sec() };
            failed_attempts[msg->from->id] = 0;
            string okMsg = "Autenticacion Correcta!!!";
            if (SESSION_TTL_SEC > 0) {
                okMsg += " (La sesion expirara en " + to_string(SESSION_TTL_SEC/60) + " min";
            }
            bot.getApi().sendMessage(msg->chat->id, okMsg);
        } else {
            int &n = failed_attempts[msg->from->id];
            n++;
            string warm = "Contraseña incorecta";
            if (n >= 5) warm += " muchas fallas: espera un rato antes de volver a intentar. ";
            bot.getApi().sendMessage(msg->chat->id, warm);
            
        } 
    });

    //logout
    bot.getEvents().onCommand("logout",require_auth([&](TgBot::Message::Ptr msg){
        sessions.erase(msg->from->id);
        bot.getApi().sendMessage(msg->chat->id,"Sesion cerrada");
    }));

    //example of protected command
    bot.getEvents().onCommand("secret",require_auth([&](TgBot::Message::Ptr msg){
        bot.getApi().sendMessage(msg->chat->id,"comando secreto disponible solo para usuarios autenticados.");
    }));

    //global handler

    bot.getEvents().onAnyMessage([&](TgBot::Message::Ptr msg){
        if (!msg || !msg->from) return;
        
        //ignore commands here, have there own flow
        if (!msg->text.empty() && msg->text[0] == '/') return;
        //user isn't authenticated send relogin message
        if(!is_authed(msg->from->id)) {
            bot.getApi().sendMessage(msg->chat->id, "Usá /login <contraseña> para comenzar.");
            return;
        }

        //logic to authenticated users
        if (!msg->text.empty()) {
            string user = (msg->from->username.empty() ? "sinuser" : msg->from->username);
            cout << " [ @" << user << " ] -> " << msg->text << endl;
            bot.getApi().sendMessage(msg->chat->id,"[ @"+ user + " ] Escribe: " + msg->text);
        }
    });

    //long polling
    TgBot::TgLongPoll long_poll(bot);
    cout << "[BOT] -> runing correctly bot Telegram API" <<endl;
    while (true) {
        try {
            long_poll.start();
        } catch(const exception &e) {
             cerr << "[SYSTEM]: Error ->  " << e.what() << endl;
        }
    }
    return 0;
}

