#ifndef TDetector_hpp
#define TDetector_hpp 1

#include <memory>
#include <vector>

#include "DigiPar.hpp"
#include "EveData.hpp"
#include "TDigitizer.hpp"

class TDetector
{
 public:
  TDetector();
  ~TDetector();

  void LoadParameter(std::vector<Parameter_t> parVec) { fParVec = parVec; };

  bool CheckAndOpen();

  // For DAQ-Middleware components
  void Start();
  void Stop();
  void Pause();
  void Unconfig();
  void Config();

  void ReadEvents();
  std::vector<EveData_t *> *GetData();

  void SendSWTrigger(unsigned int nTrigger = 1);

 protected:
 private:
  std::vector<std::unique_ptr<TDigitizer>> fDigiVec;
  std::vector<EveData_t *> *fpDataVec;

  std::vector<Parameter_t> fParVec;
};

#endif
