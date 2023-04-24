#include "DigiSettings.h"

DigiSettings::DigiSettings(Digitizer ** digi, unsigned int nDigi, QMainWindow *parent): QMainWindow(parent){

  this->digi = digi;
  this->nDigi = nDigi;

  enableSignalSlot = false;

  setWindowTitle("Digitizer Settings");
  setGeometry(0, 0, 1000, 800);  

}

DigiSettings::~DigiSettings(){
  printf("%s \n", __func__);

}
