#ifndef EDGE_SERVER_H
#define EDGE_SERVER_H
#include "shm.h"
#include "task.h"
#include "message.h"
#include "log.h"
#include "task_shm.h"

void edgeserver(edgeServer *server, int i, int *p);
void *workercpu(void *ptr);
void *workermonitor(void *ptr);
#endif //EDGE_SERVER_H
