#include <cstdio>
#include <iostream>
#include "../FRCVLib/Manager.h"

using namespace std;

int main()
{
    Manager manager = Manager();
    auto logs = manager.getAllLogs();

    for (std::vector<Log*>::iterator i = logs->begin(); i != logs->end();i++) {
        cout << (*i)->GetMessage() << endl;
    }

    int apriltagSink = manager.createApriltagSink();

    int source = manager.createImageFileSource("");

    manager.bindSourceToSink(source, apriltagSink);

    manager.startAllSinks();

    cout << manager.getSinkResult(apriltagSink);

    manager.stopAllSinks();

    return 0;
}