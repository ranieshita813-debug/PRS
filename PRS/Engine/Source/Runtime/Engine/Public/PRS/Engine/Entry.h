#pragma once
#include "PRS/Engine/Application.h"

// Include this in exactly one .cpp of your game, and define PRS::CreateApplication().
int main()
{
    auto app = PRS::CreateApplication();
    return app->Run();
}
