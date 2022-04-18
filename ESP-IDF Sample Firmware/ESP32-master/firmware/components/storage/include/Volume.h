#pragma once

namespace storage {

namespace Volume {
static constexpr auto m_basePath = "/vol";
static constexpr auto m_optPath = "/opt";

bool Init();
void Deinit();

bool FormatAllPartitions();

}  // namespace Volume

}  // namespace storage
