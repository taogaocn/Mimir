#ifndef COMMUNICATOR_H
#define COMMUNICATOR_H

#include <mpi.h>
#include "dataobject.h"

namespace MAPREDUCE_NS {

class Communicator{
public:
  Communicator(MPI_Comm _comm, int _commtype, int _tnum);
  
  virtual ~Communicator();

  // main thread
  virtual int setup(int, int, int kvtype=0, int ksize=0, int vsize=0, int nbuf=2);

  // main thread
  virtual void init(DataObject *data = NULL);

  // multi-threads
  virtual int sendKV(int, int, char *, int, char *, int) = 0;

  // multi-threads
  virtual void twait(int tid) = 0;

  // main thread
  virtual void wait() = 0;

protected:
  int fetch_and_add_with_max(int *counter, int adder, int maxnum){
    int val=0;
    do{
      val = *counter;
      if(val+adder>maxnum) break;
      if(__sync_bool_compare_and_swap(counter, val, val+adder))
        break;
    }while(1);
    return val;
  }

  // communicator and thread information
  MPI_Comm comm;
  int rank, size, tnum;

  // 0 for Alltoall, 1 for Isend/Irecv
  int commtype;   

  // terminate flag
  int medone; // if me done
  int tdone;  // done count for thread
  int pdone;  // done count for process

  // received data added into this object
  int *blocks;
  DataObject *data;

  // kv type
  int kvtype, ksize, vsize;

  // buffer size information
  int lbufsize, gbufsize, nbuf;

  char **local_buffers;   // local buffers for threads
  int  **local_offsets;   // local offsets for threads

  char **global_buffers;  // global buffers
  int  **global_offsets;  // global offsets
};

class Alltoall : public Communicator{
public:
  Alltoall(MPI_Comm, int);
  ~Alltoall();

  int setup(int, int, int kvtype=0, int ksize=0, int vsize=0, int nbuf=2);

  void init(DataObject *);
 
  int sendKV(int, int, char *, int, char *, int);
 
  // multi-threads
  void twait(int tid);

  // main thread
  void wait();

private:

   // exchange kv buffer
  void exchange_kv();

  void save_data(int);

   // data struct for type 0
  int switchflag;

  // global buffer informatoion
  int ibuf;
  char *buf;
  int  *off;

  // used for MPI_Ialltoall
  int *send_displs;   
  int *recv_displs;

  int **recv_count;
  char **recv_buf;     
  int  *recvcounts; 
  MPI_Request *reqs;
};

class P2P : public Communicator{
public:
  P2P(MPI_Comm, int);
  ~P2P();

  // main thread
  int setup(int, int, int kvtype=0, int ksize=0, int vsize=0, int nbuf=2);

  // main thread
  void init(DataObject *);

  // multi-thread
  int sendKV(int, int, char *, int, char *, int);

  // thread wait
  void twait(int tid);

  // process wait
  void wait();

private:
  void exchange_kv();

  int  *flags;
  
  int  *ibuf;

  char **buf;
  int  **off;

  MPI_Request **reqs;
};

}

#endif
