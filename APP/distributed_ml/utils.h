#ifndef __UTILS_H__
#define __UTILS_H__

#include <string>
#include <iostream>
#include <unistd.h>
#include <iomanip>

void refresh(float a)
{
    if (a > 1)
        a = 1;
    int pa = a * 50;
    cout << "\33[1A";
    cout << "[" + string(pa, '=') + ">" + string(50 - pa, ' ') << "]  " << a*100 << "%" << endl;
    fflush(stdout);
}



#endif
