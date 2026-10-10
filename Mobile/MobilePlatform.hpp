#pragma once
#include <string>
namespace jely::mobile {
void captureFramebuffer();
bool active();
int smokeFrames(int argc,char** argv);
std::string storagePath();
float uiScale();
std::string verifyContext();
unsigned int graphicsErrors();
}
