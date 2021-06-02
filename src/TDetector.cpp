#include "TDetector.hpp"

#include <CAENDigitizer.h>

#include <algorithm>
#include <iostream>

#include "TWaveform.hpp"

TDetector::TDetector() {}

TDetector::~TDetector() {}

bool TDetector::CheckAndOpen()
{
  CAEN_DGTZ_ErrorCode ret = CAEN_DGTZ_Success;
  // for (auto &&par : fLinkParVec) {
  const int nDigi = fParVec.size();
  for (auto i = 0; i < nDigi; i++) {
    int handler = -1;
    auto linkPar = fParVec[i].Link;
    ret = CAEN_DGTZ_OpenDigitizer(linkPar.LinkType, linkPar.LinkNum,
                                  linkPar.ConetNode, linkPar.VMEBaseAddress,
                                  &handler);
    if (ret < 0) {
      std::cerr << "CAEN_DGTZ_OpenDigitizer failed with code: " << ret
                << std::endl;
      return false;
    }

    CAEN_DGTZ_BoardInfo_t info;
    ret = CAEN_DGTZ_GetInfo(handler, &info);
    if (ret < 0) {
      std::cerr << "CAEN_DGTZ_GetInfo failed" << std::endl;
      return false;
    }

    uint32_t majorNumber = atoi(info.AMC_FirmwareRel);
    auto fw = FWCode::Others;
    if (majorNumber == 128) {
      fw = FWCode::DPP_PHA;  // It will be never used at ELI?
    } else if (majorNumber == 130) {
      fw = FWCode::Others;
    } else if (majorNumber == 131) {
      fw = FWCode::DPP_PSD;
    } else if (majorNumber == 132) {
      fw = FWCode::DPP_PSD;
    } else if (majorNumber == 136) {
      fw = FWCode::DPP_PSD;  // NOTE: valid also for x725
    } else if (majorNumber == 139) {
      fw = FWCode::DPP_PHA;  // NOTE: valid also for x725
    } else {
      fw = FWCode::STD;
    }

    // std::cout << "Firmware is " << int(fw) << std::endl;
    std::unique_ptr<TDigitizer> digi;
    if (fw == FWCode::STD)
      digi.reset(new TWaveform());
    else if (fw == FWCode::DPP_PSD)
      digi.reset(new TWaveform());  // PSD is NYI.  Soon replaced
    else {
      std::cout << "The FW of this board is not supported.  Or program bug.\t"
                << majorNumber << "\t" << int(fw) << std::endl;
      return false;
    }

    digi->SetHandler(handler);
    digi->SetLinkPar(linkPar);

    auto digiPar = fParVec[i].Digitizer;
    digiPar.BrdID = i;
    digiPar.NumBrd = nDigi;
    digi->SetDigiPar(digiPar);
    digi->GetDigiInfo();
    fDigiVec.push_back(std::move(digi));
  }

  fpDataVec = new std::vector<EveData_t *>;
  fpDataVec->reserve(gnMaxEvents * fDigiVec.size());

  return true;
}

void TDetector::Start()
{
  for (auto &&digi : fDigiVec) digi->Start();
}

void TDetector::Stop()
{
  for (auto &&digi : fDigiVec) digi->Stop();
}

void TDetector::Pause() {}

void TDetector::Unconfig() {}

void TDetector::Config()
{
  // Checking and loading config file is needed.

  for (auto &&digi : fDigiVec) digi->Config();
}

void TDetector::ReadEvents()
{
  for (auto &&digi : fDigiVec) digi->ReadEvents();
}

std::vector<EveData_t *> *TDetector::GetData()
{
  // Deleting elements is done by digi.  Not this
  fpDataVec->resize(0);
  for (auto &&digi : fDigiVec) {
    auto data = digi->GetData();
    std::copy(data->begin(), data->end(), std::back_inserter(*fpDataVec));
  }
  return fpDataVec;
}

void TDetector::SendSWTrigger(unsigned int nTrigger)
{
  for (auto &&digi : fDigiVec) digi->SendSWTrigger(nTrigger);
}
