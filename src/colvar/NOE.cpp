/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   Copyright (c) 2014,2015 The plumed team
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
#include "tools/NeighborList.h"

#include <string>
#include <cmath>

using namespace std;

namespace PLMD {
namespace colvar {

//+PLUMEDOC COLVAR NOE 
/*
Calculates the deviation of current distances from experimental NOE derived distances.

NOE distances are calculated between couple of atoms, averaging over equivalent couples.
Each NOE is defined by two groups containing the same number of atoms, distances are
calculated in pairs and saved as components.

\f[
NOE() = (\frac{1}{N_{eq}}\sum_j^{N_{eq}} (\frac{1}{r_j^6}))^{\frac{-1}{6}} 
\f]

\par Examples
In the following examples three noes are defined, the first is calculated based on the distances
of atom 1-2 and 3-2; the second is defined by the distance 5-7 and the third by the distances
4-15,4-16,8-15,8-16.

\verbatim
NOE ...
GROUPA1=1,3 GROUPB1=2,2 
GROUPA2=5 GROUPB2=7
GROUPA3=4,4,8,8 GROUPB3=15,16,15,16
LABEL=noes
... NOE

PRINT ARG=noes.* FILE=colvar
\endverbatim
(See also \ref PRINT) 

*/
//+ENDPLUMEDOC

class NOE : public Colvar {   
private:
  bool             pbc;
  bool             serial;
  vector<unsigned> nga;
  vector<double>   noedist;
  NeighborList     *nl;
public:
  static void registerKeywords( Keywords& keys );
  explicit NOE(const ActionOptions&);
  ~NOE();
  virtual void calculate();
};

PLUMED_REGISTER_ACTION(NOE,"NOE")

void NOE::registerKeywords( Keywords& keys ){
  Colvar::registerKeywords( keys );
  keys.add("numbered","GROUPA","the atoms involved in each of the contacts you wish to calculate. "
                   "Keywords like GROUPA1, GROUPA2, GROUPA3,... should be listed and one contact will be "
                   "calculated for each ATOM keyword you specify.");
  keys.add("numbered","GROUPB","the atoms involved in each of the contacts you wish to calculate. "
                   "Keywords like GROUPB1, GROUPB2, GROUPB3,... should be listed and one contact will be "
                   "calculated for each ATOM keyword you specify.");
  keys.reset_style("GROUPA","atoms");
  keys.reset_style("GROUPB","atoms");
  keys.addFlag("ADDDISTANCES",false,"Set to TRUE if you want to have fixed components with the experimetnal values.");  
  keys.add("numbered","NOEDIST","Add an experimental value for each NOE.");
  keys.addFlag("SERIAL",false,"Perform the calculation in serial - for debug purpose");
  keys.addOutputComponent("noe","default","the # NOE");
}

NOE::NOE(const ActionOptions&ao):
PLUMED_COLVAR_INIT(ao),
pbc(true),
serial(false)
{
  parseFlag("SERIAL",serial);

  bool nopbc=!pbc;
  parseFlag("NOPBC",nopbc);
  pbc=!nopbc;  

  // Read in the atoms
  vector<AtomNumber> t, ga_lista, gb_lista;
  for(int i=1;;++i ){
     parseAtomList("GROUPA", i, t );
     if( t.empty() ) break;
     for(unsigned j=0;j<t.size();j++) ga_lista.push_back(t[j]);
     nga.push_back(t.size());
     t.resize(0); 
  }
  vector<unsigned> ngb;
  for(int i=1;;++i ){
     parseAtomList("GROUPB", i, t );
     if( t.empty() ) break;
     for(unsigned j=0;j<t.size();j++) gb_lista.push_back(t[j]);
     ngb.push_back(t.size());
     if(ngb[i-1]!=nga[i-1]) error("The same number of atoms is expected for the same GROUPA-GROUPB couple");
     t.resize(0); 
  }
  if(nga.size()!=ngb.size()) error("There should be the same number of GROUPA and GROUPB keywords");
  // Create neighbour lists
  nl= new NeighborList(ga_lista,gb_lista,true,pbc,getPbc());

  bool addistance=false;
  parseFlag("ADDDISTANCES",addistance);

  if(addistance) {
    noedist.resize( nga.size() ); 
    unsigned ntarget=0;

    for(unsigned i=0;i<nga.size();++i){
       if( !parseNumbered( "NOEDIST", i+1, noedist[i] ) ) break;
       ntarget++; 
    }
    if( ntarget!=nga.size() ) error("found wrong number of NOEDIST values");
  }

  // Ouput details of all contacts
  unsigned index=0; 
  for(unsigned i=0;i<nga.size();++i){
    log.printf("  The %uth NOE is calculated using %u equivalent couples of atoms\n", i, nga[i]);
    for(unsigned j=0;j<nga[i];j++) {
      log.printf("    couple %u is %d %d.\n", j, ga_lista[index].serial(), gb_lista[index].serial() );
      index++;
    }
  }

  if(!serial)  log.printf("  The NOEs are calculated in parallel\n");
  else         log.printf("  The NOEs are calculated in serial\n");
  if(pbc)      log.printf("  using periodic boundary conditions\n");
  else         log.printf("  without periodic boundary conditions\n");

  for(unsigned i=0;i<nga.size();i++) {
    std::string num; Tools::convert(i,num);
    addComponentWithDerivatives("noe_"+num);
    componentIsNotPeriodic("noe_"+num);
  }

  if(addistance) {
    for(unsigned i=0;i<nga.size();i++) {
      std::string num; Tools::convert(i,num);
      addComponent("exp_"+num);
      componentIsNotPeriodic("exp_"+num);
      Value* comp=getPntrToComponent("exp_"+num); comp->set(noedist[i]);
    }
  }

  requestAtoms(nl->getFullAtomList());
  checkRead();
}

NOE::~NOE(){
  delete nl;
} 

void NOE::calculate(){ 
  unsigned sga = nga.size();
  std::vector<Vector> deriv(getNumberOfAtoms());
  std::vector<Tensor> dervir(sga);
  vector<double> noe(sga);

  // internal parallelisation
  unsigned stride=comm.Get_size();
  unsigned rank=comm.Get_rank();
  if(serial){
    stride=1;
    rank=0;
  }
 
  for(unsigned i=0;i<sga;i++) noe[i]=0.;

  unsigned index=0; for(unsigned k=0;k<rank;k++) index += nga[k];

  for(unsigned i=rank;i<sga;i+=stride) { //cycle over the number of noe 
    for(unsigned j=0;j<nga[i];j++) {
      double aver=1./((double)nga[i]);
      unsigned i0=nl->getClosePair(index).first;
      unsigned i1=nl->getClosePair(index).second;
      Vector distance;
      if(pbc){
        distance=pbcDistance(getPosition(i0),getPosition(i1));
      } else {
        distance=delta(getPosition(i0),getPosition(i1));
      }
      double d=distance.modulo();
      double r2=d*d;
      double r6=r2*r2*r2;
      double r8=r6*r2;
      double tmpir6=aver/r6;
      double tmpir8=-6.*aver/r8;

      noe[i] += tmpir6;

      deriv[i0] = -tmpir8*distance;
      deriv[i1] = +tmpir8*distance;

      dervir[i] += Tensor(distance,deriv[i0]);

      index++;
    }
    for(unsigned k=i+1;k<i+stride;k++) index += nga[k];
  }

  if(!serial){
    comm.Sum(&noe[0],sga);
    comm.Sum(&deriv[0][0],3*deriv.size());
    comm.Sum(&dervir[0][0][0],9*sga);
  }

  index = 0;
  for(unsigned i=0;i<sga;i++) { //cycle over the number of noe 
    Value* val=getPntrToComponent(i);
    val->set(noe[i]);
    setBoxDerivatives(val, dervir[i]);
    for(unsigned j=0;j<nga[i];j++) {
      unsigned i0=nl->getClosePair(index).first;
      unsigned i1=nl->getClosePair(index).second;
      setAtomsDerivatives(val, i0, deriv[i0]);
      setAtomsDerivatives(val, i1, deriv[i1]); 
      index++;
    }
  }

}

}
}
