#pragma once
#include <string>

struct Clipboard {
    std::string data;
    std::string type;
};

Clipboard clipboardCopy(class MiddleWare &mw, std::string url);

void presetCopy(std::string url, std::string name);
void presetPaste(std::string url, std::string name);
void presetPaste(std::string url, int);
void presetDelete(int);
void presetRescan();
std::string presetClipboardType();
bool presetCheckClipboardType();

extern MiddleWare *middlewarepointer;
