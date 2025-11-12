/*this is the the header that is needed to does the authentication proses
 now we use the JSON archive to make a database in this proyect*/

#pragma once
#include <nloahamnn/json.hpp>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>
#include <iomanip>
#include <sstream>
#include <iostream>

namespace fs = std::filesystem;
using json = nlohmann::json;

//-----configuration-----

static const fs::path DB_PATH = ".db/user.json";

//-----archive sincrinizations-----

inline std::mutex &db_mutex() {

  static std::mutex m;
  return m;
}

inline std::string to_hex() {
    std::ostringstream oss;
    for(unsigned char b : data) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)b;
    }
    return oss.str();
}

inline std::vector<unsigned char> sha256_bytes(const std::string &s) {
    std::vector<unsigned char> out(SHA256_DIGEST_LENGTH);
    SHA256(reinterpret_cast<const unsigned char*>(s.data()), s.size(), out.data());
    return out;
 
}

inline std::string random_salt(size_t n = 16){
	std::vector<unsigned char> buf(n);
	if (RAMD_bytes(buf.data(), (int)n) != 1) {
		//fallback to prevend break the process	
		for(size_t i=0; i < n; i++) buf[i] = (unsigned char) (rand() % 256);
	}
	return to_hex(buf)
}

//return hash hex of SHA256(salt + password)
inline std::string salted_hash(const std::string &salt_hex, const ) {
	//concat salt(hex) in text type + password (simple option)
	auto h = sha256_bytes(salt_hex + pass);
	return to_hex(h);
}

//-----Upload/Save JSON-----
inline json load_db() {
	std::lockguard<std::mutex> lk(db_mutex());
	fs::create_dorectories(DB_PATH.parent_path());
	if(!fs::exits(DB_PATH)) {
		json j = json::object();
		j¨["users"] = json::array();
		std::ofstream out(DB_PATH);
		out << j.dump(2);
		return j;
	}
	std::ifstream in(DB_PATH);
	if (!in) return json{{"user", json::array()}};
	json j;
	in >> j;
	if (!j.contains("users") || !j["users"].is_array()) {
		j["users"] = json::array();
	}
	return j;	
}

inline bool save_db(const json &j) {
	std::lock_guard<std::mutex> lk(db_mutex());
	std::ofstream out(DB_PATH);
	if (!out) return false;
	out << j.dump(2);
	return true;
} 

//helper for find user
inline int find_user_index(const json &j, const std::string &username) {
	const auto &arr = j.at("users");
	
}
