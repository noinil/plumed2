/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   Copyright (c) 2012-2014 The plumed team
   (see the PEOPLE file at the root of the distribution for a list of names)

   See http://www.plumed-code.org for more information.

   This file is part of plumed, version 2.

   plumed is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   plumed is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with plumed.  If not, see <http://www.gnu.org/licenses/>.
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
#include "Colvar.h"
#include "ActionRegister.h"
#include "core/PlumedMain.h"
#include "tools/Matrix.h"

#include <string>
#include <cmath>

using namespace std;

namespace PLMD{
namespace colvar{

//+PLUMEDOC COLVAR GYRATION
/*
Calculate the radius of gyration, or other properties related to it.

The different properties can be calculated and selected by the TYPE keyword:
the Radius of Gyration (RADIUS); the Trace of the Gyration Tensor (TRACE); 
the Largest Principal Moment of the Gyration Tensor (GTPC_1); the middle Principal Moment of the Gyration Tensor (GTPC_2);
the Smallest Principal Moment of the Gyration Tensor (GTPC_3); the Asphericiry (ASPHERICITY); the Acylindricity (ACYLINDRICITY);
the Relative Shape Anisotropy (KAPPA2); the Smallest Principal Radius Of Gyration (GYRATION_3); 
the Middle Principal Radius of Gyration (GYRATION_2); the Largest Principal Radius of Gyration (GYRATION_1).
A derivation of all these different variants can be found in \cite Vymetal:2011gv

The radius of gyration is calculated using:

\f[
s_{\rm Gyr}=\Big ( \frac{\sum_i^{n}  
 m_i \vert {r}_i -{r}_{\rm COM} \vert ^2 }{\sum_i^{n} m_i} \Big)^{1/2} 
\f]

with the position of the center of mass \f${r}_{\rm COM}\f$ given by:

\f[
{r}_{\rm COM}=\frac{\sum_i^{n} {r}_i\ m_i }{\sum_i^{n} m_i}
\f]


\par Examples

The following input tells plumed to print the radius of gyration of the 
chain containing atoms 10 to 20.
\verbatim
GYRATION TYPE=RADIUS ATOMS=10-20 LABEL=rg
PRINT ARG=rg STRIDE=1 FILE=colvar 
\endverbatim
(See also \ref PRINT)

*/
//+ENDPLUMEDOC

class Gyration : public Colvar {
private:
  enum CV_TYPE {RADIUS, TRACE, GTPC_1, GTPC_2, GTPC_3, ASPHERICITY, ACYLINDRICITY, KAPPA2, GYRATION_3, GYRATION_2, GYRATION_1, TOT};
  int rg_type;
  bool use_masses;
public:
  static void registerKeywords( Keywords& keys );
  Gyration(const ActionOptions&);
  virtual void calculate();
};

PLUMED_REGISTER_ACTION(Gyration,"GYRATION")

void Gyration::registerKeywords( Keywords& keys ){
  Colvar::registerKeywords( keys );
  keys.add("compulsory","TYPE","RADIUS","The type of calculation relative to the Gyration Tensor you want to perform");
  keys.add("atoms","ATOMS","the group of atoms that you are calculating the Gyration Tensor for");
  keys.addFlag("NOT_MASS_WEIGHTED",false,"set the masses of all the atoms equal to one");
}

Gyration::Gyration(const ActionOptions&ao):
PLUMED_COLVAR_INIT(ao),
use_masses(true)
{
  std::vector<AtomNumber> atoms;
  parseAtomList("ATOMS",atoms);
  if(atoms.size()==0) error("no atoms specified");
  bool not_use_masses=!use_masses;
  parseFlag("NOT_MASS_WEIGHTED",not_use_masses);
  use_masses=!not_use_masses;
  std::string Type;
  parse("TYPE",Type);
  checkRead();

  if(Type=="RADIUS") rg_type=RADIUS;
  else if(Type=="TRACE") rg_type=TRACE;
  else if(Type=="GTPC_1") rg_type=GTPC_1;
  else if(Type=="GTPC_2") rg_type=GTPC_2;
  else if(Type=="GTPC_3") rg_type=GTPC_3;
  else if(Type=="ASPHERICITY") rg_type=ASPHERICITY;
  else if(Type=="ACYLINDRICITY") rg_type=ACYLINDRICITY;
  else if(Type=="KAPPA2") rg_type=KAPPA2;
  else if(Type=="RGYR_3") rg_type=GYRATION_3;
  else if(Type=="RGYR_2") rg_type=GYRATION_2;
  else if(Type=="RGYR_1") rg_type=GYRATION_1;
  else error("Unknown GYRATION type");

  switch(rg_type)
  {
    case RADIUS:   log.printf("  GYRATION RADIUS (Rg);"); break;
    case TRACE:  log.printf("  TRACE OF THE GYRATION TENSOR;"); break;
    case GTPC_1: log.printf("  THE LARGEST PRINCIPAL MOMENT OF THE GYRATION TENSOR (S'_1);"); break;
    case GTPC_2: log.printf("  THE MIDDLE PRINCIPAL MOMENT OF THE GYRATION TENSOR (S'_2);");  break;
    case GTPC_3: log.printf("  THE SMALLEST PRINCIPAL MOMENT OF THE GYRATION TENSOR (S'_3);"); break;
    case ASPHERICITY: log.printf("  THE ASPHERICITY (b');"); break;
    case ACYLINDRICITY: log.printf("  THE ACYLINDRICITY (c');"); break; 
    case KAPPA2: log.printf("  THE RELATIVE SHAPE ANISOTROPY (kappa^2);"); break;
    case GYRATION_3: log.printf("  THE SMALLEST PRINCIPAL RADIUS OF GYRATION (r_g3);"); break;
    case GYRATION_2: log.printf("  THE MIDDLE PRINCIPAL RADIUS OF GYRATION (r_g2);"); break;
    case GYRATION_1: log.printf("  THE LARGEST PRINCIPAL RADIUS OF GYRATION (r_g1);"); break;
  }
  if(rg_type>TRACE) log<<"  Bibliography "<<plumed.cite("Jirí Vymetal and Jirí Vondrasek, J. Phys. Chem. A 115, 11455 (2011)"); log<<"\n";

  log.printf("  atoms involved : ");
  for(unsigned i=0;i<atoms.size();++i) log.printf("%d ",atoms[i].serial());
  log.printf("\n");

  addValueWithDerivatives(); setNotPeriodic();
  requestAtoms(atoms);
}

void Gyration::calculate(){

  std::vector<Vector> derivatives( getNumberOfAtoms() );
  double totmass = 0.; 
  double d=0., rgyr=0.;
  Vector pos0, com, diff;
  Matrix<double> gyr_tens(3,3);
  for(unsigned i=0;i<3;i++)  for(unsigned j=0;j<3;j++) gyr_tens(i,j)=0.;

  // Find the center of mass
  pos0=getPosition(0); 
  com.zero();
  if(use_masses) {
    totmass=getMass(0);
    for(unsigned i=1;i<getNumberOfAtoms();i++){
      diff=delta( pos0, getPosition(i) );
      totmass += getMass(i);
      com += getMass(i)*diff;
    }
  } else {
    totmass=(double)getNumberOfAtoms();
    for(unsigned i=1;i<getNumberOfAtoms();i++){
      diff=delta( pos0, getPosition(i) );
      com += diff;
    }
  }
  com = com / totmass + pos0;

  switch(rg_type) {
    case RADIUS:
    case TRACE:
      // Now compute radius of gyration
      if( use_masses ){
        for(unsigned i=0;i<getNumberOfAtoms();i++){
          diff=delta( com, getPosition(i) );
          d=diff.modulo();
          rgyr += getMass(i)*d*d;
          derivatives[i]=diff*getMass(i);
        }
      } else {
        for(unsigned i=0;i<getNumberOfAtoms();i++){
          diff=delta( com, getPosition(i) );
          d=diff.modulo();
          rgyr += d*d;
          derivatives[i]=diff;
        }
      }
      break;
    default: 
      //calculate gyration tensor
      if( use_masses ) {
        for(unsigned i=0;i<getNumberOfAtoms();i++){
          diff=delta( com, getPosition(i) );
          gyr_tens[0][0]+=getMass(i)*diff[0]*diff[0];
          gyr_tens[1][1]+=getMass(i)*diff[1]*diff[1];
          gyr_tens[2][2]+=getMass(i)*diff[2]*diff[2];
          gyr_tens[0][1]+=getMass(i)*diff[0]*diff[1];
          gyr_tens[0][2]+=getMass(i)*diff[0]*diff[2];
          gyr_tens[1][2]+=getMass(i)*diff[1]*diff[2];
        }
      } else {
        for(unsigned i=0;i<getNumberOfAtoms();i++){
          diff=delta( com, getPosition(i) );
          gyr_tens[0][0]+=diff[0]*diff[0];
          gyr_tens[1][1]+=diff[1]*diff[1];
          gyr_tens[2][2]+=diff[2]*diff[2];
          gyr_tens[0][1]+=diff[0]*diff[1];
          gyr_tens[0][2]+=diff[0]*diff[2];
          gyr_tens[1][2]+=diff[1]*diff[2];
        }
      }
      break;
  }

  switch(rg_type) {
    case RADIUS:
    {
      rgyr=sqrt(rgyr/totmass);
      double fact=1./(rgyr*totmass);
      for(unsigned i=0;i<getNumberOfAtoms();i++){
        derivatives[i] *= fact;
        setAtomsDerivatives(i,derivatives[i]);
      }
      break;
    }
    case TRACE:
    {
      rgyr *= 2.;
      for(unsigned i=0;i<getNumberOfAtoms();i++) {
        derivatives[i] *= 4.;  
        setAtomsDerivatives(i,derivatives[i]);
      }
      break;
    }
    default:
    {
      // first make the matrix symmetric
      gyr_tens[1][0] = gyr_tens[0][1];
      gyr_tens[2][0] = gyr_tens[0][2];
      gyr_tens[2][1] = gyr_tens[1][2];

      Matrix<double> ttransf(3,3), transf(3,3);
      std::vector<double> princ_comp(3), prefactor(3);
      prefactor[0]=prefactor[1]=prefactor[2]=0.;

      //diagonalize gyration tensor
      diagMat(gyr_tens, princ_comp, ttransf);
      transpose(ttransf, transf);
      //sort eigenvalues and eigenvectors
      if (princ_comp[0]<princ_comp[1]){
        double tmp=princ_comp[0]; princ_comp[0]=princ_comp[1]; princ_comp[1]=tmp;
        for (unsigned i=0; i<3; i++){tmp=transf[i][0]; transf[i][0]=transf[i][1]; transf[i][1]=tmp;}
      }
      if (princ_comp[1]<princ_comp[2]){
        double tmp=princ_comp[1]; princ_comp[1]=princ_comp[2]; princ_comp[2]=tmp;
        for (unsigned i=0; i<3; i++){tmp=transf[i][1]; transf[i][1]=transf[i][2]; transf[i][2]=tmp;}
      }
      if (princ_comp[0]<princ_comp[1]){
        double tmp=princ_comp[0]; princ_comp[0]=princ_comp[1]; princ_comp[1]=tmp;
        for (unsigned i=0; i<3; i++){tmp=transf[i][0]; transf[i][0]=transf[i][1]; transf[i][1]=tmp;}      
      }
      //calculate determinant of transformation matrix
      double det;  
      det = transf[0][0]*transf[1][1]*transf[2][2]+transf[0][1]*transf[1][2]*transf[2][0]+
            transf[0][2]*transf[1][0]*transf[2][1]-transf[0][2]*transf[1][1]*transf[2][0]-
            transf[0][1]*transf[1][0]*transf[2][2]-transf[0][0]*transf[1][2]*transf[2][1]; 
      // trasformation matrix for rotation must have positive determinant, otherwise multiply one column by (-1)
      if(det<0) { 
        for(unsigned j=0;j<3;j++) transf[j][2]=-transf[j][2];
        det = -det;
      } 
      if(fabs(det-1.)>0.0001) error("Plumed Error: Cannot diagonalize gyration tensor\n");
      switch(rg_type) {
        case GTPC_1:
        case GTPC_2:
        case GTPC_3:
        {
          int pc_index = rg_type-2; //index of principal component
          rgyr=sqrt(princ_comp[pc_index]/totmass);
          double rm = rgyr*totmass;
          if(rm>1e-6) prefactor[pc_index]=1.0/rm; //some parts of derivate
          break;
        }
	case GYRATION_3:        //the smallest principal radius of gyration
        {
          rgyr=sqrt((princ_comp[1]+princ_comp[2])/totmass);
          double rm = rgyr*totmass;
	  if (rm>1e-6){
            prefactor[1]=1.0/rm;
            prefactor[2]=1.0/rm;
	  }
          break;
        }
	case GYRATION_2:       //the midle principal radius of gyration
        {
          rgyr=sqrt((princ_comp[0]+princ_comp[2])/totmass);
          double rm = rgyr*totmass;
	  if (rm>1e-6){
            prefactor[0]=1.0/rm;
            prefactor[2]=1.0/rm;
	  }
          break;
        }
	case GYRATION_1:      //the largest principal radius of gyration
        {
          rgyr=sqrt((princ_comp[0]+princ_comp[1])/totmass);
          double rm = rgyr*totmass;
	  if (rm>1e-6){
            prefactor[0]=1.0/rm;
            prefactor[1]=1.0/rm;
	  }
          break;             
        }
	case ASPHERICITY:
        {
          rgyr=sqrt((princ_comp[0]-0.5*(princ_comp[1]+princ_comp[2]))/totmass); 
          double rm = rgyr*totmass;
	  if (rm>1e-6){   
            prefactor[0]= 1.0/rm;
            prefactor[1]=-0.5/rm;
            prefactor[2]=-0.5/rm;
	  }
	  break;
        }
	case ACYLINDRICITY:
        {
          rgyr=sqrt((princ_comp[1]-princ_comp[2])/totmass); 
          double rm = rgyr*totmass;
	  if (rm>1e-6){   //avoid division by zero  
            prefactor[1]= 1.0/rm;
            prefactor[2]=-1.0/rm;
	  }
          break;
        }
	case KAPPA2: // relative shape anisotropy
        {
          double trace = princ_comp[0]+princ_comp[1]+princ_comp[2];
          double tmp=princ_comp[0]*princ_comp[1]+ princ_comp[1]*princ_comp[2]+ princ_comp[0]*princ_comp[2]; 
          rgyr=1.0-3*(tmp/(trace*trace));
	  if (rgyr>1e-6){
            prefactor[0]= -3*((princ_comp[1]+princ_comp[2])-2*tmp/trace)/(trace*trace) *2;
            prefactor[1]= -3*((princ_comp[0]+princ_comp[2])-2*tmp/trace)/(trace*trace) *2;
            prefactor[2]= -3*((princ_comp[0]+princ_comp[1])-2*tmp/trace)/(trace*trace) *2;
	  }
          break;
        }
      }
      if(use_masses) { 
        for(unsigned i=0;i<getNumberOfAtoms();i++){
          Vector tX;
          diff=delta( com,getPosition(i) );
          //project atomic postional vectors to diagonalized frame
          for(unsigned j=0;j<3;j++) tX[j]=transf[0][j]*diff[0]+transf[1][j]*diff[1]+transf[2][j]*diff[2];  
          for(unsigned j=0;j<3;j++) derivatives[i][j]=getMass(i)*(prefactor[0]*transf[j][0]*tX[0]+
                                                                  prefactor[1]*transf[j][1]*tX[1]+
                                                                  prefactor[2]*transf[j][2]*tX[2]);
          setAtomsDerivatives(i,derivatives[i]);
        }
      } else {
        for(unsigned i=0;i<getNumberOfAtoms();i++){
          Vector tX;
          diff=delta( com,getPosition(i) );
          //project atomic postional vectors to diagonalized frame
          for(unsigned j=0;j<3;j++) tX[j]=transf[0][j]*diff[0]+transf[1][j]*diff[1]+transf[2][j]*diff[2];  
          for(unsigned j=0;j<3;j++) derivatives[i][j]=prefactor[0]*transf[j][0]*tX[0]+
                                                      prefactor[1]*transf[j][1]*tX[1]+
                                                      prefactor[2]*transf[j][2]*tX[2];
          setAtomsDerivatives(i,derivatives[i]);
        }
      }
      break;
    }
  }
  setValue(rgyr);
  setBoxDerivativesNoPbc();
}

}
}
