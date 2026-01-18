#ifndef MYMPI_H

#define MYMPI_H

void mympi_init();

int mympi_rank();
int mympi_size();

void mympi_send(const void *buf, int count, int dst);
void mympi_recv( void *buf, int count, int src);

void mympi_finalize();

#endif