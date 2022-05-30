/**
 * @file latency.h
 * @author MICA authors, akalia
 */

#pragma once
#include <stdio.h>
#include <time.h>

/*
 * @brief Fast but approximate latency distribution measurement for latency
 * values up to 4000 microseconds (i.e., 4 ms). Adding a latency sample is
 * fast, but computing a statistic is slow.
 */
struct Latency {
  // [0, 128) us
  size_t bin0_[128];
  // [128, 384) us
  size_t bin1_[128];
  // [384, 896) us
  size_t bin2_[128];
  // [896, 1920) us
  size_t bin3_[128];
  // [1920, 3968) us
  size_t bin4_[128];
  // [3968, inf) us
  size_t bin5_;
};


void ResetLat(struct Latency* lat);
void AddLat(struct Latency* lat,size_t us) ;
void CombLat(struct Latency* lat ,const struct Latency* o) ;
size_t CountLat(struct Latency* lat);
size_t SumLat(struct Latency* lat);
double AvgLat(struct Latency* lat)  ;
size_t MinLat(struct Latency* lat);
size_t MaxLat(struct Latency* lat) ;
size_t PercLat(struct Latency* lat,double p); 
void print(FILE* fp,struct Latency* lat);

size_t ns_since(struct timespec* t0);