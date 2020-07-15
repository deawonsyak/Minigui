#ifndef __GUI_DEBUG_H__
#define __GUI_DEBUG_H__


#define GUI_ERR(fmt,args...) printf("[GUI ERR][%s %d] \033[31m"fmt"\033[0m\n",__FILE__,__LINE__,##args);
#define GUI_WARN(fmt,args...) printf("[GUI WARNNING][%s %d] \033[33m"fmt"\033[0m\n",__FILE__,__LINE__,##args);
#define GUI_INFO(fmt,args...) printf("[GUI INFO][%s %d] \033[34m"fmt"\033[0m\n",__FILE__,__LINE__,##args);
#define GUI_DEBUG(fmt,args...) printf("[GUI DEBUG] "fmt"\n",##args);


#endif

