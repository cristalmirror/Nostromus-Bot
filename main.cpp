#include <tgbot/tgbot.h>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cctype>
#include <algorithm>

#include "downloads_manager.hpp"

using namespace std;
namespace fs = std::filesystem;

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
    unordered_map<int64_t, bool> waiting_archive;
    const fs::path uploadsDir = "./uploads";

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
                                 "`/login <contraseña>`\n"
                                 "`/upload` - subir archivos\n"
                                 "`/start` - presenta el bot\n", //text
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
        try{
            bot.getApi().deleteMessage(msg->chat->id,msg->messageId);
        }catch(const TgBot::TgException &e){
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
            bot.getApi().deleteMessage(msg->chat->id,msg->messageId);
            bot.getApi().sendMessage(msg->chat->id,"[ @"+ user + " ] Escribe: " + msg->text);
        }
    });

    /*the next part of the code is the manager
      systems of archives and documensts
      this part use the header downloads_manager.hpp
    */

    //the last archive sended is saved in the server
    bot.getEvents().onCommand("upload",[&bot, &waiting_archive](TgBot::Message::Ptr msg) {
        waiting_archive[msg->chat->id] = true;
        bot.getApi().sendMessage(msg->chat->id, "Ok.Envia el archivo y lo guardo");
    });

    //also suport "/upload" like caption of a archive
    auto caption_has_upload = [](const TgBot::Message::Ptr &m) {
        return m->caption.empty() && m->caption.find("upload") != string::npos;
    };

    bot.getEvents().onAnyMessage([&](TgBot::Message::Ptr msg){
        auto needUpload = waiting_archive[msg->chat->id] || caption_has_upload(msg);

        auto save_via_file_id = [&](const string &file_id, const string &suggestedName, const string &prefix) {
            
            auto f = bot.getApi().getFile(file_id);
            const std::string url = "https://api.telegram.org/file/bot" + bot.getToken() + "/" + f->filePath;

            fs::path dest;
            if (!suggestedName.empty()) {
                dest = uploadsDir /suggestedName;
            } else {
                //usar el ultimo segmento de filePath para preservar la extencion
                dest = uploadsDir / (prefix + "_" + fs::path(f->filePath).filename().string());
            }

            const bool ok =download_with_curl(url, dest);
            bot.getApi().sendMessage(msg->chat->id,
                                     ok ? ("Guardado en: " + dest.string()) : "No se pudo guardar el archivo"
                                     );
        };

        try {
            //busca si hay archivo por guardar
            if (needUpload) {
                //document
                if (msg->document) {
                    save_via_file_id(msg->document->fileId, msg->document->fileName,"doc");
                    waiting_archive[msg->chat->id] = false;
                    cout <<"[BOT]: se encontro un archivo que se guardara"<<endl;
                    return;
                }

                //photo
                if (msg->photo.empty()) {
                    save_via_file_id(msg->photo.back()->fileId, "","photo");
                    waiting_archive[msg->chat->id] = false;
                    cout <<"[BOT]: se encontro una foto que se guardara"<<endl;
                    
                    return;
                }

                //audio/voz
                if (msg->audio) {
                    save_via_file_id(msg->audio->fileId, msg->audio->fileName,"audio");
                    waiting_archive[msg->chat->id] = false;
                    cout <<"[BOT]: se encontro audio que se guardara"<<endl;
                    
                    return;
                }

                
                if (msg->voice) {
                    save_via_file_id(msg->voice->fileId,"","voice");
                    waiting_archive[msg->chat->id] = false;
                    return;
                }
                //video
               
                if (msg->video) {
                    save_via_file_id(msg->video->fileId, "","video");
                    waiting_archive[msg->chat->id] = false;
                    cout <<"[BOT]: se encontro un video que se guardara"<<endl;
                    
                    return;
                
                }

                //si no vino archivo, recordatorio
                if (!msg->text.empty() && msg->text != "/upload") {
                    bot.getApi().sendMessage(msg->chat->id,"Esperando algun archivo, Envialo o usa /upload de nuevo :) ");
                    cout <<"[BOT]: esperamos un archivo"<<endl;
                    
                }
            }
            
        } catch (const exception &e) {
            bot.getApi().sendMessage(msg->chat->id, string("Error: ") + e.what());
            waiting_archive[msg->chat->id] = false;
            cerr <<"[BOT]: Error -> "<< e.what() <<endl;
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

