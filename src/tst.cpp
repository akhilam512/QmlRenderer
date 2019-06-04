#include "tst.h"

tst::tst(QObject *parent) : QObject(parent)
{

}

void tst::test()
{
    std::unique_ptr<int> t ;
    t = std::make_unique<int>(2);

    vec.append(t);

}
