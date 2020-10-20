#ifndef TWaveform_hpp
#define TWaveform_hpp 1

#include "TDigitizer.hpp"

class TWaveform : public TDigitizer
{
 public:
  TWaveform();
  TWaveform(int h);
  ~TWaveform();

  void Config();

  void AllocateMemory();
  void FreeMemory();
  void ReadEvents();

 private:
  void InitWaveform();
  void SetChPar();

  // Memory
  char *fpReadoutBuffer;                 // readout buffer
  CAEN_DGTZ_UINT16_EVENT_t *fpEventStd;  // events buffer
};

#endif
