#pragma once
#include <qtmetamacros.h>
namespace QSpace::Core
{
Q_NAMESPACE
enum class TaskPriority
{
  Urgent,
  Bulk,
  Auto
};
Q_ENUM_NS(TaskPriority)
} // namespace QSpace::Core