#ifndef TParLoader_hpp
#define TParLoader_hpp 1

#include <string>
#include <vector>

#include "DigiPar.hpp"

class TParLoader
{
 public:
  TParLoader();
  ~TParLoader();

  std::vector<Parameter_t> FromFile(
      std::string fileName = "digiTES_Config.txt");

 private:
  std::vector<Parameter_t> fParVec;

  int fBoardID;
  int fChID;

  // Check functions also set parameters into fParVec
  int CheckNBoards(const std::vector<std::vector<std::string>> parVec);
  int CheckLink(const std::vector<std::vector<std::string>> parVec);
  int CheckPar(const std::vector<std::vector<std::string>> parVec);
};

#endif
