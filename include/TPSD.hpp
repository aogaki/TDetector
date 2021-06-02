#ifndef TPSD_hpp
#define TPSD_hpp 1

#include "TDigitizer.hpp"

class TPSD : public TDigitizer
{
 public:
  TPSD();
  TPSD(int h);
  ~TPSD();

  void Config();

  void AllocateMemory();
  void FreeMemory();
  void ReadEvents();

 private:
  void InitPSD();
  void SetChPar();

  // Memory
  CAEN_DGTZ_DPP_PSD_Event_t **fppPSDEvents;      // events buffer
  CAEN_DGTZ_DPP_PSD_Waveforms_t *fpPSDWaveform;  // waveforms buffer
  CAEN_DGTZ_UINT16_EVENT_t *fpEventStd;          // events buffer
};

#endif
