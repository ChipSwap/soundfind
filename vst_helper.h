#ifndef VST_HELPER_H
#define VST_HELPER_H

// for HMODULE, LoadLibrary
#include <Windows.h>

// For AEffect
#define   WIN32 // to import the right types
#include "pluginterfaces/vst2.x/aeffectx.h"
#include "pluginterfaces/vst2.x/vstfxstore.h"

#include <vector>
#include <string>

class VSTHelper
{
public:

  void Init(const std::string& vst_path, float sample_rate);
  void GenerateOutput(VstInt32 block_size, float* output[2]);
  
  void LoadProgram(const std::string& path);
  void SaveCurrentProgram(const std::string& path);
  
  //unsigned int GetNumParams() { return params_.size(); }
  
  void  SetParam(unsigned int index, float value);
  float GetParam(unsigned int index);

private:

  HMODULE         module_;
  AEffect*        effect_;

  //typedef struct
  //{
  //  std::string name_;
  //  std::string label_;
  //  std::string display_;
  //  float value;
  //} Param;

  bool in_chunks_;
  //std::vector<Param> params_;

};

#endif