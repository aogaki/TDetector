#ifndef TDigiPar_hpp
#define TDigiPar_hpp 1

#include <CAENDigitizerType.h>

constexpr uint MAX_NCH = 32;  // This should be 64 for 740 series?

constexpr int START_MODE_INDEP_SW = 0;
constexpr int START_MODE_SYNCIN_1ST_SW = 1;
constexpr int START_MODE_SYNCIN_1ST_HW = 2;
constexpr int START_MODE_TRGIN_1ST_SW = 3;
constexpr int START_MODE_TRGIN_1ST_HW = 4;

constexpr int SYNCIN_MODE_DISABLED = 0;
constexpr int SYNCIN_MODE_TSTAMP_RESET = 1;
constexpr int SYNCIN_MODE_RUN_CTRL = 2;

constexpr int TRGIN_MODE_DISABLED = 0;
constexpr int TRGIN_MODE_GLOBAL_TRG = 1;
constexpr int TRGIN_MODE_VETO = 2;
constexpr int TRGIN_MODE_GATE = 3;
constexpr int TRGIN_MODE_COINC = 4;

constexpr int TRGOUT_MODE_DISABLED = 0;
constexpr int TRGOUT_MODE_CH_TRG = 1;
constexpr int TRGOUT_MODE_PROP_TRGIN = 2;
constexpr int TRGOUT_MODE_SYNC_OUT = 3;
constexpr int TRGOUT_MODE_SQR_1KHZ = 4;
constexpr int TRGOUT_MODE_PLS_1KHZ = 5;
constexpr int TRGOUT_MODE_SQR_10KHZ = 6;
constexpr int TRGOUT_MODE_PLS_10KHZ = 7;
constexpr int TRGOUT_CLOCK = 8;
constexpr int TRGOUT_SIGSCOPE = 9;

// start on software command
constexpr int RUN_START_ON_SOFTWARE_COMMAND = 0xC;
// start on S-IN level (logical high = run; logical low = stop)
constexpr int RUN_START_ON_SIN_LEVEL = 0xD;
// start on first TRG-IN or Software Trigger
constexpr int RUN_START_ON_TRGIN_RISING_EDGE = 0xE;
// start on LVDS I/O level
constexpr int RUN_START_ON_LVDS_IO = 0xF;

// options for HW coincidences/anticoincidences and trigger propagation
// no hardware trigger logic is applied (acquire singles)
constexpr int COINC_DISABLED = 0x00;
// acquire when the num of fired channels >= Majority Threshold
constexpr int COINC_MAJORITY = 0x01;
// both channels of a couple acquire when both are fired
constexpr int COINC_PAIRED_AND = 0x02;
// coincidence between a common ref channel (typ. ch 0) and any other channel
constexpr int COINC_CH0_AND_ANY = 0x03;
// acquire when all enabled channels are fired
constexpr int COINC_AND_ALL = 0x04;
// acquire when the num of fired channels < Majority Threshold
constexpr int COINC_MINORITY = 0x81;
// all enabled channels acquire if any channel is fired (trigger propagation from any channel to all)
constexpr int COINC_OR_ALL = 0x0A;
// all enabled channels acquire if channel 0 is fired (trigger propagation from channel 0 to all)
constexpr int COINC_CH0_TO_ALL = 0x0B;
// both channels of a couple acquire when one at least on of the two is fired (trigger propagation in the couple)
constexpr int COINC_PAIRED_OR = 0x0C;
// one channel of a couple acquires when the other one is not fired (anti coincidence)
constexpr int COINC_PAIRED_NAND = 0x82;
// any channel acquires is there is no coincidence with the ref. channel
constexpr int COINC_CH0_NAND_ANY = 0x83;

enum class FWCode {
  DPP_PSD,
  DPP_PHA,
  DPP_QDC,
  DPP_CI,
  STD,
  DPP_PHA_724,
  DPP_nPHA_724,
  DPP_PHA_730,
  DPP_PSD_720,
  DPP_PSD_730,
  DPP_PSD_751,
  Others,
};

class LinkPar
{
 public:
  LinkPar()
  {
    LinkType = CAEN_DGTZ_USB;
    LinkNum = 0;
    ConetNode = 0;
    VMEBaseAddress = 0;
  };
  LinkPar(CAEN_DGTZ_ConnectionType linkType, int linkNum = 0, int conetNode = 0,
          uint32_t vmeAddress = 0)
  {
    LinkType = linkType;
    LinkNum = linkNum;
    ConetNode = conetNode;
    VMEBaseAddress = vmeAddress;
  };
  ~LinkPar(){};

  CAEN_DGTZ_ConnectionType LinkType;
  int LinkNum;
  int ConetNode;
  uint32_t VMEBaseAddress;

 private:
};

class DigiPar
{
 public:
  DigiPar()
  {
    EnableMask = 0b1111111111111111;
    TrgoutMask = 0b1111111111111111;
    RecordLength = 256;
    PreTrigger = 20;
    IOLevel = CAEN_DGTZ_IOLevel_NIM;
    StartMode = START_MODE_INDEP_SW;
    SyncinMode = SYNCIN_MODE_RUN_CTRL;
    TrginMode = TRGIN_MODE_DISABLED;
    TrgoutMode = TRGOUT_MODE_SYNC_OUT;
    TrgHoldOff = 1;
    BrdID = 0;
    NumBrd = 1;
    EventBuffering = 0;
    CoincWindow = 100;
    CoincMode = COINC_DISABLED;
    AntiCoincidence = 0;
    MajorityLevel = 4;

    ChParInit();
  };
  ~DigiPar(){};

  uint32_t EnableMask;
  uint32_t TrgoutMask;
  int RecordLength;
  int PreTrigger;
  CAEN_DGTZ_IOLevel_t IOLevel;

  int StartMode;
  int SyncinMode;
  int TrginMode;
  int TrgoutMode;
  int TrgHoldOff;  // Trigger Hold off (in ns)
  int BrdID;       // 0 to 8?  ID for Sync
  int NumBrd;
  int EventBuffering;

  // Coincidences and trigger logic implemented in FPGA
  int CoincWindow;  // Coincidence Window (in ns)
  int CoincMode;  // Coincidence Mode (0=disabled, 1 = majority, 2=couples, 3=one_to_all, 4=ext_trg)
  int AntiCoincidence;  // When 1, it negates coincide logic (invert accepted and rejected events)
  int MajorityLevel;  // Min number of channels triggered to validate the majority

  // Ch settings
  void ChParInit()
  {
    for (auto i = 0; i < MAX_NCH; i++) {
      EnableInput[i] = 1;
      TrgThreshold[i] = 100;
      PulsePolarity[i] = CAEN_DGTZ_PulsePolarityNegative;
      // PulsePolarity[i] = CAEN_DGTZ_PulsePolarityPositive;
      BaselineDCoffset[i] = 10;
      ZeroVoltLevel[i] = 0;  // For STD FW
      VetoWindow[i] = 0.;
      Decimation[i] = 0;
    }
  };
  int EnableInput[MAX_NCH];   // Enable input
  int TrgThreshold[MAX_NCH];  // Trigger Threshold
  CAEN_DGTZ_PulsePolarity_t
      PulsePolarity[MAX_NCH];          // Pulse Polarity (0=pos, 1=neg)
  uint32_t BaselineDCoffset[MAX_NCH];  // 0 to 100
  int ZeroVoltLevel[MAX_NCH];
  float VetoWindow[MAX_NCH];

  int Decimation[MAX_NCH];  // 0=disabled, 1=1/2, 2=1/4, 3 = 1/8 (DPP_PHA)

 private:
};
#endif
