/*
 * time.h
 * 
 * Struct declaration for dealing with time.
 */

#ifndef _TIME_H_
#define _TIME_H_

struct tm
{
  int	tm_sec;
  int	tm_min;
  int	tm_hour;
  int	tm_mday;
  int	tm_mon;
  int	tm_year;
  int	tm_wday;
  int	tm_yday;
  int	tm_isdst;
};

#endif /* _TIME_H_ */

