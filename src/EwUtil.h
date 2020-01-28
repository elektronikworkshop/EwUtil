/* A humble utility library for daily recurring code patterns on Arduino
 *
 * EwUtil.h
 *
 *  Created on: May 19, 2019
 *      Author: uli
 */

#ifndef EWUTIL_H_
#define EWUTIL_H_

#include <Arduino.h>
#include <functional>

/* C++ 11 implementation */

class Periodical
{
public:
  typedef std::function<void(void)> Func;
  Periodical(unsigned long ms, Func &&func)
    : m_ms(ms)
    , m_prev(0)
    , m_func(func)
  {}
  void operator()()
  {
    run();
  }
  void run()
  {
    if (m_func) {
      auto now = millis();
      if (now - m_prev > m_ms) {
        m_prev = now;
        m_func();
      }
    }
  }
private:
  unsigned long m_ms, m_prev;
  Func m_func;
};

/* Using CRTP to create derived classes and get rid of the function
 * object overhead.
 *
 * Use with your derived class like this:
 *
 * @code{.cpp}
    class MyPeriodicalTask
      : public PeriodicalBase<MyPeriodicalTask>
    {
    public:
      const unsigned long MyPeriodMs = 1000;
      MyPeriodicalTask()
        : PeriodicalBase<MyPeriodicalTask>(MyPeriodMs)
      {}
      void task()
      {
        // here comes your code
      }
    };
   @endcode
 */
template <class T>
class PeriodicalBase
{
public:
  PeriodicalBase(unsigned long period_ms)
    : m_period_ms(period_ms)
    , m_prev(0)
  {}
  void operator()()
  {
    static_cast<T*>(this)->run();
  }
  /** Run function to be called in main loop. Can be overridden.
   * Your task must be a function in your derived class with
   * the signature void(void).
   */
  void run()
  {
    auto now = millis();
    if (now - m_prev > m_period_ms) {
      m_prev = now;
      static_cast<T*>(this)->task();
    }
  }
  void
  setPeriodMs(unsigned long period_ms)
  {
    m_period_ms = period_ms;
  }
  void
  setPeriodS(unsigned long period_s)
  {
    setPeriodMs(period_s * 1000UL);
  }
  unsigned long
  getPeriodMs() const
  {
    return m_period_ms;
  }
  unsigned long
  getPeriodS() const
  {
    return m_period_ms / 1000UL;
  }
private:
  unsigned long m_period_ms;
  unsigned long m_prev;
};


// TODO: implement a CRTP version of this
// TODO: implement a callback version of this
class Timer
{
public:
    typedef unsigned long ms_t;
    typedef enum {
        OneShot,
        Periodic,
    } Mode;

    // todo: perhaps slave and master require different settings (e.g. timeout)
    Timer(const ms_t & timeout = 0,
          const Mode & mode = OneShot)
        : m_mode(mode)
        , m_running(false)
        , m_timeoutMs(timeout)
        , m_timerLastMs(0)
    { }
    bool expired()
    {
        if (m_running and m_timeoutMs) {
            auto now = millis();
            if (now - m_timerLastMs > m_timeoutMs) {
                if (m_mode == OneShot) {
                    m_running = false;
                } else {
                    m_timerLastMs = now;
                }
                return true;
            }
        }
        return false;
    }
    void start(void)
    {
      start(m_timeoutMs);
    }
    void start(ms_t timeoutMs)
    {
        m_timerLastMs = millis();
        m_timeoutMs = timeoutMs;
        m_running = true;
    }
    void setTimeout(ms_t timeoutMs)
    {
        m_timeoutMs = timeoutMs;
    }
    ms_t getTimeout(void) const
    {
        return m_timeoutMs;
    }
    void setMode(Mode mode)
    {
      m_mode = mode;
    }
    void stop(void)
    {
        m_running = false;
    }

    bool running(void) const
    {
        return m_running;
    }

private:
    Mode m_mode;
    bool m_running;
    ms_t m_timeoutMs;
    ms_t m_timerLastMs;
};

/** Placing the print functions into a separate namespace.
 * This way anyone can decide to swap it in with
 *
 *   using namespace ewprt;
 *
 * or by accessing them via namespace (and not having the
 * advantage of the stream operator)
 *
 */
namespace ew {

template<class T> inline Print &operator <<(Print &obj, T arg) { obj.print(arg); return obj; }

const char*
prtFmt(char* buf, size_t buflen, const char *fmt, ... )
  __attribute__ ((format (printf, 3, 4)));

inline const char*
prtFmt(char* buf, size_t buflen, const char *fmt, ... )
{
  va_list args;
  va_start (args, fmt );
  vsnprintf(buf, buflen, fmt, args);
  va_end (args);
  return buf;
}

template <size_t BufSize=64>
Print&
prtFmt(Print& prt, const char *fmt, ...)
  __attribute__ ((format (printf, 2, 3)));

template <size_t BufSize>
inline Print&
prtFmt(Print& prt, const char *fmt, ...)
{
  char buf[BufSize]{0};

  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, BufSize, fmt, args);
  va_end(args);

  prt.print(buf);
  return prt;
}

template <size_t BufSize=64>
inline String&
prtFmt(String& str, const char *fmt, ...)
  __attribute__ ((format (printf, 2, 3)));

template <size_t BufSize>
inline String&
prtFmt(String& str, const char *fmt, ...)
{
  char buf[BufSize]{0};

  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, BufSize, fmt, args);
  va_end(args);

  str = buf;
  return str;
}


const unsigned long SECS_PER_MIN  (60UL);
const unsigned long SECS_PER_HOUR (3600UL);
const unsigned long SECS_PER_DAY  (SECS_PER_HOUR * 24UL);


inline unsigned long
numberOfSeconds(unsigned long seconds)
{
  return seconds % SECS_PER_MIN;
}

inline unsigned long
numberOfMinutes(unsigned long seconds)
{
  return (seconds / SECS_PER_MIN) % SECS_PER_MIN;

}

inline unsigned long
numberOfHours(unsigned long seconds)
{
  return (seconds% SECS_PER_DAY) / SECS_PER_HOUR;

}

inline unsigned long
numberOfDays(unsigned long seconds)
{
  return seconds / SECS_PER_DAY;
}

inline String &
fmtElapsed(String &str,
           unsigned long seconds,
           bool all = false,
           const char *fmts = "%lus",
           const char *fmtm = "%lum %02lus",
           const char *fmth = "%luh %02lum %02lus",
           const char *fmtd = "%lud %02luh %02lum %02lus")
{
  auto d = numberOfDays(seconds);
  auto h = numberOfHours(seconds);
  auto m = numberOfMinutes(seconds);
  auto s = numberOfSeconds(seconds);

  // note: in case some sort of flagging is
  // implemented:
  // 
  //  when printing only up to h we have to 
  //  multiply the d with number of h per d
  //  ... and so on for "up to m"

  if (d or all) {
    return prtFmt(str, fmtd, d, h, m, s);
  } else if (h) {
    return prtFmt(str, fmth, h, m, s);
  } else if (m) {
    return prtFmt(str, fmtm, m, s);
  } else {
    return prtFmt(str, fmts, s);
  }
}

} // namespace ewprt

#endif /* EWUTIL_H_ */
