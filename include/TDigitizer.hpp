#ifndef TDigitizer_hpp
#define TDigitizer_hpp 1

#include <string>
#include <vector>

#include "DigiPar.hpp"
#include "EveData.hpp"

class TDigitizer
{
 public:
  TDigitizer();
  TDigitizer(int h);
  virtual ~TDigitizer();

  void SetHandler(int h) { fHandler = h; };
  int GetHandler() { return fHandler; };

  void SetDigiPar(DigiPar par) { fDigiPar = par; };
  DigiPar GetDigiPar() { return fDigiPar; };

  void SetLinkPar(LinkPar par) { fLinkPar = par; };
  LinkPar GetLinkPar() { return fLinkPar; };

  void GetDigiInfo();

  void OpenDigitizer();
  void CloseDigitizer();

  void SetStartMode();
  void SetTrgInMode();
  void SetTrgOutMode();
  void SetHWCoincidence();

  // Start, Stop, Pause is only for stand alone digitizers.
  // In the case of synchronization, making and using a
  // controller class like a TDetector.
  void Start();
  void Stop();
  void Pause();
  void Unconfig();
  virtual void Config() = 0;

  virtual void AllocateMemory() = 0;
  virtual void FreeMemory() = 0;
  virtual void ReadEvents() = 0;
  std::vector<EveData_t *> *GetData() { return fpDataVec; }

  void SendSWTrigger(unsigned int nTrigger = 1);

 protected:
  virtual void Init();
  LinkPar fLinkPar;
  int fHandler;
  DigiPar fDigiPar;
  int fDigiModel;
  int fNCh;
  int fTimeSample;
  int fADCRes;
  FWCode fFW;
  int fFWrev;
  int fSTU;  // Time step for some parameters

  // time stamp for STD and others (if using)
  uint64_t fTimeOffset;
  uint64_t fPreviousTime;

  std::vector<EveData_t *> *fpDataVec;
  void ClearDataVec();

  bool fTraceStep;

  void PrintError(const CAEN_DGTZ_ErrorCode &err,
                  const std::string &funcName) const;

  CAEN_DGTZ_ErrorCode RegisterSetBits(uint16_t addr, int start_bit, int end_bit,
                                      int val);

  bool IsDPP();

  char *fpReadoutBuffer;  // readout buffer

 private:
  bool fPauseFlag;
};

#endif
