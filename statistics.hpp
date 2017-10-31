/*
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, You can
 * obtain one at http://mozilla.org/MPL/2.0/.
 * 
 * 
 * Copyright (c) 2016, Lutz, Clemens <lutzcle@cml.li>
 */

#ifndef STATISTICS_HPP
#define STATISTICS_HPP

#include <cstdint>
#include <cfloat>
#include <algorithm>

namespace Statistics {

// Use Welford's variance algorithm
// http://jonisalonen.com/2013/deriving-welfords-method-for-computing-variance/
class Statistics
{
public:
  Statistics() :
    count_(0), mean_(0.0), sum_(0.0), min_(DBL_MAX), max_(DBL_MIN)
  {}

  template <typename T>
  void update(T value) {
    double dbl_value = (double) value;
    double delta = 0.0;

    ++count_;
    delta = dbl_value - mean_;
    mean_ += delta / ((double)count_);
    sum_ += delta * (dbl_value - mean_); 

    min_ = std::min(min_, dbl_value);
    max_ = std::max(max_, dbl_value);
  }

  double mean() {
    return mean_;
  }

  double variance() {
    return sum_ / ((double)count_);
  }

  double min() {
    return min_;
  }

  double max() {
    return max_;
  }

  void reset() {
    count_ = 0;
    mean_ = 0.0;
    sum_ = 0.0;
  }

private:
  uint64_t count_;
  double mean_;
  double sum_;
  double min_;
  double max_;
};

}

#endif // STATISTICS_HPP

