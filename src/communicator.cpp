#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#include "log.h"
#include "config.h"

#include "const.h"

#include "memory.h"

#include "communicator.h"
#include "alltoall.h"
#include "ptop.h"

#include "stat.h"

using namespace MAPREDUCE_NS;

Communicator* Communicator::Create(MPI_Comm _comm, int _tnum, int _commmode){
  Communicator *c=NULL;
  if(_commmode==0)
    c = new Alltoall(_comm, _tnum);
  else if(_commmode==1)
    c = new Ptop(_comm, _tnum);
  else
    LOG_ERROR("Create communicator error mode=%d!\n", _commmode);
  return c;
}


Communicator::Communicator(MPI_Comm _comm, int _commtype, int _tnum){

  comm = _comm;
  commtype = _commtype;
  tnum = _tnum;
  
  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);

  kvtype = ksize = vsize = 0;

  thread_buf_size = send_buf_size = 0;
  nbuf = 0;

  thread_buffers = NULL;
  thread_offsets = NULL;
  send_buffers = NULL;
  send_offsets = NULL;

  blocks = new int[tnum];

  //init();

  //printf("communicator start\n"); fflush(stdout);
}

Communicator::~Communicator(){

  if(!data){
    for(int i=0; i<tnum; i++)
      if(blocks[i] !=- 1) data->release_block(blocks[i]);
  }
  delete [] blocks;

  for(int i = 0; i < tnum; i++){
    //printf("free: buffers[%d]=%p\n", i, local_buffers[i]);
    if(thread_buffers && thread_buffers[i]) mem_aligned_free(thread_buffers[i]);
    if(thread_offsets && thread_offsets[i]) mem_aligned_free(thread_offsets[i]);
  }

  for(int i = 0; i < nbuf; i++){
    if(send_buffers && send_buffers[i]) mem_aligned_free(send_buffers[i]);
    if(send_offsets && send_offsets[i]) mem_aligned_free(send_offsets[i]);
  }

  if(thread_buffers) delete [] thread_buffers;
  if(thread_offsets) delete [] thread_offsets;

  if(send_buffers) delete [] send_buffers;
  if(send_offsets) delete [] send_offsets;
}

int Communicator::setup(int64_t _tbufsize, int64_t _sbufsize, int _kvtype, int _ksize, int _vsize, int _nbuf){
  //fprintf(stdout, "thread_buf_size=%ld, thread_buf_size=%ld", _tbufsize, _sbufsize);fflush(stdout);

  //if(_tbufsize%size!=0||_sbufsize%size!=0){
  //  LOG_ERROR("%s", "Error: the send buffer size should be divided into processes evently!");
  //}

  thread_buf_size = _tbufsize;
  send_buf_size = _sbufsize;

  if(thread_buf_size>send_buf_size){
    LOG_ERROR("Error: thread local buffer size (%ld per process) cannot be larger than send buffer size (%ld per process)!", \
     thread_buf_size, send_buf_size);
  }
  //fprintf(stdout, "thread_buf_size=%ld, thread_buf_size=%ld", thread_buf_size, send_buf_size);fflush(stdout);

  kvtype = _kvtype;
  ksize = _ksize;
  vsize = _vsize;
  nbuf = _nbuf;

  thread_buffers = new char*[tnum];
  thread_offsets = new int*[tnum];

#pragma omp parallel
  {
    int tid = omp_get_thread_num();
    thread_buffers[tid] = (char*)mem_aligned_malloc(MEMPAGE_SIZE, size*thread_buf_size);
    thread_offsets[tid]   = (int*)mem_aligned_malloc(MEMPAGE_SIZE, size*sizeof(int));
    for(int i = 0; i < size; i++) thread_offsets[tid][i] = 0;
    PROFILER_RECORD_COUNT(tid, COUNTER_COMM_THREAD_BUF, \
      thread_buf_size*size);
  }
 
  send_buffers = new char*[nbuf];
  send_offsets = new int*[nbuf];

  size_t total_send_buf_size=(size_t)send_buf_size*size;
  for(int i = 0; i < nbuf; i++){
    send_buffers[i] = (char*)mem_aligned_malloc(MEMPAGE_SIZE, total_send_buf_size);
    send_offsets[i] = (int*)mem_aligned_malloc(MEMPAGE_SIZE, size*sizeof(int));
    for(int j = 0; j < size; j++) send_offsets[i][j] = 0;
  }

  PROFILER_RECORD_COUNT(0, COUNTER_COMM_SEND_BUF, total_send_buf_size*nbuf);

  for(int i = 0; i < tnum; i++){
    if(!thread_buffers[i]){
      LOG_ERROR("%s", "Error: communication buffer is overflow!\n");
    }
  }

  for(int i = 0; i < nbuf; i++){
    if(!send_buffers[i]){
      LOG_ERROR("%s", "Error: communication buffer is overflow!\n");
    }
  }

  return 0;
}

void Communicator::init(DataObject *_data){
  medone = tdone = pdone = 0;
  data = _data; 

 if(send_buf_size>data->blocksize){
    LOG_ERROR("Error: send buffer size (%ld per process) cannot be larger than block size (%ld)!\n", send_buf_size, data->blocksize);
  }

  for(int i = 0; i < tnum; i++) blocks[i] = -1;
}


