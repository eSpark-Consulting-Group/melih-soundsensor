#pragma once

#include <list>

namespace core {
class Thread;
}  // namespace core

namespace scenes {

bool initializeComponent( std::list<core::Thread>& threadsList );

}  // namespace scenes
