#ifndef SPLITPOLEHit_H
#define SPLITPOLEHit_H

#include "Isotope.h"

#include <cmath>
#include <random>

// static double randZeroToOne() {
//     return static_cast<double>(rand()) / RAND_MAX;
// }

// // Box-Muller transform to generate random Gaussian numbers
// static double generateGaussian(double mean, double stddev) {
//     static bool hasSpare = false;
//     static double spare;
//     if (hasSpare) {
//         hasSpare = false;
//         return mean + stddev * spare;
//     } else {
//         double u, v, s;
//         do {
//             u = 2.0 * randZeroToOne() - 1.0;
//             v = 2.0 * randZeroToOne() - 1.0;
//             s = u * u + v * v;
//         } while (s >= 1.0 || s == 0.0);

//         s = std::sqrt(-2.0 * std::log(s) / s);
//         spare = v * s;
//         hasSpare = true;
//         return mean + stddev * u * s;
//     }
// }

namespace SPS{
  namespace ChMap{

    const short ScinR = 0;
    const short ScinL = 1;
    const short dFR = 9;
    const short dFL = 8;
    const short dBR = 10;
    const short dBL = 11;
    const short Cathode = 7;
    const short AnodeF = 12;
    const short AnodeB = 15;

  };

  const double c = 299.792458; // mm/ns
  const double pi = M_PI;
  const double deg2rad = pi/180.;

  const double DISPERSION = 1.96; // x-position/rho
  const double MAGNIFICATION = 0.39; // in x-position
  const double X1X2Separation = 36; // cm

}

class SplitPoleHit{

public:
  SplitPoleHit(){
    ClearData();
  }

  unsigned int eSR; unsigned long long tSR;
  unsigned int eSL; unsigned long long tSL;
  unsigned int eFR; unsigned long long tFR;
  unsigned int eFL; unsigned long long tFL;
  unsigned int eBR; unsigned long long tBR;
  unsigned int eBL; unsigned long long tBL;
  unsigned int eCath; unsigned long long tCath;
  unsigned int eAF; unsigned long long tAF;
  unsigned int eAB; unsigned long long tAB;

  float eSAvg;
  float x1, x2, theta;
  float xAvg;

  double GetQ0() const {return Q0;}
  double GetRho0() const {return rho0;}
  double GetZoffset() const {return zOffset;}

  void SetMassTablePath(std::string path){
    target.SetMassTablePath(path);
    beam.SetMassTablePath(path);
    recoil.SetMassTablePath(path);
    heavyRecoil.SetMassTablePath(path);
  }

  void CalConstants(std::string targetStr, std::string beamStr, std::string recoilStr, double energyMeV, double angleDeg){
    target.SetIsoByName(targetStr);
    beam.SetIsoByName(beamStr);
    recoil.SetIsoByName(recoilStr);
    heavyRecoil.SetIso(target.A + beam.A - recoil.A, target.Z + beam.Z - recoil.Z);

    angleDegree = angleDeg; // degree
    beamKE = energyMeV; // MeV

    Ei = target.Mass + beamKE + beam.Mass;
    k1 = sqrt( 2*beam.Mass*beamKE + beamKE*beamKE);
    cs = cos(angleDegree * SPS::deg2rad);
    ma = recoil.Mass;
    mb = heavyRecoil.Mass;

    isConstantCal = true;

    printf("============================================\n");
    printf("  Beam Mass : %20.4f MeV/c2\n", beam.Mass);
    printf("    beam KE : %20.4f MeV\n", beamKE);
    printf("Target Mass : %20.4f MeV/c2\n", target.Mass);
    printf("Recoil Mass : %20.4f MeV/c2\n", recoil.Mass);
    printf("H. Rec Mass : %20.4f MeV/c2\n", heavyRecoil.Mass);

    printf("      angle : %20.4f deg\n", angleDegree);
    printf("         k1 : %20.4f MeV/c\n", k1);
    printf("         Ei : %20.4f MeV\n", Ei);

  }

  double CalRecoilMomentum(double Ex){

    if( !isConstantCal ) return 0;

    double p =  Ei*Ei - k1*k1;
    double q = ma*ma - (mb + Ex)*(mb + Ex);

    double x = k1* ( p + q) * cs;
    double y = pow( p, 2) + pow(q, 2)- 2 * Ei * Ei * (ma* ma + (mb + Ex)*(mb + Ex)) + 2 * k1 * k1 * (ma*ma * cos(2* angleDegree * SPS::deg2rad) + (mb + Ex)*(mb + Ex));
    double z = 2 * ( Ei*Ei - k1*k1 * cs * cs) ;

    return (x + Ei * sqrt(y))/z;

  }

  double Momentum2Rho(double ka){
    return ka / (recoil.Z * Bfield * SPS::c);
  }

  double Momentum2Ex(double ka){
    return sqrt( Ei*Ei - k1*k1 + ma*ma + 2 * cs * k1 * ka - 2*Ei*sqrt(ma*ma + ka*ka)) - mb;
  }

  double Rho2Ex(double rhoInM){
    double ka = rhoInM * (recoil.Z * Bfield * SPS::c);
    return Momentum2Ex(ka);
  }

  void CalZoffset(double magFieldinT){

    Bfield = magFieldinT;

    if( !isConstantCal ) return;

    double recoilP = CalRecoilMomentum(0);

    Q0 = target.Mass + beam.Mass - recoil.Mass - heavyRecoil.Mass;

    double recoilKE = sqrt(ma*ma + recoilP* recoilP) - ma;

    printf("Q value : %f \n", Q0);
    printf("recoil enegry for ground state: %f MeV = %f MeV/c\n", recoilKE, recoilP);

    rho0 = recoilP/(recoil.Z * Bfield * SPS::c); // in m

    double haha = sqrt( ma * beam.Mass * beamKE / recoilKE );
    double k = haha * sin(angleDegree * SPS::deg2rad) / ( ma + mb - haha * cs);


    zOffset = -100.0 * rho0 * k * SPS::DISPERSION * SPS::MAGNIFICATION;

    printf("rho: %f m; z-offset: %f cm\n", rho0, zOffset);

  }

  void ClearData(){
    eSR = 0;  tSR = 0;
    eSL = 0;  tSL = 0;
    eFR = 0;  tFR = 0;
    eFL = 0;  tFL = 0;
    eBR = 0;  tBR = 0;
    eBL = 0;  tBL = 0;
    eCath = 0;  tCath = 0;
    eAF = 0;  tAF = 0;
    eAB = 0;  tAB = 0;

    eSAvg = -1;
    x1 = NAN;
    x2 = NAN;
    theta = NAN;
    xAvg = NAN;

    isConstantCal = false;
  }

  void CalData(float scale = 2.){

    if( eSR  > 0 && eSL  > 0 ) eSAvg = (eSR + eSL)/2;
    if( eSR  > 0 && eSL == 0 ) eSAvg = eSR;
    if( eSR == 0 && eSL  > 0 ) eSAvg = eSL;

    if( tFR > 0 && tFL > 0 ) x1 = (tFL - tFR)/scale/2.1;
    if( tBR > 0 && tBL > 0 ) x2 = (tBL - tBR)/scale/1.98;

    if( !std::isnan(x1)  && !std::isnan(x2)) {

      if( x2 > x1 ) {
        theta = atan((x2-x1)/SPS::X1X2Separation);
      }else if(x2 < x1){
        theta = SPS::pi + atan((x2-x1)/SPS::X1X2Separation);
      }else{
        theta = SPS::pi * 0.5;
      }

      double w1 = 0.5 - zOffset/4.28625;
      xAvg = w1 * x1 + (1-w1)* x2;

    }

  }

private:

  Isotope target;
  Isotope beam;
  Isotope recoil;
  Isotope heavyRecoil;

  double Bfield;
  double angleDegree;
  double beamKE;

  double zOffset;
  double Q0, rho0;

  bool isConstantCal;

  double Ei, k1, cs, ma, mb;

};

#endif