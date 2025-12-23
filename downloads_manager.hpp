#pragma once
#include <curl/curl.h>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <thread>

namespace fs = std::filesystem;

//libcurl callback to write archives
inline size_t write_to_file(void *ptr, size_t size, size_t nmemb, void *stream) {

    std::ofstream *out =static_cast<std::ofstream*>(stream);
    out->write(static_cast<const char*>(ptr),size * nmemb);
    return size * nmemb;
    
}

//download and from url and save in destpath
inline bool download_with_curl(const std::string &url, const fs::path &destPath) {
    CURL *curl = curl_easy_init();
	std::string blue ="\033[0;34m";
	std::string inv_blue ="\033[7;34m";
	std::string nc = "\033[0m";
	
	if(!curl) return false;

    fs::create_directories(destPath.parent_path()); //asegura carpeta
    std::ofstream out(destPath, std::ios::binary);
    if (!out) {
        curl_easy_cleanup(curl);
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_file);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out);
    //timeouts
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120L);
    
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    out.close();
    std::cout << blue <<"[BOT]:  **descarga desde**  -->> "<< nc << std::endl <<inv_blue << url << nc <<std::endl;
    return (res == CURLE_OK);
    
    
}

inline void start_threaded_download(const std::string url,
                                    const fs::path destPath) {
  std::thread t([url, destPath]() {
    std::string orange = "\033[38;5;208m";
    std::string red = "\033[0;31m";
    std::string green = "\033[0;32m";
    std::string nc = "\033[0m";
      
    std::cout << orange <<"××××××××××××××××××××××××××××××××××"<< nc << std::endl;
    std::cout << orange << "[THREAD] Descarga en segundo plano." <<std::endl << destPath.filename() << nc << std::endl;
  
    bool ok = download_with_curl(url, destPath);
  
    if (ok) {
      std::cout << green << "=============================" << nc << std::endl;
      std::cout << green << "[THREAD] Inciando la descarga" << nc << std::endl;
      std::cout << green << "=============================" << nc << std::endl;
    } else {
      std::cout << red << "==========================" << nc << std::endl;
      std::cerr << red << "[THREAD] La descarga fallo" << nc << std::endl;
      std::cout << red << "==========================" << nc << std::endl;
    }
    std::cout << orange << "××××××××××××××××××××××××××××××××××" << nc << std::endl;
  });

  // librera el hilo de ejecucion
  t.detach();
}
