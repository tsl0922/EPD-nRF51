#ifndef _LUNAR_H_
#define _LUNAR_H_
#include <stdint.h>
#include <string.h>

#define YEAR0 (1900)       /* The first year */
#define EPOCH_YR (1970)    /* EPOCH = Jan 1 1970 00:00:00 */
#define SEC_PER_DY (86400) // 一天的秒数
#define SEC_PER_HR (3600)  // 一小时的秒数

typedef struct devtm
{
    uint16_t tm_year;
    uint8_t tm_mon;
    uint8_t tm_mday;
    uint8_t tm_hour;
    uint8_t tm_min;
    uint8_t tm_sec;
    uint8_t tm_wday;
} tm_t;

struct Lunar_Date
{
    uint8_t IsLeap;
    uint8_t Date;
    uint8_t Month;
    uint16_t Year;
};

const char Lunar_MonthString[13][7] = {
    "未知",
    "正月", "二月", "三月", "四月", "五月", "六月", "七月", "八月", "九月", "十月",
    "冬月", "腊月"};

const char Lunar_MonthLeapString[2][4] = {
    " ",
    "闰"};

const char Lunar_DateString[31][7] = {
    "未知",
    "初一", "初二", "初三", "初四", "初五", "初六", "初七", "初八", "初九", "初十",
    "十一", "十二", "十三", "十四", "十五", "十六", "十七", "十八", "十九", "二十",
    "廿一", "廿二", "廿三", "廿四", "廿五", "廿六", "廿七", "廿八", "廿九", "三十"};

const char Lunar_DayString[7][4] = {
    "日",
    "一", "二", "三", "四", "五", "六"};

const char Lunar_ZodiacString[12][4] = {
    "猴", "鸡", "狗", "猪", "鼠", "牛", "虎", "兔", "龙", "蛇", "马", "羊"};

const char Lunar_StemStrig[10][4] = {
    "庚", "辛", "壬", "癸", "甲", "乙", "丙", "丁", "戊", "已"};

const char Lunar_BranchStrig[12][4] = {
    "申", "酉", "戌", "亥", "子", "丑", "寅", "卯", "辰", "巳", "午", "未"};

extern const char JieQiStr[24][7];

void LUNAR_SolarToLunar(struct Lunar_Date *lunar, uint16_t solar_year, uint8_t solar_month, uint8_t solar_date);
uint8_t LUNAR_GetZodiac(const struct Lunar_Date *lunar);
uint8_t LUNAR_GetStem(const struct Lunar_Date *lunar);
uint8_t LUNAR_GetBranch(const struct Lunar_Date *lunar);
uint8_t GetJieQiStr(uint16_t myear, uint8_t mmonth, uint8_t mday, uint8_t *day);
uint8_t GetJieQi(uint16_t myear, uint8_t mmonth, uint8_t mday, uint8_t *JQdate);

void transformTime(uint32_t unix_time, struct devtm *result);
uint32_t transformTimeStruct(struct devtm *result);
uint8_t get_first_day_week(uint16_t year, uint8_t month);
uint8_t get_last_day(uint16_t year, uint8_t month);
unsigned char day_of_week_get(unsigned char month, unsigned char day, unsigned short year);
uint8_t thisMonthMaxDays(uint8_t year, uint8_t month);

#endif
