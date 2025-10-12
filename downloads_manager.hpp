#pragma once
#include <curl/curl.h>
#include <fstream>
#include <filesystem>
#include <iostream>
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
    std::cout <<"[BOT]:  **descarga desde**  -->> "<< url <<std::endl;
    return (res == CURLE_OK);
    
    
}
