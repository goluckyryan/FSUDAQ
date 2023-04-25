#ifndef DigiSettings_H
#define DigiSettings_H

#include <QMainWindow>
#include <QTabWidget>
#include <QLineEdit>
#include <QGridLayout>
#include <QCheckBox>
#include <QGroupBox>
#include <QPushButton>

#include "macro.h"
#include "ClassDigitizer.h"
#include "CustomWidgets.h"

class DigiSettingsPanel : public QMainWindow{
  Q_OBJECT

public:
  DigiSettingsPanel(Digitizer ** digi, unsigned int nDigi, QMainWindow * parent = nullptr);
  ~DigiSettingsPanel();

private slots:
  void UpdatePanelFromMemory();
  void ReadSettingsFromBoard();


  void SetAutoDataFlush();

signals:
  void SendLogMsg(const QString &msg);


private:

  void SetUpInfo(QString label, std::string value, QGridLayout *gLayout, int row, int col);
  void SetUpCheckBox(QCheckBox * &chkBox, QString label, QGridLayout *gLayout, int row, int col, Register::Reg para, std::pair<unsigned short, unsigned short> bit);
  void SetUpComboBox(RComboBox * &cb, QString label, QGridLayout *gLayout, int row, int col, std::vector<std::pair<QString, int>> items);

  void CleanUpGroupBox(QGroupBox * & gBox);
  void SetUpPHABoard();
  void SetUpPSDBoard();

  Digitizer ** digi;
  unsigned int nDigi;
  unsigned short ID;

  bool enableSignalSlot;

  QTabWidget * tabWidget;

  QGroupBox * infoBox[MaxNDigitizer];
  QGridLayout * infoLayout[MaxNDigitizer];

  QPushButton * bnRefreshSetting; // read setting from board

  QGroupBox * boardSettingBox[MaxNDigitizer];
  QGridLayout * settingLayout[MaxNDigitizer];

  QCheckBox * chkAutoDataFlush[MaxNDigitizer];
  QCheckBox * chkDecimateTrace[MaxNDigitizer];
  QCheckBox * chkTrigPropagation[MaxNDigitizer];
  QCheckBox * chkDualTrace[MaxNDigitizer];
  QCheckBox * chkTraceRecording[MaxNDigitizer];
  QCheckBox * chkEnableExtra2[MaxNDigitizer];

  RComboBox * cbAnaProbe1[MaxNDigitizer];
  RComboBox * cbAnaProbe2[MaxNDigitizer];
  RComboBox * cbDigiProbe1[MaxNDigitizer];
  RComboBox * cbDigiProbe2[MaxNDigitizer];

  RComboBox * cbAggOrg[MaxNDigitizer];


};


#endif