#ifndef HISTOGRAM_2D_H
#define HISTOGRAM_2D_H

#include "qcustomplot.h"
#include "macro.h"

const QList<QPair<QColor, QString>> colorCycle = { {QColor(Qt::red), "Red"},
                                                   {QColor(Qt::blue), "Blue"},
                                                   {QColor(Qt::darkGreen), "Dark Geen"},
                                                   {QColor(Qt::darkCyan), "Dark Cyan"}, 
                                                   {QColor(Qt::darkYellow), "Drak Yellow"}, 
                                                   {QColor(Qt::magenta),  "Magenta"},
                                                   {QColor(Qt::darkMagenta), "Dark Magenta"},
                                                   {QColor(Qt::gray), "Gray"}}; 


//^==============================================
//^==============================================
class Histogram2D : public QCustomPlot{
  
public:
  Histogram2D(QString title, QString xLabel, QString yLabel, 
              int xbin, double xmin, double xmax, 
              int ybin, double ymin, double ymax, 
              QWidget * parent = nullptr,
              QString defaultPath = QDir::homePath());

  void SetXTitle(QString xTitle) { xAxis->setLabel(xTitle);}
  void SetYTitle(QString yTitle) { yAxis->setLabel(yTitle);}
  void Rebin(int xbin, double xmin, double xmax, int ybin, double ymin, double ymax);
  void RebinY(int ybin, double ymin, double ymax);

  void SetChannelMap(bool onOff, int tickStep = 1) { isChannelMap = onOff; this->tickStep = tickStep;}

  void UpdatePlot(){ colorMap->rescaleDataRange(); replot(); }
  void Clear(); // Clear Data and histrogram

  void Fill(double x, double y);

  void DrawCut();
  void ClearAllCuts();

  QList<QPolygonF> GetCutList() const{return cutList;} // this list may contain empty element
  QList<int> GetCutEntryList() const{ return cutEntryList;}
  QList<QString> GetCutNameList() const { return cutNameList;}
  void PrintCutEntry() const;

  double GetXNBin() const {return xBin;}
  double GetXMin()  const {return xMin;}
  double GetXMax()  const {return xMax;}
  double GetYNBin() const {return yBin;}
  double GetYMin()  const {return yMin;}
  double GetYMax()  const {return yMax;}

  void SaveCuts(QString cutFileName);
  void LoadCuts(QString cutFileName);

private:
  double xMin, xMax, yMin, yMax;
  int xBin, yBin;

  bool isChannelMap; 
  int tickStep;
  bool isLogZ;

  QCPColorMap * colorMap;
  QCPColorScale *colorScale;

  bool usingMenu;

  int entry[3][3]; // overflow counter, entrt[1][1] is entry in the plot;

  QCPItemRect * box[3][3];
  QCPItemText * txt[3][3];
  
  QPolygonF tempCut;
  int tempCutID; // only incresing;
  int numCut;
  QList<QPolygonF> cutList;
  QList<QString> cutNameList; // name of the cut
  QList<int> cutEntryList;   // number of entry inside the cut.
  QList<int> cutIDList;      // ID of the cut
  QList<int> cutTextIDList;  // 
  QList<int> plottableIDList; 
  bool isDrawCut;
  int lastPlottableID;

  QCPItemLine * line;
  double oldMouseX = 0.0, oldMouseY = 0.0;

  bool isBusy;

  void rightMouseClickMenu(QMouseEvent * event);
  void rightMouseClickRebin();

  QString settingPath;

};

//^###############################################
//^###############################################

inline Histogram2D::Histogram2D(QString title, QString xLabel, QString yLabel, int xbin, double xmin, double xmax, int ybin, double ymin, double ymax, QWidget * parent, QString defaultPath) : QCustomPlot(parent){
  // DebugPrint("%s", "Histogram2D");

  settingPath = defaultPath;
  for( int i = 0; i < 3; i ++ ){
    for( int j = 0; j < 3; j ++ ){
      box[i][j] = nullptr;
      txt[i][j] = nullptr;       
    }
  }

  isChannelMap = false;
  tickStep = 1; // only used when isChannelMap = true
  isLogZ = false;

  axisRect()->setupFullAxesBox(true);
  xAxis->setLabel(xLabel);
  yAxis->setLabel(yLabel);

  colorMap = new QCPColorMap(xAxis, yAxis);
  Rebin(xbin, xmin, xmax, ybin, ymin, ymax);
  colorMap->setInterpolate(false);

  QCPTextElement *titleEle = new QCPTextElement(this, title, QFont("sans", 12));
  plotLayout()->insertRow(0);
  plotLayout()->addElement(0, 0, titleEle);

  colorScale = new QCPColorScale(this);
  plotLayout()->addElement(1, 1, colorScale); 
  colorScale->setType(QCPAxis::atRight); 
  colorMap->setColorScale(colorScale);


  QCPColorGradient color;
  color.setNanHandling(QCPColorGradient::NanHandling::nhNanColor);
  color.setNanColor(QColor("white"));
  color.clearColorStops();
  // color.setColorStopAt( 0.0, QColor("white" ));
  color.setColorStopAt( 0.0, QColor("purple" ));
  color.setColorStopAt( 0.2, QColor("blue"));
  color.setColorStopAt( 0.4, QColor("cyan"));
  color.setColorStopAt( 0.6, QColor("green"));
  color.setColorStopAt( 0.8, QColor("yellow"));
  color.setColorStopAt( 1.0, QColor("red"));
  colorMap->setGradient(color);

  double xPosStart = 0.02;
  double xPosStep = 0.07;
  double yPosStart = 0.02;
  double yPosStep = 0.05;

  for( int i = 0; i < 3; i ++ ){
    for( int j = 0; j < 3; j ++ ){
      box[i][j] = new QCPItemRect(this);

      box[i][j]->topLeft->setType(QCPItemPosition::ptAxisRectRatio);
      box[i][j]->topLeft->setCoords(xPosStart + xPosStep*i, yPosStart + yPosStep*j);
      box[i][j]->bottomRight->setType(QCPItemPosition::ptAxisRectRatio);
      box[i][j]->bottomRight->setCoords(xPosStart + xPosStep*(i+1), yPosStart + yPosStep*(j+1));

      txt[i][j] = new QCPItemText(this);
      txt[i][j]->setPositionAlignment(Qt::AlignLeft);
      txt[i][j]->position->setType(QCPItemPosition::ptAxisRectRatio);
      txt[i][j]->position->setCoords(xPosStart + xPosStep/2 + xPosStep*i, yPosStart + yPosStep*j);;
      txt[i][j]->setText("0");
      txt[i][j]->setFont(QFont("Helvetica", 9));

    }
  }

  cutList.clear();
  cutEntryList.clear();

  rescaleAxes();

  usingMenu = false;
  isDrawCut = false;
  tempCutID = -1;
  numCut = 0;
  lastPlottableID = -1;

  line = new QCPItemLine(this);
  line->setPen(QPen(Qt::gray, 1, Qt::DashLine));
  line->setVisible(false);

  isBusy = false;

  connect(this, &QCustomPlot::mouseMove, this, [=](QMouseEvent *event){
    double x = xAxis->pixelToCoord(event->pos().x());
    double y = yAxis->pixelToCoord(event->pos().y());
    int xI, yI;
    colorMap->data()->coordToCell(x, y, &xI, &yI);
    double z = colorMap->data()->cell(xI, yI);

    QString coordinates = QString("X: %1, Y: %2, Z: %3").arg(x).arg(y).arg(z);
    QToolTip::showText(event->globalPosition().toPoint(), coordinates, this);

    //when drawing cut, show dashhed line
    if( isDrawCut && tempCut.size() > 0  ){ 
      line->end->setCoords(x,y); 
      line->setVisible(true); 
      replot();
    }

  });

  connect(this, &QCustomPlot::mousePress, this, [=](QMouseEvent * event){
    
    if (event->button() == Qt::LeftButton && !usingMenu && !isDrawCut){
      setSelectionRectMode(QCP::SelectionRectMode::srmZoom);
    }

    if (event->button() == Qt::LeftButton && isDrawCut){

      oldMouseX = xAxis->pixelToCoord(event->pos().x());
      oldMouseY = yAxis->pixelToCoord(event->pos().y());

      tempCut.push_back(QPointF(oldMouseX,oldMouseY));

      line->start->setCoords(oldMouseX, oldMouseY);
      line->end->setCoords(oldMouseX, oldMouseY);
      line->setVisible(true);

      DrawCut();
    }

    //^================= right click
    if (event->button() == Qt::RightButton) rightMouseClickMenu(event);
  });

  //connect( this, &QCustomPlot::mouseDoubleClick, this, [=](QMouseEvent *event){
  connect( this, &QCustomPlot::mouseDoubleClick, this, [=](){
    if( isDrawCut) {
      tempCut.push_back(tempCut[0]);
      DrawCut();
      isDrawCut = false;
      line->setVisible(false);

      plottableIDList.push_back(plottableCount() -1 );

      cutNameList.push_back("Cut-" + QString::number(cutList.count()));
      cutEntryList.push_back(0);

      QCPItemText * text = new QCPItemText(this);
      text->setText(cutNameList.last());
      text->position->setCoords(tempCut[0].rx(), tempCut[0].ry());
      int colorID = tempCutID% colorCycle.count();
      text->setColor(colorCycle[colorID].first);
      cutTextIDList.push_back(itemCount() - 1);

      replot();

      cutList.push_back(tempCut);
      cutIDList.push_back(tempCutID);

      // qDebug() << "----------- end of create cut";
      // qDebug() << "      cutIDList " << cutIDList ;
      // qDebug() << "plottableIDList " << plottableIDList << ", " << plottableCount();
      // qDebug() << "  cutTextIDList "  << cutTextIDList << ", " << itemCount();

    }
  });

  connect(this, &QCustomPlot::mouseRelease, this, [=](){

  });
}



inline void Histogram2D::Fill(double x, double y){
  // DebugPrint("%s", "Histogram2D");
  if( isBusy ) return;
  int xIndex, yIndex;
  colorMap->data()->coordToCell(x, y, &xIndex, &yIndex);
  //printf("%f,  %d  %d| %f, %d %d\n", x, xIndex, xBin, y, yIndex, yBin);

  int xk = 1, yk = 1;
  if( xIndex < 0 ) xk = 0;
  if( xIndex >= xBin ) xk = 2;
  if( yIndex < 0 ) yk = 2;
  if( yIndex >= yBin ) yk = 0;
  entry[xk][yk] ++;

  txt[xk][yk]->setText(QString::number(entry[xk][yk]));

  if( xk == 1 && yk == 1 ) {
    double value = colorMap->data()->cell(xIndex, yIndex);
    if( std::isnan(value) ){
      colorMap->data()->setCell(xIndex, yIndex, 1);
    }else{
      colorMap->data()->setCell(xIndex, yIndex, value + 1);
    }

    for( int i = 0; i < cutList.count(); i++){
      if( cutList[i].isEmpty() ) continue;
      if( cutList[i].containsPoint(QPointF(x,y), Qt::OddEvenFill) ) cutEntryList[i] ++;
    }
  }
}

inline  void Histogram2D::Rebin(int xbin, double xmin, double xmax, int ybin, double ymin, double ymax){
  // DebugPrint("%s", "Histogram2D");
  xMin = xmin;
  xMax = xmax;
  yMin = ymin;
  yMax = ymax;
  xBin = xbin + 2;
  yBin = ybin + 2;

  colorMap->data()->clear();
  colorMap->data()->setSize(xBin, yBin);
  colorMap->data()->setRange(QCPRange(xMin, xMax), QCPRange(yMin, yMax));

  for( int i = 0; i < xBin; i++){
    for( int j = 0; j < yBin; j++){
      colorMap->data()->setCell(i, j, NAN);
    }
  }

  if( isChannelMap ){
    QCPAxis * xAxis = colorMap->keyAxis();
    xAxis->ticker()->setTickCount(xbin/tickStep);
    xAxis->ticker()->setTickOrigin(0);
  }

  for( int i = 0; i < 3; i ++){
    for( int j = 0; j < 3; j ++){
      entry[i][j] = 0;
      if( txt[i][j] ) txt[i][j]->setText("0");
    }
  }

  rescaleAxes();
  UpdatePlot();

}

inline  void Histogram2D::RebinY(int ybin, double ymin, double ymax){
  Rebin(xBin-2, xMin, xMax, ybin, ymin, ymax);
}

inline void Histogram2D::Clear(){
  DebugPrint("%s", "Histogram2D");
  for( int i = 0; i < 3; i ++){
    for( int j = 0; j < 3; j ++){
      entry[i][j] = 0;
      txt[i][j]->setText("0");
    }
  }
  colorMap->data()->clear();
  colorMap->data()->setSize(xBin, yBin);
  colorMap->data()->setRange(QCPRange(xMin, xMax), QCPRange(yMin, yMax));
  for( int i = 0; i < xBin; i++){
    for( int j = 0; j < yBin; j++){
      colorMap->data()->setCell(i, j, NAN);
    }
  }

  UpdatePlot();
}


inline void Histogram2D::ClearAllCuts(){
  DebugPrint("%s", "Histogram2D");
  numCut = 0;
  tempCutID = -1;
  lastPlottableID = -1;
  cutList.clear();
  cutIDList.clear();
  for( int i = cutTextIDList.count() - 1; i >= 0 ; i--){
    if( cutTextIDList[i] < 0 ) continue;
    removeItem(cutTextIDList[i]);
    removePlottable(plottableIDList[i]);
  }
  replot();

  cutTextIDList.clear();
  plottableIDList.clear();
  cutNameList.clear();
  cutEntryList.clear();
}

inline void Histogram2D::PrintCutEntry() const{
  DebugPrint("%s", "Histogram2D");
  if( numCut == 0 ) return;
  printf("=============== There are %d cuts. (%lld, %lld)\n", numCut, cutList.count(), cutEntryList.count());
  for( int i = 0; i < cutList.count(); i++){
    if( cutList[i].isEmpty() ) continue;
    printf("%10s | %d \n", cutNameList[i].toStdString().c_str(), cutEntryList[i]);  
  }
}

inline void Histogram2D::DrawCut(){
  DebugPrint("%s", "Histogram2D");
  //The histogram is the 1st plottable.
  // the lastPlottableID should be numCut+ 1
  if( lastPlottableID != numCut ){
    removePlottable(lastPlottableID);
  }
  
  if(tempCut.size() > 0) {
      QCPCurve *polygon = new QCPCurve(xAxis, yAxis);
      lastPlottableID = plottableCount() - 1;

      int colorID = tempCutID% colorCycle.count();
      QPen pen(colorCycle[colorID].first);
      pen.setWidth(1);
      polygon->setPen(pen);

      QVector<QCPCurveData> dataPoints;
      for (const QPointF& point : tempCut) {
          dataPoints.append(QCPCurveData(dataPoints.size(), point.x(), point.y()));
      }
      polygon->data()->set(dataPoints, false);
  }
  replot();

  // qDebug() << "Plottable count : " <<  plottableCount() << ", cutList.count :" << cutList.count() << ", cutID :" << lastPlottableID;
}

inline void Histogram2D::rightMouseClickMenu(QMouseEvent * event){
  DebugPrint("%s", "Histogram2D");
  usingMenu = true;
  setSelectionRectMode(QCP::SelectionRectMode::srmNone);

  QMenu *menu = new QMenu(this);
  menu->setAttribute(Qt::WA_DeleteOnClose);

  QAction * a1 = menu->addAction("UnZoom");
  QAction * a6 = menu->addAction("Set/UnSet Log-Z");
  QAction * a2 = menu->addAction("Clear hist.");
  QAction * a3 = menu->addAction("Toggle Stat.");
  QAction * a4 = menu->addAction("Rebin (clear histogram)");
  QAction * a8 = menu->addAction("Load Cut(s)");
  QAction * a5 = menu->addAction("Create a Cut");
  QAction * a7 = nullptr;
  if( numCut > 0 ) {
    a7 = menu->addAction("Save Cut(s)");
    menu->addSeparator();
    menu->addAction("Add/Edit names to Cuts");
    menu->addAction("Clear all Cuts");
  }
  for( int i = 0; i < cutList.size(); i++){
    if( cutList[i].isEmpty()) continue;
    QString haha = "";
    menu->addAction("Delete " + cutNameList[i] + " ["+ colorCycle[cutIDList[i]%colorCycle.count()].second+"]");
  }

  QAction *selectedAction = menu->exec(event->globalPosition().toPoint());

  if( selectedAction == a1 ){
    xAxis->setRangeLower(xMin);
    xAxis->setRangeUpper(xMax);
    yAxis->setRangeLower(yMin);
    yAxis->setRangeUpper(yMax);
    replot();
    usingMenu = false;
    return;
  }

  if( selectedAction == a2 ) {
    Clear();
    usingMenu = false;
    return;
  }

  if( selectedAction == a3 ){
    for( int i = 0; i < 3; i ++ ){
      for( int j = 0; j < 3; j ++ ){
        box[i][j]->setVisible( !box[i][j]->visible());
        txt[i][j]->setVisible( !txt[i][j]->visible());
      }
    }
    replot();
    usingMenu = false;
    return;
  }

  if( selectedAction == a4){
    rightMouseClickRebin();
    usingMenu = false;
    return;
  }

  if( selectedAction == a5 ){
    tempCut.clear();
    tempCutID ++;
    isDrawCut= true;
    usingMenu = false;
    numCut ++;
    return;
  }

  if( selectedAction == a6){
    if( !isLogZ ){
      colorMap->setDataScaleType(QCPAxis::stLogarithmic);
      isLogZ = true;
    }else{
      colorMap->setDataScaleType(QCPAxis::stLinear);
      isLogZ = false;
    }
    replot();
  }

  if( selectedAction && numCut > 0 && selectedAction->text().contains("Delete ") ){

    QString haha = selectedAction->text();
    int index1 = haha.indexOf("-");
    int index2 = haha.indexOf("[");
    int cutID = haha.mid(index1+1, index2-index1-1).remove(' ').toInt();
    
    removeItem(cutTextIDList[cutID]);
    removePlottable(plottableIDList[cutID]);
    replot();

    numCut --;
    cutList[cutID].clear();
    cutIDList[cutID] = -1;
    cutTextIDList[cutID] = -1;
    plottableIDList[cutID] = -1;
    cutNameList[cutID] = "";
    cutEntryList[cutID] = -1;

    for( int i = cutID + 1; i < cutTextIDList.count() ; i++){
        cutTextIDList[i] --;
        plottableIDList[i] --;
    }

    if( numCut == 0 ){
      tempCutID = -1;
      lastPlottableID = -1;
      cutList.clear();
      cutIDList.clear();
      cutTextIDList.clear();
      plottableIDList.clear();
      cutNameList.clear();
      cutEntryList.clear();
    }
    return;
  }

  if( selectedAction && numCut > 0 && selectedAction->text().contains("Clear all Cuts") ){
    ClearAllCuts();
    return;
  }

  if( selectedAction && numCut > 0 && selectedAction->text().contains("Add/Edit names to Cuts") ){
    
    QDialog dialog(this);
    dialog.setWindowTitle("Add/Edit name of cuts ");

    QFormLayout layout(&dialog);

    for(int i = 0; i < cutTextIDList.count(); i++){
      if( cutTextIDList[i] < 0 ) continue;
      QLineEdit * le = new QLineEdit(&dialog);
      layout.addRow(colorCycle[i%colorCycle.count()].second, le);
      le->setText( cutNameList[i] );
      connect(le, &QLineEdit::textChanged, this, [=](){
        le->setStyleSheet("color : blue;");
      });
      connect(le, &QLineEdit::returnPressed, this, [=](){
        le->setStyleSheet("");
        cutNameList[i] = le->text();
        ((QCPItemText *) this->item(cutTextIDList[i]))->setText(le->text());
        replot();
      });
    }
    dialog.exec();
    return;
  }

  if( selectedAction == a8 ){ // load Cuts
    QString filePath = QFileDialog::getOpenFileName(this, 
                                                "Load Cuts from File", 
                                                settingPath, 
                                                "Text file (*.txt)");

    if (!filePath.isEmpty()) LoadCuts(filePath);      

  }

  if( selectedAction == a7 ){ // Save Cuts
    
    QString filePath = QFileDialog::getSaveFileName(this, 
                                                    "Save Cuts to File", 
                                                    settingPath, 
                                                    "Text file (*.txt)");

    if (!filePath.isEmpty()) SaveCuts(filePath);

  }


}

inline void Histogram2D::rightMouseClickRebin(){
  DebugPrint("%s", "Histogram2D");
  QDialog dialog(this);
  dialog.setWindowTitle("Rebin histogram");

  QFormLayout layout(&dialog);

  QLabel * info = new QLabel(&dialog);
  info->setStyleSheet("color:red;");
  info->setText("This will also clear histogram!!");
  layout.addRow(info);

  QStringList nameListX = {"Num. x-Bin", "x-Min", "x-Max"};
  QLineEdit* lineEditX[3];
  for (int i = 0; i < 3; ++i) {
      lineEditX[i] = new QLineEdit(&dialog);
      layout.addRow(nameListX[i] + " : ", lineEditX[i]);
  }
  lineEditX[0]->setText(QString::number(xBin-2));
  lineEditX[1]->setText(QString::number(xMin));
  lineEditX[2]->setText(QString::number(xMax));

  QStringList nameListY = {"Num. y-Bin", "y-Min", "y-Max"};
  QLineEdit* lineEditY[3];
  for (int i = 0; i < 3; ++i) {
      lineEditY[i] = new QLineEdit(&dialog);
      layout.addRow(nameListY[i] + " : ", lineEditY[i]);
  }
  lineEditY[0]->setText(QString::number(yBin-2));
  lineEditY[1]->setText(QString::number(yMin));
  lineEditY[2]->setText(QString::number(yMax));

  QLabel * msg = new QLabel(&dialog);
  msg->setStyleSheet("color:red;");
  layout.addRow(msg);

  QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
  layout.addRow(&buttonBox);

  double number[3][2];

  QObject::connect(&buttonBox, &QDialogButtonBox::accepted, [&]() {
      int OKcount = 0;
      bool conversionOk = true;
      for( int i = 0; i < 3; i++ ){
        number[i][0] = lineEditX[i]->text().toDouble(&conversionOk);
        if( conversionOk ){
          OKcount++;
        }else{
          msg->setText(nameListX[i] + " is invalid.");
          return;
        }
      }
      for( int i = 0; i < 3; i++ ){
        number[i][1] = lineEditY[i]->text().toDouble(&conversionOk);
        if( conversionOk ){
          OKcount++;
        }else{
          msg->setText(nameListY[i] + " is invalid.");
          return;
        }
      }

      if( OKcount == 6 ) {
        if( number[0][0] <= 0 ) {
          msg->setText( nameListX[0] + " is zero or negative" );
          return;
        }
        if( number[0][0] <= 0 ) {
          msg->setText( nameListX[0] + " is zero or negative" );
          return;
        }

        if( number[2][0] > number[1][0] && number[2][1] > number[1][1]  ) {
          dialog.accept();
        }else{
          if( number[2][0] > number[1][0] ){
            msg->setText(nameListX[2] + " is smaller than " + nameListX[1]);
          }
          if( number[2][1] > number[1][1] ){
            msg->setText(nameListY[2] + " is smaller than " + nameListY[1]);
          }
        }
      }
  });

  QObject::connect(&buttonBox, &QDialogButtonBox::rejected, [&]() { dialog.reject();});

  if( dialog.exec() == QDialog::Accepted ){
    isBusy = true;
    Rebin((int)number[0][0], number[1][0], number[2][0], (int)number[0][1], number[1][1], number[2][1]);
    isBusy = false;
  }

}

inline void Histogram2D::SaveCuts(QString cutFileName){

  QFile file(cutFileName);
  if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QTextStream out(&file);

    // Define the text to write
    QStringList lines;

    for( int i = 0; i < cutList.size(); i++){
      lines << "====== "+ cutNameList[i];
      for( int pt = 0 ; pt < cutList[i].size(); pt ++){
        lines << QString::number(cutList[i][pt].rx(), 'g', 5) + "," +  QString::number(cutList[i][pt].ry(), 'g', 5);
      }
    }

    lines << "#===== End of File";

    // Write each line to the file
    for (const QString &line : lines) out << line << "\n";

    // Close the file
    file.close();
    qDebug() << "File written successfully to" << cutFileName;
  }else{
    qWarning() << "Unable to open file" << cutFileName;
  }

}

inline void Histogram2D::LoadCuts(QString cutFileName){

  QFile file(cutFileName);
  QString cutNameTemp;

  // Open the file in read mode
  if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      QTextStream in(&file);

      ClearAllCuts();
      tempCut.clear();

      // Read each line and append to the QStringList

      while (!in.atEnd()) {
        QString line = in.readLine();

        if( line.contains("======") ){
          if( !tempCut.isEmpty() ) {
            DrawCut();
            plottableIDList.push_back(plottableCount() -1 );
            cutNameList.push_back(cutNameTemp);
            cutEntryList.push_back(0);
            QCPItemText * text = new QCPItemText(this);
            text->setText(cutNameList.last());
            text->position->setCoords(tempCut[0].rx(), tempCut[0].ry());
            int colorID = tempCutID% colorCycle.count();
            text->setColor(colorCycle[colorID].first);
            cutTextIDList.push_back(itemCount() - 1);
            // cutList.push_back(tempCut);
            cutIDList.push_back(tempCutID);
          }
          tempCut.clear();
          tempCutID ++;
          numCut ++;

          int spacePos = line.indexOf(' ');
          cutNameTemp = line.mid(spacePos + 1);
          continue;
        }

        if( line.contains("#==") ) {
          DrawCut();
          plottableIDList.push_back(plottableCount() -1 );
          cutNameList.push_back(cutNameTemp);
          cutEntryList.push_back(0);
          QCPItemText * text = new QCPItemText(this);
          text->setText(cutNameList.last());
          text->position->setCoords(tempCut[0].rx(), tempCut[0].ry());
          int colorID = tempCutID% colorCycle.count();
          text->setColor(colorCycle[colorID].first);
          cutTextIDList.push_back(itemCount() - 1);
          cutList.push_back(tempCut);
          cutIDList.push_back(tempCutID);
          break;
        }else{
          QStringList haha = line.split(",");
          // qDebug() << haha;
          if( haha.size() == 2 ){
            tempCut.push_back(QPointF(haha[0].toFloat(), haha[1].toFloat()));
            DrawCut();
          }
        }

      }

      // Close the file
      file.close();
      qDebug() << "File read successfully from" << cutFileName;

      // PrintCutEntry();
      // DrawCut();
      replot();

  } else {
      qWarning() << "Unable to open file" << cutFileName;
  }

}


#endif