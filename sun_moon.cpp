/***********************************************************
  日・月の出・南中・入時刻、
  日・月の出・入方位角、
  日・月の南中高度  の計算

    DATE        AUTHOR       VERSION
    2021.04.09  mk-mode.com  1.00 新規作成
    2021.07.24  mk-mode.com  1.01 月の出・南中・入 時の方位角・高度が
                                  0.0 未満の場合の表示を改修。

  Copyright(C) 2021 mk-mode.com All Rights Reserved.
------------------------------------------------------------
  引数 : 99999999 [+-]999.99 [+-]999.99 [+]9999.99
         第1: 日付 [必須]
              計算対象の日付(グレゴリオ暦)を半角8桁数字で指定
         第2: 緯度 [必須]
              緯度を 度 で指定（度・分・秒は度に換算して指定すること）
              (北緯はプラス、南緯はマイナス。桁数は特に制限なし)
         第3: 経度 [必須]
              経度を 度 で指定（度・分・秒は度に換算して指定すること）
              (東経はプラス、西経はマイナス。桁数は特に制限なし)
         第4: 標高 [必須]
              標高をメートルで指定(マイナス値は指定不可)
              (桁数は特に制限なし)
***********************************************************/
#include "calc.hpp"
#include "time.hpp"

#include <cstdlib>   // for EXIT_XXXX
#include <ctime>
#include <iomanip>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
  namespace ns = sun_moon;
  std::string tm_str;        // time string
  unsigned int s_tm;         // size of time string
  struct timespec jst;       // JST
  struct tm t = {};          // for work
  double lat;                // latitude
  double lng;                // lngitude
  double ht;                 // hight
  char   s_lat = 'N';        // N: 北緯, S: 南緯
  char   s_lng = 'E';        // E: 東経, W: 西経
  struct ns::TmAh tm_ah_sr;  // 日の出
  struct ns::TmAh tm_ah_ss;  // 日の入
  struct ns::TmAh tm_ah_sm;  // 日の南中
  struct ns::TmAh tm_ah_mr;  // 月の出
  struct ns::TmAh tm_ah_ms;  // 月の入
  struct ns::TmAh tm_ah_mm;  // 月の南中

  try {
    // コマンドライン引数取得
    if (argc < 5) {
      std::cout << "[USAGE] ./sun_moon YYYYMMDD LATITUDE LONGITUDE HEIGHT"
                << std::endl;
      return EXIT_FAILURE;
    }
    // [日付]
    tm_str = argv[1];
    s_tm = tm_str.size();
    if (s_tm > 8) { 
      std::cout << "[ERROR] Over 8-digits!" << std::endl;
      return EXIT_FAILURE;
    }
    std::istringstream is(tm_str);
    is >> std::get_time(&t, "%Y%m%d%H%M%S");
    jst.tv_sec  = mktime(&t);
    jst.tv_nsec = 0;
    // [緯度・経度・標高]
    lat = std::stod(argv[2]);
    lng = std::stod(argv[3]);
    ht  = std::stod(argv[4]);
    if (lat < 0.0) {
      s_lat = 'S';
      lat = std::abs(lat);
    }
    if (lng < 0.0) {
      s_lng = 'W';
      lng = std::abs(lng);
    }

    // 各種計算
    ns::Calc o_c(jst, lat, lng, ht);
    tm_ah_sr = o_c.calc_sun(0);   // 日の出
    tm_ah_ss = o_c.calc_sun(1);   // 日の入
    tm_ah_sm = o_c.calc_sun(2);   // 日南中
    tm_ah_mr = o_c.calc_moon(0);  // 月の出
    tm_ah_ms = o_c.calc_moon(1);  // 月の入
    tm_ah_mm = o_c.calc_moon(2);  // 月南中

    std::cout << "[" << ns::gen_time_str(jst).substr(0, 10) << "JST "
              << std::fixed << std::setprecision(4)
              << lat << s_lat << " " << lng << s_lng << " " << ht << "m]"
              << std::endl;
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "日の出 " << ns::gen_time_str(tm_ah_sr.time).substr(11, 8)
              << " (方位角 "
              << std::setw(6) << tm_ah_sr.ah << "°)"
              << std::endl;
    std::cout << "日南中 " << ns::gen_time_str(tm_ah_sm.time).substr(11, 8)
              << " (　高度 "
              << std::setw(6) << tm_ah_sm.ah << "°)"
              << std::endl;
    std::cout << "日の入 " << ns::gen_time_str(tm_ah_ss.time).substr(11, 8)
              << " (方位角 "
              << std::setw(6) << tm_ah_ss.ah << "°)"
              << std::endl;
    if (tm_ah_mr.ah < 0.0) {
      std::cout << "月の出 --:--:-- (方位角 ---.--°)" << std::endl;
    } else {
      std::cout << "月の出 " << ns::gen_time_str(tm_ah_mr.time).substr(11, 8)
                << " (方位角 "
                << std::setw(6) << tm_ah_mr.ah << "°)"
                << std::endl;
    }
    if (tm_ah_mm.ah < 0.0) {
      std::cout << "月南中 --:--:-- (　高度 ---.--°)" << std::endl;
    } else {
      std::cout << "月南中 " << ns::gen_time_str(tm_ah_mm.time).substr(11, 8)
                << " (　高度 "
                << std::setw(6) << tm_ah_mm.ah << "°)"
                << std::endl;
    }
    if (tm_ah_ms.ah < 0.0) {
      std::cout << "月の入 --:--:-- (方位角 ---.--°)" << std::endl;
    } else {
      std::cout << "月の入 " << ns::gen_time_str(tm_ah_ms.time).substr(11, 8)
                << " (方位角 "
                << std::setw(6) << tm_ah_ms.ah << "°)"
                << std::endl;
    }
  } catch (...) {
      std::cerr << "EXCEPTION!" << std::endl;
      return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

