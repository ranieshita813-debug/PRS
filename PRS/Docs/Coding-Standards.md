# Coding standards

- Namespace `PRS`, types PascalCase, members `m_`.
- No raw `std::thread` in gameplay code; use Jobs.
- No heap allocation in per-frame hot paths; use LinearAllocator.
