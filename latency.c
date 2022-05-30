#include "latency.h"

#include <string.h>
#include <inttypes.h>

size_t ns_since(struct timespec* t0) {
  struct timespec t1;
  clock_gettime(CLOCK_REALTIME, &t1);
  long tmp = (t1.tv_sec - t0->tv_sec) * 1000000000l+ (t1.tv_nsec - t0->tv_nsec);
  return (size_t)tmp;
}

void ResetLat(struct Latency* lat) { memset(lat, 0, sizeof(struct Latency)); }

  /// Add a latency sample
void AddLat(struct Latency* lat,size_t us) {
    if (us < 128)
        lat->bin0_[us]++;
    else if (us < 384)
        lat->bin1_[(us - 128) / 2]++;
    else if (us < 896)
        lat->bin2_[(us - 384) / 4]++;
    else if (us < 1920)
        lat->bin3_[(us - 896) / 8]++;
    else if (us < 3968)
        lat->bin4_[(us - 1920) / 16]++;
    else
        lat->bin5_++;
  }

  /// Combine two distributions
void CombLat(struct Latency* lat ,const struct Latency* o){
    size_t i;
    for (i = 0; i < 128; i++) lat->bin0_[i] += o->bin0_[i];
    for (i = 0; i < 128; i++) lat->bin1_[i] += o->bin1_[i];
    for (i = 0; i < 128; i++) lat->bin2_[i] += o->bin2_[i];
    for (i = 0; i < 128; i++) lat->bin3_[i] += o->bin3_[i];
    for (i = 0; i < 128; i++) lat->bin4_[i] += o->bin4_[i];
    lat->bin5_ += o->bin5_;
}

/// Return the total number of samples
size_t CountLat(struct Latency* lat){
    size_t count = 0;
    size_t i;
    for (i = 0; i < 128; i++) count += lat->bin0_[i];
    for (i = 0; i < 128; i++) count += lat->bin1_[i];
    for (i = 0; i < 128; i++) count += lat->bin2_[i];
    for (i = 0; i < 128; i++) count += lat->bin3_[i];
    for (i = 0; i < 128; i++) count += lat->bin4_[i];
    count += lat->bin5_;
    return count;
}

  /// Return the (approximate) sum of all samples
size_t SumLat(struct Latency* lat){
    size_t sum = 0;
    size_t i;
    for (i = 0; i < 128; i++) sum += lat->bin0_[i] * (0 + i * 1);
    for (i = 0; i < 128; i++) sum += lat->bin1_[i] * (128 + i * 2);
    for (i = 0; i < 128; i++) sum += lat->bin2_[i] * (384 + i * 4);
    for (i = 0; i < 128; i++) sum += lat->bin3_[i] * (896 + i * 8);
    for (i = 0; i < 128; i++) sum += lat->bin4_[i] * (1920 + i * 16);
    sum += lat->bin5_ * 3968;
    return sum;
}

  /// Return the (approximate) average sample
double AvgLat(struct Latency* lat)  {
    double sum = 1.0*SumLat(lat);
    size_t tmp = CountLat(lat);
    double cnt = (tmp==0)?1.0:(1.0*tmp);
    return sum/cnt;
}

  /// Return the (approximate) minimum sample
size_t MinLat(struct Latency* lat){
    size_t i;
    for (i = 0; i < 128; i++)
      if (lat->bin0_[i] != 0) return 0 + i * 1;
    for (i = 0; i < 128; i++)
      if (lat->bin1_[i] != 0) return 128 + i * 2;
    for (i = 0; i < 128; i++)
      if (lat->bin2_[i] != 0) return 384 + i * 4;
    for (i = 0; i < 128; i++)
      if (lat->bin3_[i] != 0) return 896 + i * 8;
    for (i = 0; i < 128; i++)
      if (lat->bin4_[i] != 0) return 1920 + i * 16;
    return 3968;
}

  /// Return the (approximate) max sample
size_t MaxLat(struct Latency* lat) {
    int64_t i;
    if (lat->bin5_ != 0) return 3968;
    for (i = 127; i >= 0; i--)
      if (lat->bin4_[i] != 0) return 1920 + ((size_t)i) * 16;
    for (i = 127; i >= 0; i--)
      if (lat->bin3_[i] != 0) return 896 + ((size_t)i) * 8;
    for (i = 127; i >= 0; i--)
      if (lat->bin2_[i] != 0) return 384 + ((size_t)i) * 4;
    for (i = 127; i >= 0; i--)
      if (lat->bin1_[i] != 0) return 128 + ((size_t)i) * 2;
    for (i = 127; i >= 0; i--)
      if (lat->bin0_[i] != 0) return 0 + ((size_t)i) * 1;
    return 0;
  }

  /// Return the (approximate) p-th percentile sample
size_t PercLat(struct Latency* lat,double p)  {
    size_t i;
    size_t thres = (size_t)(p * ((double)CountLat(lat)));
    for (i = 0; i < 128; i++)
      if(thres < lat->bin0_[i]) return 0 + i * 1;
    for (i = 0; i < 128; i++)
      if(thres < lat->bin1_[i]) return 128 + i * 2;
    for (i = 0; i < 128; i++)
      if(thres < lat->bin2_[i]) return 384 + i * 4;
    for (i = 0; i < 128; i++)
      if(thres < lat->bin3_[i]) return 896 + i * 8;
    for (i = 0; i < 128; i++)
      if(thres < lat->bin4_[i]) return 1920 + i * 16;
    return 3968;
}

  /// Print the distribution to a file
void print(FILE* fp,struct Latency* lat){
    size_t i;
    for (i = 0; i < 128; i++)
      if (lat->bin0_[i] != 0)
        fprintf(fp, "%4" PRIu64 " %6" PRIu64 "\n", 0 + i * 1, lat->bin0_[i]);
    for (i = 0; i < 128; i++)
      if (lat->bin1_[i] != 0)
        fprintf(fp, "%4" PRIu64 " %6" PRIu64 "\n", 128 + i * 2, lat->bin1_[i]);
    for (i = 0; i < 128; i++)
      if (lat->bin2_[i] != 0)
        fprintf(fp, "%4" PRIu64 " %6" PRIu64 "\n", 384 + i * 4, lat->bin2_[i]);
    for (i = 0; i < 128; i++)
      if (lat->bin3_[i] != 0)
        fprintf(fp, "%4" PRIu64 " %6" PRIu64 "\n", 896 + i * 8, lat->bin3_[i]);
    for (i = 0; i < 128; i++)
      if (lat->bin4_[i] != 0)
        fprintf(fp, "%4" PRIu64 " %6" PRIu64 "\n", 1920 + i * 16, lat->bin4_[i]);
    if (lat->bin5_ != 0) fprintf(fp, "%4d %6" PRIu64 "\n", 3968, lat->bin5_);
}

