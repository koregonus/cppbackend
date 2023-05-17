#include "my_logger.h"
#include <memory>

void Logger::SetTimestamp(std::chrono::system_clock::time_point ts)
{
	std::lock_guard<std::mutex> lock(m_);
	manual_ts_ = ts;
}

void Logger::CheckTimestamp()
{
    const auto now = GetTime();
    auto dp = floor<std::chrono::days>(now);
    std::chrono::year_month_day ymd{dp};

    auto y = ymd.year();
    auto m = ymd.month();
    auto d = ymd.day();
    if(y != prev_year_ && m != prev_month_ && d != prev_day_)
    {
        const auto t_c = std::chrono::system_clock::to_time_t(now);
        std::ostringstream oss;
        oss << "/var/logs/" << std::put_time(std::localtime(&t_c), "%Y_%m_%d") << ".log";
        auto str = oss.str();
        log_file_.close();
        log_file_.open(str, std::ios::app);
    }
}