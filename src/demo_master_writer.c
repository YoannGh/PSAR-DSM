#include "dsm.h"

int main()
{
	unsigned int nb_proc = 3;
	unsigned int nb_increment = 101;

	void *base_addr;
	int *entier1;
	int *entier2;
	int *entier3;

	base_addr = InitMaster(5555, 10);

	entier1 = base_addr;
	entier2 = base_addr + 13337;
	entier3 = base_addr + 33333;

	sync_barrier(nb_proc);
	for(unsigned int i = 0; i < nb_increment; i++) {
		lock_write(entier1);
		(*entier1)++;
		unlock_write(entier1);
		lock_write(entier2);
		(*entier2)++;
		unlock_write(entier2);
		lock_write(entier3);
		(*entier3)++;
		unlock_write(entier3);
	}
	sync_barrier(nb_proc);

	QuitDSM();
	return 0;
}