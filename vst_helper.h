#ifndef VST_HELPER_H
#define VST_HELPER_H

// for HMODULE, LoadLibrary
#include <Windows.h>

// For AEffect
#define   WIN32 // to import the right types
#include "pluginterfaces/vst2.x/aeffectx.h"

#include <vector>
#include <string>

class VSTHelper
{
public:

  void Init(const std::string& vst_path);

private:

  HMODULE         module_;
  AEffect*        effect_;

  std::vector<std::string> param_names_;
  std::vector<std::string> param_labels_;
  std::vector<std::string> param_displays_;
  std::vector<float> param_values_;

};

#endif