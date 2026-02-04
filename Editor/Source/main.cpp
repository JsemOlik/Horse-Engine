#include "EditorWindow.h"
#include "HorseEngine/Core/Logging.h"
#include "HorseEngine/Core/Time.h"

#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    
    Horse::Logger::Initialize();
    Horse::Time::Initialize();
    
    HORSE_LOG_CORE_INFO("Horse Engine Editor v0.1.0");
    
    EditorWindow window;
    window.show();
    
    int result = app.exec();
    
    Horse::Logger::Shutdown();
    
    return result;
}
