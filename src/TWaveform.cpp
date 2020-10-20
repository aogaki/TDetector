#include <CAENDigitizer.h>

#include <algorithm>
#include <bitset>
#include <cstring>
#include <iostream>

#include "TWaveform.hpp"

TWaveform::TWaveform() : TDigitizer() { InitWaveform(); }
TWaveform::TWaveform(int h) : TDigitizer(h) { InitWaveform(); }

TWaveform::~TWaveform() {}

void TWaveform::InitWaveform()
{
  // These pointers are handled by CAEN libraries.
  // Do not need to do new/delete for these.
  fpReadoutBuffer = nullptr;
  fpEventStd = nullptr;

  fTimeOffset = 0;
  fPreviousTime = 0;
}

void TWaveform::Config()
{
  // Think: When and how to load the parameters
  fTraceStep = true;
  if (fTraceStep) std::cout << "\nBoard configuration start" << std::endl;

  CAEN_DGTZ_ErrorCode ret = CAEN_DGTZ_Success;

  if (fTraceStep) std::cout << "CAEN_DGTZ_Reset" << std::endl;
  ret = CAEN_DGTZ_Reset(fHandler);
  PrintError(ret, "CAEN_DGTZ_Reset");

  if (fTraceStep) std::cout << "CAEN_DGTZ_SetChannelEnableMask" << std::endl;
  uint32_t digiMask = (1 << fNCh) - 1;
  fDigiPar.EnableMask = digiMask & fDigiPar.EnableMask;
  std::cout << "EnableMask:\t" << std::bitset<32>(fDigiPar.EnableMask)
            << std::endl;
  ret = CAEN_DGTZ_SetChannelEnableMask(fHandler, fDigiPar.EnableMask);
  fDigiPar.TrgoutMask &= fDigiPar.EnableMask;
  PrintError(ret, "CAEN_DGTZ_SetChannelEnableMask");

  if (fTraceStep) std::cout << "Fan speed" << std::endl;
  // ret = CAEN_DGTZ_WriteRegister(fHandler, 0x8168, 0b111000); // high
  ret = CAEN_DGTZ_WriteRegister(fHandler, 0x8168, 0b110000);  // low or auto
  PrintError(ret, "Fan speed");

  if (fTraceStep) std::cout << "CAEN_DGTZ_SetRecordLength" << std::endl;
  ret = CAEN_DGTZ_SetRecordLength(fHandler, fDigiPar.RecordLength, -1);
  PrintError(ret, "CAEN_DGTZ_SetRecordLength");

  if (fTraceStep) std::cout << "CAEN_DGTZ_SetIOLevel" << std::endl;
  ret = CAEN_DGTZ_SetIOLevel(fHandler, fDigiPar.IOLevel);
  PrintError(ret, "CAEN_DGTZ_SetIOLevel");

  if (fTraceStep) std::cout << "Set the start mode" << std::endl;
  SetStartMode();

  if (fTraceStep) std::cout << "Set the trgin mode" << std::endl;
  SetTrgInMode();

  if (fTraceStep) std::cout << "Set the trgout mode" << std::endl;
  SetTrgOutMode();

  if (fTraceStep) std::cout << "Set Aggregation Mode" << std::endl;
  if (IsDPP()) {
    ret =
        CAEN_DGTZ_SetDPPEventAggregation(fHandler, fDigiPar.EventBuffering, 0);
    PrintError(ret, "CAEN_DGTZ_SetDPPEventAggregation");
    // Number of buffers per BLT
    ret = CAEN_DGTZ_SetMaxNumAggregatesBLT(fHandler, 1023);
    PrintError(ret, "CAEN_DGTZ_SetMaxNumAggregatesBLT");
  } else {
    ret = CAEN_DGTZ_SetMaxNumEventsBLT(fHandler, 1023);
    PrintError(ret, "CAEN_DGTZ_SetMaxNumEventsBLT");
  }

  if (fTraceStep) std::cout << "Set HW coincidence mode" << std::endl;
  SetHWCoincidence();

  if (fTraceStep) std::cout << "Set Channel Parameters" << std::endl;
  SetChPar();
}

void TWaveform::SetChPar()
{
  CAEN_DGTZ_ErrorCode ret = CAEN_DGTZ_Success;
  for (auto iCh = 0; iCh < fNCh; iCh++) {
    if (fTraceStep && iCh == 0)
      std::cout << "CAEN_DGTZ_SetChannelDCOffset and Polarity" << std::endl;
    if (fDigiPar.BaselineDCoffset[iCh] >= 0 &&
        fDigiPar.BaselineDCoffset[iCh] <= 100) {
      uint16_t offs;
      if (fDigiPar.PulsePolarity[iCh] == CAEN_DGTZ_PulsePolarityNegative)
        offs = (uint16_t)((fDigiPar.BaselineDCoffset[iCh] * 65535) / 100);
      else  // positive
        offs =
            (uint16_t)(((100 - fDigiPar.BaselineDCoffset[iCh]) * 65535) / 100);
      ret = CAEN_DGTZ_SetChannelDCOffset(fHandler, iCh, offs);
    } else {
      // ret = CAEN_DGTZ_SetChannelDCOffset(fHandler, iCh, fDigiPar.DCoffset[iCh]);
      std::cerr << "BaselineDCoffset is out of range (0 to 100): "
                << fDigiPar.BaselineDCoffset[iCh] << std::endl;
      exit(0);
    }

    if (fTraceStep && iCh == 0)
      std::cout << "CAEN_DGTZ_SetPostTriggerSize" << std::endl;
    // expressed in percent of the record length
    uint32_t postTrgSize = 100 * (fDigiPar.RecordLength - fDigiPar.PreTrigger) /
                           fDigiPar.RecordLength;
    ret = CAEN_DGTZ_SetPostTriggerSize(fHandler, postTrgSize);

    if (fTraceStep && iCh == 0)
      std::cout << "CAEN_DGTZ_SetChannelSelfTrigger" << std::endl;
    ret = CAEN_DGTZ_SetChannelSelfTrigger(
        fHandler, CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT, (1 << iCh));

    if (fTraceStep && iCh == 0)
      std::cout << "CAEN_DGTZ_SetChannelTriggerThreshold" << std::endl;
    //Think ZeroVoltLevel.  It should be calculated by DCOffset IMO.
    if (fDigiPar.PulsePolarity[iCh] == CAEN_DGTZ_PulsePolarityPositive) {
      ret = CAEN_DGTZ_SetChannelTriggerThreshold(
          fHandler, iCh,
          fDigiPar.ZeroVoltLevel[iCh] + fDigiPar.TrgThreshold[iCh]);
      ret = CAEN_DGTZ_SetTriggerPolarity(fHandler, iCh,
                                         CAEN_DGTZ_TriggerOnRisingEdge);
    } else {
      ret = CAEN_DGTZ_SetChannelTriggerThreshold(
          fHandler, iCh,
          fDigiPar.ZeroVoltLevel[iCh] - fDigiPar.TrgThreshold[iCh]);
      ret = CAEN_DGTZ_SetTriggerPolarity(fHandler, iCh,
                                         CAEN_DGTZ_TriggerOnFallingEdge);
    }
  }
}

void TWaveform::AllocateMemory()
{
  CAEN_DGTZ_ErrorCode err;
  uint32_t size;

  // HACK: workaround to prevent memory allocation bug in the library:
  // allocate for all channels
  uint32_t EnableMask;
  CAEN_DGTZ_GetChannelEnableMask(fHandler, &EnableMask);
  CAEN_DGTZ_WriteRegister(fHandler, 0x8120, 0xFFFF);
  err = CAEN_DGTZ_MallocReadoutBuffer(fHandler, &(fpReadoutBuffer), &size);
  PrintError(err, "MallocReadoutBuffer");
  CAEN_DGTZ_WriteRegister(fHandler, 0x8120, EnableMask);

  err = CAEN_DGTZ_AllocateEvent(fHandler, (void **)&fpEventStd);
  PrintError(err, "AllocateEvent");
}

void TWaveform::FreeMemory()
{
  CAEN_DGTZ_ErrorCode err;

  if (fpReadoutBuffer != nullptr) {
    err = CAEN_DGTZ_FreeReadoutBuffer(&(fpReadoutBuffer));
    fpReadoutBuffer = nullptr;
    PrintError(err, "FreeReadoutBuffer");
    err = CAEN_DGTZ_FreeEvent(fHandler, (void **)&fpEventStd);
    PrintError(err, "FreeEvent");
  }
}

void TWaveform::ReadEvents()
{
  // Delete all past events
  ClearDataVec();

  // Readout from digitizer
  CAEN_DGTZ_EventInfo_t eventInfo;
  char *pEventPtr;

  CAEN_DGTZ_ErrorCode err;
  uint32_t bufferSize;
  err = CAEN_DGTZ_ReadData(fHandler, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT,
                           fpReadoutBuffer, &bufferSize);
  PrintError(err, "ReadData");

  uint32_t nEvents;
  err = CAEN_DGTZ_GetNumEvents(fHandler, fpReadoutBuffer, bufferSize, &nEvents);
  PrintError(err, "GetNumEvents");

  for (auto iEve = 0; iEve < nEvents; iEve++) {
    err = CAEN_DGTZ_GetEventInfo(fHandler, fpReadoutBuffer, bufferSize, iEve,
                                 &eventInfo, &pEventPtr);
    PrintError(err, "GetEventInfo");
    // std::cout << "Event number:\t" << iEve << '\n'
    //           << "Event size:\t" << eventInfo.EventSize << '\n'
    //           << "Board ID:\t" << eventInfo.BoardId << '\n'
    //           << "Pattern:\t" << eventInfo.Pattern << '\n'
    //           << "Ch mask:\t" << eventInfo.ChannelMask << '\n'
    //           << "Event counter:\t" << eventInfo.EventCounter << '\n'
    //           << "Trigger time tag:\t" << eventInfo.TriggerTimeTag << std::endl;

    err = CAEN_DGTZ_DecodeEvent(fHandler, pEventPtr, (void **)&fpEventStd);
    PrintError(err, "DecodeEvent");

    uint64_t timeStamp = (eventInfo.TriggerTimeTag + fTimeOffset) * fTimeSample;
    if (timeStamp < fPreviousTime) {
      constexpr uint32_t maxTime = 0xFFFFFFFF / 2;  // Check manual
      timeStamp += maxTime * fTimeSample;
      fTimeOffset += maxTime;
    }
    fPreviousTime = timeStamp;

    for (uint32_t iCh = 0; iCh < fNCh; iCh++) {
      if (!((eventInfo.ChannelMask >> iCh) & 0x1)) continue;

      const uint32_t size = fpEventStd->ChSize[iCh];
      EveData_t *dataEle = new EveData(size);
      dataEle->ModNumber = fDigiPar.BrdID;
      dataEle->ChNumber = iCh;
      dataEle->TimeStamp = timeStamp;
      dataEle->SamplingPeriod = fTimeSample;
      dataEle->Trace1.resize(size);
      std::copy(&fpEventStd->DataChannel[iCh][0],
                &fpEventStd->DataChannel[iCh][size], dataEle->Trace1.begin());

      fpDataVec->push_back(dataEle);
    }
  }
}
