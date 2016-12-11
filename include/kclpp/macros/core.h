#pragma once

#define KCLPP_DISABLE_COPY_AND_ASSIGN(cls) \
  cls(const cls&) = delete; \
  cls& operator=(const cls&) = delete;
