#pragma once

struct Location {
  const char* label;
  double latitude;
  double longitude;
  const char* timezone;
};

// Approximate estate coordinates; change if you want a different weather point.
// `static` gives each translation unit its own read-only copy. That avoids
// requiring C++17 inline variables, which older Arduino toolchains omit.
static constexpr Location kHomeLocation{
    "Amrapali Princely Estate, Sector 76, Noida",
    28.56533,
    77.37966,
    "Asia/Kolkata",
};
