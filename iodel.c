/*
hongkong 純正クライアントが giveio をインストールし直したり、排他するのでつけた
*/
#include "giveio.h"

int main(int c, char **v)
{
	giveio_stop(GIVEIO_REMOVE);
	giveio_start();
	giveio_stop(GIVEIO_STOP);
	return 0;
}
