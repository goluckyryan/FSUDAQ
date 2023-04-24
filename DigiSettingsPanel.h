#ifndef DigiSettings_H
#define DigiSettings_H

#include <QMainWindow>
#include <QTabWidget>
#include <QLineEdit>
#include <QGridLayout>

#include "macro.h"
#include "ClassDigitizer.h"
#include "CustomWidgets.h"

class DigiSettingsPanel : public QMainWindow{
  Q_OBJECT

public:
  DigiSettingsPanel(Digitizer ** digi, unsigned int nDigi, QMainWindow * parent = nullptr);
  ~DigiSettingsPanel();

private slots:

signals:
  void SendLogMsg(const QString &msg);

private:

  void SetUpInfo(QString name, std::string value, QGridLayout *gLayout, int row, int col);

  Digitizer ** digi;
  unsigned int nDigi;

  bool enableSignalSlot;

  QTabWidget * tabWidget;

};


#endif