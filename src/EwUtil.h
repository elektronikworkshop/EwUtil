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

#define PdS(NAME, MS) Periodical NAME(3000,[]()
#define PdE(NAME) ); NAME();

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

/* A more C-ish implementation */

#define _P_CONCAT(A, B) A ## B

#define PS(NAME, MS) \
  static unsigned long _P_CONCAT(NAME, _prev) = 0; \
  if (millis() - _P_CONCAT(NAME, _prev) > MS) { \
    _P_CONCAT(NAME, _prev) = millis();
#define PE() }

/** Placing the print functions into a separate namespace.
 * This way anyone can decide to swap it in with
 *
 *   using namespace ewprt;
 *
 * or by accessing them via namespace (and not having the
 * advantage of the stream operator)
 *
 */
namespace ewprt {

template<class T> inline Print &operator <<(Print &obj, T arg) { obj.print(arg); return obj; }

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

} // namespace ewprt

#endif /* EWUTIL_H_ */
