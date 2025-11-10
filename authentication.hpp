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

