#ifndef SUN_MOON_CALC_HPP_
#define SUN_MOON_CALC_HPP_

#include "time.hpp"

#include <ctime>
#include <iomanip>
#include <iostream>
//#include <string>

namespace sun_moon {

// 時刻、方位角／高度
struct TmAh {
  struct timespec time;
  double           ah;
};
// 座標
struct Coord {
  double lat;
  double lng;
};

class Calc {
  struct timespec jst;  // JST
  double  lat_o;        // 観測者: 緯度
  double  lng_o;        // 観測者: 経度
  double  ht_o;         // 観測者: 標高
  double  dlt_t_d;      // ΔTの日換算値
  double dip;           // 地平線伏角
  double day_p;         // 2000年1月1日力学時正午からの経過日数(日)

public:
  Calc(struct timespec, double, double, double);  // コンストラクタ
  struct TmAh calc_sun(unsigned int);             // 計算（日の出／入）
  struct TmAh calc_moon(unsigned int);            // 計算（月の出／入）
private:
  double calc_day_progress();           // 計算: 2000年1月1日力学時正午からの経過日数
  double calc_time_sun(unsigned int);   // 計算: 日の出・入・南中時刻
  double calc_lmd_sun(double);          // 計算: 太陽の黄経
  double calc_dist_sun(double);         // 計算: 太陽の距離
  double calc_time_moon(unsigned int);  // 計算: 日の出・入・南中時刻
  double calc_bet_moon(double);         // 計算: 月の黄緯
  double calc_lmd_moon(double);         // 計算: 月の黄経
  double calc_dist_moon(double);        // 計算: 月の距離
  double norm_ang(double);              // 計算: 角度の正規化
  double tm_sidereal(double, double);   // 計算: 観測地点の恒星時Θ(度)
  double hour_ang_diff(Coord, double, double, unsigned int);
                                        // 計算: 出入点の時角と天体の時角差
  double calc_angle(Coord, double, double);
                                        // 計算: 時刻(t)における黄経、黄緯の天体の方位角
  double calc_height(Coord, double, double);
                                        // 計算: 時刻(t)における黄経、黄緯の天体の高度
  double calc_diff_moon(double);        // 計算: 月の視差
  struct Coord ko2se(double, Coord);    // 変換: 黄道座標 -> 赤道座標
};

}  // namespace sun_moon

#endif

