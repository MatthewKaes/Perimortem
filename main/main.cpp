// Perimortem Engine
// Copyright © Matt Kaes

#include <clocale>
#include <iostream>

#include <string>
#include <vector>

#include <chrono>
#include <thread>

using namespace std;

int perimortem_main(const string &binary,
                    const std::vector<std::string> &args) {
  cout << "Hello World!" << endl;
  for (const auto &arg : args)
    cout << "arg: " << arg << endl;

  return 0;
}

#ifdef PERI_WINDOWS

#include <Windows.h>

HINSTANCE perimortem_hinstance = nullptr;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
  perimortem_hinstance = hInstance;

  LPWSTR *winArgv;
  int winArgc;
  int result;

  winArgv = CommandLineToArgvW(GetCommandLineW(), &winArgc);

  if (nullptr == winArgv) {
    wprintf(L"CommandLineToArgvW failed\n");
    return 0;
  }

  // TODO: Windows when I feel like it.
  return -1;
}

#else
int main(int argc, char **argv) {
  vector<string> args;
  string binary = argv[0];
  for (int i = 1; i < argc; i++) {
    args.push_back(argv[i]);
  }

  return perimortem_main(binary, args);
}
#endif
