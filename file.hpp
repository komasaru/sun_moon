#ifndef SUN_MOON_FILE_HPP_
#define SUN_MOON_FILE_HPP_

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace sun_moon {

class File {

public:
  bool get_leap_sec_list(std::vector<std::vector<std::string>>&);  // 取得: うるう秒一覧
  bool get_dut1_list(std::vector<std::vector<std::string>>&);      // 取得: DUT1 一覧
};

}  // namespace sun_moon

#endif

