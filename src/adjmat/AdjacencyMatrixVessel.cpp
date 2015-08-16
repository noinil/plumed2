/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   Copyright (c) 2013-2015 The plumed team
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
#include "vesselbase/ActionWithVessel.h"
#include "AdjacencyMatrixVessel.h"
#include "AdjacencyMatrixBase.h" 

namespace PLMD {
namespace adjmat {

void AdjacencyMatrixVessel::registerKeywords( Keywords& keys ){
  StoreDataVessel::registerKeywords(keys);
  keys.addFlag("SYMMETRIC",false,"is the matrix symmetric");
  keys.addFlag("HBONDS",false,"can we think of the matrix as a undirected graph");
  keys.add("compulsory","NROWS","number of rows");
  keys.add("compulsory","NCOLS","number of columns");
}

AdjacencyMatrixVessel::AdjacencyMatrixVessel( const vesselbase::VesselOptions& da ):
StoreDataVessel(da)
{
  function=dynamic_cast<AdjacencyMatrixBase*>( getAction() );
  plumed_assert( function );
  parse("NROWS",nrows); parse("NCOLS",ncols);
  parseFlag("SYMMETRIC",symmetric); parseFlag("HBONDS",hbonds);
  if( symmetric && hbonds ) error("matrix should be either symmetric or hbonds");
  if( symmetric && nrows!=ncols ) error("matrix is supposed to be symmetric but nrows!=ncols");
  if( hbonds &&  nrows!=ncols ) error("matrix is supposed to be hbonds but nrows!=ncols");
}

bool AdjacencyMatrixVessel::isSymmetric() const {
  return symmetric;
}

bool AdjacencyMatrixVessel::undirectedGraph() const {
  return ( symmetric || hbonds );
}

unsigned AdjacencyMatrixVessel::getNumberOfStoredValues() const {
  if( symmetric ){ unsigned nnodes=function->getNumberOfNodes(); return 0.5*nnodes*(nnodes-1); }
  return nrows*ncols;
}

unsigned AdjacencyMatrixVessel::getStoreIndexFromMatrixIndices( const unsigned& ielem, const unsigned& jelem ) const {
  if( !symmetric ) return nrows*ielem + jelem;
  if( ielem>jelem ) return 0.5*ielem*(ielem-1)+jelem;
  return 0.5*jelem*(jelem-1) + ielem;
}

unsigned AdjacencyMatrixVessel::getStoreIndex( const unsigned& myelem ) const {
  unsigned ielem, jelem;
  getMatrixIndices( myelem, ielem, jelem );
  return getStoreIndexFromMatrixIndices( ielem, jelem );
  if( symmetric ) return 0.5*ielem*(ielem-1)+jelem;
  return nrows*ielem + jelem;
}

AdjacencyMatrixBase* AdjacencyMatrixVessel::getMatrixAction() {
  return function;
}

void AdjacencyMatrixVessel::getMatrixIndices( const unsigned& code, unsigned& i, unsigned& j ) const {
  std::vector<unsigned> myatoms; function->decodeIndexToAtoms( function->getTaskCode(code), myatoms ); 
  i=myatoms[0]; j=myatoms[1]; 
}

void AdjacencyMatrixVessel::retrieveMatrix( DynamicList<unsigned>& myactive_elements, Matrix<double>& mymatrix ){
  myactive_elements.deactivateAll(); std::vector<double> vals(2);
  for(unsigned i=0;i<getNumberOfStoredValues();++i){
      // Ignore any non active members
      if( !storedValueIsActive(i) ) continue ;
      myactive_elements.activate(i);
      unsigned j, k; getMatrixIndices( i, k, j );
      retrieveValue( i, false, vals );
      if( symmetric ) mymatrix(k,j)=mymatrix(j,k)=vals[1] / vals[0];
      else mymatrix(k,j) = vals[1] / vals[0];
  }
  myactive_elements.updateActiveMembers();  
}

void AdjacencyMatrixVessel::retrieveAdjacencyLists( std::vector<unsigned>& nneigh, Matrix<unsigned>& adj_list ){
  plumed_dbg_assert( undirectedGraph() );
  // Currently everything has zero neighbors
  for(unsigned i=0;i<nneigh.size();++i) nneigh[i]=0;

  // And set up the adjacency list
  for(unsigned i=0;i<getNumberOfStoredValues();++i){
      // Ignore any non active members
      if( !storedValueIsActive(i) ) continue ;
      unsigned j, k; getMatrixIndices( i, k, j );
      adj_list(k,nneigh[k])=j; nneigh[k]++;
      adj_list(j,nneigh[j])=k; nneigh[j]++;
  }
}

void AdjacencyMatrixVessel::retrieveEdgeList( unsigned& nedge, std::vector<std::pair<unsigned,unsigned> >& edge_list ){
  plumed_dbg_assert( undirectedGraph() ); nedge=0;
  for(unsigned i=0;i<getNumberOfStoredValues();++i){
      // Ignore any non active members
      if( !storedValueIsActive(i) ) continue ;
      getMatrixIndices( i, edge_list[nedge].first, edge_list[nedge].second );
      nedge++;
  }
}

void AdjacencyMatrixVessel::retrieveDerivatives( const unsigned& myelem, const bool& normed, MultiValue& myvals ){
  StoreDataVessel::retrieveDerivatives( myelem, normed, myvals );
  if( !function->weightHasDerivatives ) return ;

  std::vector<double> vals(2); retrieveValue( myelem, normed, vals ); double pref = vals[1]/(vals[0]*vals[0]);
  for(unsigned i=0;i<myvals.getNumberActive();++i){
      unsigned jder=myvals.getActiveIndex(i);
      myvals.setDerivative( 1, jder, myvals.getDerivative(1, jder)/vals[0] - pref*myvals.getDerivative(0, jder) );
  }
}

void AdjacencyMatrixVessel::recalculateStoredQuantity( const unsigned& myelem, MultiValue& myvals ){
  function->recalculateMatrixElement( myelem, myvals );
}

}
}
