#include "TParLoader.hpp"

#include <fstream>
#include <iostream>
#include <numeric>
#include <regex>
#include <string>

#include "MyTool.hpp"

TParLoader::TParLoader()
{
  fBoardID = -1;
  fChID = -1;
}

TParLoader::~TParLoader() {}

std::vector<Parameter_t> TParLoader::FromFile(std::string fileName)
{
  // Read from file
  // Read from DB is NYI
  auto fin = std::ifstream(fileName);
  if (!fin.is_open()) {
    std::cerr << "File " << fileName << " is not found." << std::endl;
    exit(1);
  }
  std::string buf;
  std::vector<std::vector<std::string>> parVec;
  while (!fin.eof()) {
    std::getline(fin, buf);
    if (buf[0] == '#' || buf[0] == '\r' || buf[0] == '\n') continue;

    // Sanitizing?  Original config file from digiTes has many garbages
    // auto test = std::regex_replace(buf, std::regex("[\\[\\]\t]"), " ");
    auto cleanBuf = std::regex_replace(buf, std::regex("[\\[\\]\t\r\n]"), " ");
    auto par = MyTool::split(cleanBuf);
    const auto nEle = par.size();
    for (auto iEle = 0; iEle < nEle; iEle++) {
      if (par[iEle][0] == '#') {
        par.resize(iEle);
        break;
      }
    }

    parVec.push_back(par);
  }
  fin.close();

  auto nBoards = CheckLink(parVec);
  std::cout << nBoards << " boards are in the config file " << fileName
            << std::endl;
  fParVec.resize(nBoards);

  CheckPar(parVec);

  return fParVec;
}

int TParLoader::CheckNBoards(std::vector<std::vector<std::string>> parVec)
{
  // Check BOARD Open statement and board number in parameters
  std::array<int, 256> counter;
  for (auto &&val : counter) val = 0;

  for (auto &&par : parVec) {
    if (par.size() < 3) continue;
    if (par[0] == "BOARD" && par[2] == "Open") counter[stoi(par[1])] = 1;
  }

  return std::accumulate(counter.begin(), counter.end(), 0);
}

int TParLoader::CheckLink(std::vector<std::vector<std::string>> parVec)
{
  // Check BOARD Open statement and board number in parameters
  constexpr int knBoards = 256;
  std::array<int, knBoards> counter;
  for (auto &&val : counter) val = 0;

  std::vector<LinkPar> linkVec(knBoards);

  for (auto &&par : parVec) {
    if (par.size() < 3) continue;
    if (par[0] == "BOARD" && par[2] == "Open") {
      counter[stoi(par[1])] += 1;
      CAEN_DGTZ_ConnectionType conType = CAEN_DGTZ_USB;
      if (par[3] == "USB") {
        conType = CAEN_DGTZ_USB;
      } else if (par[3] == "PCI") {
        conType = CAEN_DGTZ_OpticalLink;
      } else {
        std::cerr << "The link type " << par[3] << " is not supported.\n"
                  << "You can choose USB or PCI (PCI includes any NON USB "
                     "connections at this moment.)"
                  << std::endl;

        exit(1);
      }

      LinkPar link;
      if (par.size() == 6) {
        link = LinkPar(conType, stoi(par[4]), stoi(par[5]));
      } else if (par.size() == 7) {
        link = LinkPar(conType, stoi(par[4]), stoi(par[5]), stoi(par[6]));
      } else {
        std::cerr << "Check the board open statement.\n"
                  << "It looks like a wrong." << std::endl;
        exit(1);
      }

      linkVec[stoi(par[1])] = link;
    }
  }

  auto size = 0;
  for (auto i = 0; i < knBoards; i++) {
    if (counter[i] > 0) size++;

    if (counter[i] > 1) {
      std::cerr << "Configuration of board " << i << " is defined "
                << counter[i] << " times.\n"
                << "The last (lowest) line is used." << std::endl;
    }
  }
  fParVec.resize(size);

  for (auto i = 0; i < knBoards; i++) {
    if (counter[i] > 0) {
      if (i < fParVec.size())
        fParVec[i].Link = linkVec[i];
      else {
        std::cerr << "The Board numbers have to be continuous "
                  << "(0, 1, 2, or 2, 0, 1, ...)"
                  << " without missing numbers." << std::endl;
        exit(1);
      }
    }
  }

  return size;
}

int TParLoader::CheckPar(std::vector<std::vector<std::string>> parVec)
{
  for (auto &&par : parVec) {
    // Taking (key, value) pair or simple [control] statement only
    if (par.size() > 2) continue;
    // if (par[0] == "BOARD" && par[2] == "Open") counter[stoi(par[1])] = 1;
    for (auto &&val : par) std::cout << val << "\t";
    std::cout << std::endl;
  }

  return 0;
}