#include <TApplication.h>
#include <TCanvas.h>
#include <TGraph.h>
#include <TH1.h>
#include <fcntl.h>
#include <termios.h>

#include <iostream>
#include <memory>
#include <vector>

#include "TDetector.hpp"
#include "TParLoader.hpp"

int kbhit(void)
{
  struct termios oldt, newt;
  int ch;
  int oldf;

  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

  ch = getchar();

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);

  if (ch != EOF) {
    ungetc(ch, stdin);
    return 1;
  }

  return 0;
}

int main(int argc, char *argv[])
{
  // If you use arguments, the information should be taken before this line.
  std::unique_ptr<TApplication> app{new TApplication("app", &argc, argv)};

  std::unique_ptr<TDetector> detector{new TDetector()};

  // TParLoader loader;
  // loader.FromFile("digiTES_Config.txt");

  std::vector<Parameter_t> parameters;
  Parameter_t par;
  par.Link = LinkPar(CAEN_DGTZ_USB, 0, 0, 0);
  par.Digitizer = DigiPar();
  parameters.push_back(par);
  detector->LoadParameter(parameters);

  if (!detector->CheckAndOpen()) {
    std::cerr << "Can not start" << std::endl;
    exit(0);
  };
  detector->Config();

  // For plot the data
  std::unique_ptr<TGraph> graph(new TGraph(1));
  graph->SetMaximum(18000);
  graph->SetMinimum(0);
  std::unique_ptr<TH1D> hist(
      new TH1D("hist", "Energy distribution", 30000, 0.5, 30000.5));
  std::unique_ptr<TCanvas> canv(new TCanvas("canv", "test", 1200, 500));
  canv->Divide(2, 1);
  canv->cd(1);
  graph->Draw("AL");
  canv->cd(2);
  hist->Draw();
  canv->cd();
  canv->Modified();
  canv->Update();

  detector->Start();

  // for (auto i = 0; i < 100; i++) {
  for (auto i = 0; true; i++) {
    // detector->SendSWTrigger(100);
    detector->ReadEvents();
    auto data = detector->GetData();

    const int nEve = data->size();
    for (auto iEve = 0; iEve < nEve; iEve++) {
      auto eve = (*data)[iEve];

      if (eve->ChNumber == 3) {
        hist->Fill(eve->ChargeLong);
        const auto nPoint = eve->Trace1.size();
        const auto timeStep = eve->SamplingPeriod;
        for (auto iPoint = 0; iPoint < nPoint; iPoint++) {
          graph->SetPoint(iPoint, iPoint * timeStep, eve->Trace1[iPoint]);
        }
      }
    }
    canv->cd(1);
    graph->Draw("AL");
    canv->cd(2);
    hist->Draw();
    canv->cd();
    canv->Modified();
    canv->Update();

    if (kbhit()) break;
    usleep(1000);
  }

  detector->Stop();

  detector->Unconfig();

  // app->Run();

  return 0;
}
