#ifndef __GIVEIO_H__
#define __GIVEIO_H__

/* result of giveio_start() */
#define GIVEIO_ERROR   0
#define GIVEIO_WIN95   1  /* windows 95/98/Me, unnecessary */
#define GIVEIO_OPEN    2  /* giveio opened                 */
#define GIVEIO_START   3  /* giveio service start & opened */
#define GIVEIO_INSTALL 4  /* giveio install,start & opened */
int giveio_start(void);

/* parameter of giveio_stop() */
#define GIVEIO_STOP   3   /* giveio stop service         */
#define GIVEIO_REMOVE 4   /* giveio stop & remove driver */
int giveio_stop(int param);

#endif
