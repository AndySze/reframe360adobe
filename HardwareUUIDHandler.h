#pragma once
#include <cstdint>
#include <string>



void getMacHash(uint16_t& mac1, uint16_t& mac2);
uint16_t getVolumeHash();
uint16_t getCpuHash();
const char* getMachineName();

static bool validate(std::string testIdString);
const char* getSystemUniqueId();
static uint16_t* computeSystemUniqueId();

