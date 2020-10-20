#include <CAENDigitizer.h>
#include <CAENDigitizerType.h>

#include <algorithm>
#include <iostream>

#include "ErrorCodeMap.hpp"
#include "TDigitizer.hpp"

TDigitizer::TDigitizer() : fHandler(-1) { Init(); }

TDigitizer::TDigitizer(int h) : fHandler(h) { Init(); }

TDigitizer::~TDigitizer()
{
  ClearDataVec();
  delete fpDataVec;

  CloseDigitizer();
}

void TDigitizer::ClearDataVec()
{
  // Delete all data.  But not change reserved memory size;
  for (auto &&ptr : *fpDataVec) {
    delete ptr;
    ptr = nullptr;
  }
  fpDataVec->clear();
}

void TDigitizer::Init()
{
  // For parameters?
  fpDataVec = new std::vector<EveData_t *>;
  fpDataVec->reserve(gnMaxEvents);

  fPauseFlag = false;
}

void TDigitizer::GetDigiInfo()
{
  // For digitizer
  CAEN_DGTZ_ErrorCode ret = CAEN_DGTZ_Success;
  CAEN_DGTZ_BoardInfo_t info;
  ret = CAEN_DGTZ_GetInfo(fHandler, &info);
  PrintError(ret, "GetInfo");

  fNCh = info.Channels;

  if (info.FamilyCode == 5) {
    fDigiModel = 751;
    fTimeSample = 1;
    fADCRes = 10;
  } else if (info.FamilyCode == 7) {
    fDigiModel = 780;
    fTimeSample = 10;
    fADCRes = 14;
  } else if (info.FamilyCode == 13) {
    fDigiModel = 781;
    fTimeSample = 10;
    fADCRes = 14;
  } else if (info.FamilyCode == 0) {
    fDigiModel = 724;
    fTimeSample = 10;
    fADCRes = 14;
  } else if (info.FamilyCode == 11) {
    fDigiModel = 730;
    fTimeSample = 2;
    fADCRes = 14;
  } else if (info.FamilyCode == 14) {
    fDigiModel = 725;
    fTimeSample = 4;
    fADCRes = 14;
  } else if (info.FamilyCode == 3) {
    fDigiModel = 720;
    fTimeSample = 4;
    fADCRes = 12;
  } else if (info.FamilyCode == 999) {  // temporary code for Hexagon
    fDigiModel = 5000;
    fTimeSample = 10;
    fADCRes = 14;
  } else {
    std::cerr << "ERROR: Unknown digitizer model\n" << std::endl;
    exit(0);
  }

  uint32_t majorNumber = atoi(info.AMC_FirmwareRel);      // xxx.yyy x is major
  uint32_t minorNumber = atoi(&info.AMC_FirmwareRel[4]);  // xxx.yyy y is minor
  fFWrev = minorNumber;
  fFW = FWCode::Others;
  // if (majorNumber == 128) {
  //   fFW = FWCode::DPP_PHA;  // It will be never used at ELI?
  // } else if (majorNumber == 130) {
  //   fFW = FWCode::Others;
  // } else if (majorNumber == 131) {
  //   fFW = FWCode::DPP_PSD;
  // } else if (majorNumber == 132) {
  //   fFW = FWCode::DPP_PSD;
  // } else if (majorNumber == 136) {
  //   fFW = FWCode::DPP_PSD;  // NOTE: valid also for x725
  // } else if (majorNumber == 139) {
  //   fFW = FWCode::DPP_PHA;  // NOTE: valid also for x725
  // } else {
  //   fFW = FWCode::STD;
  // }

  if (fDigiModel == 5000) {  // Hexagon
    fFW = FWCode::DPP_nPHA_724;
  } else if (majorNumber == 128) {
    if (minorNumber >= 0x40) {
      fFW = FWCode::DPP_nPHA_724;
    } else {
      fFW = FWCode::DPP_PHA_724;
    }
  } else if (majorNumber == 130) {
    fFW = FWCode::DPP_CI;
  } else if (majorNumber == 131) {
    fFW = FWCode::DPP_PSD_720;
  } else if (majorNumber == 132) {
    fFW = FWCode::DPP_PSD_751;
  } else if (majorNumber == 136) {
    fFW = FWCode::DPP_PSD_730;  // NOTE: valid also for x725
  } else if (majorNumber == 139) {
    fFW = FWCode::DPP_PHA_730;  // NOTE: valid also for x725
  } else {
    fFW = FWCode::STD;
  }

  auto license = std::string(info.License);
  if (fFW == FWCode::STD) {
    license = "Using std, wave record, free FW";
  } else if (fFW == FWCode::DPP_PSD || fFW == FWCode::DPP_PHA) {
    uint32_t val;
    CAEN_DGTZ_ReadRegister(fHandler, 0x8158, &val);
    if (val == 0x53D4) {
      license = "The DPP is licensed";
    } else {
      if (val > 0) {
        auto rest = (int)((float)val / 0x53D4 * 30);
        license = "WARNING: DPP not licensed: " + std::to_string(rest) +
                  " minutes remaining";
      } else {
        std::cerr << "ERROR: DPP not licensed: time expired" << std::endl;
        exit(0);
      }
    }
  }

  std::cout << "\nHandler number:\t" << fHandler << "\n"
            << "Model name:\t" << info.ModelName << "\n"
            << "Model number:\t" << info.Model << "\n"
            << "No. channels:\t" << info.Channels << "\n"
            << "Format factor:\t" << info.FormFactor << "\n"
            << "Family code:\t" << info.FamilyCode << "\n"
            << "Firmware revision of the FPGA on the mother board (ROC):\t"
            << info.ROC_FirmwareRel << "\n"
            << "Firmware revision of the FPGA on the daughter board (AMC):\t"
            << info.AMC_FirmwareRel << "\n"
            << "Serial number:\t" << info.SerialNumber << "\n"
            << "PCB revision:\t" << info.PCB_Revision << "\n"
            << "No. bits of the ADC:\t" << info.ADC_NBits << "\n"
            << "Device handler of CAENComm:\t" << info.CommHandle << "\n"
            << "Device handler of CAENVME:\t" << info.VMEHandle << "\n"
            << "License:\t" << license << std::endl;

  if ((fDigiModel == 724) || (fDigiModel == 780) || (fDigiModel == 781))
    fSTU = 10;
  else if (fDigiModel == 725)
    fSTU = 16;
  else
    fSTU = 8;
  // correct Tsampl (i.e. sampling period) for decimation
  if ((fFW == FWCode::DPP_PHA_724) || (fFW == FWCode::DPP_nPHA_724)) {
    // HACK: assuming the same for all channels
    fTimeSample *= (1 << fDigiPar.Decimation[0]);
    fSTU *= (1 << fDigiPar.Decimation[0]);
  }
}

void TDigitizer::OpenDigitizer()
{
  CAEN_DGTZ_ErrorCode ret = CAEN_DGTZ_Success;
  ret = CAEN_DGTZ_OpenDigitizer(fLinkPar.LinkType, fLinkPar.LinkNum,
                                fLinkPar.ConetNode, fLinkPar.VMEBaseAddress,
                                &fHandler);
  if (ret < 0) {
    std::cerr << "CAEN_DGTZ_OpenDigitizer failed" << std::endl;
    exit(0);
  }
}

void TDigitizer::CloseDigitizer()
{
  CAEN_DGTZ_ErrorCode ret = CAEN_DGTZ_Success;
  ret = CAEN_DGTZ_CloseDigitizer(fHandler);
  if (ret < 0) {
    std::cerr << "CAEN_DGTZ_CloseDigitizer failed" << std::endl;
    // exit(0);
  }
}

void TDigitizer::Start()
{
  if (!fPauseFlag) AllocateMemory();
  fPauseFlag = false;

  CAEN_DGTZ_ErrorCode err;
  err = CAEN_DGTZ_SWStartAcquisition(fHandler);
  PrintError(err, "StartAcquisition");
}

void TDigitizer::Stop()
{
  CAEN_DGTZ_ErrorCode err;
  err = CAEN_DGTZ_SWStopAcquisition(fHandler);
  PrintError(err, "StopAcquisition");

  fTimeOffset = 0;
  fPreviousTime = 0;

  FreeMemory();
}

void TDigitizer::Pause()
{
  CAEN_DGTZ_ErrorCode err;
  err = CAEN_DGTZ_SWStopAcquisition(fHandler);
  PrintError(err, "StopAcquisition");

  fPauseFlag = true;
}

void TDigitizer::Unconfig() {}

void TDigitizer::SetStartMode()
{
  CAEN_DGTZ_ErrorCode ret = CAEN_DGTZ_Success;
  uint32_t d32;

  switch (fDigiPar.StartMode) {
    case START_MODE_INDEP_SW:
      ret = CAEN_DGTZ_SetAcquisitionMode(fHandler, CAEN_DGTZ_SW_CONTROLLED);
      PrintError(ret, "START_MODE_INDEP_SW");
      break;

    case START_MODE_SYNCIN_1ST_SW:
    case START_MODE_SYNCIN_1ST_HW:
      if (fDigiPar.SyncinMode != SYNCIN_MODE_RUN_CTRL) {
        std::cout
            << "WARNING: SyncinMode must be set as RUN_CTRL; forced option"
            << std::endl;
        fDigiPar.SyncinMode = SYNCIN_MODE_RUN_CTRL;
      }
      if (fDigiPar.TrgoutMode != TRGOUT_MODE_SYNC_OUT) {
        std::cout
            << "WARNING: TrgoutMode must be set as SYNC_OUT; forced option"
            << std::endl;
        fDigiPar.TrgoutMode = TRGOUT_MODE_SYNC_OUT;
      }
      ret = CAEN_DGTZ_ReadRegister(fHandler, CAEN_DGTZ_ACQ_CONTROL_ADD, &d32);
      ret = CAEN_DGTZ_WriteRegister(
          fHandler, CAEN_DGTZ_ACQ_CONTROL_ADD,
          (d32 & 0xFFFFFFF0) |
              RUN_START_ON_SIN_LEVEL);  // Arm acquisition (Run will start when
                                        // SIN goes high)
      // Run Delay to deskew the start of acquisition
      if (fDigiPar.BrdID == 0)
        ret = CAEN_DGTZ_WriteRegister(fHandler, 0x8170,
                                      (fDigiPar.NumBrd - 1) * 3 + 1);
      else
        ret = CAEN_DGTZ_WriteRegister(
            fHandler, 0x8170, (fDigiPar.NumBrd - fDigiPar.BrdID - 1) * 3);
      break;

    case START_MODE_TRGIN_1ST_SW:
    case START_MODE_TRGIN_1ST_HW:
      ret = CAEN_DGTZ_ReadRegister(fHandler, CAEN_DGTZ_ACQ_CONTROL_ADD, &d32);
      ret = CAEN_DGTZ_WriteRegister(
          fHandler, CAEN_DGTZ_ACQ_CONTROL_ADD,
          (d32 & 0xFFFFFFF0) |
              RUN_START_ON_TRGIN_RISING_EDGE);  // Arm acquisition (Run will
                                                // start with 1st trigger)
      // Run Delay to deskew the start of acquisition
      if (fDigiPar.BrdID == 0)
        ret = CAEN_DGTZ_WriteRegister(fHandler, 0x8170,
                                      (fDigiPar.NumBrd - 1) * 3 + 1);
      else
        ret = CAEN_DGTZ_WriteRegister(
            fHandler, 0x8170, (fDigiPar.NumBrd - fDigiPar.BrdID - 1) * 3);
      break;
  }
}

void TDigitizer::SetTrgInMode()
{
  CAEN_DGTZ_ErrorCode ret = CAEN_DGTZ_Success;

  if (fDigiModel != 5000) {
    if (fDigiPar.TrginMode == TRGIN_MODE_DISABLED) {
      ret = CAEN_DGTZ_WriteRegister(
          fHandler, CAEN_DGTZ_TRIGGER_SRC_ENABLE_ADD,
          0x80000000);  // accept SW trg only (ext trig is disabled)
    } else if (fDigiPar.TrginMode == TRGIN_MODE_GLOBAL_TRG) {
      ret = CAEN_DGTZ_WriteRegister(fHandler, CAEN_DGTZ_TRIGGER_SRC_ENABLE_ADD,
                                    0xC0000000);  // accept ext trg_in or SW trg
    } else if ((fDigiPar.TrginMode == TRGIN_MODE_VETO) ||
               (fDigiPar.TrginMode == TRGIN_MODE_GATE)) {
      ret =
          CAEN_DGTZ_WriteRegister(fHandler, CAEN_DGTZ_TRIGGER_SRC_ENABLE_ADD,
                                  0x80000000);  // accept SW trg only (ext trig
                                                // is used for the veto/gate)
      ret = RegisterSetBits(0x811C, 10, 11, 3);
      // propagate ext-trg "as is" to channels (will be
      // used as a validation for the self triggers)
      for (auto i = 0; i < 8; i++)
        ret = CAEN_DGTZ_WriteRegister(fHandler, 0x8180 + i * 4,
                                      0);  // not used
      if (fFW == FWCode::DPP_PSD_730) ret = RegisterSetBits(0x8084, 4, 5, 1);
      // set individual trgin mode = from MB (not
      // used, masks are disabled)
      if (fFW == FWCode::DPP_PHA_730) ret = RegisterSetBits(0x80A0, 4, 5, 1);
      // set individual trgin mode = from MB (not
      // used, masks are disabled)

    } else if (fDigiPar.TrginMode ==
               TRGIN_MODE_COINC) {  // TrgIn fan out to each channel (individual
                                    // trg validation)
      ret = CAEN_DGTZ_WriteRegister(fHandler, CAEN_DGTZ_TRIGGER_SRC_ENABLE_ADD,
                                    0x80000000);  // accept SW trg only
      for (auto i = 0; i < 8; i++)
        ret = CAEN_DGTZ_WriteRegister(fHandler, 0x8180 + i * 4, 0x40000000);
    }
  } else {
    if (fDigiPar.TrginMode == TRGIN_MODE_DISABLED) {
      // ret = RegisterSetBits(fHandler, 0x8048, 2, 2, 0);   // disable
      // external trigger
    } else if (fDigiPar.TrginMode == TRGIN_MODE_GLOBAL_TRG) {
      ret = RegisterSetBits(0x8048, 2, 2, 1);
      // enable external trigger
      // set trigger mode = propagate (To do)
    } else if (fDigiPar.TrginMode == TRGIN_MODE_COINC) {  //
      ret = RegisterSetBits(0x8048, 2, 2, 1);
      // enable external trigger
      // set trigger mode = coincidence (To do)
    }
  }
}

void TDigitizer::SetTrgOutMode()
{
  CAEN_DGTZ_ErrorCode ret = CAEN_DGTZ_Success;
  if (fDigiModel != 5000) {
    if (fDigiPar.TrgoutMode == TRGOUT_MODE_DISABLED) {  // disabled (OR mask=0)
      ret = CAEN_DGTZ_WriteRegister(fHandler, 0x8110, 0);
    } else if (fDigiPar.TrgoutMode ==
               TRGOUT_MODE_PROP_TRGIN) {  // propagate trgin
      ret = CAEN_DGTZ_WriteRegister(fHandler, 0x8110, 0x40000000);
    } else if (fDigiPar.TrgoutMode ==
               TRGOUT_MODE_CH_TRG) {  // propagate self triggers (with mask)
      ret = CAEN_DGTZ_WriteRegister(fHandler, 0x8110, fDigiPar.TrgoutMask);
    } else if (fDigiPar.TrgoutMode ==
               TRGOUT_MODE_SYNC_OUT) {  // propagate sync signal (start/stop)
      ret = RegisterSetBits(0x811C, 16, 17, 0x1);
      ret = RegisterSetBits(0x811C, 18, 19, 0x0);
    } else if (fDigiPar.TrgoutMode ==
               TRGOUT_CLOCK) {  // propagate internal clock
      ret = CAEN_DGTZ_WriteRegister(fHandler, CAEN_DGTZ_FRONT_PANEL_IO_CTRL_ADD,
                                    0x00050000);
    } else if (fDigiPar.TrgoutMode == TRGOUT_SIGSCOPE) {  //
      ret = CAEN_DGTZ_WriteRegister(fHandler, 0x8110, fDigiPar.TrgoutMask);
      if (fFW == FWCode::DPP_PSD_730) {
        // replace selftrg with an internal digital probe for
        // debugging (on x730/x725 PSD only)
        ret = RegisterSetBits(0x8084, 20, 23, 7);
        // 1  => overth;
        // 2  => selftrg;
        // 3  => pu_trg;
        // 4  => pu_trg or selftrg;
        // 5  => veto;
        // 6  => coincp;
        // 7  => trgval;
        // 8  => tvaw;
        // 9  => npulse;
        // 10 => gpulse;
      }
    } else {  // Use TRGOUT/GPO as a test pulser
      ret = RegisterSetBits(0x811C, 15, 15, 1);
      if (fDigiPar.TrgoutMode == TRGOUT_MODE_SQR_1KHZ)
        ret = RegisterSetBits(0x8168, 0, 2, 1);  // 1 KHz square wave
      if (fDigiPar.TrgoutMode == TRGOUT_MODE_PLS_1KHZ)
        ret = RegisterSetBits(0x8168, 0, 2, 2);  // 1 KHz pulses
      if (fDigiPar.TrgoutMode == TRGOUT_MODE_SQR_10KHZ)
        ret = RegisterSetBits(0x8168, 0, 2, 3);  // 10 KHz square wave
      if (fDigiPar.TrgoutMode == TRGOUT_MODE_PLS_10KHZ)
        ret = RegisterSetBits(0x8168, 0, 2, 4);  // 10 KHz pulses
    }
  } else {  // Hegagon
            // nothing to do for the moment
  }
}

void TDigitizer::SetHWCoincidence()
{
  CAEN_DGTZ_ErrorCode ret = CAEN_DGTZ_Success;

  if (((fDigiPar.CoincMode > 0) || (fDigiPar.TrginMode == TRGIN_MODE_VETO) ||
       (fDigiPar.TrginMode == TRGIN_MODE_GATE)) &&
      IsDPP()) {
    char Msg[500];
    int paired_channels;  // x730 and x725 boards have paired channel triggers
    int coincmode = 1;
    uint32_t maskindex, vwreg;
    uint32_t DppCtrl2;
    uint32_t ChTrgMask =
        0;  // Mask of the channel self-triggers to be used in the coincidence
            // logic (individual trigger validation masks)
    if ((fDigiModel == 730) ||
        (fDigiModel ==
         725)) {  // in the x730, 2 channels go in one bit of the mask
      paired_channels = 1;
      for (auto i = 0; i < MAX_NCH; i++)
        if (fDigiPar.EnableInput[i] && (i < fNCh)) ChTrgMask |= (1 << (i / 2));
    } else {
      paired_channels = 0;
      ChTrgMask = fDigiPar.EnableMask;
    }
    if ((fFW == FWCode::DPP_PHA_724) || (fFW == FWCode::DPP_nPHA_724))
      coincmode = 2;

    for (auto i = 0; i < MAX_NCH; i++) {
      if (fDigiPar.EnableInput[i] && (i < fNCh)) {
        // index of the channel trigger mask for the trigger validation
        maskindex = ((fDigiModel == 730) || (fDigiModel == 725)) ? i / 2 : i;
        DppCtrl2 = (fFW == FWCode::DPP_PHA_730) ? 0x10A0 : 0x1084;

        if (paired_channels) {
          ret = RegisterSetBits(DppCtrl2 + (i << 8), 2, 2,
                                1);  // enable couple trgout
          ret = RegisterSetBits(DppCtrl2 + (i << 8), 6, 6,
                                1);  // enable trg validation input
        }

        // Set Extra Coinc Window Width (for PHA firmware, the extra window
        // width is hardcoded)
        if ((fFW != FWCode::DPP_PHA_730) && (fFW != FWCode::DPP_PHA_724) &&
            (fFW != FWCode::DPP_nPHA_724)) {
          ret = CAEN_DGTZ_WriteRegister(fHandler, 0x106C + (i << 8), 9);
        }

        // Veto Window Register (for x730/x725)
        if (fDigiPar.VetoWindow[i] < (float)(65535 * 8))
          vwreg = (uint32_t)(fDigiPar.VetoWindow[i] / fSTU);
        else if (fDigiPar.VetoWindow[i] < (float)(65535 * 2000))
          vwreg = 0x10000 | (uint32_t)(fDigiPar.VetoWindow[i] / 2000);
        else if (fDigiPar.VetoWindow[i] < (float)((uint64_t)65535 * 524000))
          vwreg = 0x20000 | (uint32_t)(fDigiPar.VetoWindow[i] / 524000);
        else
          vwreg = 0x30000 | (uint32_t)(fDigiPar.VetoWindow[i] / 134000000);

        // program trigger and coincidence logic
        if (fDigiPar.TrginMode == TRGIN_MODE_VETO) {  // TRGIN = VETO
          sprintf(Msg, "INFO: external TRGIN acts as a VETO\n");
          // enable anti-coincindence mode (wait for
          // trg validation to reject)
          ret = RegisterSetBits(0x8080, 18, 19, 3);

          if ((fFW == FWCode::DPP_PHA_730) && (fFWrev >= 4)) {
            // gate window width (0 = as long as input signal)
            ret = CAEN_DGTZ_WriteRegister(fHandler, 0x10D4 + (i << 8), vwreg);
            // set veto source = TRGIN
            ret = RegisterSetBits(DppCtrl2 + (i << 8), 14, 15, 1);
          } else if (fFW == FWCode::DPP_PSD_730) {
            ret = CAEN_DGTZ_WriteRegister(fHandler, 0x10D4 + (i << 8), vwreg);
            // set veto source = TRGIN
            ret = RegisterSetBits(DppCtrl2 + (i << 8), 18, 19, 1);
          }
          ret = CAEN_DGTZ_WriteRegister(fHandler, 0x8180 + maskindex * 4, 0);
        } else if (fDigiPar.TrginMode == TRGIN_MODE_GATE) {
          // TRGIN = GATE (= anti-veto)
          // enable coincindence mode (wait for trg validation to reject)
          std::cout << "INFO: external TRGIN acts as a GATE" << std::endl;
          ret = RegisterSetBits(0x8080, 18, 19, coincmode);
          if ((fFW == FWCode::DPP_PHA_730) && (fFWrev >= 4)) {
            // gate window width (0 = as long as input signal)
            ret = CAEN_DGTZ_WriteRegister(fHandler, 0x10D4 + (i << 8), vwreg);
            // set veto source = TRGIN (antiveto = gate)
            ret = RegisterSetBits(DppCtrl2 + (i << 8), 14, 15, 1);
          } else if (fFW == FWCode::DPP_PSD_730) {
            ret = CAEN_DGTZ_WriteRegister(fHandler, 0x10D4 + (i << 8), vwreg);
          }
          ret = CAEN_DGTZ_WriteRegister(fHandler, 0x8180 + maskindex * 4, 0);
        } else {
          // Channel Trigout Width (it determines the coincidence window).
          if ((fDigiModel == 724) || (fDigiModel == 780) || (fDigiModel == 781))
            ret = CAEN_DGTZ_WriteRegister(fHandler, 0x1084 + (i << 8),
                                          fDigiPar.CoincWindow / 10);
          else if (fFW == FWCode::DPP_PHA_730)
            ret = CAEN_DGTZ_WriteRegister(fHandler, 0x1084 + (i << 8),
                                          fDigiPar.CoincWindow / fSTU);
          else
            ret = CAEN_DGTZ_WriteRegister(fHandler, 0x1070 + (i << 8),
                                          fDigiPar.CoincWindow / fSTU);

          switch (fDigiPar.CoincMode) {
            case COINC_MAJORITY:  // acquire when the num of fired channels >=
                                  // Majority Threshold
              if (paired_channels) {
                std::cout
                    << "INFO: Coincidence with Majority (fired couples >= "
                    << fDigiPar.MajorityLevel << ")" << std::endl;
                // couple trgout = OR
                ret = RegisterSetBits(DppCtrl2 + (i << 8), 0, 1, 3);
                // trigger validation comes from the other couples
                ret = RegisterSetBits(DppCtrl2 + (i << 8), 4, 5, 1);
              } else {
                std::cout
                    << "INFO: Coincidence with Majority (fired channels >= "
                    << fDigiPar.MajorityLevel << ")" << std::endl;
              }
              // enable coincindence mode (wait
              // for trg validation to acquire)
              ret = RegisterSetBits(0x8080, 18, 19, coincmode);
              ret = CAEN_DGTZ_WriteRegister(
                  fHandler, 0x8180 + maskindex * 4,
                  0x200 | ChTrgMask | ((fDigiPar.MajorityLevel - 1) << 10));
              break;

            case COINC_MINORITY:  // acquire when the num of fired channels <
                                  // Majority Threshold
              if (paired_channels) {
                std::cout << "INFO: Coincidence with Minority (fired couples < "
                          << fDigiPar.MajorityLevel << ")" << std::endl;
                // couple trgout = OR
                ret = RegisterSetBits(DppCtrl2 + (i << 8), 0, 1, 3);
                // trigger validation comes from the other couples
                ret = RegisterSetBits(DppCtrl2 + (i << 8), 4, 5, 1);
              } else {
                sprintf(
                    Msg,
                    "INFO: Coincidence with Minority (fired channels < %d)\n",
                    fDigiPar.MajorityLevel);
              }
              // enable anti-coincindence mode (wait
              // for trg validation to reject)
              ret = RegisterSetBits(0x8080, 18, 19, 3);
              ret = CAEN_DGTZ_WriteRegister(
                  fHandler, 0x8180 + maskindex * 4,
                  0x200 | ChTrgMask | ((fDigiPar.MajorityLevel - 1) << 10));
              break;

            case COINC_PAIRED_AND:  // both channels of a couple acquire when
                                    // both are fired (0 and 1, 2 and 3, etc...)
              sprintf(Msg,
                      "INFO: Coincidence in Paired Mode: CH[even] AND CH[odd]) "
                      " \n");
              // enable coincindence mode (wait
              // for trg validation to acquire)
              ret = RegisterSetBits(0x8080, 18, 19, coincmode);
              if (paired_channels) {
                // signals from mother-board
                ret = CAEN_DGTZ_WriteRegister(fHandler, 0x8180 + maskindex * 4,
                                              0);
                // (inter-couple) are not used
                // trigger validation comes from the other channel in the couple
                ret = RegisterSetBits(DppCtrl2 + (i << 8), 4, 5, 2);
                // couple trgout = AND
                ret = RegisterSetBits(DppCtrl2 + (i << 8), 0, 1, 0);
                // extra-time window shorter incase of paired channels
                ret = CAEN_DGTZ_WriteRegister(fHandler, 0x106C + (i << 8), 2);
              } else {
                if ((fDigiModel == 780) || (fDigiModel == 790))
                  ret = CAEN_DGTZ_WriteRegister(fHandler,
                                                0x8188 + maskindex * 4, 0x10C);
                else
                  ret =
                      CAEN_DGTZ_WriteRegister(fHandler, 0x8180 + maskindex * 4,
                                              0x100 | (3 << (i & 0xFFFE)));
              }
              fDigiPar.TrgHoldOff =
                  std::max((2 * fDigiPar.CoincWindow), fDigiPar.TrgHoldOff);
              break;

            case COINC_PAIRED_NAND:  // one channel of a couple acquires when
                                     // the other one is not fired (anti
                                     // coincidence)
              std::cout << "INFO: Anti-Coincidence in Paired Mode: "
                           "Not(CH[even] AND CH[odd])"
                        << std::endl;
              ret = RegisterSetBits(0x8080, 18, 19, 3);
              // enable anti-coincindence mode (wait
              // for trg validation to reject)
              if (paired_channels) {
                // signals from mother-board
                // (inter-couple) are not used
                ret = CAEN_DGTZ_WriteRegister(fHandler, 0x8180 + maskindex * 4,
                                              0);
                // trigger validation comes from the
                // other channel in the couple
                ret = RegisterSetBits(DppCtrl2 + (i << 8), 4, 5, 2);
                // couple trgout = OR (NOTE: doesn't
                // reflect the acq. trigger!!!)
                ret = RegisterSetBits(DppCtrl2 + (i << 8), 0, 1, 3);
                // don't need extra-time for the coincidence window
                ret = CAEN_DGTZ_WriteRegister(fHandler, 0x106C + (i << 8), 0);
              } else {
                ret = CAEN_DGTZ_WriteRegister(fHandler, 0x8180 + maskindex * 4,
                                              0x100 | (3 << (i / 2)));
              }
              fDigiPar.TrgHoldOff =
                  std::max((2 * fDigiPar.CoincWindow), fDigiPar.TrgHoldOff);
              break;

            case COINC_PAIRED_OR:  // both channels of a couple acquire when one
                                   // at least on of the two is fired (trigger
                                   // propagation in the couple)
              if (fFW == FWCode::DPP_PHA_724) {
                sprintf(Msg,
                        "WARNING: PAIRED_OR mode is not possible for this "
                        "DPP_PHA\n");
              } else {
                sprintf(Msg,
                        "INFO: Trigger propagation in Paired Mode: CH[even] OR "
                        "CH[odd]\n");
                if (fFW != FWCode::DPP_PHA_724)
                  // disable self trg
                  ret = RegisterSetBits(0x1080 + (i << 8), 24, 24, 1);
                if (paired_channels) {
                  // signals from mother-board (inter-couple) are not used
                  ret = CAEN_DGTZ_WriteRegister(fHandler,
                                                0x8180 + maskindex * 4, 0);
                  // trigger propagation between the
                  // channels in the couple
                  ret = RegisterSetBits(DppCtrl2 + (i << 8), 4, 5, 3);
                  // couple trgout = OR (NOTE: doesn't
                  // reflect the acq. trigger!!!)
                  ret = RegisterSetBits(DppCtrl2 + (i << 8), 0, 1, 3);
                } else {
                  ret = CAEN_DGTZ_WriteRegister(
                      fHandler, 0x8180 + maskindex * 4, 3 << (i / 2));
                }
              }
              break;

            case COINC_CH0_AND_ANY:  // coinc/anticoinc between a common ref
                                     // channel (for now, hardcoded to ch 0) and
                                     // any other channel (but ch 1)
            case COINC_CH0_NAND_ANY:
              if (fDigiPar.CoincMode == COINC_CH0_AND_ANY) {
                // enable coincindence mode (wait for trg
                // validation to acquire)
                ret = RegisterSetBits(0x8080, 18, 19, coincmode);
                sprintf(Msg,
                        "INFO: Coincidence one to all: CH[0] AND CH[any]\n");
              } else {
                // enable anti-coincindence mode (wait
                // for trg validation to reject)
                ret = RegisterSetBits(0x8080, 18, 19, 3);
                sprintf(Msg,
                        "INFO: Anti-Coincidence one to all: Not(CH[0] AND "
                        "CH[any])\n");
              }
              if (paired_channels) {
                if (i == 0) {
                  // couple trgout = ch0 only (for couple 0)
                  ret = RegisterSetBits(DppCtrl2, 0, 1, 1);
                  // trigger validation comes from other couples
                  ret = RegisterSetBits(DppCtrl2, 4, 5, 1);
                  ret = CAEN_DGTZ_WriteRegister(fHandler, 0x8180,
                                                0xFE & ChTrgMask);
                } else if (i == 1) {
                  ret = RegisterSetBits(0x1080 + (i << 8), 24, 24, 1);
                  // disable self trg (ch1 cannot be
                  // used in this coinc mode)
                  if (fDigiPar.EnableInput[i]) {
                    std::cout
                        << "WARNING: Channel 1 cannot be used in COMMON_REFCH "
                           "mode; disabled"
                        << std::endl;
                  }
                } else {
                  /*ret = RegisterSetBits(fHandler, DppCtrl2 + (i<<8), 0, 1,
                  3);  // couple trgout = OR ret = RegisterSetBits(fHandler,
                  DppCtrl2 + (i<<8), 4, 5, 1);  // trigger validation comes from
                  other couples ret = CAEN_DGTZ_WriteRegister(fHandler,
                  0x8180 + maskindex*4, 0x01);*/
                  if (fDigiPar.CoincMode == COINC_CH0_AND_ANY) {
                    // couple trgout = OR
                    ret = RegisterSetBits(DppCtrl2 + (i << 8), 0, 1, 3);
                    // trigger validation comes from other couples
                    ret = RegisterSetBits(DppCtrl2 + (i << 8), 4, 5, 1);
                    ret = CAEN_DGTZ_WriteRegister(fHandler,
                                                  0x8180 + maskindex * 4, 0x01);
                  } else {
                    // other channels acquires in singles
                    ret = RegisterSetBits(0x1080 + (i << 8), 18, 19, 0);
                    // couple trgout = OR
                    ret = RegisterSetBits(DppCtrl2 + (i << 8), 0, 1, 3);
                    ret = CAEN_DGTZ_WriteRegister(fHandler,
                                                  0x8180 + maskindex * 4, 0x00);
                  }
                }
              } else {
                if (i == 0)
                  ret = CAEN_DGTZ_WriteRegister(fHandler, 0x8180,
                                                0xFE & ChTrgMask);
                else
                  ret = CAEN_DGTZ_WriteRegister(fHandler,
                                                0x8180 + maskindex * 4, 0x01);
              }
              fDigiPar.TrgHoldOff =
                  std::max((2 * fDigiPar.CoincWindow), fDigiPar.TrgHoldOff);
              break;

            case COINC_AND_ALL:  // coincidence between all the enabled channels
              sprintf(Msg, "INFO: Coincidence between all enabled channels\n");
              // enable coincindence mode (wait
              // for trg validation to acquire)
              ret = RegisterSetBits(0x8080, 18, 19, coincmode);
              ret = CAEN_DGTZ_WriteRegister(fHandler, 0x8180 + maskindex * 4,
                                            0x100 | ChTrgMask);
              if (paired_channels) {
                // trigger validation comes from other couples
                ret = RegisterSetBits(DppCtrl2 + (i << 8), 4, 5, 1);
                if ((i & 1) == 0) {
                  if (fDigiPar.EnableInput[i + 1])
                    // couple trgout = AND
                    ret = RegisterSetBits(DppCtrl2 + (i << 8), 0, 1, 0);
                  else
                    // couple trgout = even only
                    ret = RegisterSetBits(DppCtrl2 + (i << 8), 0, 1, 1);
                } else {
                  if (fDigiPar.EnableInput[i - 1])
                    // couple trgout = AND
                    ret = RegisterSetBits(DppCtrl2 + (i << 8), 0, 1, 0);
                  else
                    // couple trgout = odd only
                    ret = RegisterSetBits(DppCtrl2 + (i << 8), 0, 1, 2);
                }
              }
              fDigiPar.TrgHoldOff =
                  std::max((2 * fDigiPar.CoincWindow), fDigiPar.TrgHoldOff);
              break;

            case COINC_OR_ALL:  // any self trigger makes all channels to
                                // acquire
              if (fFW == FWCode::DPP_PHA_724) {
                sprintf(
                    Msg,
                    "WARNING: OR_ALL mode is not possible for this DPP_PHA\n");
              } else {
                sprintf(Msg,
                        "INFO: Trigger propagation from any channel to all\n");
                // disable self trg
                ret = RegisterSetBits(0x1080 + (i << 8), 24, 24, 1);
                ret = CAEN_DGTZ_WriteRegister(fHandler, 0x8180 + maskindex * 4,
                                              ChTrgMask);
                if (paired_channels) {
                  // trigger propagation comes from other couples
                  ret = RegisterSetBits(DppCtrl2 + (i << 8), 4, 5, 1);
                  // couple trgout = OR
                  ret = RegisterSetBits(DppCtrl2 + (i << 8), 0, 1, 3);
                }
              }
              break;

            case COINC_CH0_TO_ALL:  // all enabled channels acquire if channel 0
                                    // is fired (trigger propagation from
                                    // channel 0 to all)
              if (fFW != FWCode::DPP_PHA_724) {
                // disable self trg
                ret = RegisterSetBits(0x1080 + (i << 8), 24, 24, 1);
                ret = CAEN_DGTZ_WriteRegister(fHandler, 0x8180 + maskindex * 4,
                                              0x01);
              } else {
                if (i > 0) {
                  ret = CAEN_DGTZ_WriteRegister(fHandler,
                                                0x8180 + maskindex * 4, 0x01);
                  // disable self trg
                  ret = RegisterSetBits(0x1080 + (i << 8), 24, 24, 1);
                }
              }
              if (paired_channels) {
                sprintf(Msg,
                        "INFO: Trigger propagation from channel 0/1 to all\n");
                // couple trgout = OR
                ret = RegisterSetBits(DppCtrl2 + (i << 8), 0, 1, 3);
                // trigger propagation comes from other couples
                ret = RegisterSetBits(DppCtrl2 + (i << 8), 4, 5, 1);
              } else {
                sprintf(Msg,
                        "INFO: Trigger propagation from channel 0 to all\n");
              }
              break;

            default:
              break;
          }
        }
      }
    }
  }
}

void TDigitizer::PrintError(const CAEN_DGTZ_ErrorCode &err,
                            const std::string &funcName) const
{
  if (err < 0) {  // 0 is success
    std::cout << "In " << funcName << ", error code = " << err << ", "
              << ErrorCodeMap[err] << std::endl;
    // CAEN_DGTZ_CloseDigitizer(fHandler);
    // throw err;
  }
}

CAEN_DGTZ_ErrorCode TDigitizer::RegisterSetBits(uint16_t addr, int start_bit,
                                                int end_bit, int val)
{
  CAEN_DGTZ_ErrorCode ret;
  uint32_t mask{0}, reg{0};
  if (((addr & 0xFF00) == 0x8000) && (addr != 0x8000) && (addr != 0x8004) &&
      (addr !=
       0x8008)) {  // broadcast access to channel individual registers (loop over channels)
    for (auto ch = 0; ch < fNCh; ch++) {
      ret = CAEN_DGTZ_ReadRegister(fHandler, 0x1000 | (addr & 0xFF) | (ch << 8),
                                   &reg);
      for (auto i = start_bit; i <= end_bit; i++) mask |= 1 << i;
      reg = reg & ~mask | ((val << start_bit) & mask);
      ret = CAEN_DGTZ_WriteRegister(fHandler,
                                    0x1000 | (addr & 0xFF) | (ch << 8), reg);
    }
  } else {  // access to channel individual register or mother board register
    ret = CAEN_DGTZ_ReadRegister(fHandler, addr, &reg);
    for (auto i = start_bit; i <= end_bit; i++) mask |= 1 << i;
    reg = reg & ~mask | ((val << start_bit) & mask);
    ret = CAEN_DGTZ_WriteRegister(fHandler, addr, reg);
  }
  return ret;
}

bool TDigitizer::IsDPP()
{
  if (fFW == FWCode::STD || fFW == FWCode::Others) {
    return false;
  }
  //else
  return true;
}

void TDigitizer::SendSWTrigger(unsigned int nTrigger)
{
  CAEN_DGTZ_ErrorCode err;
  for (auto i = 0; i < nTrigger; i++) err = CAEN_DGTZ_SendSWtrigger(fHandler);
  PrintError(err, "SendSWTrigger");
}
