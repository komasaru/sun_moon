#include "file.hpp"

namespace sun_moon {

// 定数
constexpr char kFLeapSec[13] = "LEAP_SEC.txt";
constexpr char kFDut1[9]     = "DUT1.txt";

/*
 * @brief       UTC - TAI (協定世界時と国際原子時の差 = うるう秒の総和) 一覧取得
 *
 * @param[ref]  UTC - TAI 一覧(vector<vector<string>>)
 * @return      <none>
 */
bool File::get_leap_sec_list(std::vector<std::vector<std::string>>& data) {
  std::string f(kFLeapSec);  // ファイル名
  std::string buf;           // 1行分バッファ

  try {
    // ファイル OPEN
    std::ifstream ifs(f);
    if (!ifs) return false;  // 読み込み失敗

    // ファイル READ
    while (getline(ifs, buf)) {
      std::vector<std::string> rec;  // 1行分ベクタ
      std::istringstream iss(buf);   // 文字列ストリーム
      // 1行分文字列を1行分ベクタに追加
      std::string s;
      while (iss >> s) rec.push_back(s);
      // 1行分ベクタを data ベクタに追加
      if (rec.size() != 0) data.push_back(rec);
    }
  } catch (...) {
    return false;
  }

  return true;
}

/*
 * @brief       DUT1 (UT1(世界時1) と UTC(協定世界時)の差) 一覧取得
 *
 * @param[ref]  DUT1 一覧(vector<vector<string>>)
 * @return      <none>
 */
bool File::get_dut1_list(std::vector<std::vector<std::string>>& data) {
  std::string f(kFDut1);  // ファイル名
  std::string buf;        // 1行分バッファ

  try {
    // ファイル OPEN
    std::ifstream ifs(f);
    if (!ifs) return false;  // 読み込み失敗

    // ファイル READ
    while (getline(ifs, buf)) {
      std::vector<std::string> rec;  // 1行分ベクタ
      std::istringstream iss(buf);   // 文字列ストリーム
      // 1行分文字列を1行分ベクタに追加
      std::string s;
      while (iss >> s) rec.push_back(s);
      // 1行分ベクタを data ベクタに追加
      if (rec.size() != 0) data.push_back(rec);
    }
  } catch (...) {
    return false;
  }

  return true;
}

}  // namespace sun_moon

