/***********************************************************************
 * 
 *  This is Isotope.h, To extract the isotope mass from massXX.txt
 * 
 *-------------------------------------------------------
 *  created by Ryan (Tsz Leung) Tang, Nov-18, 2018
 *  email: goluckyryan@gmail.com
 * ********************************************************************/


#ifndef ISOTOPE_H
#define ISOTOPE_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>  //atoi
#include <algorithm>
using namespace std;

const double mp = 938.2720813; // MeV/c^2
const double mn = 939.56542052; // MeV/c^2
const double amu = 931.0;

const string massData="analyzers/mass20.txt";

// about the mass**.txt
// Mass Excess = (ATOMIC MASS - A)*amu | e.g. n : (1.088664.91585E-6-1)*amu
// mass excess uncertaintly
// BEA = (Z*M(1H) + N*M(1n) - Me(A,Z))/A , Me is the mass with electrons
// BEA = (Z*mp + N*mn - M(A,Z))/A , M is the mass without electrons

class Isotope {
public:
  int A, Z;
  double Mass, MassError, BEA;
  string Name, Symbol;
  string dataSource;
  
  Isotope(){findHeliosPath();};
  Isotope(int a, int z){ findHeliosPath();SetIso(a,z);  };
  Isotope(string name){ findHeliosPath(); SetIsoByName(name); };

  void SetIso(int a, int z);
  void SetIsoByName(string name);

  double CalSp(int Np, int Nn); // this for the Np-proton, Nn-neutron removal 
  double CalSp2(int a, int z); // this is for (a,z) nucleus removal

  double CalBeta(double T){
    //double Etot = Mass + T;
    double gamma = 1. + T/Mass;
    double beta = sqrt(1. - 1. / gamma / gamma ) ;
    return beta;
  }

  void Print();
  void ListShell();

  string GetMassTabelPath() const{ return dataSource;}
   
private:
  void FindMassByAZ(int a, int z); // give mass, massError, BEA, Name, Symbol;
  void FindMassByName(string name); // give Z, mass, massError, BEA;

  int TwoJ(int nShell);
  string Orbital(int nShell);
  int magic(int i){
    switch (i){
      case 0: return   2; break;
      case 1: return   8; break;
      case 2: return  20; break;
      case 3: return  28; break;
      case 4: return  40; break;
      case 5: return  50; break;
      case 6: return  82; break;
      case 7: return 128; break;
    }
    return 0;
  }

  int magicShellID(int i){
    switch (i){
      case 0: return   0; break;
      case 1: return   2; break;
      case 2: return   5; break;
      case 3: return   6; break;
      case 4: return   9; break;
      case 5: return  10; break;
      case 6: return  15; break;
      case 7: return  21; break;
    }
    return 0;
  }
  
  int fileStartLine;
  int fileEndLine;
  int lineMass050_099;
  int lineMass100_149;
  int lineMass150_199;
  int lineMass200;
  
  
  void setFileLines(){
    fileStartLine = 37;
    fileEndLine = 3594;
    
    lineMass050_099 = 466;
    lineMass100_149 = 1160;
    lineMass150_199 = 1994;
    lineMass200     = 2774;
  }
  
  char * heliosPath;
  bool isFindOnce;

  void findHeliosPath(){
    heliosPath = getenv("HELIOSSYS");
    if( heliosPath ){
      dataSource = heliosPath;
      dataSource += "/analysis" + massData;
    }else{
      dataSource =  massData;
    }
  }
  
};

inline void Isotope::SetIso(int a, int z){
    this->A = a;
    this->Z = z;
    FindMassByAZ(a,z); 
}

inline void Isotope::SetIsoByName(string name){
    FindMassByName(name); 
}

inline void Isotope::FindMassByAZ(int A, int Z){
  string line;
  int    lineNum=0;
  int    list_A, list_Z;

  ifstream myfile;
  int    flag=0;

  setFileLines();

  int numLineStart = fileStartLine;
  int numLineEnd  = fileEndLine;

  if ( A >= 50 && A < 100) numLineStart = lineMass050_099;
  if ( A >=100 && A < 150) numLineStart = lineMass100_149;
  if ( A >=150 && A < 200) numLineStart = lineMass150_199;
  if ( A >=200 ) numLineStart           = lineMass200;

  myfile.open(dataSource.c_str());

  if (myfile.is_open()) {
    while (/*! myfile.eof() &&*/ flag == 0 && lineNum <numLineEnd){
      lineNum ++ ;
      //printf("%3d  ",lineNum);
      getline (myfile,line);

      if (lineNum >= numLineStart ){
        list_Z     = atoi((line.substr(10,5)).c_str());
      	list_A     = atoi((line.substr(15,5)).c_str());

      	if ( A == list_A && Z == list_Z) {
            this->BEA       = atof((line.substr(54,11)).c_str());
      		this->Mass      = list_Z*mp + (list_A-list_Z)*mn - this->BEA/1000*list_A;
            this->MassError = atof((line.substr(65,7)).c_str());
            string str = line.substr(20,2);
            str.erase(remove(str.begin(), str.end(), ' '), str.end());
            this->Symbol    = str;
            
            ostringstream ss;
            ss << A << this->Symbol;
            this->Name      = ss.str();
     		   flag = 1;
      	}else if ( list_A > A) {
          this->BEA  = -404;
          this->Mass = -404;
          this->MassError = -404;
          this->Symbol = "non";
          this->Name   = "non";
          break;
        }

      }
    }
    
    if( this->Name == "1H" ) this->Name = "p";
    if( this->Name == "2H" ) this->Name = "d";
    if( this->Name == "3H" ) this->Name = "t";
    if( this->Name == "4He" ) this->Name = "a";
    
    myfile.close();
  }else {
    printf("Unable to open %s\n", dataSource.c_str());
  }
}

inline void Isotope::FindMassByName(string name){

    // done seperate the Mass number and the name 
  if( name == "n" ) {
    this->Name = "1n";
    this->BEA       = 0;
    this->Mass      = mn;
    this->MassError = 0;
    this->Name      = "n";
    this->A         = 1;
    this->Z         = 0;
    return;
  }
    if( name == "p" ) name = "1H";
    if( name == "d" ) name = "2H";
    if( name == "t" ) name = "3H";
    if( name == "a" ) name = "4He";
    
    string temp = name;
    int lastDigit = 0;

    for(int i=0; temp[i]; i++){
      if(temp[i] == '0') lastDigit =  i; 
      if(temp[i] == '1') lastDigit =  i; 
      if(temp[i] == '2') lastDigit =  i; 
      if(temp[i] == '3') lastDigit =  i; 
      if(temp[i] == '4') lastDigit =  i; 
      if(temp[i] == '5') lastDigit =  i; 
      if(temp[i] == '6') lastDigit =  i; 
      if(temp[i] == '7') lastDigit =  i; 
      if(temp[i] == '8') lastDigit =  i; 
      if(temp[i] == '9') lastDigit =  i; 
    }

    this->Symbol = temp.erase(0,  lastDigit +1);
    //check is Symbol is 2 charaters, if not, add " " at the end
    if( this->Symbol.length() == 1 ){
      this->Symbol = this->Symbol + " ";
    }


    temp = name;
    int len = temp.length();
    temp = temp.erase(lastDigit+1, len);
    
    this->A = atoi(temp.c_str());    
    //printf(" Symbol = |%s| , Mass = %d\n", this->Symbol.c_str(), this->A);

    // find the nucleus in the data
    string line;
    int    lineNum=0;
    int    list_A;
    string list_symbol;

    ifstream myfile;
    int    flag=0;

    setFileLines();

    int numLineStart = fileStartLine;
    int numLineEnd  = fileEndLine;

    if ( A >= 50 && A < 100) numLineStart = lineMass050_099;
    if ( A >=100 && A < 150) numLineStart = lineMass100_149;
    if ( A >=150 && A < 200) numLineStart = lineMass150_199;
    if ( A >=200 ) numLineStart           = lineMass200;

    myfile.open(dataSource.c_str());

    if (myfile.is_open()) {
      while (/*! myfile.eof() &&*/ flag == 0 && lineNum <numLineEnd){
        lineNum ++ ;
        //printf("%3d  ",lineNum);
        getline (myfile,line);

        if (lineNum >= numLineStart ){
          list_symbol  = line.substr(20,2);
          list_A       = atoi((line.substr(15,5)).c_str());

          //printf(" A = %d, Sym = |%s| \n", list_A, list_symbol.c_str());

          if ( this->A == list_A &&  this->Symbol == list_symbol) {
            this->Z         = atoi((line.substr(10,5)).c_str());
            this->BEA       = atof((line.substr(54,11)).c_str());
          	this->Mass      = this->Z*mp + (list_A-this->Z)*mn - this->BEA/1000*list_A;
            this->MassError = atof((line.substr(65,7)).c_str());
            
            string str = line.substr(20,2);
            str.erase(remove(str.begin(), str.end(), ' '), str.end());
            this->Symbol    = str;
            
            ostringstream ss;
            ss << this->A << this->Symbol;
            this->Name      = ss.str();
            flag = 1;
          }else if ( list_A > this->A) {
            this->BEA  = -404;
            this->Mass = -404;
            this->MassError = -404;
            this->Symbol = "non";
            this->Name   = "non";
            break;
          }

        }
      }
      myfile.close();
    }else {
      printf("Unable to open %s\n", dataSource.c_str());
    }
}

inline double Isotope::CalSp(int Np, int Nn){
  Isotope nucleusD(A - Np - Nn, Z - Np);  

  if( nucleusD.Mass != -404){
    return nucleusD.Mass + Nn*mn + Np*mp - this->Mass;
  }else{
    return -404;
  }
}

inline double Isotope::CalSp2(int a, int z){
  Isotope nucleusD(A - a , Z - z);
  Isotope nucleusS(a,z);  

  if( nucleusD.Mass != -404 && nucleusS.Mass != -404){
    return nucleusD.Mass + nucleusS.Mass - this->Mass;
  }else{
    return -404;
  }
}

inline int Isotope::TwoJ(int nShell){

  switch(nShell){
    case  0: return  1; break; // 0s1/2
    case  1: return  3; break; // 0p3/2
    case  2: return  1; break; // 0p1/2 -- 8
    case  3: return  5; break; // 0d5/2
    case  4: return  1; break; // 1s1/2
    case  5: return  3; break; // 0d3/2 -- 20
    case  6: return  7; break; // 0f7/2 -- 28
    case  7: return  3; break; // 1p3/2
    case  8: return  1; break; // 1p1/2
    case  9: return  5; break; // 0f5/2 -- 40
    case 10: return  9; break; // 0g9/2 -- 50
    case 11: return  7; break; // 0g7/2
    case 12: return  5; break; // 1d5/2
    case 13: return 11; break; // 0h11/2
    case 14: return  3; break; // 1d3/2
    case 15: return  1; break; // 2s1/2 -- 82
    case 16: return  9; break; // 0h9/2
    case 17: return  7; break; // 1f7/2
    case 18: return 13; break; // 0i13/2
    case 19: return  3; break; // 2p3/2
    case 20: return  5; break; // 1f5/2
    case 21: return  1; break; // 1p1/2 -- 126
    case 22: return  9; break; // 1g9/2
    case 23: return 11; break; // 0i11/2
    case 24: return 15; break; // 0j15/2
    case 25: return  5; break; // 2d5/2
    case 26: return  1; break; // 3s1/2
    case 27: return  3; break; // 2d3/2
    case 28: return  7; break; // 1g7/2
  }

  return 0;
}

inline string Isotope::Orbital(int nShell){

  switch(nShell){
    case  0: return  "0s1 "; break;  //
    case  1: return  "0p3 "; break;  //
    case  2: return  "0p1 "; break;  //-- 8
    case  3: return  "0d5 "; break;  //
    case  4: return  "1s1 "; break;  //
    case  5: return  "0d3 "; break;  //-- 20
    case  6: return  "0f7 "; break;  //-- 28
    case  7: return  "1p3 "; break;  //
    case  8: return  "1p1 "; break;  //
    case  9: return  "0f5 "; break;  //-- 40
    case 10: return  "0g9 "; break;  //-- 50
    case 11: return  "0g7 "; break;  //
    case 12: return  "1d5 "; break;  //
    case 13: return  "0h11"; break;  //
    case 14: return  "1d3 "; break;  //
    case 15: return  "2s1 "; break;  //-- 82
    case 16: return  "0h9 "; break;  //
    case 17: return  "1f7 "; break;  //
    case 18: return  "0i13"; break;  //
    case 19: return  "2p3 "; break;  //
    case 20: return  "1f5 "; break;  //
    case 21: return  "1p1 "; break;  //-- 126
    case 22: return  "1g9 "; break;  //
    case 23: return  "0i11"; break;  //
    case 24: return  "0j15"; break;  //
    case 25: return  "2d5 "; break;  //
    case 26: return  "3s1 "; break;  //
    case 27: return  "2d3 "; break;  //
    case 28: return  "1g7 "; break;  //
  }

  return "nan";
}

inline void Isotope::ListShell(){

  if( Mass < 0 ) return;

  int n = A-Z;
  int p = Z;

  int k = min(n,p);
  int nMagic = 0;
  for( int i = 0; i < 7; i++){
    if( magic(i) < k && k <= magic(i+1) ){
      nMagic = i;
      break;
    }
  }

  int coreShell = magicShellID(nMagic-1);
  int coreZ1 = magic(nMagic-1);
  int coreZ2 = magic(nMagic);

  Isotope core1( 2*coreZ1, coreZ1);
  Isotope core2( 2*coreZ2, coreZ2);

  printf("------------------ Core:%3s, inner Core:%3s \n", (core2.Name).c_str(), (core1.Name).c_str());
  printf("         || ");
  int t = max(n,p);
  int nShell = 0;
  do{
    int occ = TwoJ(nShell)+1;
    if( nShell > coreShell) {
      printf("%4s", Orbital(nShell).c_str());
         if( nShell == 0 || nShell == 2 || nShell == 5 || nShell ==6 || nShell == 9  || nShell == 10 || nShell == 15 || nShell == 21){
        printf("|");
      }else{
        printf(",");
      } 
    }
    t = t - occ;
    nShell++;
  }while( t > 0  && nShell < 29);
  for( int i = 1; i <= 6; i++){
    if (nShell < 28) {
      printf("%4s,", Orbital(nShell).c_str());
    }else if( nShell == 28 ) {
      printf("%4s", Orbital(nShell).c_str());
    }
    nShell ++;
  }
  if (nShell < 29) printf("%4s", Orbital(nShell).c_str());
  printf("\n");


  printf(" Z = %3d || ", p);
  nShell = 0;
  do{
    int occ = TwoJ(nShell)+1;
    if( nShell > coreShell ){
      if( p > occ ) {
         printf("%-4d", occ);
         if( nShell == 0 || nShell == 2 || nShell == 5 || nShell ==6 || nShell == 9  || nShell == 10 || nShell == 15 || nShell == 21){
          printf("|");
        }else{
          printf(",");
        } 
      }else{
        printf("%-4d", p);
      }
    }
    p = p - occ;
    nShell++;
  }while( p > 0  && nShell < 29);
  printf("\n");

  printf(" N = %3d || ", n);
  nShell = 0;  
  do{
    int occ = TwoJ(nShell)+1;
    if ( nShell > coreShell ){
      if( n > occ ) {
         printf("%-4d", occ);
         if( nShell == 0 || nShell == 2 || nShell == 5 || nShell ==6 || nShell == 9  || nShell == 10 || nShell == 15 || nShell == 21){
          printf("|");
        }else{
          printf(",");
        } 
      }else{
        printf("%-4d", n);
      }
    }
    n = n - occ;
    nShell++;
  }while( n > 0  && nShell < 29);
  printf("\n");

  printf("------------------ \n");
}

inline void Isotope::Print(){

  if (Mass > 0){  
    
    findHeliosPath();
      
    printf(" using mass data : %s \n", dataSource.c_str());
    printf(" mass of \e[47m\e[31m%s\e[m nucleus (Z,A)=(%3d,%3d) is \e[47m\e[31m%12.5f\e[m MeV, BE/A=%7.5f MeV\n", Name.c_str(), Z, A, Mass, BEA/1000.);     
    printf(" total BE    : %12.5f MeV\n",BEA*A/1000.);
    printf(" mass in amu : %12.5f u\n",Mass/amu);
    printf(" mass excess : %12.5f MeV\n", Mass + Z*0.510998950 - A*amu);
    printf("-------------- Seperation energy \n");
    printf(" S1p: %8.4f| S1n: %8.4f| S(2H ): %8.4f| S1p1n : %8.4f\n", CalSp(1, 0), CalSp(0, 1), CalSp2(2, 1), CalSp(1, 1));
    printf(" S2p: %8.4f| S2n: %8.4f| S(3He): %8.4f| S(3H) : %8.4f\n", CalSp(2, 0), CalSp(0, 2), CalSp2(3, 2), CalSp2(3, 1));
    printf(" S3p: %8.4f| S3n: %8.4f| S(4He): %8.4f|\n",               CalSp(3, 0), CalSp(0, 3), CalSp2(4, 2));
    printf(" S4p: %8.4f| S4n: %8.4f| \n",                             CalSp(4, 0), CalSp(0, 4));

  }else{
    printf("Error %6.0f, no nucleus with (Z,A) = (%3d,%3d). \n", Mass, Z, A);
  }


}

#endif
