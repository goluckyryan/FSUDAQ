#ifndef DigiSettings_H
#define DigiSettings_H

#include <QMainWindow>

#include "macro.h"
#include "ClassDigitizer.h"
#include "CustomWidgets.h"

class DigiSettings : public QMainWindow{
  Q_OBJECT

public:
  DigiSettings(Digitizer ** digi, unsigned int nDigi, QMainWindow * parent = nullptr);
  ~DigiSettings();

private slots:

signals:
  void SendLogMsg(const QString &msg);

private:

  Digitizer ** digi;
  unsigned int nDigi;

  bool enableSignalSlot;

};


#endif