#ifndef EveData_hpp
#define EveData_hpp 1

#include <CAENDigitizerType.h>

#include <vector>

// Using the vector, not the array pointer for.
// TDigiTes uses the array pointer style.
// CHeck the speed, if needed

// 1024 means the maximum number of events in one event buffer.
// In CAEN digitizer, it set 1023.
constexpr unsigned int gnMaxEvents = 1024;

class EveData
{  // no getter setter.  using public member variables.
 public:
  // EveData()
  // {
  //   ModNumber = 0;
  //   ChNumber = 0;
  //   TimeStamp = 0;
  //   FineTS = 0;
  //   ChargeShort = 0;
  //   ChargeLong = 0;
  //   fRecordLength = 0;
  //   Trace1.reserve(0);
  //   Trace2.reserve(0);
  //   DTrace1.reserve(0);
  //   DTrace2.reserve(0);
  //   DTrace3.reserve(0);
  //   DTrace4.reserve(0);
  // };

  EveData(uint32_t nSamples = 0)
  {
    ModNumber = 0;
    ChNumber = 0;
    TimeStamp = 0;
    FineTS = 0;
    SamplingPeriod = 1;
    ChargeShort = 0;
    ChargeLong = 0;
    fRecordLength = nSamples;

    // Reserve only.  Used trace should be resized
    // If memory space is not enough or need speed, think smarter way
    // Trace1.reserve(nSamples);
    // Trace2.reserve(nSamples);
    // DTrace1.reserve(nSamples);
    // DTrace2.reserve(nSamples);
    // DTrace3.reserve(nSamples);
    // DTrace4.reserve(nSamples);
  };

  ~EveData(){};

  bool WithWaveform() { return (fRecordLength > 0); };

  unsigned char ModNumber;
  unsigned char ChNumber;
  uint64_t TimeStamp;
  double FineTS;
  int SamplingPeriod;
  int16_t ChargeShort;
  int16_t ChargeLong;
  std::vector<uint16_t> Trace1;
  std::vector<uint16_t> Trace2;
  std::vector<uint8_t> DTrace1;
  std::vector<uint8_t> DTrace2;
  std::vector<uint8_t> DTrace3;
  std::vector<uint8_t> DTrace4;

 private:
  uint32_t fRecordLength;
};
typedef EveData EveData_t;

#endif
