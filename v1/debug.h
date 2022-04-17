#ifndef __DEBUG_H__
#define __DEBUG_H__

#ifdef  __cplusplus
extern "C" {
#endif

#ifdef DEBUG
#include <stdio.h>
#ifndef DB_INF
#define DB_INF(fmt,ARGS...)     printf("[INF] [%s@%d]"fmt"\r\n",__FUNCTION__,__LINE__,##ARGS)
#endif /*DB_INF*/
#ifndef DB_WAR
#define DB_WAR(fmt,ARGS...)     printf("[WAR] [%s@%d]"fmt"\r\n",__FUNCTION__,__LINE__,##ARGS)
#endif /*DB_WAR*/
#ifndef DB_ERR
#define DB_ERR(fmt,ARGS...)     printf("[ERR] [%s@%d]"fmt"\r\n",__FUNCTION__,__LINE__,##ARGS)
#endif /*DB_ERR*/
#ifndef DB_POS
#define DB_POS()                printf("[POS] [%s@%d]\r\n",__FUNCTION__,__LINE__)
#endif /*DB_POS*/
#else /*not define DEBUG*/
#ifndef DB_INF
#define DB_INF(fmt,ARGS...)
#endif /*DB_INF*/
#ifndef DB_WAR
#define DB_WAR(fmt,ARGS...)
#endif /*DB_WAR*/
#ifndef DB_ERR
#define DB_ERR(fmt,ARGS...)
#endif /*DB_ERR*/
#ifndef DB_POS
#define DB_POS()
#endif /*DB_POS*/
#endif /*end of DEBUG*/

#include <assert.h>
#define ASSERT(x)       assert(x)

#ifdef  __cplusplus
}
#endif

#endif /*__DEBUG_H__*/
