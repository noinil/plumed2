#ifndef __PLUMED_NeighborList_h
#define __PLUMED_NeighborList_h

#include <vector>
#include "Vector.h"
#include "Pbc.h"
#include "AtomNumber.h"

namespace PLMD{

/// A class that implements neighbor lists from two lists or a single list of atoms
class NeighborList  
{
  bool do_pair_,do_pbc_,twolists_;
  const PLMD::Pbc& pbc_;
  std::vector<PLMD::AtomNumber> fullatomlist_,requestlist_;
  std::vector< std::pair<unsigned,unsigned> > neighbors_;
  double distance_;
  unsigned stride_,nlist0_,nlist1_,nallpairs_,lastupdate_;
/// Initialize the neighbor list with all possible pairs
  void initialize();
/// Return the pair of indexes in the positions array
/// of the two atoms forming the i-th pair among all possible pairs  
  std::pair<unsigned,unsigned> getIndexPair(unsigned i);
/// Extract the list of atoms from the current list of close pairs
  void setRequestList();
public:
  NeighborList(const std::vector<PLMD::AtomNumber>& list0,
               const std::vector<PLMD::AtomNumber>& list1,
               const bool& do_pair, const bool& do_pbc, const PLMD::Pbc& pbc, 
               const double& distance=1.0e+30, const unsigned& stride=0);
  NeighborList(const std::vector<PLMD::AtomNumber>& list0, const bool& do_pbc,
               const PLMD::Pbc& pbc, const double& distance=1.0e+30,
               const unsigned& stride=0);
/// Return the list of all atoms. These are needed to rebuild the neighbor list.                         
  std::vector<PLMD::AtomNumber>& getFullAtomList();
/// Update the indexes in the neighbor list to match the
/// ordering in the new positions array
/// and return the new list of atoms that must be requested to the main code
  std::vector<PLMD::AtomNumber>& getReducedAtomList();
/// Update the neighbor list and prepare the new
/// list of atoms that will be requested to the main code  
  void update(const std::vector<PLMD::Vector>& positions);
/// Get the update stride of the neighbor list
  unsigned getStride() const;
/// Get the last step in which the neighbor list was updated  
  unsigned getLastUpdate() const;
/// Set the step of the last update
  void setLastUpdate(unsigned step);
/// Get the size of the neighbor list
  unsigned size() const;
/// Get the i-th pair of the neighbor list
  std::pair<unsigned,unsigned> getClosePair(unsigned i) const;
/// Get the list of neighbors of the i-th atom
  std::vector<unsigned> getNeighbors(unsigned i);
  ~NeighborList(){};
};

}

#endif