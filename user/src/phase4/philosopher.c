#include "philosopher.h"

// TODO: define some sem if you need
int lock;
int chopstick[5];
void init()
{
  // init some sem if you need
  for (int i = 0; i < 5; i++)
    chopstick[i] = sem_open(1);
  lock = sem_open(1);
  // TODO();
}

void philosopher(int id)
{
  // implement philosopher, remember to call `eat` and `think`
  while (1)
  {
    think(id);
    sem_p(lock);
    sem_p(chopstick[id]);
    sem_p(chopstick[(id + 1) % 5]);
    sem_v(lock);
    eat(id);
    sem_v(chopstick[id]);
    sem_v(chopstick[(id + 1) % 5]);
    // sem_v(lock);
    //  TODO();
  }
}
